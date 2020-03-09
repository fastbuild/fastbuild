// Client.cpp
//------------------------------------------------------------------------------

// Includes
//------------------------------------------------------------------------------
#include "Client.h"

#include "Tools/FBuild/FBuildCore/Protocol/Protocol.h"
#include "Tools/FBuild/FBuildCore/FBuild.h"
#include "Tools/FBuild/FBuildCore/FLog.h"
#include "Tools/FBuild/FBuildCore/Graph/CompilerNode.h"
#include "Tools/FBuild/FBuildCore/Graph/TestNode.h"
#include "Tools/FBuild/FBuildCore/Graph/FileNode.h"
#include "Tools/FBuild/FBuildCore/Graph/Node.h"
#include "Tools/FBuild/FBuildCore/Graph/ObjectNode.h"
#include <Tools/FBuild/FBuildCore/Helpers/MultiBuffer.h>
#include "Tools/FBuild/FBuildCore/WorkerPool/Job.h"
#include "Tools/FBuild/FBuildCore/WorkerPool/JobQueue.h"

#include "Core/Env/ErrorFormat.h"
#include "Core/FileIO/ConstMemoryStream.h"
#include "Core/FileIO/FileIO.h"
#include "Core/FileIO/FileStream.h"
#include "Core/FileIO/MemoryStream.h"
#include "Core/Math/Random.h"
#include "Core/Process/Atomic.h"
#include "Core/Profile/Profile.h"
#include "Core/Time/Time.h"
#include "Core/Tracing/Tracing.h"

// Defines
//------------------------------------------------------------------------------
#define CLIENT_STATUS_UPDATE_FREQUENCY_SECONDS ( 0.1f )
#define SYSTEM_ERROR_ATTEMPT_COUNT ( 3 )
#define DIST_INFO( ... ) if ( m_DetailedLogging ) { FLOG_OUTPUT( __VA_ARGS__ ); }

// CONSTRUCTOR
//------------------------------------------------------------------------------
Client::Client( const Array< AString > & settingsWorkers,
                uint16_t port,
                int32_t workerListRefreshLimitSec,
                int32_t workerConnectionRetryLimitSec,
                uint32_t workerConnectionLimit,
                bool detailedLogging )
    : m_WorkerList( 256, true )  // start with 256 capacity
    , m_RetryingWorkers( true )  // begin with retrying of workers
    , m_NotifyJobQueue( false )  // don't notify yet, notify when no longer retrying workers
    , m_WorkerListRefreshIntervalSec( 0 )  // try immediately the first time
    , m_WorkerListRefreshElapsedTimeSec( 0 )
    , m_WorkerListRefreshLimitSec( workerListRefreshLimitSec )
    , m_WorkerConnectionRetryLimitSec( workerConnectionRetryLimitSec )
    , m_ShouldExit( false )
    , m_Exited( false )
    , m_DetailedLogging( detailedLogging )
    , m_WorkerConnectionLimit( workerConnectionLimit )
    , m_Port( port )
{
    for( const AString & settingsWorker : settingsWorkers )
    {
        WorkerBrokerage::WorkerInfo workerInfo;
        workerInfo.basePath = ""; // none, since static worker list
        workerInfo.name = settingsWorker;
        m_SettingsWorkers.Append( workerInfo );
    }

    m_Thread = Thread::CreateThread( ThreadFuncStatic,
                                     "Client",
                                     ( 64 * KILOBYTE ),
                                     this );
    ASSERT( m_Thread );
}

// DESTRUCTOR
//------------------------------------------------------------------------------
Client::~Client()
{
    SetShuttingDown();

    AtomicStoreRelaxed( &m_ShouldExit, true );
    Thread::WaitForThread( m_Thread );

    ShutdownAllConnections();

    Thread::CloseHandle( m_Thread );

    MutexHolder mh( m_ServerListMutex );
    const size_t numServers = m_ServerList.GetSize();
    for ( size_t i=0; i<numServers; ++i )
    {
        ServerState * const ss = m_ServerList.Get( i );
        if ( ss )
        {
            if (ss->m_Connection != nullptr)
            {
                ss->m_Connection->SetUserData( nullptr );
            }
            FDELETE ss;
        }
    }
    m_ServerList.Clear();
}

//------------------------------------------------------------------------------
/*virtual*/ void Client::OnDisconnected( const ConnectionInfo * connection )
{
    ASSERT( connection );
    ServerState * ss = (ServerState *)connection->GetUserData();
    if (ss != nullptr)
    {
        ss->Disconnect();
    }
}

// ThreadFuncStatic
//------------------------------------------------------------------------------
/*static*/ uint32_t Client::ThreadFuncStatic( void * param )
{
    PROFILE_SET_THREAD_NAME( "ClientThread" )

    Client * c = (Client *)param;
    c->ThreadFunc();
    return 0;
}

// ThreadFunc
//------------------------------------------------------------------------------
void Client::ThreadFunc()
{
    PROFILE_FUNCTION

    // ensure first status update will be sent more rapidly
    m_StatusUpdateTimer.Start();

    for ( ;; )
    {
        LookForWorkers();
        if ( AtomicLoadRelaxed( &m_ShouldExit ) )
        {
            break;
        }

        CommunicateJobAvailability();
        if ( AtomicLoadRelaxed( &m_ShouldExit ) )
        {
            break;
        }

        Thread::Sleep( 1 );
        if ( AtomicLoadRelaxed( &m_ShouldExit ) )
        {
            break;
        }
    }

    m_Exited = true;
}

