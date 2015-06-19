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
	m_DistributableAvailableJobs( 1024, true ),
	m_DistributableJobsMemoryUsage( 0 ),
	m_DistributedJobsRemote( 1204, true ),
	m_DistributedJobsLocal( 128, true ),
	m_DistributedJobsCancelled( 128, true ),
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
	while( m_LocalJobs_Available.GetCount() > 0 )
	{
		Job * job = m_LocalJobs_Available.RemoveJob();
		FDELETE job;
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
	MutexHolder m( m_DistributableAvailableJobsMutex );
	return m_DistributableAvailableJobs.GetSize();
}

// GetJobStats
//------------------------------------------------------------------------------
void JobQueue::GetJobStats( uint32_t & numJobs, 
						    uint32_t & numJobsActive, 
							uint32_t & numJobsDist, 
							uint32_t & numJobsDistActive ) const
{
	{
		numJobs = m_LocalJobs_Available.GetCount();
		MutexHolder m( m_DistributableAvailableJobsMutex );
		numJobsDist = (uint32_t)m_DistributableAvailableJobs.GetSize();
	}
	numJobsActive = m_NumLocalJobsActive;

	MutexHolder m( m_DistributedJobsMutex );
	numJobsDistActive = (uint32_t)( m_DistributedJobsRemote.GetSize() +
									m_DistributedJobsLocal.GetSize() +
									m_DistributedJobsBeingRaced.GetSize() );
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
			case Node::NODE_RESULT_FAILED:	node->SetState( Node::FAILED ); break;
			case Node::NODE_RESULT_OK:		node->SetState( Node::UP_TO_DATE ); break;
			default:						ASSERT( false ); break;
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
    m_LocalJobs_Staging.Clear();
}

// QueueJob2
//------------------------------------------------------------------------------
void JobQueue::QueueJob2( Job * job )
{
	ASSERT( job->GetNode()->GetState() == Node::BUILDING );

	{
		MutexHolder m( m_DistributableAvailableJobsMutex );
		m_DistributableAvailableJobs.Append( job );

		// track size of distributable jobs
		m_DistributableJobsMemoryUsage += job->GetDataSize();
	}

	ASSERT( m_NumLocalJobsActive > 0 );
	AtomicDecU32( &m_NumLocalJobsActive ); // job converts from active to pending remote
}

// GetDistributableJobToProcess
//------------------------------------------------------------------------------
Job * JobQueue::GetDistributableJobToProcess( bool remote )
{
	Job * job( nullptr );
	{
		MutexHolder m( m_DistributableAvailableJobsMutex );
		if ( m_DistributableAvailableJobs.IsEmpty() )
		{
			return nullptr;
		}

		// building jobs in the order they are queued
		job = m_DistributableAvailableJobs[ 0 ];
		m_DistributableAvailableJobs.PopFront();

		// track size of distributable jobs
		m_DistributableJobsMemoryUsage -= job->GetDataSize();
	}

	{
		MutexHolder m( m_DistributedJobsMutex );
		if ( remote )
		{
			m_DistributedJobsRemote.Append( job );
		}
		else
		{
			m_DistributedJobsLocal.Append( job );
		}
	}

	return job;
}

// GetDistributableJobToRace
//------------------------------------------------------------------------------
Job * JobQueue::GetDistributableJobToRace()
{
	MutexHolder m( m_DistributedJobsMutex );
	if ( m_DistributedJobsRemote.IsEmpty() )
	{
		return nullptr;
	}

	// take newest job, which is least likely to finish first
	// compared to older distributed jobs
	Job * job = m_DistributedJobsRemote.Top();
	m_DistributedJobsRemote.Pop();
	m_DistributedJobsBeingRaced.Append( job );

	// TODO:B We should defer the remote cancellation
	// to have a proper race
	CancelledJob c( job, job->GetJobId() );
	m_DistributedJobsCancelled.Append( c );

	return job;
}

