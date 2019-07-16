// Client.cpp
//------------------------------------------------------------------------------

// Includes
//------------------------------------------------------------------------------
#include "Client.h"

#include "Tools/FBuild/FBuildCore/Protocol/Protocol.h"
#include "Tools/FBuild/FBuildCore/FBuild.h"
#include "Tools/FBuild/FBuildCore/FLog.h"
#include "Tools/FBuild/FBuildCore/Graph/CompilerNode.h"
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
#include "Core/Tracing/Tracing.h"

// Defines
//------------------------------------------------------------------------------
#define CLIENT_STATUS_UPDATE_FREQUENCY_SECONDS ( 0.1f )
#define SYSTEM_ERROR_ATTEMPT_COUNT ( 3 )
#define DIST_INFO( ... ) if ( m_DetailedLogging ) { FLOG_BUILD( __VA_ARGS__ ); }

// CONSTRUCTOR
//------------------------------------------------------------------------------
Client::Client( const Array< AString > & settingsWorkers,
                uint16_t port,
                int32_t workerListRefreshLimitSec,
                uint32_t workerConnectionLimit,
                bool detailedLogging )
    : m_SettingsWorkers( settingsWorkers )
    , m_WorkerList( 256, true )  // start with 256 capacity
    , m_RetryingWorkers( true )  // begin with retrying of workers
    , m_WorkerListRefreshIntervalSec( 0 )  // try immediately the first time
    , m_WorkerListRefreshElapsedTimeSec( 0 )
    , m_WorkerListRefreshLimitSec( workerListRefreshLimitSec )
    , m_ShouldExit( false )
    , m_Exited( false )
    , m_DetailedLogging( detailedLogging )
    , m_NextIncrementServerId( 0 )
    , m_WorkerConnectionLimit( workerConnectionLimit )
    , m_Port( port )
{
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
}

//------------------------------------------------------------------------------
/*virtual*/ void Client::OnDisconnected( const ConnectionInfo * connection )
{
    ASSERT( connection );
    ServerState * ss = GetServer( connection );
    if ( ss )
    {
        ss->Disconnect();

        // since erasing server state below, only hold mutex for connection change
        {
            MutexHolder ssMH( ss->m_Mutex );
            if (ss->m_Connection != nullptr)
            {
                ss->m_Connection->SetUserData( nullptr );
            }
        }

        MutexHolder mh( m_ServerListMutex );
        RemoveServer( ss->m_Id );
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
        const uint32_t minutes = uint32_t( retryLimitSec / 60.0f );
        const uint32_t seconds = uint32_t( retryLimitSec ) - ( minutes * 60 );
        if ( minutes > 0 )
        {
            timeLimitString.Format( "%um %us\n", minutes, seconds );
        }
        else
        {
            timeLimitString.Format( "%us\n", seconds );
        }
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
        const uint32_t minutes = uint32_t( retryIntervalSec / 60.0f );
        const uint32_t seconds = uint32_t( retryIntervalSec ) - ( minutes * 60 );
        if ( minutes > 0 )
        {
            timeIntervalString.Format( "%um %us\n", minutes, seconds );
        }
        else
        {
            timeIntervalString.Format( "%us\n", seconds );
        }
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
                // reset timer here, since will be refreshing the worker list below
                m_WorkerListRefreshTimer.Start();  // reset time
                if ( m_WorkerListRefreshIntervalSec == 0 )
                {
                    m_WorkerListRefreshIntervalSec = 5;  // next retry is in >= 5 seconds
                }
                else
                {
                    // so we don't hit the network file share so much, use exponential backoff
                    m_WorkerListRefreshIntervalSec *= 2;
                }

                Array< AString > previousWorkers(m_WorkerList);

                m_WorkerList.Clear();
                m_WorkerBrokerage.FindWorkers(
                    m_ExcludedWorkers,
                    m_WorkerList );

                Array< AString > currentWorkers(m_WorkerList);
                if ( currentWorkers.IsEmpty() )
                {
                    FLOG_WARN( "No matching workers found in '%s'", m_WorkerBrokerage.GetBrokerageRoot().Get() );
                }

                Array< AString > addedWorkers;
                GetWorkerChanges( previousWorkers, currentWorkers, addedWorkers );
                // don't remove any workers during refresh, since previous workers could be rebooting
                Array< AString > removedWorkers;
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
                    DIST_INFO( "Will refresh worker list in %s, retry limit %s\n", timeIntervalString.Get(), timeLimitString.Get() );
                }
                else
                {
                    DIST_INFO( "No more refreshes of worker list\n" );
                    m_RetryingWorkers = false;
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
            DIST_INFO( "Not refreshing worker list, since you specified a .Workers list in your Settings node\n" );

            // use the settings workers for the first element of the worker list,
            // since all jobs for static workers will use the same list
            // append 1d array to 1st element of 2d array
            m_WorkerList.Append( m_SettingsWorkers );

            m_RetryingWorkers = false;
            Array< AString > removedWorkers;  // pass empty, since no removes
            UpdateServerList( removedWorkers, m_SettingsWorkers );  // pass workers in
            OutputNumWorkers();
        }
    }
}

