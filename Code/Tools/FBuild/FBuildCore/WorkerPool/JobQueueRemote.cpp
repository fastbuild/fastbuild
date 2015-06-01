// JobQueueRemote
//------------------------------------------------------------------------------

// Includes
//------------------------------------------------------------------------------
#include "Tools/FBuild/FBuildCore/PrecompiledHeader.h"

#include "JobQueueRemote.h"
#include "Job.h"
#include "WorkerThreadRemote.h"

#include "Tools/FBuild/FBuildCore/FBuild.h"
#include "Tools/FBuild/FBuildCore/FLog.h"
#include "Tools/FBuild/FBuildCore/Graph/Node.h"
#include "Tools/FBuild/FBuildCore/Graph/ObjectNode.h"

// Core
#include "Core/Containers/AutoPtr.h"
#include "Core/FileIO/FileIO.h"
#include "Core/FileIO/FileStream.h"
#include "Core/FileIO/PathUtils.h"
#include "Core/Profile/Profile.h"
#include "Core/Time/Timer.h"
#include "Core/Tracing/Tracing.h"

// CONSTRUCTOR
//------------------------------------------------------------------------------
JobQueueRemote::JobQueueRemote( uint32_t numWorkerThreads ) :
	m_PendingJobs( 1024, true ),
	m_CompletedJobs( 1024, true ),
	m_CompletedJobsFailed( 1024, true ),
	m_Workers( numWorkerThreads, false )
{
	WorkerThread::InitTmpDir( true ); // remote == true

	for ( uint32_t i=0; i<numWorkerThreads; ++i )
	{
		// identify each worker with an id starting from 1
		// (the "main" thread is considered 0)
		uint32_t threadIndex = ( i + 1001 );
		WorkerThread * wt = FNEW( WorkerThreadRemote( threadIndex ) );
		wt->Init();
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

// QueueJob (Main Thread)
//------------------------------------------------------------------------------
void JobQueueRemote::QueueJob( Job * job )
{
	MutexHolder m( m_PendingJobsMutex );
	m_PendingJobs.Append( job );
}


// GetCompletedJob
//------------------------------------------------------------------------------
Job * JobQueueRemote::GetCompletedJob()
{
	MutexHolder m( m_CompletedJobsMutex );

	// completed jobs
	if ( !m_CompletedJobs.IsEmpty() ) 
	{
		Job * job = m_CompletedJobs[ 0 ];
		m_CompletedJobs.PopFront();
		job->GetNode()->SetState( Node::UP_TO_DATE );
		return job;
	}

	// failed jobs
	if ( !m_CompletedJobsFailed.IsEmpty() ) 
	{
		Job * job = m_CompletedJobsFailed[ 0 ];
		m_CompletedJobsFailed.PopFront();
		job->GetNode()->SetState( Node::FAILED );
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
	Job ** it = m_InFlightJobs.Begin();
	while ( it != m_InFlightJobs.End() )
	{
		if ( ( *it )->GetUserData() == userData )
		{
			( *it )->SetUserData( nullptr );
		}
		++it;
	}
}

// GetJobToProcess (Worker Thread)
//------------------------------------------------------------------------------
Job * JobQueueRemote::GetJobToProcess()
{
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
void JobQueueRemote::FinishedProcessingJob( Job * job, bool success )
{
//	ASSERT( job->GetNode()->GetState() == Node::BUILDING );

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

// DoBuild
//------------------------------------------------------------------------------
/*static*/ Node::BuildResult JobQueueRemote::DoBuild( Job * job, bool racingRemoteJob )
{
	Timer timer; // track how long the item takes

	ObjectNode * node = job->GetNode()->CastTo< ObjectNode >();

	// remote tasks must output to a tmp file
	if ( job->IsLocal() == false )
	{
		// file name should be the same as on host
		const char * fileName = ( job->GetRemoteName().FindLast( NATIVE_SLASH ) + 1 );

		AStackString<> tmpFileName;
		WorkerThread::CreateTempFilePath( fileName, tmpFileName );
		node->ReplaceDummyName( tmpFileName );

		#ifdef DEBUG
			DEBUGSPAM( "REMOTE: %s (%s)\n", fileName, job->GetRemoteName().Get() );
		#endif
	}

	ASSERT( node->IsAFile() );

	// make sure the output path exists
	if ( Node::EnsurePathExistsForFile( node->GetName() ) == false )
	{
		// error already output by EnsurePathExistsForFile
		return Node::NODE_RESULT_FAILED;
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
        PROFILE_SECTION( "RACE" );
    	result = ((Node *)node )->DoBuild2( job, racingRemoteJob );
    }

	uint32_t timeTakenMS = uint32_t( timer.GetElapsedMS() );

	if ( result == Node::NODE_RESULT_OK )
	{
		// record new build time only if built (i.e. if failed, the time
		// does not represent how long it takes to create this resource)
		node->SetLastBuildTime( timeTakenMS );
		node->SetStatFlag( Node::STATS_BUILT );
		//FLOG_INFO( "-Build: %u ms\t%s", timeTakenMS, node->GetName().Get() );

		#ifdef DEBUG
			if ( job->IsLocal() )
			{
				// record new file time for remote job we built locally
				ASSERT( node->m_Stamp == FileIO::GetFileLastWriteTime(node->GetName()) );
			}
		#endif
	}

	if ( result == Node::NODE_RESULT_FAILED )
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
	else
	{
		// build completed ok, or retrieved from cache...
		ASSERT( result == Node::NODE_RESULT_OK );

		// TODO:A Also read into job if cache is being used
		if ( job->IsLocal() == false )
		{
			// read results into memory to send back to client	
			if ( ReadResults( job ) == false )
			{	
				result = Node::NODE_RESULT_FAILED;
			}
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
	
	return result;
}

// ReadResults
//------------------------------------------------------------------------------
/*static*/ bool JobQueueRemote::ReadResults( Job * job )
{
	const ObjectNode * node = job->GetNode()->CastTo< ObjectNode >();
	const bool includePDB = (  node->IsUsingPDB() && ( job->IsLocal() == false ) );

	// main object
	FileStream fs;
	if ( fs.Open( node->GetName().Get() ) == false )
	{
		FLOG_ERROR( "File missing despite success: '%s'", node->GetName().Get() );
		return false;
	}
	uint32_t size = (uint32_t)fs.GetFileSize();
	uint32_t size2 = 0;

	// pdb file if present
	FileStream fs2;
	if ( includePDB )
	{
		AStackString<> pdbName;
		node->GetPDBName( pdbName );
		if ( fs2.Open( pdbName.Get() ) == false )
		{
			FLOG_ERROR( "File missing despite success: '%s'", pdbName.Get() );
			return false;
		}
		size2 = (uint32_t)fs2.GetFileSize();
	}

	// calc memory required
	size_t memSize = sizeof( uint32_t ); // write size of first file
	memSize += size;
	if ( includePDB )
	{
		memSize += sizeof( uint32_t ); // write size of second file
		memSize += size2;
	}

	// allocate entire buffer
	AutoPtr< char > mem( (char *)ALLOC( memSize ) );

	// write first file size
	*( (uint32_t *)mem.Get() ) = size;

	// read first file
	if ( fs.Read( mem.Get() + sizeof( uint32_t ), size ) != size )
	{
		FLOG_ERROR( "File read error for '%s'", node->GetName().Get() );
		return false;
	}

	if ( includePDB )
	{
		// write second file size
		*( (uint32_t *)( mem.Get() + sizeof( uint32_t ) + size ) ) = size2;

		// read second file
		if ( fs2.Read( mem.Get() + sizeof( uint32_t ) + size + sizeof( uint32_t ), size2 ) != size2 )
		{
			FLOG_ERROR( "File read error for '%s'", node->GetName().Get() );
			return false;
		}
	}

	// transfer data to job
	job->OwnData( mem.Release(), memSize );

	return true;
}

//------------------------------------------------------------------------------
