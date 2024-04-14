// JobQueueRemote
//------------------------------------------------------------------------------

// Includes
//------------------------------------------------------------------------------
#include "JobQueueRemote.h"
#include "Job.h"
#include "WorkerThreadRemote.h"

#include "Tools/FBuild/FBuildCore/FBuild.h"
#include "Tools/FBuild/FBuildCore/FLog.h"
#include "Tools/FBuild/FBuildCore/Graph/Node.h"
#include "Tools/FBuild/FBuildCore/Graph/ObjectNode.h"
#include "Tools/FBuild/FBuildCore/Helpers/BuildProfiler.h"
#include "Tools/FBuild/FBuildCore/Helpers/MultiBuffer.h"

// Core
#include "Core/Env/ErrorFormat.h"
#include "Core/FileIO/FileIO.h"
#include "Core/FileIO/FileStream.h"
#include "Core/FileIO/PathUtils.h"
#include "Core/Process/ThreadPool.h"
#include "Core/Profile/Profile.h"
#include "Core/Time/Timer.h"
#include "Core/Tracing/Tracing.h"

// CONSTRUCTOR
//------------------------------------------------------------------------------
JobQueueRemote::JobQueueRemote( uint32_t numWorkerThreads ) :
    m_PendingJobs( 1024 ),
    m_CompletedJobs( 1024 ),
    m_CompletedJobsFailed( 64 ),
    m_CompletedJobsAborted( 64 ),
    m_Workers( numWorkerThreads )
{
    WorkerThread::InitTmpDir( true ); // remote == true

    // Create thread pool
    m_ThreadPool = FNEW( ThreadPool( numWorkerThreads ) );

    // Create a job to run on each thread
    for ( uint32_t i=0; i<numWorkerThreads; ++i )
    {
        // identify each worker with an id starting from 1
        // (the "main" thread is considered 0)
        const uint16_t threadIndex = static_cast<uint16_t>( i + 1001 );
        WorkerThread * wt = FNEW( WorkerThreadRemote( threadIndex ) );
        wt->Init( m_ThreadPool );
        m_Workers.Append( wt );
    }
}

// DESTRUCTOR
//------------------------------------------------------------------------------
JobQueueRemote::~JobQueueRemote()
{
    // signal all workers to stop - ok if this has already been done
    SignalStopWorkers();

    // wait for workers to finish - ok if they stopped before this
    const size_t numWorkerThreads = m_Workers.GetSize();
    for ( size_t i=0; i<numWorkerThreads; ++i )
    {
        m_Workers[ i ]->WaitForStop();
        FDELETE m_Workers[ i ];
    }

    FDELETE m_ThreadPool;
}

// SignalStopWorkers (Main Thread)
//------------------------------------------------------------------------------
void JobQueueRemote::SignalStopWorkers()
{
    const size_t numWorkerThreads = m_Workers.GetSize();
    for ( size_t i=0; i<numWorkerThreads; ++i )
    {
        m_Workers[ i ]->Stop();
    }

    // Signal threads (both active and idle)
    // (We don't know which threads are in any given state, so we signal
    // the worst case for both states)
    m_WorkerThreadSemaphore.Signal( static_cast<uint32_t>(numWorkerThreads) );
    m_WorkerThreadSleepSemaphore.Signal( static_cast<uint32_t>(numWorkerThreads) );
}

// HaveWorkersStopped
//------------------------------------------------------------------------------
bool JobQueueRemote::HaveWorkersStopped() const
{
    const size_t numWorkerThreads = m_Workers.GetSize();
    for ( size_t i=0; i<numWorkerThreads; ++i )
    {
        if ( m_Workers[ i ]->HasExited() == false )
        {
            return false;
        }
    }
    return true;
}

// GetWorkerStatus
//------------------------------------------------------------------------------
void JobQueueRemote::GetWorkerStatus( size_t index, AString & hostName, AString & status, bool & isIdle ) const
{
    ( (WorkerThreadRemote *)m_Workers[ index ] )->GetStatus( hostName, status, isIdle );
}

