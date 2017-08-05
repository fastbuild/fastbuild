// JobQueue
//------------------------------------------------------------------------------

// Includes
//------------------------------------------------------------------------------
#include "Tools/FBuild/FBuildCore/PrecompiledHeader.h"

#include "JobQueue.h"
#include "Job.h"
#include "WorkerThread.h"

#include "Tools/FBuild/FBuildCore/FBuild.h"
#include "Tools/FBuild/FBuildCore/FLog.h"
#include "Tools/FBuild/FBuildCore/Graph/Node.h"
#include "Tools/FBuild/FBuildCore/Graph/ObjectNode.h"

#include "Core/Time/Timer.h"
#include "Core/FileIO/FileIO.h"
#include "Core/Process/Atomic.h"
#include "Core/Profile/Profile.h"

// JobCostSorter
//------------------------------------------------------------------------------
class JobCostSorter
{
public:
    inline bool operator () ( const Job * job1, const Job * job2 ) const
    {
        return ( job1->GetNode()->GetRecursiveCost() < job2->GetNode()->GetRecursiveCost() );
    }
};

// JobSubQueue CONSTRUCTOR
//------------------------------------------------------------------------------
JobSubQueue::JobSubQueue()
    : m_Count( 0 )
    , m_Jobs( 1024, true )
{
}

// JobSubQueue DESTRUCTOR
//------------------------------------------------------------------------------
JobSubQueue::~JobSubQueue()
{
    ASSERT( m_Jobs.IsEmpty() );
    ASSERT( m_Count == 0 );
}

// JobSubQueue:QueueJobs
//------------------------------------------------------------------------------
void JobSubQueue::QueueJobs( Array< Node * > & nodes )
{
    // Create wrapper Jobs around Nodes
    Array< Job * > jobs( nodes.GetSize() );
    for ( Node * node : nodes )
    {
        Job * job = FNEW( Job( node ) );
        jobs.Append( job );
    }

    // Sort Jobs by cost
    JobCostSorter sorter;
    jobs.Sort( sorter );

    // lock to add job
    MutexHolder mh( m_Mutex );
    const bool wasEmpty = m_Jobs.IsEmpty();

    m_Jobs.Append( jobs );
    m_Count += (uint32_t)jobs.GetSize();

    if ( wasEmpty )
    {
        return; // skip re-sorting
    }

    // sort merged lists
    m_Jobs.Sort( sorter );
}

// RemoveJob
//------------------------------------------------------------------------------
Job * JobSubQueue::RemoveJob()
{
    // lock-free early out if there are no jobs
    if ( m_Count == 0 )
    {
        return nullptr;
    }

    // lock to remove job
    MutexHolder mh( m_Mutex );

    // possible that job has been removed between job count check and mutex lock
    if ( m_Jobs.IsEmpty() )
    {
        return nullptr;
    }

    ASSERT( m_Count );
    --m_Count;

    Job * job = m_Jobs.Top();
    m_Jobs.Pop();

    return job;
}

// CONSTRUCTOR
//------------------------------------------------------------------------------
JobQueue::JobQueue( uint32_t numWorkerThreads ) :
    m_NumLocalJobsActive( 0 ),
    m_DistributableJobs_Available( 1024, true ),
    m_DistributableJobs_InProgress( 1024, true ),
    m_DistributableJobsMemoryUsage( 0 ),
    m_CompletedJobs( 1024, true ),
    m_CompletedJobsFailed( 1024, true ),
    m_CompletedJobs2( 1024, true ),
    m_CompletedJobsFailed2( 1024, true ),
    m_Workers( numWorkerThreads, false )
{
    PROFILE_FUNCTION

    WorkerThread::InitTmpDir();

    for ( uint32_t i=0; i<numWorkerThreads; ++i )
    {
        // identify each worker with an id starting from 1
        // (the "main" thread is considered 0)
        uint32_t threadIndex = ( i + 1 );
        WorkerThread * wt = FNEW( WorkerThread( threadIndex ) );
        wt->Init();
        m_Workers.Append( wt );
    }
}