// GetWorkerChanges
//------------------------------------------------------------------------------
/*static*/ void Client::GetWorkerChanges(
    const Array< AString > & previousWorkers,
    const Array< AString > & currentWorkers,
    Array< AString > & addedWorkers )
{
    const size_t numCurrentWorkers = currentWorkers.GetSize();

    // don't remove any workers during refresh, since previous workers could be rebooting

    // find workers that were added (or changed)
    for ( size_t i=0; i<numCurrentWorkers; ++i )
    {
        const AString & currentWorker = currentWorkers[ i ];
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
void Client::ExcludeWorker( ServerState & ss, Array< AString > & newlyExcludedWorkers )
{
    if ( !ss.m_Excluded )
    {
        ss.m_Excluded = true;
        if ( !m_ExcludedWorkers.Find( ss.m_RemoteName ) )
        {
            m_ExcludedWorkers.Append( ss.m_RemoteName );
            newlyExcludedWorkers.Append( ss.m_RemoteName );
        }
    }
}

// HandleExcludedWorkers
//------------------------------------------------------------------------------
void Client::HandleExcludedWorkers( const Array< AString > & newlyExcludedWorkers )
{
    if ( !newlyExcludedWorkers.IsEmpty() )
    {
        Array< AString > addedWorkers;  // leave empty, since not adding here
        UpdateServerList( newlyExcludedWorkers, addedWorkers );
        // don't call OutputNumWorkers() here, since too frequent
    }
}

// UpdateServerList
//------------------------------------------------------------------------------
bool Client::UpdateServerList(
    const Array < AString > & removedWorkers,
    const Array < AString > & addedWorkers )
{
    bool anyChangesApplied = false;

    // remove servers
    const size_t numRemovedWorkers = removedWorkers.GetSize();
    const size_t numServers = m_ServerList.GetSize();
    Array< ServerState * > serverStatesToRemove;
    for ( size_t i=0; i<numRemovedWorkers; ++i )
    {
        const AString & removedWorker = removedWorkers[ i ];
        // find server to remove
        for ( size_t j=0; j<numServers; ++j )
        {
            ServerState & ss = m_ServerList[ j ];

            if ( ss.m_RemoteName == removedWorker )
            {
                serverStatesToRemove.Append( &ss );
                anyChangesApplied = true;
                break;  // server found, so break out of inner loop
            }
        }
    }
    for ( size_t i=0; i<serverStatesToRemove.GetSize(); ++i )
    {
        ServerState * const ss = serverStatesToRemove[ i ];
        ASSERT( ss );

        // since erasing server state below, only hold mutex for connection change
        {
            MutexHolder ssMH( ss->m_Mutex );
            if (ss->m_Connection != nullptr)
            {
                ss->m_Connection->SetUserData( nullptr );
            }
        }

        // caller already holds m_ServerListMutex
        RemoveServer( ss->m_Id );
    }

    // add servers
    const size_t numAddedWorkers = addedWorkers.GetSize();
    size_t serverIndex = m_ServerList.GetSize();
    m_ServerList.SetSize( serverIndex + numAddedWorkers );
    const size_t newSize = m_ServerList.GetSize();
    for ( size_t workerIndex = 0;
          serverIndex < newSize;
          ++workerIndex, ++serverIndex )
    {
        ServerState & ss = m_ServerList[ serverIndex ];

        // lock the server state
        MutexHolder mhSS( ss.m_Mutex );

        if ( !m_FreeServerIds.IsEmpty() )
        {
            ss.m_Id = m_FreeServerIds.Top();
            m_FreeServerIds.Pop();
        }
        else
        {
            // No elements in free array, so use next available
            ss.m_Id = m_NextIncrementServerId++;
        }
        ss.m_DetailedLogging = m_DetailedLogging;
        ss.m_RemoteName = addedWorkers[ workerIndex ];
        anyChangesApplied = true;
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
    Tracing::Output( workerOut.Get() );
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
        ServerState & ss = m_ServerList[ i ];
        if ( AtomicLoadRelaxed( &( ss.m_Connection ) ) )
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
    // get a random worker
    Random randomWorker;
    size_t workerStartIndex = randomWorker.GetRandIndex( (uint32_t)numWorkers );
    Array< AString > newlyExcludedWorkers;
    for ( size_t i=0; i<numWorkers; ++i )
    {
        const size_t j( ( i + workerStartIndex ) % numWorkers );
        const AString & worker = m_WorkerList[ j ];
        ServerState * ss = nullptr;
        for ( size_t m=0; m<numWorkers; ++m )
        {
            ServerState & stateToCheck = m_ServerList[ m ];
            if ( stateToCheck.m_RemoteName == worker )
            {
                ss = &stateToCheck;
                break;
            }
        }

        if ( ss == nullptr || AtomicLoadRelaxed( &( ss->m_Connection ) ) )
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

        DIST_INFO( "Connecting to: %s\n", ss->m_RemoteName.Get() );
        const ConnectionInfo * ci = Connect( ss->m_RemoteName, m_Port, 2000 ); // 2000ms connection timeout
        if ( ci == nullptr )
        {
            DIST_INFO( " - connection: %s (FAILED)\n", ss->m_RemoteName.Get() );
            if ( ss->m_ConnectionRetryIntervalSec == 0 )
            {
                ss->m_ConnectionRetryIntervalSec = 5;  // next retry is in >= 5 seconds
            }
            else
            {
                // use exponential backoff, so
                // 1. we don't hit the network so much, and
                // 2. over time we choose machines that are connectible
                //    rather than this machine
                ss->m_ConnectionRetryIntervalSec *= 2;
            }

            // there is no time limit to connection retries,
            // we want to continue retrying since a machine may become
            // connectible later

            AStackString<> timeIntervalString;
            const uint32_t minutes = uint32_t( ss->m_ConnectionRetryIntervalSec / 60.0f );
            const uint32_t seconds = uint32_t( ss->m_ConnectionRetryIntervalSec ) - ( minutes * 60 );
            if ( minutes > 0 )
            {
                timeIntervalString.Format( "%um %us\n", minutes, seconds );
            }
            else
            {
                timeIntervalString.Format( "%us\n", seconds );
            }
            DIST_INFO( "%s did not respond, retrying in >= %s\n", ss->m_RemoteName.Get(), timeIntervalString.Get() );
        }
        else
        {
            DIST_INFO( " - connection: %s (OK)\n", ss->m_RemoteName.Get() );
            uint32_t numJobsAvailableForWorker = 0;
            if ( JobQueue::IsValid() )
            {
                numJobsAvailableForWorker =
                    (uint32_t)(JobQueue::Get().GetNumDistributableJobsAvailable());
            }

            ci->SetUserData( (void *)(ss->m_Id) );
            AtomicStoreRelaxed( &( ss->m_Connection ), ci ); // success!
            ss->m_ConnectionRetryIntervalSec = 0;
            ss->m_NumJobsAvailableForWorker = numJobsAvailableForWorker;
            ss->m_StatusTimer.Start();

            // send connection msg
            Protocol::MsgConnection msg( numJobsAvailableForWorker );
            SendMessageInternal( ci, msg );
        }
        // limit to one connection attempt per LookForWorkers()
        break;
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
        ServerState & ss = m_ServerList[ i ];

        MutexHolder ssMH( ss.m_Mutex );
        if ( const ConnectionInfo * connection = AtomicLoadRelaxed( &( ss.m_Connection ) ) )
        {
            uint32_t numJobsAvailableForWorker =
                (uint32_t)(JobQueue::Get().GetNumDistributableJobsAvailable());
            if ( ss.m_NumJobsAvailableForWorker != numJobsAvailableForWorker )
            {
                PROFILE_SECTION( "UpdateJobAvailability" )
                Protocol::MsgStatus msg( numJobsAvailableForWorker );
                SendMessageInternal( connection, msg );
                ss.m_NumJobsAvailableForWorker = numJobsAvailableForWorker;
            }
        }
    }
}

// GetServer
//------------------------------------------------------------------------------
Client::ServerState * Client::GetServer( const ConnectionInfo * connection ) const
{
    ASSERT( connection );

    const size_t serverId = (size_t)connection->GetUserData();
    const size_t numServers = m_ServerList.GetSize();
    for ( size_t i=0; i < numServers; ++i )
    {
        const ServerState & ss = m_ServerList[ i ];
        if ( ss.m_Id == serverId )
        {
            return &(m_ServerList[ i ]);
        }
    }
    return nullptr;
}

// RemoveServer
//------------------------------------------------------------------------------
void Client::RemoveServer( const size_t serverId )
{
    const size_t numServers = m_ServerList.GetSize();
    for ( size_t i=0; i < numServers; ++i )
    {
        ServerState & ss = m_ServerList[ i ];
        if ( ss.m_Id == serverId )
        {
            m_ServerList.EraseIndex( i );
            m_FreeServerIds.Append( serverId );
            break;
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

    DIST_INFO( "Send Failed: %s (Type: %u, Size: %u)\n",
                GetServer( connection )->m_RemoteName.Get(),
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

    DIST_INFO( "Send Failed: %s (Type: %u, Size: %u, Payload: %u)\n",
                GetServer( connection )->m_RemoteName.Get(),
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

    ServerState * ss = GetServer( connection );
    ASSERT( ss );

    void * payload = nullptr;
    size_t payloadSize = 0;

    {
        MutexHolder ssMH( ss->m_Mutex );
        // are we expecting a msg, or the payload for a msg?
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
        default:
        {
            // unknown message type
            ASSERT( false ); // this indicates a protocol bug
            DIST_INFO( "Protocol Error: %s\n", ss->m_RemoteName.Get() );
            Disconnect( connection );
            break;
        }
    }

    // free everything
    FREE( (void *)( ss->m_CurrentMessage ) );
    FREE( payload );
    ss->m_CurrentMessage = nullptr;
}

// Process( MsgRequestJob )
//------------------------------------------------------------------------------
void Client::Process( const ConnectionInfo * connection, const Protocol::MsgRequestJob * )
{
    PROFILE_SECTION( "MsgRequestJob" )

    ServerState * ss = GetServer( connection );
    ASSERT( ss );

    // no jobs for blacklisted workers
    if ( ss->m_Blacklisted )
    {
        MutexHolder mh( ss->m_Mutex );
        Protocol::MsgNoJobAvailable msg;
        SendMessageInternal( connection, msg );
        return;
    }

    Job * job = JobQueue::Get().GetDistributableJobToProcess( true );
    if ( job == nullptr )
    {
        PROFILE_SECTION( "NoJob" )
        // tell the client we don't have anything right now
        // (we completed or gave away the job already)
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

    // if tool is explicity specified, get the id of the tool manifest
    Node * n = job->GetNode()->CastTo< ObjectNode >()->GetCompiler();
    const ToolManifest & manifest = n->CastTo< CompilerNode >()->GetManifest();
    uint64_t toolId = manifest.GetToolId();
    ASSERT( toolId );

    // output to signify remote start
    FLOG_BUILD( "-> Obj: %s <REMOTE: %s>\n", job->GetNode()->GetName().Get(), ss->m_RemoteName.Get() );
    FLOG_MONITOR( "START_JOB %s \"%s\" \n", ss->m_RemoteName.Get(), job->GetNode()->GetName().Get() );

    {
        PROFILE_SECTION( "SendJob" )
        Protocol::MsgJob msg( toolId );
        SendMessageInternal( connection, msg, stream );
    }
}

// Process( MsgJobResult )
//------------------------------------------------------------------------------
void Client::Process( const ConnectionInfo * connection, const Protocol::MsgJobResult *, const void * payload, size_t payloadSize )
{
    PROFILE_SECTION( "MsgJobResult" )

    // find server
    ServerState * ss = GetServer( connection );
    ASSERT( ss );

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

    DIST_INFO( "Got Result: %s - %s%s\n", ss->m_RemoteName.Get(),
                                          job->GetNode()->GetName().Get(),
                                          job->GetDistributionState() == Job::DIST_RACE_WON_REMOTELY ? " (Won Race)" : "" );

    job->SetMessages( messages );

    if ( result == true )
    {
        // built ok - serialize to disc
        MultiBuffer mb( data, ms.GetSize() - ms.Tell() );

        ObjectNode * objectNode = job->GetNode()->CastTo< ObjectNode >();
        const AString & nodeName = objectNode->GetName();
        if ( Node::EnsurePathExistsForFile( nodeName ) == false )
        {
            FLOG_ERROR( "Failed to create path for '%s'", nodeName.Get() );
            result = false;
        }
        else
        {
            size_t fileIndex = 0;

            const ObjectNode * on = job->GetNode()->CastTo< ObjectNode >();

            // 1. Object file
            result = WriteFileToDisk( nodeName, mb, fileIndex++ );

            // 2. PDB file (optional)
            if ( result && on->IsUsingPDB() )
            {
                AStackString<> pdbName;
                on->GetPDBName( pdbName );
                result = WriteFileToDisk( pdbName, mb, fileIndex++ );
            }

            // 3. .nativecodeanalysis.xml (optional)
            if ( result && on->IsUsingStaticAnalysisMSVC() )
            {
                AStackString<> xmlFileName;
                on->GetNativeAnalysisXMLPath( xmlFileName );
                result = WriteFileToDisk( xmlFileName, mb, fileIndex++ );
            }

            if ( result )
            {
                // record new file time
                objectNode->RecordStampFromBuiltFile();

                // record time taken to build
                objectNode->SetLastBuildTime( buildTime );
                objectNode->SetStatFlag(Node::STATS_BUILT);
                objectNode->SetStatFlag(Node::STATS_BUILT_REMOTE);

                // commit to cache?
                if ( FBuild::Get().GetOptions().m_UseCacheWrite &&
                        on->ShouldUseCache() )
                {
                    objectNode->WriteToCache( job );
                }
            }
            else
            {
                objectNode->GetStatFlag( Node::STATS_FAILED );
            }
        }

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
            FLOG_INFO( "Remote System Failure!\n"
                       " - Blacklisted Worker: %s\n"
                       " - Node              : %s\n"
                       " - Job Error Count   : %u / %u\n",
                       ss->m_RemoteName.Get(),
                       job->GetNode()->GetName().Get(),
                       job->GetSystemErrorCount(), SYSTEM_ERROR_ATTEMPT_COUNT
                      );

            // should we retry on another worker?
            AStackString<> tmp;
            if ( job->GetSystemErrorCount() < SYSTEM_ERROR_ATTEMPT_COUNT )
            {
                // re-queue job, job will be re-attempted on another worker
                JobQueue::Get().ReturnUnfinishedDistributableJob( job );
                // failed on this worker, add info about this to error output
                tmp.Format( "FBuild: System error from worker %s\n", ss->m_RemoteName.Get() );
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
                      ss->m_RemoteName.Get(),
                      job->GetNode()->GetName().Get(),
                      msgBuffer.Get() );
    }

    JobQueue::Get().FinishedProcessingJob( job, result, true ); // remote job
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

// FindManifest
//------------------------------------------------------------------------------
const ToolManifest * Client::FindManifest( const ConnectionInfo * connection, uint64_t toolId ) const
{
    ServerState * ss = GetServer( connection );
    ASSERT( ss );

    MutexHolder mh( ss->m_Mutex );

    for ( Job ** it = ss->m_Jobs.Begin();
          it != ss->m_Jobs.End();
          ++it )
    {
        Node * n = ( *it )->GetNode()->CastTo< ObjectNode >()->GetCompiler();
        const ToolManifest & m = n->CastTo< CompilerNode >()->GetManifest();
        if ( m.GetToolId() == toolId )
        {
            // found a job with the same toolid
            return &m;
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
Client::ServerState::ServerState()
    : m_Connection( nullptr )
    , m_CurrentMessage( nullptr )
    , m_ConnectionRetryIntervalSec( 0 )  // try immediately the first time
    , m_ConnectionRetryElapsedTimeSec( 0 )
    , m_NumJobsAvailableForWorker( 0 )
    , m_Jobs( 16, true )
    , m_Blacklisted( false )
    , m_Excluded( false )
    , m_DetailedLogging( false )
{
}

// DESTRUCTOR( ServerState )
//------------------------------------------------------------------------------
Client::ServerState::~ServerState()
{
    // no need to get mutex since in dtor
    ClearConnectionFields();
    if ( !m_RemoteName.IsEmpty() )
    {
        m_ConnectionRetryIntervalSec = 0;
        m_ConnectionRetryElapsedTimeSec = 0;
        m_RemoteName.Clear();
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
        DIST_INFO( "Disconnected: %s\n", m_RemoteName.Get() );
        if ( m_Jobs.IsEmpty() == false )
        {
            Job ** it = m_Jobs.Begin();
            const Job * const * end = m_Jobs.End();
            while ( it != end )
            {
                FLOG_MONITOR( "FINISH_JOB TIMEOUT %s \"%s\" \n", m_RemoteName.Get(), (*it)->GetNode()->GetName().Get() );
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