// ShouldRetry
//------------------------------------------------------------------------------
/* static */ bool Client::ShouldRetry(
    const float timeSinceLastRetry,
    const uint32_t retryIntervalSec,
    const int32_t retryLimitSec,
    float & retryElapsedTimeSec,
    AString & timeIntervalString,
    AString & timeLimitString )
{
    bool shouldRetry = false;
    if ( retryLimitSec >= 0 )  // finite retries
    {
        const bool outputFractionalDigits = false;
        Time::FormatTime( (float)retryLimitSec, outputFractionalDigits, timeLimitString );
        if ( retryLimitSec > 0 )
        {
            retryElapsedTimeSec += timeSinceLastRetry;
            if ( retryElapsedTimeSec + retryIntervalSec <= (uint32_t) retryLimitSec )
            {
                  shouldRetry = true;
            }
            else  // retry limit exceeded
            {
                // leave shouldRetry = false
            }
        }
        else  // retryLimitSec == 0 (no retries)
        {
            // leave shouldRetry = false
        }
    }
    else  // infinite retries
    {
        timeLimitString.Assign( "infinite" );
        shouldRetry = true;
    }
    if ( shouldRetry )
    {
        const bool outputFractionalDigits = false;
        Time::FormatTime( (float)retryIntervalSec, outputFractionalDigits, timeIntervalString );
    }
    return shouldRetry;
}

// RetryWorkers
//------------------------------------------------------------------------------
void Client::RetryWorkers()
{
    if ( m_RetryingWorkers )
    {
        if ( m_SettingsWorkers.IsEmpty() )  // if dynamic workers
        {
            float timeSinceLastRetry = m_WorkerListRefreshTimer.GetElapsed();
            if ( timeSinceLastRetry > m_WorkerListRefreshIntervalSec )
            {
                Array< Tags > requiredTagsPerJob;
                if ( JobQueue::IsValid() )
                {
                    JobQueue::Get().GetDistributableJobWorkerTags(
                            requiredTagsPerJob );
                }
                if ( !requiredTagsPerJob.IsEmpty() )
                {
                    // reset timer here, since will be refreshing the worker list below
                    m_WorkerListRefreshTimer.Start();  // reset time
                    if ( m_WorkerListRefreshIntervalSec == 0 )
                    {
                        m_WorkerListRefreshIntervalSec = 4;  // next retry is in >= 4 seconds
                    }
                    else
                    {
                        // so we don't hit the network file share so much, use exponential backoff
                        m_WorkerListRefreshIntervalSec *= 2;
                    }

                    Array< WorkerBrokerage::WorkerInfo > previousWorkers;
                    FlattenWorkersPerJob( m_WorkerList, previousWorkers );

                    m_WorkerList.Clear();
                    m_WorkerBrokerage.FindWorkers(
                        requiredTagsPerJob,
                        m_ExcludedWorkers,
                        m_WorkerList );

                    Array< WorkerBrokerage::WorkerInfo > currentWorkers;
                    FlattenWorkersPerJob( m_WorkerList, currentWorkers );
                    if ( currentWorkers.IsEmpty() )
                    {
                        FLOG_WARN( "No matching workers found in '%s'", m_WorkerBrokerage.GetBrokerageRootPaths().Get() );
                    }

                    Array< WorkerBrokerage::WorkerInfo > addedWorkers;
                    GetWorkerChanges( previousWorkers, currentWorkers, addedWorkers );
                    // don't remove any workers during refresh, since previous workers could be rebooting
                    Array< WorkerBrokerage::WorkerInfo > removedWorkers;
                    UpdateServerList( removedWorkers, addedWorkers );

                    const size_t numPreviousWorkers( previousWorkers.GetSize() );
                    const size_t numCurrentWorkers( currentWorkers.GetSize() );
                    if ( numCurrentWorkers != numPreviousWorkers )
                    {
                        OutputNumWorkers();
                    }

                    AStackString<> timeIntervalString;
                    AStackString<> timeLimitString;
                    if ( ShouldRetry( timeSinceLastRetry,
                        m_WorkerListRefreshIntervalSec,
                        m_WorkerListRefreshLimitSec,
                        m_WorkerListRefreshElapsedTimeSec,
                        timeIntervalString,
                        timeLimitString ) )
                    {
                        OUTPUT( "Will refresh worker list in %s, retry limit %s\n", timeIntervalString.Get(), timeLimitString.Get() );
                    }
                    else
                    {
                        OUTPUT( "No more refreshes of worker list\n" );
                        m_RetryingWorkers = false;
                        // set to notify the job queue
                        m_NotifyJobQueue = true;
                    }
                }
                else
                {
                    // no jobs available, so use the existing list
                }
            }
            else
            {
                // elapsed time has not yet met retry interval, so use the existing list
            }
        }
        else  // static workers
        {
            // this code block only runs once per run of the app
            OUTPUT( "Not refreshing worker list, since you specified a .Workers list in your Settings node\n" );

            // clear 2d array
            m_WorkerList.Clear();

            // use the settings workers for the first element of the worker list,
            // since all jobs for static workers will use the same list
            // append 1d array to 1st element of 2d array
            m_WorkerList.Append( m_SettingsWorkers );

            m_RetryingWorkers = false;
            Array< WorkerBrokerage::WorkerInfo > removedWorkers;  // pass empty, since no removes
            UpdateServerList( removedWorkers, m_SettingsWorkers );  // pass workers in
            OutputNumWorkers();
            // set to notify the job queue
            m_NotifyJobQueue = true;
        }
    }
    // always call NotifyJobQueue() each time,
    // since initial notify could be a noop
    // due to job queue not being valid yet
    NotifyJobQueue();
}

// FlattenWorkersPerJob
//------------------------------------------------------------------------------
void Client::FlattenWorkersPerJob(
    const WorkersPerJob & workersPerJob,
    Array< WorkerBrokerage::WorkerInfo > & workers ) const
{
    for ( size_t i=0; i<workersPerJob.GetSize(); ++i )
    {
        const Array< WorkerBrokerage::WorkerInfo > & workersForJob = workersPerJob.Get( i );
        for ( size_t j=0; j<workersForJob.GetSize(); ++j )
        {
            const WorkerBrokerage::WorkerInfo & worker = workersForJob.Get( j );
            if ( !workers.Find( worker ) && !m_ExcludedWorkers.Find( worker ) )
            {
                workers.Append( worker );
            }
        }
    }
}