// DESTRUCTOR
//------------------------------------------------------------------------------
JobQueue::~JobQueue()
{
    // signal all workers to stop - ok if this has already been done
    SignalStopWorkers();

    // delete incomplete jobs
    while ( Job * job = m_LocalJobs_Available.RemoveJob() )
    {
        DestroyJob( job );
    }

    // wait for workers to finish - ok if they stopped before this
    const size_t numWorkerThreads = m_Workers.GetSize();
    for ( size_t i=0; i<numWorkerThreads; ++i )
    {
        m_Workers[ i ]->WaitForStop();
        FDELETE m_Workers[ i ];
    }

    ASSERT( m_CompletedJobs.IsEmpty() );
    ASSERT( m_CompletedJobsFailed.IsEmpty() );
    ASSERT( m_DistributableJobsMemoryUsage == 0 );
}

// SignalStopWorkers (Main Thread)
//------------------------------------------------------------------------------
void JobQueue::SignalStopWorkers()
{
    const size_t numWorkerThreads = m_Workers.GetSize();
    for ( size_t i=0; i<numWorkerThreads; ++i )
    {
        m_Workers[ i ]->Stop();
    }
    if ( numWorkerThreads > 0 )
    {
        m_WorkerThreadSemaphore.Signal( (uint32_t)numWorkerThreads );
    }
}

// HaveWorkersStopped
//------------------------------------------------------------------------------
bool JobQueue::HaveWorkersStopped() const
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

// GetNumDistributableJobsAvailable
//------------------------------------------------------------------------------
size_t JobQueue::GetNumDistributableJobsAvailable() const
{
    MutexHolder m( m_DistributedJobsMutex );
    return m_DistributableJobs_Available.GetSize();
}

// GetJobStats
//------------------------------------------------------------------------------
void JobQueue::GetJobStats( uint32_t & numJobs,
                            uint32_t & numJobsActive,
                            uint32_t & numJobsDist,
                            uint32_t & numJobsDistActive ) const
{
    MutexHolder m( m_DistributedJobsMutex );

    numJobs = m_LocalJobs_Available.GetCount();
    numJobsDist = (uint32_t)m_DistributableJobs_Available.GetSize();
    numJobsActive = m_NumLocalJobsActive;
    numJobsDistActive = (uint32_t)m_DistributableJobs_InProgress.GetSize();
}

// AddJobToBatch (Main Thread)
//------------------------------------------------------------------------------
void JobQueue::AddJobToBatch( Node * node )
{
    ASSERT( node->GetState() == Node::DYNAMIC_DEPS_DONE );

    // mark as building
    node->SetState( Node::BUILDING );

    // trivial build tasks are processed immediately and returned
    if ( node->GetControlFlags() & Node::FLAG_TRIVIAL_BUILD )
    {
        Job localJob( node );
        Node::BuildResult result = DoBuild( &localJob );
        switch( result )
        {
            case Node::NODE_RESULT_FAILED:  node->SetState( Node::FAILED ); break;
            case Node::NODE_RESULT_OK:      node->SetState( Node::UP_TO_DATE ); break;
            default:                        ASSERT( false ); break;
        }
        return;
    }

    m_LocalJobs_Staging.Append( node );
}

// FlushJobBatch (Main Thread)
//------------------------------------------------------------------------------
void JobQueue::FlushJobBatch()
{
    if ( m_LocalJobs_Staging.IsEmpty() )
    {
        return;
    }

    m_LocalJobs_Available.QueueJobs( m_LocalJobs_Staging );
    m_WorkerThreadSemaphore.Signal( (uint32_t)m_LocalJobs_Staging.GetSize() );
    m_LocalJobs_Staging.Clear();
}

