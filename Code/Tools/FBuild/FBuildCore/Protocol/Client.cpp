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
#include "Tools/FBuild/FBuildCore/Helpers/BuildProfiler.h"
#include <Tools/FBuild/FBuildCore/Helpers/MultiBuffer.h>
#include "Tools/FBuild/FBuildCore/WorkerPool/Job.h"
#include "Tools/FBuild/FBuildCore/WorkerPool/JobQueue.h"

#include "Core/Env/ErrorFormat.h"
#include "Core/FileIO/ConstMemoryStream.h"
#include "Core/FileIO/MemoryStream.h"
#include "Core/Math/Random.h"
#include "Core/Process/Atomic.h"
#include "Core/Profile/Profile.h"

// Defines
//------------------------------------------------------------------------------
#define CLIENT_STATUS_UPDATE_FREQUENCY_SECONDS ( 0.1f )
#define CONNECTION_REATTEMPT_DELAY_TIME ( 10.0f )
#define SYSTEM_ERROR_ATTEMPT_COUNT ( 3 )
#define DIST_INFO( ... ) do { if ( m_DetailedLogging ) { FLOG_OUTPUT( __VA_ARGS__ ); } } while( false )

// CONSTRUCTOR
//------------------------------------------------------------------------------
Client::Client( const Array< AString > & workerList,
                uint16_t port,
                uint32_t workerConnectionLimit,
                bool detailedLogging )
    : m_WorkerList( workerList )
    , m_ShouldExit( false )
    , m_DetailedLogging( detailedLogging )
    , m_WorkerConnectionLimit( workerConnectionLimit )
    , m_Port( port )
{
    // allocate space for server states
    m_ServerList.SetSize( workerList.GetSize() );

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
    PROFILE_FUNCTION;

    SetShuttingDown();

    m_ShouldExit.Store( true );
    Thread::WaitForThread( m_Thread );

    ShutdownAllConnections();

    Thread::CloseHandle( m_Thread );
}

//------------------------------------------------------------------------------
/*virtual*/ void Client::OnDisconnected( const ConnectionInfo * connection )
{
    ASSERT( connection );
    ServerState * ss = (ServerState *)connection->GetUserData();
    ASSERT( ss );

    MutexHolder mh( ss->m_Mutex );
    DIST_INFO( "Disconnected: %s\n", ss->m_RemoteName.Get() );
    if ( ss->m_Jobs.IsEmpty() == false )
    {
        Job ** it = ss->m_Jobs.Begin();
        const Job * const * end = ss->m_Jobs.End();
        while ( it != end )
        {
            FLOG_MONITOR( "FINISH_JOB TIMEOUT %s \"%s\" \n", ss->m_RemoteName.Get(), (*it)->GetNode()->GetName().Get() );
            JobQueue::Get().ReturnUnfinishedDistributableJob( *it );
            ++it;
        }
        ss->m_Jobs.Clear();
    }

    // This is usually null here, but might need to be freed if
    // we had the connection drop between message and payload
    FREE( (void *)( ss->m_CurrentMessage ) );

    ss->m_RemoteName.Clear();
    AtomicStoreRelaxed( &ss->m_Connection, static_cast< const ConnectionInfo * >( nullptr ) );
    ss->m_CurrentMessage = nullptr;
}

// ThreadFuncStatic
//------------------------------------------------------------------------------
/*static*/ uint32_t Client::ThreadFuncStatic( void * param )
{
    PROFILE_SET_THREAD_NAME( "ClientThread" );

    Client * c = (Client *)param;
    c->ThreadFunc();
    return 0;
}

// ThreadFunc
//------------------------------------------------------------------------------
void Client::ThreadFunc()
{
    PROFILE_FUNCTION;

    // ensure first status update will be sent more rapidly
    m_StatusUpdateTimer.Start();

    for ( ;; )
    {
        LookForWorkers();
        if ( m_ShouldExit.Load() )
        {
            break;
        }

        CommunicateJobAvailability();
        if ( m_ShouldExit.Load() )
        {
            break;
        }

        Thread::Sleep( 1 );
        if ( m_ShouldExit.Load() )
        {
            break;
        }
    }
}