// GetWorkerChanges
//------------------------------------------------------------------------------
/*static*/ void Client::GetWorkerChanges(
    const Array< WorkerBrokerage::WorkerInfo > & previousWorkers,
    const Array< WorkerBrokerage::WorkerInfo > & currentWorkers,
    Array< WorkerBrokerage::WorkerInfo > & addedWorkers )
{
    const size_t numCurrentWorkers = currentWorkers.GetSize();

    // don't remove any workers during refresh, since previous workers could be rebooting

    // find workers that were added (or changed)
    for ( size_t i=0; i<numCurrentWorkers; ++i )
    {
        const WorkerBrokerage::WorkerInfo & currentWorker = currentWorkers.Get( i );
        if ( !previousWorkers.Find( currentWorker ) )
        {
            // not found, so add it
            addedWorkers.Append( currentWorker );
        }
        else
        {
            // skip worker, since in previous workers
        }
    }
}

// ExcludeWorker
//------------------------------------------------------------------------------
void Client::ExcludeWorker( ServerState & ss, Array< WorkerBrokerage::WorkerInfo > & newlyExcludedWorkers )
{
    if ( !ss.m_Excluded )
    {
        ss.m_Excluded = true;
        if ( !m_ExcludedWorkers.Find( ss.m_RemoteWorker ) )
        {
            m_ExcludedWorkers.Append( ss.m_RemoteWorker );
            newlyExcludedWorkers.Append( ss.m_RemoteWorker );
        }
    }
}

// HandleExcludedWorkers
//------------------------------------------------------------------------------
void Client::HandleExcludedWorkers( const Array< WorkerBrokerage::WorkerInfo > & newlyExcludedWorkers )
{
    if ( !newlyExcludedWorkers.IsEmpty() )
    {
        Array< WorkerBrokerage::WorkerInfo > addedWorkers;  // leave empty, since not adding here
        UpdateServerList( newlyExcludedWorkers, addedWorkers );
        // don't call OutputNumWorkers() here, since too frequent
        if ( !m_RetryingWorkers )
        {
            // set to notify the job queue of the change in num workers
            m_NotifyJobQueue = true;
        }
    }
}

// UpdateServerList
//------------------------------------------------------------------------------
bool Client::UpdateServerList(
    const Array < WorkerBrokerage::WorkerInfo > & removedWorkers,
    const Array < WorkerBrokerage::WorkerInfo > & addedWorkers )
{
    bool anyChangesApplied = false;

    // remove servers
    const size_t numRemovedWorkers = removedWorkers.GetSize();
    const size_t numServers = m_ServerList.GetSize();
    Array< ServerState * > serverStatesToRemove;
    for ( size_t i=0; i<numRemovedWorkers; ++i )
    {
        const WorkerBrokerage::WorkerInfo & removedWorker = removedWorkers.Get( i );
        // find server to remove
        for ( size_t j=0; j<numServers; ++j )
        {
            ServerState * const ss = m_ServerList.Get( j );
            if ( ss && ss->m_RemoteWorker == removedWorker )
            {
                serverStatesToRemove.Append( ss );
                anyChangesApplied = true;
                break;  // server found, so break out of inner loop
            }
        }
    }
    for ( size_t i=0; i<serverStatesToRemove.GetSize(); ++i )
    {
        ServerState * const ss = serverStatesToRemove.Get( i );
        if ( ss )
        {
            m_ServerList.FindAndErase( ss );
            if (ss->m_Connection != nullptr)
            {
                ss->m_Connection->SetUserData( nullptr );
            }
            FDELETE ss;
        }
    }

    // add servers
    const size_t numAddedWorkers = addedWorkers.GetSize();
    m_ServerList.SetCapacity( numAddedWorkers );
    for ( size_t i=0; i<numAddedWorkers; ++i )
    {
        ServerState * const ss = FNEW( ServerState( m_DetailedLogging ) );
        if ( ss )
        {
            // lock the server state
            MutexHolder mhSS( ss->m_Mutex );
            ss->m_RemoteWorker = addedWorkers.Get( i );
            ss->m_RemoteWorkerTags.SetWorkerName( ss->m_RemoteWorker.name );
            m_ServerList.Append( ss );
            anyChangesApplied = true;
        }
    }

    return anyChangesApplied;
}

// OutputNumWorkers
//------------------------------------------------------------------------------
void Client::OutputNumWorkers()
{
    AStackString<> workerOut;
    const size_t numWorkers( m_ServerList.GetSize() );
    if ( numWorkers > 0 )
    {
        AStackString<> compilationPrefix( "Distributed Compilation : " );
        workerOut.Append( compilationPrefix );
    }
    AStackString<> numWorkersOut;
    numWorkersOut.Format( "%u Worker(s) in list\n", (uint32_t)numWorkers );
    workerOut.Append( numWorkersOut );
    OUTPUT_STRING( workerOut.Get() );
}

// NotifyJobQueue
//------------------------------------------------------------------------------
void Client::NotifyJobQueue()
{
    if ( m_NotifyJobQueue )
    {
        // use the existing worker list from now on
        // notify the job queue that we are not retrying workers
        if ( JobQueue::IsValid() )
        {
            JobQueue::Get().SetRetryingWorkers( m_ServerList.GetSize(), m_RetryingWorkers );
            // done notifying job queue, so can set the changed bool to false
            m_NotifyJobQueue = false;
        }
    }
}