// OnReturnRemoteJob
//------------------------------------------------------------------------------
Job * JobQueue::OnReturnRemoteJob( uint32_t jobId, bool & cancelled )
{
	MutexHolder m( m_DistributedJobsMutex );
	CancelledJob * it = m_DistributedJobsCancelled.Find( jobId );
	if ( it )
	{
		cancelled = true;
		Job * cancelledJob = it->m_Job; // pointer value is ok, but object may no longer exist!
		m_DistributedJobsCancelled.Erase( it );
		return cancelledJob;
	}
	cancelled = false;

	// not cancelled, so we need to make sure it's not stolen since wethe
	// caller will serialize the result
	Job ** distIt = m_DistributedJobsRemote.FindDeref( jobId );
	ASSERT( distIt );
	Job * job = *distIt;
	m_DistributedJobsRemote.Erase( distIt );
	return job;
}

// ReturnUnfinishedDistributableJob
//------------------------------------------------------------------------------
void JobQueue::ReturnUnfinishedDistributableJob( Job * job, bool systemError )
{
	if ( !systemError )
	{
		MutexHolder m( m_DistributedJobsMutex );
		Job ** it = m_DistributedJobsRemote.Find( job );
		if ( it == nullptr )
		{
			// not in remote job list - was it cancelled?
			CancelledJob * cIt = m_DistributedJobsCancelled.Find( job );
			if ( cIt )
			{
				// cancelled - clean up cancel
				m_DistributedJobsCancelled.Erase( cIt );
				return;
			}

			ASSERT( false ); // problem!
			return;
		}

		m_DistributedJobsRemote.Erase( it );
	}

	// re-queue job
	{
		MutexHolder m( m_DistributableAvailableJobsMutex );
		m_DistributableAvailableJobs.Append( job );

		// track size of distributable jobs
		m_DistributableJobsMemoryUsage += job->GetDataSize();
	}
}

// FinalizeCompletedJobs (Main Thread)
//------------------------------------------------------------------------------
void JobQueue::FinalizeCompletedJobs()
{
    PROFILE_FUNCTION

	{
		MutexHolder m( m_CompletedJobsMutex );
		m_CompletedJobs2.Swap( m_CompletedJobs );
		m_CompletedJobsFailed2.Swap( m_CompletedJobsFailed );
	}

	// completed jobs
	const Job * const * end = m_CompletedJobs2.End();
	for ( Job ** i = m_CompletedJobs2.Begin();
			i != end;
			i++ )
	{
		Job * job = ( *i );
		Node * n = job->GetNode();
		if ( n->Finalize() )
		{
			n->SetState( Node::UP_TO_DATE );
		}
		else
		{
			n->SetState( Node::FAILED );
		}
		FDELETE job;
	}
	m_CompletedJobs2.Clear();

	// failed jobs
	end = m_CompletedJobsFailed2.End();
	for ( Job ** i = m_CompletedJobsFailed2.Begin();
			i != end;
			i++ )
	{
		Job * job = ( *i );
		job->GetNode()->SetState( Node::FAILED );
		FDELETE job;
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
void JobQueue::FinishedProcessingJob( Job * job, bool success, bool wasARemoteJob, bool localRaceOfRemoteJob )
{
	ASSERT( job->GetNode()->GetState() == Node::BUILDING );

	if ( wasARemoteJob )
	{
		MutexHolder mh( m_DistributedJobsMutex );
		Job ** it = m_DistributedJobsLocal.Find( job );
		if ( it )
		{
			// remote job, but done locally
			m_DistributedJobsLocal.Erase( it );
		}
		else
		{
			if ( localRaceOfRemoteJob )
			{
				Job ** dIt = m_DistributedJobsBeingRaced.Find( job );
				ASSERT( dIt );
				m_DistributedJobsBeingRaced.Erase( dIt );
			}
			else
			{
				// remote job done remotely
				ASSERT( m_DistributedJobsRemote.Find( job ) == nullptr ); // should have been removed in OnReturnRemoteJob
			}
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

	return result;
}

//------------------------------------------------------------------------------