// LookForWorkers
//------------------------------------------------------------------------------
void Client::LookForWorkers()
{
    PROFILE_FUNCTION;

    MutexHolder mh( m_ServerListMutex );

    const size_t numWorkers( m_ServerList.GetSize() );

    // find out how many connections we have now
    size_t numConnections = 0;
    for ( size_t i=0; i<numWorkers; i++ )
    {
        if ( AtomicLoadRelaxed( &m_ServerList[ i ].m_Connection ) )
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

    // randomize the start index to better distribute workers when there
    // are many workers/clients - otherwise all clients will attempt to connect
    // to the same subset of workers
    Random r;
    const size_t startIndex = r.GetRandIndex( (uint32_t)numWorkers );

    // find someone to connect to
    for ( size_t j=0; j<numWorkers; j++ )
    {
        const size_t i( ( j + startIndex ) % numWorkers );

        ServerState & ss = m_ServerList[ i ];
        if ( AtomicLoadRelaxed( &ss.m_Connection ) )
        {
            continue;
        }

        // ignore deny listed workers
        if ( ss.m_Denylisted )
        {
            continue;
        }

        // lock the server state
        MutexHolder mhSS( ss.m_Mutex );

        ASSERT( ss.m_Jobs.IsEmpty() );

        if ( ss.m_DelayTimer.GetElapsed() < CONNECTION_REATTEMPT_DELAY_TIME )
        {
            continue;
        }

        DIST_INFO( "Connecting to: %s\n", m_WorkerList[ i ].Get() );
        const ConnectionInfo * ci = Connect( m_WorkerList[ i ], m_Port, 2000, &ss ); // 2000ms connection timeout
        if ( ci == nullptr )
        {
            DIST_INFO( " - connection: %s (FAILED)\n", m_WorkerList[ i ].Get() );
            ss.m_DelayTimer.Start(); // reset connection attempt delay
        }
        else
        {
            DIST_INFO( " - connection: %s (OK)\n", m_WorkerList[ i ].Get() );
            const uint32_t numJobsAvailable = (uint32_t)JobQueue::Get().GetNumDistributableJobsAvailable();

            ss.m_RemoteName = m_WorkerList[ i ];
            AtomicStoreRelaxed( &ss.m_Connection, ci ); // success!
            ss.m_NumJobsAvailable = numJobsAvailable;

            // send connection msg
            const Protocol::MsgConnection msg( numJobsAvailable );
            SendMessageInternal( ci, msg );
        }

        // limit to one connection attempt per iteration
        return;
    }
}

// CommunicateJobAvailability
//------------------------------------------------------------------------------
void Client::CommunicateJobAvailability()
{
    PROFILE_FUNCTION;

    // We send updates periodically as a baseline, but other events can result
    // us sending extra update messages
    const bool timerExpired = ( m_StatusUpdateTimer.GetElapsed() >= CLIENT_STATUS_UPDATE_FREQUENCY_SECONDS );

    // has status changed since we last sent it?
    const uint32_t numJobsAvailable = (uint32_t)JobQueue::Get().GetNumDistributableJobsAvailable();
    const Protocol::MsgStatus msg( numJobsAvailable );

    // Update each server so it knows how many jobs we have available now
    MutexHolder mh( m_ServerListMutex );
    for ( ServerState & ss : m_ServerList )
    {
        // Do we have a connection?
        MutexHolder ssMH( ss.m_Mutex );
        const ConnectionInfo * connection = AtomicLoadRelaxed( &ss.m_Connection );
        if ( connection == nullptr )
        {
            continue; // no connection
        }

        // Update the worker periodically (but only if the state has changed)
        bool sendAvailabilityToWorker = timerExpired && ( ss.m_NumJobsAvailable != numJobsAvailable );

        // Update worker when jobs become available if there were no jobs available,
        // even if the periodic update timer has not expired. This creates more traffic,
        // but significantly reduces job scheduling latency when:
        //  - a) A build starts, if workers connect before he first jobs become available.
        // OR
        //  - b) During builds, after any period of worker starvation (having zero jobs available
        //       and jobs then becoming available)
        //
        // In both cases, we avoid upto CLIENT_STATUS_UPDATE_FREQUENCY_SECONDS of latency
        if ( numJobsAvailable && ( ss.m_NumJobsAvailable == 0 ) )
        {
            sendAvailabilityToWorker = true;
        }

        if ( sendAvailabilityToWorker )
        {
            PROFILE_SECTION( "UpdateJobAvailability" );
            SendMessageInternal( connection, msg );
            ss.m_NumJobsAvailable = numJobsAvailable;
        }
    }

    // Restart periodic update timer if needed
    if ( timerExpired )
    {
        m_StatusUpdateTimer.Start();
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
                ((ServerState *)connection->GetUserData())->m_RemoteName.Get(),
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
                ((ServerState *)connection->GetUserData())->m_RemoteName.Get(),
                (uint32_t)msg.GetType(),
                msg.GetSize(),
                (uint32_t)memoryStream.GetSize() );
}

// SendMessageInternal
//------------------------------------------------------------------------------
void Client::SendMessageInternal( const ConnectionInfo * connection, const Protocol::IMessage & msg, const ConstMemoryStream & memoryStream )
{
    if ( msg.Send( connection, memoryStream ) )
    {
        return;
    }

    DIST_INFO( "Send Failed: %s (Type: %u, Size: %u, Payload: %u)\n",
                ((ServerState *)connection->GetUserData())->m_RemoteName.Get(),
                (uint32_t)msg.GetType(),
                msg.GetSize(),
                (uint32_t)memoryStream.GetSize() );
}

// OnReceive
//------------------------------------------------------------------------------
/*virtual*/ void Client::OnReceive( const ConnectionInfo * connection, void * data, uint32_t size, bool & keepMemory )
{
    keepMemory = true; // we'll take care of freeing the memory

    ServerState * ss = (ServerState *)connection->GetUserData();
    ASSERT( ss );

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
    const Protocol::MessageType messageType = imsg->GetType();

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
        case Protocol::MSG_JOB_RESULT_COMPRESSED:
        {
            const Protocol::MsgJobResultCompressed * msg = static_cast< const Protocol::MsgJobResultCompressed * >( imsg );
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
    PROFILE_SECTION( "MsgRequestJob" );

    ServerState * ss = (ServerState *)connection->GetUserData();
    ASSERT( ss );

    // no jobs for deny listed workers
    if ( ss->m_Denylisted )
    {
        MutexHolder mh( ss->m_Mutex );
        const Protocol::MsgNoJobAvailable msg;
        SendMessageInternal( connection, msg );
        return;
    }

    Job * job = JobQueue::Get().GetDistributableJobToProcess( true );
    if ( job == nullptr )
    {
        PROFILE_SECTION( "NoJob" );
        // tell the client we don't have anything right now
        // (we completed or gave away the job already)
        MutexHolder mh( ss->m_Mutex );
        const Protocol::MsgNoJobAvailable msg;
        SendMessageInternal( connection, msg );
        return;
    }

    // send the job to the client
    MemoryStream stream;
    job->Serialize( stream );

    MutexHolder mh( ss->m_Mutex );

    ss->m_Jobs.Append( job ); // Track in-flight job

    // Reset the Available Jobs count for this worker. This ensures that we send
    // another status update message to communicate new jobs becoming available.
    // Without this, we might return to the same count as before requesting the
    // current job, resuling in a missed update message.
    ss->m_NumJobsAvailable = 0;

    // if tool is explicity specified, get the id of the tool manifest
    const Node * n = job->GetNode()->CastTo< ObjectNode >()->GetCompiler();
    const ToolManifest & manifest = n->CastTo< CompilerNode >()->GetManifest();
    const uint64_t toolId = manifest.GetToolId();
    ASSERT( toolId );

    // output to signify remote start
    if ( FBuild::Get().GetOptions().m_ShowCommandSummary )
    {
        FLOG_OUTPUT( "-> Obj: %s <REMOTE: %s>\n", job->GetNode()->GetName().Get(), ss->m_RemoteName.Get() );
    }
    FLOG_MONITOR( "START_JOB %s \"%s\" \n", ss->m_RemoteName.Get(), job->GetNode()->GetName().Get() );

    // Determine compression level we'd like the Server to use for returning the results
    int16_t resultCompressionLevel = -1; // Default compression level
    if ( FBuild::IsValid() )
    {
        // If we will write the results to the cache, and this node is cacheable
        // then we want to respect higher cache compression levels if set
        const int16_t cacheCompressionLevel = FBuild::Get().GetOptions().m_CacheCompressionLevel;
        if ( ( cacheCompressionLevel != 0 ) && 
             ( FBuild::Get().GetOptions().m_UseCacheWrite ) && 
             ( job->GetNode()->CastTo< ObjectNode >()->ShouldUseCache() ) )
        {
            resultCompressionLevel = Math::Max( resultCompressionLevel, cacheCompressionLevel );
        }
    }
    
    // Take note of the results compression level so we know to expect
    // compressed results
    job->SetResultCompressionLevel( resultCompressionLevel );

    {
        PROFILE_SECTION( "SendJob" );
        const Protocol::MsgJob msg( toolId, resultCompressionLevel );
        SendMessageInternal( connection, msg, stream );
    }
}

// Process( MsgJobResult )
//------------------------------------------------------------------------------
void Client::Process( const ConnectionInfo * connection, const Protocol::MsgJobResult * /*msg*/, const void * payload, size_t payloadSize )
{
    PROFILE_SECTION( "MsgJobResult" );
    const bool compressed = false;
    ProcessJobResultCommon( connection, compressed, payload, payloadSize );
}

// Process( MsgJobResultCompressed )
//------------------------------------------------------------------------------
void Client::Process( const ConnectionInfo * connection, const Protocol::MsgJobResultCompressed * /*msg*/, const void * payload, size_t payloadSize )
{
    PROFILE_SECTION( "MsgJobResultCompressed" );
    const bool compressed = true;
    ProcessJobResultCommon( connection, compressed, payload, payloadSize );
}

// ProcessJobResultCommon
//------------------------------------------------------------------------------
void Client::ProcessJobResultCommon( const ConnectionInfo * connection, bool isCompressed, const void * payload, size_t payloadSize )
{
    // Take note of the current time. We'll consider the job to have completed at this time.
    // Doing it as soon as possible makes it more accurate, as work below can take a non-trivial
    // amount of time. (For example OnReturnRemoteJob when cancelling the local job in a race)
    const int64_t receivedResultEndTime = Timer::GetNow();

    // find server
    ServerState * ss = (ServerState *)connection->GetUserData();
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
    
    uint16_t remoteThreadId = 0;
    ms.Read( remoteThreadId );

    // get result data (built data or errors if failed)
    uint32_t dataSize = 0;
    ms.Read( dataSize );
    const void * data = (const char *)ms.GetData() + ms.Tell();

    {
        MutexHolder mh( ss->m_Mutex );
        VERIFY( ss->m_Jobs.FindDerefAndErase( jobId ) );
    }

    // Has the job been cancelled in the interim?
    // (Due to a Race by the main thread for example)
    bool raceLost = false;
    bool raceWon = false;
    const Node* node = nullptr;
    uint32_t jobSystemErrorCount = 0;
    Job * job = JobQueue::Get().OnReturnRemoteJob( jobId,
                                                   systemError,
                                                   raceLost, // Set by OnReturnRemoteJob
                                                   raceWon, // Set by OnReturnRemoteJob
                                                   node, // Set by OnReturnRemoteJob
                                                   jobSystemErrorCount ); // Set by OnReturnRemoteJob

    // Prepare failure output if needed
    AStackString< 8192 > failureOutput;
    if ( result == false )
    {
        failureOutput.Format( "PROBLEM: %s\n", node->GetName().Get() );
        for ( const AString & message : messages )
        {
            // When invoked from MSBuild (directly or from Visual Studio) and
            // using -distverbose we might output a remote error string. MSBuild
            // uses regexs to pattern match these strings and force an error
            // which we don't want. So we "clean" these strings to tweak the
            // message slightly to avoid that
            AStackString<> messageCleaned;
            Node::CleanMessageToPreventMSBuildFailure( message, messageCleaned );
            failureOutput += messageCleaned;
        }
    }

    // For system failures, mark worker so no more jobs are scheduled to it
    if ( systemError )
    {
        ss->m_Denylisted = true;

        // -distverbose message
        const size_t workerIndex = (size_t)( ss - m_ServerList.Begin() );
        const AString & workerName = m_WorkerList[ workerIndex ];
        DIST_INFO( "Remote System Failure!\n"
                    " - Deny listed Worker: %s\n"
                    " - Node              : %s\n"
                    " - Job Error Count   : %u / %u\n"
                    " - Details           :\n"
                    "%s",
                    workerName.Get(),
                    node->GetName().Get(),
                    jobSystemErrorCount, SYSTEM_ERROR_ATTEMPT_COUNT,
                    failureOutput.Get() );
    }

    // Handle build profiling output
    if ( BuildProfiler::IsValid() )
    {
        // Chose description.
        // NOTE:
        // * String lifetime must extend past BuildProfiler destruction
        // * String contents feeds into color selection for profiling json
        const char* resultStr = "Compile";
        if ( systemError )
        {
            if ( raceWon )       { resultStr = "(System Failure) (Race Won) Compile"; }
            else if ( raceLost ) { resultStr = "(System Failure) (Race Lost) Compile"; }
            else                 { resultStr = "(System Failure) Compile"; }
        }
        else if ( !result )
        {
            if ( raceWon )       { resultStr = "(Failure) (Race Won) Compile"; }
            else if ( raceLost ) { resultStr = "(Failure) (Race Lost) Compile"; }
            else                 { resultStr = "(Failure) Compile"; }
        }

        // Record information about worker
        const uint32_t workerId = static_cast<uint32_t>( ss - m_ServerList.Begin() );
        const int64_t start = receivedResultEndTime - (int64_t)( ( (double)buildTime / 1000 ) * (double)Timer::GetFrequency() );
        BuildProfiler::Get().RecordRemote( workerId,
                                           ss->m_RemoteName,
                                           remoteThreadId,
                                           start,
                                           receivedResultEndTime,
                                           resultStr,
                                           node->GetName().Get());
    }

    // Handle verbose logging
    if ( m_DetailedLogging )
    {
        const char* resultStr = "";
        if ( systemError )
        {
            if ( raceWon )       { resultStr = " (System Failure) (Race Won)"; }
            else if ( raceLost ) { resultStr = " (System Failure) (Race Lost)"; }
            else                 { resultStr = " (System Failure)"; }
        }
        else if ( !result )
        {
            if ( raceWon )       { resultStr = " (Failure) (Race Won)"; }
            else if ( raceLost ) { resultStr = " (Failure) (Race Lost)"; }
            else                 { resultStr = " (Failure)"; }
        }
        DIST_INFO( "Got Result: %s - %s%s\n", ss->m_RemoteName.Get(),
                                              node->GetName().Get(),
                                              resultStr );
    }

    if ( FLog::IsMonitorEnabled() )
    {
        AStackString<> msgBuffer;
        Job::GetMessagesForMonitorLog( messages, msgBuffer );

        FLOG_MONITOR( "FINISH_JOB %s %s \"%s\" \"%s\"\n",
                      result ? "SUCCESS" : "ERROR",
                      ss->m_RemoteName.Get(),
                      node->GetName().Get(),
                      msgBuffer.Get() );
    }

    // Should remote job be discarded?
    // Can happen in cases such as:
    //   a) local job won a race
    //   b) local race started and remote job was a system failure
    if ( job == nullptr )
    {
        // don't save result as we were cancelled
        return;
    }

    job->SetMessages( messages );

    if ( result == true )
    {
        // built ok - serialize to disc
        
        ObjectNode * objectNode = node->CastTo< ObjectNode >();

        // Store to cache if needed
        const bool writeToCache = FBuild::Get().GetOptions().m_UseCacheWrite &&
                                  objectNode->ShouldUseCache();
        if ( writeToCache )
        {
            if ( isCompressed )
            {
                // Write already compressed result to cache
                objectNode->WriteToCache_FromCompressedData( job,
                                                             data,
                                                             dataSize,
                                                             0 ); // compression time is remote and unknown
            }
            else
            {
                // Compress and write result to cache
                objectNode->WriteToCache_FromUncompressedData( job, data, dataSize );
            }
        }

        // Decompress if needed
        MultiBuffer mb( data, dataSize );
        if ( isCompressed )
        {
            mb.Decompress();
        }

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
            }
            else
            {
                objectNode->SetStatFlag( Node::STATS_FAILED );
            }
        }

        // get list of messages during remote work
        AStackString<> msgBuffer;
        job->GetMessagesForLog( msgBuffer );

        if ( objectNode->IsMSVC())
        {
            if ( objectNode->IsWarningsAsErrorsMSVC() == false )
            {
                FileNode::HandleWarningsMSVC( job, objectNode->GetName(), msgBuffer );
            }
        }
        else if ( objectNode->IsClangCl() )
        {
            if ( objectNode->IsWarningsAsErrorsMSVC() == false )
            {
                FileNode::HandleWarningsClangCl( job, objectNode->GetName(), msgBuffer );
            }
        }
        else if ( objectNode->IsClang() || objectNode->IsGCC() )
        {
            if ( objectNode->IsWarningsAsErrorsClangGCC() == false )
            {
                FileNode::HandleWarningsClangGCC( job, objectNode->GetName(), msgBuffer );
            }
        }
    }
    else
    {
        ((FileNode *)node)->SetStatFlag( Node::STATS_FAILED );

        // was it a system error?
        if ( systemError )
        {
            // should we retry on another worker?
            if ( jobSystemErrorCount < SYSTEM_ERROR_ATTEMPT_COUNT )
            {
                // re-queue job which will be re-attempted on another worker
                JobQueue::Get().ReturnUnfinishedDistributableJob( job );
                return;
            }

            // failed too many times on different workers, add info about this to
            // error output
            if ( failureOutput.EndsWith( '\n' ) == false )
            {
                failureOutput += '\n';
            }
            failureOutput.AppendFormat( "FBuild: Error: Task failed on %u different workers\n", (uint32_t)SYSTEM_ERROR_ATTEMPT_COUNT );
        }

        Node::DumpOutput( nullptr, failureOutput, nullptr );
    }

    JobQueue::Get().FinishedProcessingJob( job, result, true ); // remote job
}