// LookForWorkers
//------------------------------------------------------------------------------
void Client::LookForWorkers()
{
    PROFILE_FUNCTION

    MutexHolder mh( m_ServerListMutex );

    RetryWorkers();

    const size_t numWorkers( m_ServerList.GetSize() );

    if ( numWorkers == 0 )
    {
        return;
    }

    // find out how many connections we have now
    size_t numConnections = 0;
    for ( size_t i=0; i<numWorkers; i++ )
    {
        ServerState * const ss = m_ServerList.Get( i );
        if ( ss && AtomicLoadRelaxed( &( ss->m_Connection ) ) )
        {
            numConnections++;
        }
    }

    // limit maximum concurrent connections
    if ( numConnections >= m_WorkerConnectionLimit )
    {
        return;
    }

    // if we're connected to every possible worker already
    if ( numConnections == numWorkers )
    {
        return;
    }

    // find someone to connect to
    // randomize the start indices to better distribute workers when there
    // are many workers/clients - otherwise all clients will attempt to connect
    // to the same subset of workers
    const size_t numJobs( m_WorkerList.GetSize() );
    // get a random job
    Random randomJob;
    size_t jobStartIndex = randomJob.GetRandIndex( (uint32_t)numJobs );
    bool connectionAttempted = false;
    Array< WorkerBrokerage::WorkerInfo > newlyExcludedWorkers;
    for ( size_t i=0; i<numJobs; ++i )
    {
        const size_t j( ( i + jobStartIndex ) % numJobs );
        const Array< WorkerBrokerage::WorkerInfo > & workersForJob = m_WorkerList.Get( j );
        const size_t numWorkersForJob( workersForJob.GetSize() );
        // get a random worker for the job
        Random randomWorker;
        size_t workersStartIndex = randomWorker.GetRandIndex( (uint32_t)numWorkersForJob );
        for ( size_t k=0; k<numWorkersForJob; ++k )
        {
            const size_t l( ( k + workersStartIndex ) % numWorkersForJob );
            const WorkerBrokerage::WorkerInfo & workerForJob = workersForJob.Get( l );
            ServerState * ss = nullptr;
            for ( size_t m=0; m<numWorkers; ++m )
            {
                ServerState * const stateToCheck = m_ServerList.Get( m );
                if ( stateToCheck && stateToCheck->m_RemoteWorker == workerForJob )
                {
                    ss = stateToCheck;
                    break;
                }
            }
            if ( ss != nullptr )
            {
                if ( AtomicLoadRelaxed( &( ss->m_Connection ) ) )
                {
                    continue;
                }

                MutexHolder mhSS( ss->m_Mutex );

                // ignore blacklisted workers
                if ( ss->m_Blacklisted )
                {
                    ExcludeWorker( *ss, newlyExcludedWorkers );
                    continue;
                }

                ASSERT( ss->m_Jobs.IsEmpty() );

                float timeSinceLastRetry = ss->m_ConnectionDelayTimer.GetElapsed();
                if ( timeSinceLastRetry <= ss->m_ConnectionRetryIntervalSec )
                {
                    continue;
                }

                ss->m_ConnectionDelayTimer.Start();  // reset time

                DIST_INFO( "Connecting to: %s\n", ss->m_RemoteWorker.name.Get() );
                const ConnectionInfo * ci = Connect( ss->m_RemoteWorker.name, m_Port, 2000 ); // 2000ms connection timeout
                if ( ci == nullptr )
                {
                    DIST_INFO( " - connection: %s (FAILED)\n", ss->m_RemoteWorker.name.Get() );
                    if ( ss->m_ConnectionRetryIntervalSec == 0 )
                    {
                        ss->m_ConnectionRetryIntervalSec = 4;  // next retry is in >= 4 seconds
                    }
                    else
                    {
                        // so we don't hit the network so much, use exponential backoff
                        ss->m_ConnectionRetryIntervalSec *= 2;
                    }
                    AStackString<> timeIntervalString;
                    AStackString<> timeLimitString;
                    if ( ShouldRetry( timeSinceLastRetry,
                        ss->m_ConnectionRetryIntervalSec,
                        m_WorkerConnectionRetryLimitSec,
                        ss->m_ConnectionRetryElapsedTimeSec,
                        timeIntervalString,
                        timeLimitString ) )
                    {
                        OUTPUT( "%s did not respond, retrying in >= %s, retry limit %s\n", ss->m_RemoteWorker.name.Get(), timeIntervalString.Get(), timeLimitString.Get() );
                    }
                    else
                    {
                        OUTPUT( "%s no more retries\n", ss->m_RemoteWorker.name.Get() );
                        ExcludeWorker( *ss, newlyExcludedWorkers );
                    }
                }
                else
                {
                    DIST_INFO( " - connection: %s (OK)\n", ss->m_RemoteWorker.name.Get() );
                    uint32_t numJobsAvailable = 0;
                    uint32_t numJobsAvailableForWorker = 0;
                    if ( JobQueue::IsValid() )
                    {
                        JobQueue::Get().GetNumDistributableJobsAvailable(
                            ss->m_RemoteWorkerTags,
                            numJobsAvailable,
                            numJobsAvailableForWorker );
                    }

                    ci->SetUserData( ss );
                    AtomicStoreRelaxed( &( ss->m_Connection ), ci ); // success!
                    ss->m_NumJobsAvailableForWorker = numJobsAvailableForWorker;
                    ss->m_StatusTimer.Start();

                    // send connection msg
                    Protocol::MsgConnection msg( numJobsAvailableForWorker );
                    SendMessageInternal( ci, msg );
                }
                // limit to one connection attempt per LookForWorkers()
                connectionAttempted = true;
                break;
            }
        }
        // limit to one connection attempt per LookForWorkers()
        if ( connectionAttempted )
        {
            break;
        }
    }
    HandleExcludedWorkers( newlyExcludedWorkers );
}