// QueueDistributableJob
//------------------------------------------------------------------------------
void JobQueue::QueueDistributableJob( Job * job )
{
    ASSERT( job->GetNode()->GetState() == Node::BUILDING );
    ASSERT( job->GetDistributionState() == Job::DIST_NONE );

    {
        MutexHolder m( m_DistributedJobsMutex );

        m_DistributableJobs_Available.Append( job );

        job->SetDistributionState( Job::DIST_AVAILABLE );

        // track size of distributable jobs
        m_DistributableJobsMemoryUsage += job->GetDataSize();
    }

    ASSERT( m_NumLocalJobsActive > 0 );
    AtomicDecU32( &m_NumLocalJobsActive ); // job converts from active to pending remote

    m_WorkerThreadSemaphore.Signal();
}

// GetDistributableJobToProcess
//------------------------------------------------------------------------------
Job * JobQueue::GetDistributableJobToProcess( bool remote )
{
    MutexHolder m( m_DistributedJobsMutex );

    if ( m_DistributableJobs_Available.IsEmpty() )
    {
        return nullptr;
    }

    // building jobs in the order they are queued
    Job * job = m_DistributableJobs_Available[ 0 ];
    m_DistributableJobs_Available.PopFront();

    ASSERT( job->GetDistributionState() == Job::DIST_AVAILABLE );

    // Tag job as in-use
    job->SetDistributionState( remote ? Job::DIST_BUILDING_REMOTELY : Job::DIST_BUILDING_LOCALLY );
    m_DistributableJobs_InProgress.Append( job );
    return job;
}

// GetDistributableJobToRace
//------------------------------------------------------------------------------
Job * JobQueue::GetDistributableJobToRace()
{
    MutexHolder m( m_DistributedJobsMutex );
    if ( m_DistributableJobs_InProgress.IsEmpty() )
    {
        return nullptr;
    }

    // take newest job, which is least likely to finish first
    // compared to older distributed jobs
    const int32_t numJobs = (int32_t)m_DistributableJobs_InProgress.GetSize();
    for ( int32_t i = ( numJobs - 1 ); i >= 0; --i )
    {
        Job * job = m_DistributableJobs_InProgress[i];

        // Don't Race jobs already building locally
        const Job::DistributionState distState = job->GetDistributionState();
        if ( distState == Job::DIST_BUILDING_REMOTELY )
        {
            job->SetDistributionState( Job::DIST_RACING );
            return job;
        }

        ASSERT( ( distState == Job::DIST_BUILDING_LOCALLY ) ||
                ( distState == Job::DIST_RACING ) ||
                ( distState == Job::DIST_RACE_WON_LOCALLY ) ||
                ( distState == Job::DIST_COMPLETED_REMOTELY ) );
    }

    return nullptr; // No job found to race (all were local or races already)
}

// OnReturnRemoteJob
//------------------------------------------------------------------------------
Job * JobQueue::OnReturnRemoteJob( uint32_t jobId )
{
    MutexHolder m( m_DistributedJobsMutex );
    for ( Job * job : m_DistributableJobs_InProgress )
    {
        // Find the matching job
        if ( job->GetJobId() != jobId )
        {
            continue; // Not the right job
        }

        // What state is the job in?
        const Job::DistributionState distState = job->GetDistributionState();

        // Standard remote build?
        if ( distState == Job::DIST_BUILDING_REMOTELY )
        {
            job->SetDistributionState( Job::DIST_COMPLETED_REMOTELY );
            return job;
        }

        // Did a local race complete this already?
        if ( distState == Job::DIST_RACE_WON_LOCALLY )
        {
            VERIFY( m_DistributableJobs_InProgress.FindAndErase( job ) );
            DestroyJob( job );
            return nullptr;
        }

        // Are we still locally racing?
        if ( distState == Job::DIST_RACING )
        {
            // Ignore the remote job (we always take the local race)
            // TODO:B Cancel the local job and take the remote result
            job->SetDistributionState( Job::DIST_BUILDING_LOCALLY );
            return nullptr;
        }

        ASSERT( false ); // Job in unexpected state
    }

    ASSERT( false ); // Job not found - should be impossible
    return nullptr;
}