// MainThreadWait
//------------------------------------------------------------------------------
void JobQueueRemote::MainThreadWait( uint32_t timeoutMS )
{
    PROFILE_SECTION( "MainThreadWait" );
    m_MainThreadSemaphore.Wait( timeoutMS );
}

// WakeMainThread
//------------------------------------------------------------------------------
void JobQueueRemote::WakeMainThread()
{
    m_MainThreadSemaphore.Signal();
}

// WorkerThreadWait
//------------------------------------------------------------------------------
void JobQueueRemote::WorkerThreadWait()
{
    PROFILE_SECTION( "WorkerThreadWait" );
    m_WorkerThreadSemaphore.Wait( 1000 );
}

// WorkerThreadSleep
//------------------------------------------------------------------------------
void JobQueueRemote::WorkerThreadSleep()
{
    PROFILE_SECTION( "WorkerThreadSleep" );
    m_WorkerThreadSleepSemaphore.Wait( 1000 );
}

// QueueJob (Main Thread)
//------------------------------------------------------------------------------
void JobQueueRemote::QueueJob( Job * job )
{
    {
        MutexHolder m( m_PendingJobsMutex );
        m_PendingJobs.Append( job );
    }

    // Wake a single non-idle worker thread
    m_WorkerThreadSemaphore.Signal( 1 );
}

// GetCompletedJob
//------------------------------------------------------------------------------
Job * JobQueueRemote::GetCompletedJob( Node::BuildResult & outResult )
{
    MutexHolder m( m_CompletedJobsMutex );

    // completed jobs
    if ( !m_CompletedJobs.IsEmpty() )
    {
        Job * job = m_CompletedJobs[ 0 ];
        m_CompletedJobs.PopFront();
        job->GetNode()->SetState( Node::UP_TO_DATE );
        outResult = Node::BuildResult::eOk;
        return job;
    }

    // failed jobs
    if ( !m_CompletedJobsFailed.IsEmpty() )
    {
        Job * job = m_CompletedJobsFailed[ 0 ];
        m_CompletedJobsFailed.PopFront();
        job->GetNode()->SetState( Node::FAILED );
        outResult = Node::BuildResult::eFailed;
        return job;
    }

    // aborted jobs
    if ( !m_CompletedJobsAborted.IsEmpty() )
    {
        Job * job = m_CompletedJobsAborted[ 0 ];
        m_CompletedJobsAborted.PopFront();
        outResult = Node::BuildResult::eAborted;
        return job;
    }

    return nullptr;
}

// CancelJobsWithUserData
//------------------------------------------------------------------------------
void JobQueueRemote::CancelJobsWithUserData( void * userData )
{
    // delete queued jobs
    {
        MutexHolder m( m_PendingJobsMutex );
        Job ** it = m_PendingJobs.Begin();
        while ( it != m_PendingJobs.End() )
        {
            if ( ( *it )->GetUserData() == userData )
            {
                FDELETE *it;
                m_PendingJobs.Erase( it );
                continue;
            }
            ++it;
        }
    }

    // delete completed jobs
    {
        MutexHolder m( m_CompletedJobsMutex );
        Job ** it = m_CompletedJobs.Begin();
        while ( it != m_CompletedJobs.End() )
        {
            if ( ( *it )->GetUserData() == userData )
            {
                FDELETE *it;
                m_CompletedJobs.Erase( it );
                continue;
            }
            ++it;
        }
    }

    // unhook in-flight jobs
    // (we can't delete these now, so we let them complete and delete
    // them upon completion - see FinishedProcessingJob)
    MutexHolder mh( m_InFlightJobsMutex );
    for ( Job * job : m_InFlightJobs )
    {
        if ( job->GetUserData() == userData )
        {
            job->SetUserData( nullptr );
        }
    }
}