// CommunicateJobAvailability
//------------------------------------------------------------------------------
void Client::CommunicateJobAvailability()
{
    PROFILE_FUNCTION

    // too soon since last status update?
    if ( m_StatusUpdateTimer.GetElapsed() < CLIENT_STATUS_UPDATE_FREQUENCY_SECONDS )
    {
        return;
    }

    m_StatusUpdateTimer.Start(); // reset time

    // has status changed since we last sent it?
    MutexHolder mh( m_ServerListMutex );
    if ( m_ServerList.IsEmpty() )
    {
        return; // no servers to communicate with
    }

    // update each server to know how many jobs we have now
    for ( size_t i=0; i<m_ServerList.GetSize(); ++i )
    {
        ServerState * const ss = m_ServerList.Get( i );
        if ( ss )
        {
            MutexHolder ssMH( ss->m_Mutex );
            if ( const ConnectionInfo * connection = AtomicLoadRelaxed( &( ss->m_Connection ) ) )
            {
                uint32_t numJobsAvailable = 0;
                uint32_t numJobsAvailableForWorker = 0;
                JobQueue::Get().GetNumDistributableJobsAvailable(
                        ss->m_RemoteWorkerTags,
                        numJobsAvailable,
                        numJobsAvailableForWorker );
                if ( ss->m_NumJobsAvailableForWorker != numJobsAvailableForWorker )
                {
                    PROFILE_SECTION( "UpdateJobAvailability" )
                    Protocol::MsgStatus msg( numJobsAvailableForWorker );
                    SendMessageInternal( connection, msg );
                    ss->m_NumJobsAvailableForWorker = numJobsAvailableForWorker;
                }
                if ( numJobsAvailable > 0 &&  // if any job in queue
                     numJobsAvailableForWorker == 0 &&  // worker cannot build any job
                     ss->m_RemoteWorkerTags.IsValid() )  // worker tags valid
                {
                    // The worker can't build any of the current jobs, so disconnect from it.
                    // This allows us to try other workers that may be able to build jobs,
                    // and we avoid hitting the m_WorkerConnectionLimit with only noop workers.
                    Disconnect( connection );
                    // delay reconnecting to this worker
                    ss->m_ConnectionRetryIntervalSec = 4;  // next retry is in >= 4 seconds
                    ss->m_ConnectionDelayTimer.Start();    // reset time
                }
            }
        }
    }
}

// SendMessageInternal
//------------------------------------------------------------------------------
void Client::SendMessageInternal( const ConnectionInfo * connection, const Protocol::IMessage & msg )
{
    if ( msg.Send( connection ) )
    {
        return;
    }

    AStackString<> workerName;
    ServerState * ss = (ServerState *)connection->GetUserData();
    if (ss != nullptr)
    {
        workerName = ss->m_RemoteWorker.name;
    }

    DIST_INFO( "Send Failed: %s (Type: %u, Size: %u)\n",
                workerName.Get(),
                (uint32_t)msg.GetType(),
                msg.GetSize() );
}

// SendMessageInternal
//------------------------------------------------------------------------------
void Client::SendMessageInternal( const ConnectionInfo * connection, const Protocol::IMessage & msg, const MemoryStream & memoryStream )
{
    if ( msg.Send( connection, memoryStream ) )
    {
        return;
    }

    AStackString<> workerName;
    ServerState * ss = (ServerState *)connection->GetUserData();
    if (ss != nullptr)
    {
        workerName = ss->m_RemoteWorker.name;
    }

    DIST_INFO( "Send Failed: %s (Type: %u, Size: %u, Payload: %u)\n",
                workerName.Get(),
                (uint32_t)msg.GetType(),
                msg.GetSize(),
                (uint32_t)memoryStream.GetSize() );
}

// OnReceive
//------------------------------------------------------------------------------
/*virtual*/ void Client::OnReceive( const ConnectionInfo * connection, void * data, uint32_t size, bool & keepMemory )
{
    keepMemory = true; // we'll take care of freeing the memory

    MutexHolder mh( m_ServerListMutex );

    ServerState * ss = (ServerState *)connection->GetUserData();
    if (ss != nullptr)
    {
        // are we expecting a msg, or the payload for a msg?
        void * payload = nullptr;
        size_t payloadSize = 0;
        if ( ss->m_CurrentMessage == nullptr )
        {
            // message
            ss->m_CurrentMessage = static_cast< const Protocol::IMessage * >( data );
            if ( ss->m_CurrentMessage->HasPayload() )
            {
                return;
            }
        }
        else
        {
            // payload
            ASSERT( ss->m_CurrentMessage->HasPayload() );
            payload = data;
            payloadSize = size;
        }

        // determine message type
        const Protocol::IMessage * imsg = ss->m_CurrentMessage;
        Protocol::MessageType messageType = imsg->GetType();

        PROTOCOL_DEBUG( "Server -> Client : %u (%s)\n", messageType, GetProtocolMessageDebugName( messageType ) );

        switch ( messageType )
        {
            case Protocol::MSG_REQUEST_JOB:
            {
                const Protocol::MsgRequestJob * msg = static_cast< const Protocol::MsgRequestJob * >( imsg );
                Process( connection, msg );
                break;
            }
            case Protocol::MSG_JOB_RESULT:
            {
                const Protocol::MsgJobResult * msg = static_cast< const Protocol::MsgJobResult * >( imsg );
                Process( connection, msg, payload, payloadSize );
                break;
            }
            case Protocol::MSG_REQUEST_MANIFEST:
            {
                const Protocol::MsgRequestManifest * msg = static_cast< const Protocol::MsgRequestManifest * >( imsg );
                Process( connection, msg );
                break;
            }
            case Protocol::MSG_REQUEST_FILE:
            {
                const Protocol::MsgRequestFile * msg = static_cast< const Protocol::MsgRequestFile * >( imsg );
                Process( connection, msg );
                break;
            }
            case Protocol::MSG_SERVER_STATUS:
            {
                const Protocol::MsgServerStatus * msg = static_cast< const Protocol::MsgServerStatus * >( imsg );
                Process( connection, msg, payload, payloadSize );
                break;
            }
            default:
            {
                // unknown message type
                ASSERT( false ); // this indicates a protocol bug
                DIST_INFO( "Protocol Error: %s\n", ss->m_RemoteWorker.name.Get() );
                Disconnect( connection );
                break;
            }
        }

        // free everything
        FREE( (void *)( ss->m_CurrentMessage ) );
        FREE( payload );
        ss->m_CurrentMessage = nullptr;
    }
}