// Process( MsgRequestManifest )
//------------------------------------------------------------------------------
void Client::Process( const ConnectionInfo * connection, const Protocol::MsgRequestManifest * msg )
{
    PROFILE_SECTION( "MsgRequestManifest" );

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
    const Protocol::MsgManifest resultMsg( toolId );
    MutexHolder mh( static_cast<ServerState *>(connection->GetUserData())->m_Mutex );
    SendMessageInternal( connection, resultMsg, ms );
}

// Process ( MsgRequestFile )
//------------------------------------------------------------------------------
void Client::Process( const ConnectionInfo * connection, const Protocol::MsgRequestFile * msg )
{
    PROFILE_SECTION( "MsgRequestFile" );

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
    const Protocol::MsgFile resultMsg( toolId, fileId );
    MutexHolder mh( static_cast<ServerState *>(connection->GetUserData())->m_Mutex );
    SendMessageInternal( connection, resultMsg, ms );
}

// FindManifest
//------------------------------------------------------------------------------
const ToolManifest * Client::FindManifest( const ConnectionInfo * connection, uint64_t toolId ) const
{
    ServerState * ss = (ServerState *)connection->GetUserData();
    ASSERT( ss );

    MutexHolder mh( ss->m_Mutex );

    for ( Job ** it = ss->m_Jobs.Begin();
          it != ss->m_Jobs.End();
          ++it )
    {
        const Node * n = ( *it )->GetNode()->CastTo< ObjectNode >()->GetCompiler();
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
    , m_NumJobsAvailable( 0 )
    , m_Jobs( 16, true )
    , m_Denylisted( false )
{
    m_DelayTimer.Start( 999.0f );
}

//------------------------------------------------------------------------------