// GetJobToProcess (Worker Thread)
//------------------------------------------------------------------------------
Job * JobQueueRemote::GetJobToProcess()
{
    WorkerThreadWait();

    MutexHolder m( m_PendingJobsMutex );
    if ( m_PendingJobs.IsEmpty() )
    {
        return nullptr;
    }

    // building jobs in the order they are queued
    Job * job = m_PendingJobs[ 0 ];
    m_PendingJobs.PopFront();

    MutexHolder mh( m_InFlightJobsMutex );
    m_InFlightJobs.Append( job );

    return job;
}

// FinishedProcessingJob (Worker Thread)
//------------------------------------------------------------------------------
void JobQueueRemote::FinishedProcessingJob( Job * job, Node::BuildResult result )
{
    // remove from in-flight
    {
        MutexHolder mh( m_InFlightJobsMutex );
        Job ** it = m_InFlightJobs.Find( job );
        ASSERT( it != nullptr );
        m_InFlightJobs.Erase( it );
    }

    // handle jobs which were cancelled while in flight
    if ( job->GetUserData() == nullptr )
    {
        FDELETE job;
        return;
    }

    // push to appropriate completion queue
    {
        MutexHolder m( m_CompletedJobsMutex );
        switch ( result )
        {
            case Node::BuildResult::eFailed:            m_CompletedJobsFailed.Append( job ); break;
            case Node::BuildResult::eAborted:           m_CompletedJobsAborted.Append( job );break;
            case Node::BuildResult::eNeedSecondPass:    ASSERT( false ); break;
            case Node::BuildResult::eOk:                m_CompletedJobs.Append( job ); break;
        }
    }

    WakeMainThread();
}

// DoBuild
//------------------------------------------------------------------------------
/*static*/ Node::BuildResult JobQueueRemote::DoBuild( Job * job, bool racingRemoteJob )
{
    BuildProfilerScope profileScope( *job, WorkerThread::GetThreadIndex(), job->GetNode()->GetTypeName() );

    const Timer timer; // track how long the item takes

    ObjectNode * node = job->GetNode()->CastTo< ObjectNode >();

    if ( job->IsLocal() )
    {
        FLOG_MONITOR( "START_JOB local \"%s\" \n", job->GetNode()->GetName().Get() );
    }

    // remote tasks must output to a tmp file
    if ( job->IsLocal() == false )
    {
        // file name should be the same as on host
        const char * fileName = ( job->GetRemoteName().FindLast( NATIVE_SLASH ) + 1 );

        AStackString<> tmpFileName;
        WorkerThread::CreateTempFilePath( fileName, tmpFileName );
        node->ReplaceDummyName( tmpFileName );

        //DEBUGSPAM( "REMOTE: %s (%s)\n", fileName, job->GetRemoteName().Get() );
    }

    ASSERT( node->IsAFile() );

    // make sure the output path exists
    if ( Node::EnsurePathExistsForFile( node->GetName() ) == false )
    {
        // error already output by EnsurePathExistsForFile
        return Node::BuildResult::eFailed;
    }

    // Delete any left over PDB from a previous run (to be sure we have a clean pdb)
    if ( node->IsUsingPDB() && ( job->IsLocal() == false ) )
    {
        AStackString<> pdbName;
        node->GetPDBName( pdbName );
        FileIO::FileDelete( pdbName.Get() );
    }

    Node::BuildResult result;
    {
        PROFILE_SECTION( racingRemoteJob ? "RACE" : "LOCAL" );
        result = ((Node *)node )->DoBuild2( job, racingRemoteJob );
    }

    // Ignore result if job was cancelled
    if ( job->GetDistributionState() == Job::DIST_RACE_WON_REMOTELY_CANCEL_LOCAL )
    {
        if ( result == Node::BuildResult::eFailed )
        {
            return Node::BuildResult::eFailed;
        }
    }

    const uint32_t timeTakenMS = uint32_t( timer.GetElapsedMS() );

    switch ( result )
    {
        case Node::BuildResult::eFailed:
        case Node::BuildResult::eAborted:
        {
            // Locally we don't record the build time for failures as we
            // want to keep the last successful build time for job ordering.
            // If building remotely, we want to return the time taken however.
            if ( job->IsLocal() == false )
            {
                node->SetLastBuildTime( timeTakenMS );
            }

            if ( result == Node::BuildResult::eFailed )
            {
                node->SetStatFlag( Node::STATS_FAILED );
            }
            break;
        }
        case Node::BuildResult::eNeedSecondPass:
        {
            ASSERT( false ); // Remote jobs cannot request second passes
            break;
        }
        case Node::BuildResult::eOk:
        {
            // record new build time
            node->SetLastBuildTime( timeTakenMS );
            node->SetStatFlag( Node::STATS_BUILT );

            #ifdef DEBUG
                if ( job->IsLocal() )
                {
                    // we should have recorded the new file time for remote job we built locally
                    ASSERT( node->m_Stamp == FileIO::GetFileLastWriteTime(node->GetName()) );
                }
            #endif


            // TODO:A Also read into job if cache is being used
            if ( job->IsLocal() == false )
            {
                // read results into memory to send back to client
                if ( ReadResults( job ) == false )
                {
                    result = Node::BuildResult::eFailed;
                }
            }
            break;
        }
    }

    // if compiling to a tmp file, do cleanup
    if ( job->IsLocal() == false )
    {
        // Cleanup obj file
        FileIO::FileDelete( node->GetName().Get() );

        // Cleanup PDB file
        if ( node->IsUsingPDB() )
        {
            AStackString<> pdbName;
            node->GetPDBName( pdbName );
            FileIO::FileDelete( pdbName.Get() );
        }
    }

    // log processing time
    node->AddProcessingTime( timeTakenMS );

    if ( job->IsLocal() && FLog::IsMonitorEnabled() )
    {
        AStackString<> msgBuffer;
        job->GetMessagesForMonitorLog( msgBuffer );

        FLOG_MONITOR( "FINISH_JOB %s local \"%s\" \"%s\"\n",
                      ( result == Node::BuildResult::eFailed ) ? "ERROR" : "SUCCESS",
                      job->GetNode()->GetName().Get(),
                      msgBuffer.Get());
    }

    return result;
}