// Process( MsgRequestJob )
//------------------------------------------------------------------------------
void Client::Process( const ConnectionInfo * connection, const Protocol::MsgRequestJob * )
{
    PROFILE_SECTION( "MsgRequestJob" )

    ServerState * ss = (ServerState *)connection->GetUserData();
    if (ss != nullptr)
    {
        // no jobs for blacklisted workers
        if ( ss->m_Blacklisted )
        {
            MutexHolder mh( ss->m_Mutex );
            Protocol::MsgNoJobAvailable msg;
            SendMessageInternal( connection, msg );
            return;
        }

        bool errored = false;
        Job * job = JobQueue::Get().GetDistributableJobToProcess(
            ss->m_RemoteWorkerTags, true, errored );
        // ignore errored; only handle it for local case
        // since only want one thread checking it
        if ( job == nullptr )
        {
            PROFILE_SECTION( "NoJob" )
            // tell the client we don't have anything right now
            // (we completed, gave away the job already, or the worker did not have the required tags)
            MutexHolder mh( ss->m_Mutex );
            Protocol::MsgNoJobAvailable msg;
            SendMessageInternal( connection, msg );
            return;
        }

        // send the job to the client
        MemoryStream stream;
        job->Serialize( stream );

        MutexHolder mh( ss->m_Mutex );

        ss->m_Jobs.Append( job ); // Track in-flight job

        // if tool is explicitly specified, get the id of the tool manifest
        uint64_t toolId = 0;
        Node* node = job->GetNode();
        Node::Type nodeType = node->GetType();
        switch ( nodeType )
        {
            case Node::OBJECT_NODE:
                {
                    Node * cn = node->CastTo< ObjectNode >()->GetCompiler();
                    const ToolManifest * manifest = &(cn->CastTo< CompilerNode >()->GetManifest());
                    toolId = manifest->GetToolId();
                }
                break;
            default:
                ASSERT( false );
                break;
        }

        // output to signify remote start
        if ( FBuild::Get().GetOptions().m_ShowCommandSummary )
        {
            FLOG_OUTPUT( "-> Obj: %s <REMOTE: %s>\n",
                job->GetNode()->GetName().Get(),
                ss->m_RemoteWorker.name.Get() );
        }
        FLOG_MONITOR( "START_JOB %s \"%s\" \n",
            ss->m_RemoteWorker.name.Get(),
            job->GetNode()->GetName().Get() );

        {
            PROFILE_SECTION( "SendJob" )
            Protocol::MsgJob msg( toolId );
            SendMessageInternal( connection, msg, stream );
        }
    }
}