// ReturnUnfinishedDistributableJob
//------------------------------------------------------------------------------
void JobQueue::ReturnUnfinishedDistributableJob( Job * job )
{
    {
        MutexHolder m( m_DistributedJobsMutex );

        // Are we locally racing?
        if ( job->GetDistributionState() == Job::DIST_RACING )
        {
            // No longer racing
            job->SetDistributionState( Job::DIST_BUILDING_LOCALLY );
            return;
        }

        // Remove from in progress (keep order)
        VERIFY( m_DistributableJobs_InProgress.FindAndErase( job ) );

        // Did a local race complete?
        if ( job->GetDistributionState() == Job::DIST_RACE_WON_LOCALLY )
        {
            // Job locally completed, and we no longer reference it so it can be freed
            DestroyJob( job );
            return;
        }
        else
        {
            // If not racing, only standard remote build is valid
            ASSERT( job->GetDistributionState() == Job::DIST_BUILDING_REMOTELY );

            // Put back in available queue
            m_DistributableJobs_Available.Append( job );
            job->SetDistributionState( Job::DIST_AVAILABLE );
        }
    }

    // Signal local threads that new work is available
    m_WorkerThreadSemaphore.Signal();
}

// FinalizeCompletedJobs (Main Thread)
//------------------------------------------------------------------------------
void JobQueue::FinalizeCompletedJobs( NodeGraph & nodeGraph )
{
    PROFILE_FUNCTION

    {
        MutexHolder m( m_CompletedJobsMutex );
        m_CompletedJobs2.Swap( m_CompletedJobs );
        m_CompletedJobsFailed2.Swap( m_CompletedJobsFailed );
    }

    // completed jobs
    for ( Job * job : m_CompletedJobs2 )
    {
        Node * n = job->GetNode();
        if ( n->Finalize( nodeGraph ) )
        {
            n->SetState( Node::UP_TO_DATE );
        }
        else
        {
            n->SetState( Node::FAILED );
        }

        // Free normal jobs
        if ( job->GetDistributionState() == Job::DIST_NONE )
        {
            DestroyJob( job );
            continue;
        }

        // Distributed jobs
        {
            MutexHolder mh( m_DistributedJobsMutex );

            const Job::DistributionState distState = job->GetDistributionState();

            // Normal local or remote compilation of distributable job?
            if ( ( distState == Job::DIST_COMPLETED_LOCALLY ) || ( distState == Job::DIST_COMPLETED_REMOTELY ) )
            {
                DestroyJob( job );
                continue;
            }

            // Local race, won locally (we don't support remote wins at the moment)
            ASSERT( distState == Job::DIST_RACING );
            job->SetDistributionState( Job::DIST_RACE_WON_LOCALLY );

            // We can't delete the job yet, because it's still in use by the remote
            // job. It will be freed when the remote job completes
        }
    }
    m_CompletedJobs2.Clear();

    // failed jobs
    for ( Job * job : m_CompletedJobsFailed2 )
    {
        job->GetNode()->SetState( Node::FAILED );

        // Free normal jobs
        if ( job->GetDistributionState() == Job::DIST_NONE )
        {
            DestroyJob( job );
            continue;
        }

        // Distributed jobs
        {
            MutexHolder mh( m_DistributedJobsMutex );

            const Job::DistributionState distState = job->GetDistributionState();

            // Normal local or remote compilation of distributable job?
            if ( ( distState == Job::DIST_COMPLETED_LOCALLY ) || ( distState == Job::DIST_COMPLETED_REMOTELY ) )
            {
                DestroyJob( job );
                continue;
            }

            // Local race, won locally (we don't support remote wins at the moment)
            ASSERT( distState == Job::DIST_RACING );
            job->SetDistributionState( Job::DIST_RACE_WON_LOCALLY );

            // We can't delete the job yet, because it's still in use by the remote
            // job. It will be freed when the remote job completes
        }
    }
    m_CompletedJobsFailed2.Clear();
}