// ReadResults
//------------------------------------------------------------------------------
/*static*/ bool JobQueueRemote::ReadResults( Job * job )
{
    const ObjectNode * node = job->GetNode()->CastTo< ObjectNode >();
    const bool includePDB = node->IsUsingPDB();
    const bool usingStaticAnalysis = node->IsUsingStaticAnalysisMSVC();

    // Determine list of files to send

    // 1. Object file
    //---------------
    StackArray< AString > fileNames;
    fileNames.Append( node->GetName() );

    // 2. PDB file (optional)
    //-----------------------
    if ( includePDB )
    {
        AStackString<> pdbFileName;
        node->GetPDBName( pdbFileName );
        fileNames.Append( pdbFileName );
    }

    // 3. .nativecodeanalysis.xml file (optional)
    //--------------------------------------------
    if ( usingStaticAnalysis )
    {
        AStackString<> xmlFileName;
        node->GetNativeAnalysisXMLPath( xmlFileName );
        fileNames.Append( xmlFileName );
    }

    MultiBuffer mb;
    size_t problemFileIndex = 0;
    if ( !mb.CreateFromFiles( fileNames, &problemFileIndex ) )
    {
        job->Error( "Error reading file: '%s'", fileNames[ problemFileIndex ].Get() );
        FLOG_ERROR( "Error reading file: '%s'", fileNames[ problemFileIndex ].Get() );
    }

    // Compress result
    const int32_t compressionLevel = job->GetResultCompressionLevel();
    if ( compressionLevel != 0 )
    {
        mb.Compress( compressionLevel, job->GetAllowZstdUse() );
    }

    // transfer data to job
    size_t memSize;
    void * mem = mb.Release( memSize );
    job->OwnData( mem, memSize );

    return true;
}

//------------------------------------------------------------------------------