// Process( MsgJobResult )
//------------------------------------------------------------------------------
void Client::Process( const ConnectionInfo * connection, const Protocol::MsgJobResult *, const void * payload, size_t payloadSize )
{
    PROFILE_SECTION( "MsgJobResult" )

    // find server
    ServerState * ss = (ServerState *)connection->GetUserData();
    if (ss != nullptr)
    {
        AStackString<> workerName( ss->m_RemoteWorker.name );

        ConstMemoryStream ms( payload, payloadSize );

        uint32_t jobId = 0;
        ms.Read( jobId );

        AStackString<> name;
        ms.Read( name );

        bool result = false;
        ms.Read( result );

        bool systemError = false;
        ms.Read( systemError );

        Array< AString > messages;
        ms.Read( messages );

        uint32_t buildTime;
        ms.Read( buildTime );

        // get result data (built data or errors if failed)
        uint32_t size = 0;
        ms.Read( size );
        const void * data = (const char *)ms.GetData() + ms.Tell();

        {
            MutexHolder mh( ss->m_Mutex );
            VERIFY( ss->m_Jobs.FindDerefAndErase( jobId ) );
        }

        // Has the job been cancelled in the interim?
        // (Due to a Race by the main thread for example)
        Job * job = JobQueue::Get().OnReturnRemoteJob( jobId );
        if ( job == nullptr )
        {
            // don't save result as we were cancelled
            return;
        }

        DIST_INFO( "Got Result: %s - %s%s\n", workerName.Get(),
                                              job->GetNode()->GetName().Get(),
                                              job->GetDistributionState() == Job::DIST_RACE_WON_REMOTELY ? " (Won Race)" : "" );

        job->SetMessages( messages );

        if ( result == true )
        {
            // built ok - serialize to disc

            MultiBuffer mb( data, (size_t)(ms.GetSize() - ms.Tell()) );

            Node* node = job->GetNode();
            Node::Type nodeType = node->GetType();
            ObjectNode * objectNode = nullptr;
            switch ( nodeType )
            {
                case Node::OBJECT_NODE:
                    objectNode = node->CastTo< ObjectNode >();
                    break;
                case Node::TEST_NODE:
                    // nothing to do here
                    break;
                default:
                    ASSERT( false );
                    break;
            }

            const AString & nodeName = node->GetName();
            if ( Node::EnsurePathExistsForFile( nodeName ) == false )
            {
                FLOG_ERROR( "Failed to create path for '%s'", nodeName.Get() );
                result = false;
            }
            else
            {
                size_t fileIndex = 0;

                // 1. main output file
                result = WriteFileToDisk( nodeName, mb, fileIndex++ );

                switch ( nodeType )
                {
                    case Node::OBJECT_NODE:
                        {
                            // 2. PDB file (optional)
                            if ( result && objectNode->IsUsingPDB() )
                            {
                                AStackString<> pdbName;
                                objectNode->GetPDBName( pdbName );
                                result = WriteFileToDisk( pdbName, mb, fileIndex++ );
                            }

                            // 3. .nativecodeanalysis.xml (optional)
                            if ( result && objectNode->IsUsingStaticAnalysisMSVC() )
                            {
                                AStackString<> xmlFileName;
                                objectNode->GetNativeAnalysisXMLPath( xmlFileName );
                                result = WriteFileToDisk( xmlFileName, mb, fileIndex++ );
                            }
                        }
                        break;
                    case Node::TEST_NODE:
                        // nothing to do here, since relying on single WriteFileToDisk() above
                        break;
                    default:
                        ASSERT( false );
                        break;
                }

                if ( result == true )
                {
                    // record new file time
                    node->RecordStampFromBuiltFile();

                    // record time taken to build
                    node->SetLastBuildTime( buildTime );
                    node->SetStatFlag(Node::STATS_BUILT);
                    node->SetStatFlag(Node::STATS_BUILT_REMOTE);

                    // commit to cache?
                    switch ( nodeType )
                    {
                        case Node::OBJECT_NODE:
                            if ( FBuild::Get().GetOptions().m_UseCacheWrite &&
                                    objectNode->ShouldUseCache() )
                            {
                                AStackString<> workingDir;
                                objectNode->GetWorkingDir( job, workingDir );
                                objectNode->WriteToCache( job, workingDir );
                            }
                            break;
                        case Node::TEST_NODE:
                            // nothing to do here, since tests do not use cache
                            break;
                        default:
                            ASSERT( false );
                            break;
                    }
                }
                else
                {
                    node->GetStatFlag( Node::STATS_FAILED );
                }
            }

            switch ( nodeType )
            {
                case Node::OBJECT_NODE:
                    {
                        // get list of messages during remote work
                        AStackString<> msgBuffer;
                        job->GetMessagesForLog( msgBuffer );

                        if ( objectNode->IsMSVC() )
                        {
                            if ( objectNode->GetFlag( ObjectNode::FLAG_WARNINGS_AS_ERRORS_MSVC ) == false )
                            {
                                FileNode::HandleWarningsMSVC( job, objectNode->GetName(), msgBuffer.Get(), msgBuffer.GetLength() );
                            }
                        }
                        else if ( objectNode->IsClang() || objectNode->IsGCC() )
                        {
                            if ( !objectNode->GetFlag( ObjectNode::FLAG_WARNINGS_AS_ERRORS_CLANGGCC ) )
                            {
                                FileNode::HandleWarningsClangGCC( job, objectNode->GetName(), msgBuffer.Get(), msgBuffer.GetLength() );
                            }
                        }
                    }
                    break;
                case Node::TEST_NODE:
                    // nothing to do here
                    break;
                default:
                    ASSERT( false );
                    break;
            }
        }
        else
        {
            ((FileNode *)job->GetNode())->GetStatFlag( Node::STATS_FAILED );

            // failed - build list of errors
            const AString & nodeName = job->GetNode()->GetName();
            AStackString< 8192 > failureOutput;
            failureOutput.Format( "PROBLEM: %s\n", nodeName.Get() );
            for ( const AString * it = messages.Begin(); it != messages.End(); ++it )
            {
                failureOutput += *it;
            }

            // was it a system error?
            if ( systemError )
            {
                // blacklist misbehaving worker
                ss->m_Blacklisted = true;
                // no more jobs to this worker, so disconnect from it
                // also, disconnect here allows the exclude logic to run in LookForWorkers()
                Disconnect( ss->m_Connection );

                // take note of failure of job
                job->OnSystemError();

                // debugging message
                DIST_INFO( "Remote System Failure!\n"
                           " - Blacklisted Worker: %s\n"
                           " - Node              : %s\n"
                           " - Job Error Count   : %u / %u\n"
                           " - Details           :\n"
                           "%s",
                           workerName.Get(),
                           job->GetNode()->GetName().Get(),
                           job->GetSystemErrorCount(), SYSTEM_ERROR_ATTEMPT_COUNT,
                           failureOutput.Get()
                          );

                // should we retry on another worker?
                AStackString<> tmp;
                if ( job->GetSystemErrorCount() < SYSTEM_ERROR_ATTEMPT_COUNT )
                {
                    // re-queue job, job will be re-attempted on another worker
                    JobQueue::Get().ReturnUnfinishedDistributableJob( job );
                    // failed on this worker, add info about this to error output
                    tmp.Format( "FBuild: System error from worker %s\n", workerName.Get() );
                }
                else
                {
                    // failed too many times on different workers, add info about this to
                    // error output
                    tmp.Format( "FBuild: System error from %u different workers\n", (uint32_t)SYSTEM_ERROR_ATTEMPT_COUNT );
                }
                if ( failureOutput.EndsWith( '\n' ) == false )
                {
                    failureOutput += '\n';
                }
                failureOutput += tmp;
            }

            Node::DumpOutput( nullptr, failureOutput.Get(), failureOutput.GetLength(), nullptr );

            if ( systemError )
            {
                return;
            }
        }

        if ( FLog::IsMonitorEnabled() )
        {
            AStackString<> msgBuffer;
            job->GetMessagesForMonitorLog( msgBuffer );

            FLOG_MONITOR( "FINISH_JOB %s %s \"%s\" \"%s\"\n",
                          result ? "SUCCESS" : "ERROR",
                          workerName.Get(),
                          job->GetNode()->GetName().Get(),
                          msgBuffer.Get() );
        }

        JobQueue::Get().FinishedProcessingJob( job, result, true ); // remote job
    }
}

// Process( MsgRequestManifest )
//------------------------------------------------------------------------------
void Client::Process( const ConnectionInfo * connection, const Protocol::MsgRequestManifest * msg )
{
    PROFILE_SECTION( "MsgRequestManifest" )

    // find a job associated with this client with this toolId
    const uint64_t toolId = msg->GetToolId();
    ASSERT( toolId );
    const ToolManifest * manifest = FindManifest( connection, toolId );

    if ( manifest == nullptr )
    {
        // client asked for a manifest that is not valid
        ASSERT( false ); // this indicates a logic bug
        Disconnect( connection );
        return;
    }

    MemoryStream ms;
    manifest->SerializeForRemote( ms );

    // Send manifest to worker
    Protocol::MsgManifest resultMsg( toolId );
    resultMsg.Send( connection, ms );
}