// MainThreadWait
//------------------------------------------------------------------------------
void JobQueue::MainThreadWait( uint32_t maxWaitMS )
{
    PROFILE_SECTION( "MainThreadWait" )
    m_MainThreadSemaphore.Wait( maxWaitMS );
}

// WorkerThreadWait
//------------------------------------------------------------------------------
void JobQueue::WorkerThreadWait( uint32_t maxWaitMS )
{
    ASSERT( Thread::IsMainThread() == false );
    ASSERT( FBuild::Get().GetOptions().m_NumWorkerThreads > 0 );
    m_WorkerThreadSemaphore.Wait( maxWaitMS );
}

// GetJobToProcess (Worker Thread)
//------------------------------------------------------------------------------
Job * JobQueue::GetJobToProcess()
{
    Job * job = m_LocalJobs_Available.RemoveJob();
    if ( job )
    {
        AtomicIncU32( &m_NumLocalJobsActive );
        return job;
    }

    return nullptr;
}

// FinishedProcessingJob (Worker Thread)
//------------------------------------------------------------------------------
void JobQueue::FinishedProcessingJob( Job * job, bool success, bool wasARemoteJob )
{
    ASSERT( job->GetNode()->GetState() == Node::BUILDING );

    if ( wasARemoteJob )
    {
        MutexHolder mh( m_DistributedJobsMutex );

        // Find the in-progress job
        Job ** it = m_DistributableJobs_InProgress.Find( job );
        ASSERT( it );

        // Handle the various states
        const Job::DistributionState distState = job->GetDistributionState();

        if ( distState == Job::DIST_COMPLETED_REMOTELY )
        {
            // Normal remote build
            m_DistributableJobs_InProgress.Erase( it );
        }
        else if ( distState == Job::DIST_BUILDING_LOCALLY )
        {
            // Normal local build of a distributable job
            m_DistributableJobs_InProgress.Erase( it );
            job->SetDistributionState( Job::DIST_COMPLETED_LOCALLY );
        }
        else
        {
            // A race was complete locally
            ASSERT( distState == Job::DIST_RACING );

            // Leave in InProgress and leave state as-is (will be set to
            // DIST_RACE_WON_LOCALLY after Finalize)
        }
    }
    else
    {
        ASSERT( m_NumLocalJobsActive > 0 );
        AtomicDecU32( &m_NumLocalJobsActive );
    }

    {
        MutexHolder m( m_CompletedJobsMutex );
        if ( success )
        {
            m_CompletedJobs.Append( job );
        }
        else
        {
            m_CompletedJobsFailed.Append( job );
        }
    }

    // Wake main thread to process completed jobs
    WakeMainThread();
}