// Process ( MsgRequestFile )
//------------------------------------------------------------------------------
void Client::Process( const ConnectionInfo * connection, const Protocol::MsgRequestFile * msg )
{
    PROFILE_SECTION( "MsgRequestFile" )

    // find a job associated with this client with this toolId
    const uint64_t toolId = msg->GetToolId();
    ASSERT( toolId != 0 ); // server should not request 'no sync' tool id
    const ToolManifest * manifest = FindManifest( connection, toolId );

    if ( manifest == nullptr )
    {
        // client asked for a manifest that is not valid
        ASSERT( false ); // this indicates a logic bug
        Disconnect( connection );
        return;
    }

    const uint32_t fileId = msg->GetFileId();
    size_t dataSize( 0 );
    const void * data = manifest->GetFileData( fileId, dataSize );
    if ( !data )
    {
        ASSERT( false ); // something is terribly wrong
        Disconnect( connection );
        return;
    }

    ConstMemoryStream ms( data, dataSize );

    // Send file to worker
    Protocol::MsgFile resultMsg( toolId, fileId );
    resultMsg.Send( connection, ms );
}

// Process ( MsgServerStatus )
//------------------------------------------------------------------------------
void Client::Process( const ConnectionInfo * connection, const Protocol::MsgServerStatus * msg, const void * payload, size_t payloadSize )
{
    PROFILE_SECTION( "MsgServerStatus" )

    (void)msg;

    // find server
    ServerState * ss = (ServerState *)connection->GetUserData();
    if (ss != nullptr)
    {
        ConstMemoryStream ms( payload, payloadSize );

        Tags removedTags;
        removedTags.Read( ms );
        Tags addedTags;
        addedTags.Read( ms );

        MutexHolder mh( ss->m_Mutex );
        if ( ss->m_RemoteWorkerTags.ApplyChanges( removedTags, addedTags ) )
        {
            // if any changes, then update
            if ( JobQueue::IsValid() )
            {
                JobQueue::Get().UpdateWorkerRecords( ss->m_RemoteWorkerTags );
            }
        }
        ss->m_StatusTimer.Start();
    }
}

// FindManifest
//------------------------------------------------------------------------------
const ToolManifest * Client::FindManifest( const ConnectionInfo * connection, uint64_t toolId ) const
{
    const ToolManifest * m = nullptr;
    ServerState * ss = (ServerState *)connection->GetUserData();
    if (ss != nullptr)
    {
        MutexHolder mh( ss->m_Mutex );

        uint64_t manifestToolId = 0;
        for ( Job ** it = ss->m_Jobs.Begin();
              it != ss->m_Jobs.End();
              ++it )
        {
            Node* node = ( *it )->GetNode();
            Node::Type nodeType = node->GetType();
            switch ( nodeType )
            {
                case Node::OBJECT_NODE:
                    {
                        Node * cn = node->CastTo< ObjectNode >()->GetCompiler();
                        m = &(cn->CastTo< CompilerNode >()->GetManifest());
                        manifestToolId = m->GetToolId();
                    }
                    break;
                default:
                    ASSERT( false );
                    break;
            }
           if ( manifestToolId == toolId )
            {
                // found a job with the same toolid
                return m;
            }
        }
    }

    return nullptr;
}

// WriteFileToDisk
//------------------------------------------------------------------------------
bool Client::WriteFileToDisk( const AString & fileName, const MultiBuffer & multiBuffer, size_t index ) const
{
    if ( multiBuffer.ExtractFile( index, fileName ) == false )
    {
        FLOG_ERROR( "Failed to create file. Error: %s File: '%s'", LAST_ERROR_STR, fileName.Get() );
        return false;
    }
    return true;
}

// CONSTRUCTOR( ServerState )
//------------------------------------------------------------------------------
Client::ServerState::ServerState( const bool detailedLogging )
    : m_Connection( nullptr )
    , m_CurrentMessage( nullptr )
    , m_ConnectionRetryIntervalSec( 0 )  // try immediately the first time
    , m_ConnectionRetryElapsedTimeSec( 0 )
    , m_NumJobsAvailableForWorker( 0 )
    , m_Jobs( 16, true )
    , m_Blacklisted( false )
    , m_Excluded( false )
    , m_DetailedLogging( detailedLogging )
{
}

// DESTRUCTOR( ServerState )
//------------------------------------------------------------------------------
Client::ServerState::~ServerState()
{
    MutexHolder mh( m_Mutex );
    ClearConnectionFields();
    if ( !m_RemoteWorker.name.IsEmpty() )
    {
        m_ConnectionRetryIntervalSec = 0;
        m_ConnectionRetryElapsedTimeSec = 0;
        JobQueue::Get().RemoveWorkerRecords( m_RemoteWorkerTags.GetWorkerName() );
        m_RemoteWorkerTags.Clear();
        m_RemoteWorker.basePath.Clear();
        m_RemoteWorker.name.Clear();
    }
}

// Disconnect ( ServerState )
//------------------------------------------------------------------------------
void Client::ServerState::Disconnect()
{
    MutexHolder mh( m_Mutex );
    ClearConnectionFields();
}

// ClearConnectionFields ( ServerState )
//------------------------------------------------------------------------------
void Client::ServerState::ClearConnectionFields()
{
    if ( m_Connection )
    {
        DIST_INFO( "Disconnected: %s\n", m_RemoteWorker.name.Get() );
        if ( m_Jobs.IsEmpty() == false )
        {
            Job ** it = m_Jobs.Begin();
            const Job * const * end = m_Jobs.End();
            while ( it != end )
            {
                FLOG_MONITOR( "FINISH_JOB TIMEOUT %s \"%s\" \n", m_RemoteWorker.name.Get(), (*it)->GetNode()->GetName().Get() );
                JobQueue::Get().ReturnUnfinishedDistributableJob( *it );
                ++it;
            }
            m_Jobs.Clear();
        }

        // This is usually null here, but might need to be freed if
        // we had the connection drop between message and payload
        FREE( (void *)( m_CurrentMessage ) );

        AtomicStoreRelaxed( &m_Connection, static_cast< const ConnectionInfo * >( nullptr ) );
        m_CurrentMessage = nullptr;
        m_NumJobsAvailableForWorker = 0;
    }
}

//------------------------------------------------------------------------------