// DoBuild
//------------------------------------------------------------------------------
/*static*/ Node::BuildResult JobQueue::DoBuild( Job * job )
{
    Timer timer; // track how long the item takes

    Node * node = job->GetNode();

    bool nodeRelevantToMonitorLog = false;

    const AString & nodeName = job->GetNode()->GetName();

    if ( ( node->GetType() == Node::OBJECT_NODE ) ||
         ( node->GetType() == Node::EXE_NODE ) ||
         ( node->GetType() == Node::LIBRARY_NODE ) ||
         ( node->GetType() == Node::DLL_NODE ) ||
         ( node->GetType() == Node::CS_NODE ) ||
         ( node->GetType() == Node::TEST_NODE ) )
    {
        nodeRelevantToMonitorLog = true;
        FLOG_MONITOR( "START_JOB local \"%s\" \n", nodeName.Get() );
    }

    // make sure the output path exists for files
    // (but don't bother for input files)
    if ( node->IsAFile() && ( node->GetType() != Node::FILE_NODE ) && ( node->GetType() != Node::COMPILER_NODE ) )
    {
        if ( Node::EnsurePathExistsForFile( node->GetName() ) == false )
        {
            // error already output by EnsurePathExistsForFile
            return Node::NODE_RESULT_FAILED;
        }
    }

    Node::BuildResult result;
    {
        #ifdef PROFILING_ENABLED
            const char * profilingTag = node->GetTypeName();
            if ( node->GetType() == Node::OBJECT_NODE )
            {
                ObjectNode * on = (ObjectNode *)node;
                profilingTag = on->IsCreatingPCH() ? "PCH" : on->IsUsingPCH() ? "Obj (+PCH)" : profilingTag;
            }
            PROFILE_SECTION( profilingTag );
        #endif
        result = node->DoBuild( job );
    }

    uint32_t timeTakenMS = uint32_t( timer.GetElapsedMS() );

    if ( result == Node::NODE_RESULT_OK )
    {
        // record new build time only if built (i.e. if cached or failed, the time
        // does not represent how long it takes to create this resource)
        node->SetLastBuildTime( timeTakenMS );
        node->SetStatFlag( Node::STATS_BUILT );
        FLOG_INFO( "-Build: %u ms\t%s", timeTakenMS, node->GetName().Get() );
    }

    if ( result == Node::NODE_RESULT_FAILED )
    {
        node->SetStatFlag( Node::STATS_FAILED );
    }

    if ( result == Node::NODE_RESULT_NEED_SECOND_BUILD_PASS )
    {
        // nothing to check
    }
    else if ( node->IsAFile() )
    {
        if ( result == Node::NODE_RESULT_FAILED )
        {
            if ( node->GetControlFlags() & Node::FLAG_NO_DELETE_ON_FAIL )
            {
                // node failed, but builder wants result left on disc
            }
            else
            {
                // build of file failed - if there is a file....
                if ( FileIO::FileExists( node->GetName().Get() ) )
                {
                    // ... it is invalid, so try to delete it
                    if ( FileIO::FileDelete( node->GetName().Get() ) == false )
                    {
                        // failed to delete it - this might cause future build problems!
                        FLOG_ERROR( "Post failure deletion failed for '%s'", node->GetName().Get() );
                    }
                }
            }
        }
        else
        {
            // build completed ok, or retrieved from cache...
            ASSERT( ( result == Node::NODE_RESULT_OK ) || ( result == Node::NODE_RESULT_OK_CACHE ) );

            // (don't check existence of input files)
            if ( node->GetType() != Node::FILE_NODE )
            {
                // ... ensure file exists (to detect builder logic problems)
                if ( !FileIO::FileExists( node->GetName().Get() ) )
                {
                    FLOG_ERROR( "File missing despite success for '%s'", node->GetName().Get() );
                    result = Node::NODE_RESULT_FAILED;
                }
            }
        }
    }

    // log processing time
    node->AddProcessingTime( timeTakenMS );

    if ( nodeRelevantToMonitorLog && FLog::IsMonitorEnabled() )
    {
        const char * resultString = nullptr;
        switch ( result )
        {
            case Node::NODE_RESULT_OK:                      resultString = "SUCCESS_COMPLETE";      break;
            case Node::NODE_RESULT_NEED_SECOND_BUILD_PASS:  resultString = "SUCCESS_PREPROCESSED";  break;
            case Node::NODE_RESULT_OK_CACHE:                resultString = "SUCCESS_CACHED";        break;
            case Node::NODE_RESULT_FAILED:                  resultString = "FAILED";                break;
        }

        AStackString<> msgBuffer;
        job->GetMessagesForMonitorLog( msgBuffer );

        FLOG_MONITOR( "FINISH_JOB %s local \"%s\" \"%s\"\n",
                      resultString,
                      nodeName.Get(),
                      msgBuffer.Get() );
    }

    return result;
}

// DestroyJob
//------------------------------------------------------------------------------
void JobQueue::DestroyJob( Job * job )
{
    // Manage memory limit book-keeping if job is distributable
    if ( job->GetDistributionState() != Job::DIST_NONE )
    {
        ASSERT( m_DistributableJobsMemoryUsage >= job->GetDataSize() );
        m_DistributableJobsMemoryUsage -= job->GetDataSize();
    }

    // Normal free of all jobs
    FDELETE job;
}

//------------------------------------------------------------------------------
