// FBuild - the main application
//------------------------------------------------------------------------------

// Includes
//------------------------------------------------------------------------------
#include "Tools/FBuild/FBuildCore/PrecompiledHeader.h"

#include "FBuild.h"

#include "FLog.h"
#include "BFF/BFFMacros.h"
#include "BFF/BFFParser.h"
#include "BFF/Functions/Function.h"
#include "Cache/ICache.h"
#include "Cache/Cache.h"
#include "Cache/CachePlugin.h"
#include "Graph/Node.h"
#include "Graph/NodeGraph.h"
#include "Graph/NodeProxy.h"
#include "Helpers/Report.h"
#include "Protocol/Client.h"
#include "Protocol/Protocol.h"
#include "WorkerPool/JobQueue.h"
#include "WorkerPool/WorkerThread.h"

#include "Core/Env/Assert.h"
#include "Core/Env/Env.h"
#include "Core/Env/Types.h"
#include "Core/FileIO/FileIO.h"
#include "Core/FileIO/FileStream.h"
#include "Core/FileIO/MemoryStream.h"
#include "Core/Math/Murmur3.h"
#include "Core/Process/SystemMutex.h"
#include "Core/Profile/Profile.h"
#include "Core/Strings/AStackString.h"
#include "Core/Tracing/Tracing.h"

#include <stdio.h>
#include <time.h>

//#define DEBUG_CRT_MEMORY_USAGE // Uncomment this for (very slow) detailed mem checks
#ifdef DEBUG_CRT_MEMORY_USAGE
	#include <crtdbg.h>
#endif

// Static
//------------------------------------------------------------------------------
/*static*/ bool FBuild::s_StopBuild( false );

// CONSTRUCTOR - FBuild
//------------------------------------------------------------------------------
FBuild::FBuild( const FBuildOptions & options )
	: m_DependencyGraph( nullptr )
	, m_JobQueue( nullptr )
	, m_Client( nullptr )
	, m_Cache( nullptr )
	, m_LastProgressOutputTime( 0.0f )
	, m_LastProgressCalcTime( 0.0f )
	, m_SmoothedProgressCurrent( 0.0f )
	, m_SmoothedProgressTarget( 0.0f )
	, m_WorkerList( 0, true )
	, m_EnvironmentString( nullptr )
	, m_EnvironmentStringSize( 0 )
	, m_ImportedEnvironmentVars( 0, true )
{
	#ifdef DEBUG_CRT_MEMORY_USAGE
		_CrtSetDbgFlag( _CRTDBG_ALLOC_MEM_DF | 
						_CRTDBG_CHECK_ALWAYS_DF | //_CRTDBG_CHECK_EVERY_16_DF |
						_CRTDBG_CHECK_CRT_DF | 
						_CRTDBG_DELAY_FREE_MEM_DF | 
						_CRTDBG_LEAK_CHECK_DF );
	#endif

	m_Macros = FNEW( BFFMacros() );

	m_DependencyGraph = FNEW( NodeGraph() );

	// store all user provided options
	m_Options = options;

	// track the old working dir to restore if modified (mainly for unit tests)
	VERIFY( FileIO::GetCurrentDir( m_OldWorkingDir ) );

	// check for cache environment variable to use as default
	AStackString<> cachePath;
	if ( Env::GetEnvVariable( "FASTBUILD_CACHE_PATH", cachePath ) )
	{
		if ( cachePath.IsEmpty() == false )
		{
			SetCachePath( cachePath );
		}
	}

	// poke options where required
	FLog::SetShowInfo( m_Options.m_ShowInfo );
	FLog::SetShowErrors( m_Options.m_ShowErrors );
	FLog::SetShowProgress( m_Options.m_ShowProgress );

	Function::Create();
}

// DESTRUCTOR
//------------------------------------------------------------------------------
FBuild::~FBuild()
{
    PROFILE_FUNCTION

	Function::Destroy();

	FDELETE m_Macros;
	FDELETE m_DependencyGraph;
	FDELETE m_Client;
	FREE( m_EnvironmentString );

	if ( m_Cache )
	{
		m_Cache->Shutdown();
		FDELETE m_Cache;
	}

	// restore the old working dir to restore
	ASSERT( !m_OldWorkingDir.IsEmpty() );
	if ( !FileIO::SetCurrentDir( m_OldWorkingDir ) )
	{
		FLOG_ERROR( "Failed to restore working dir: '%s' (error: %u)", m_OldWorkingDir.Get(), Env::GetLastErr() );
	}
}

// Initialize
//------------------------------------------------------------------------------
bool FBuild::Initialize( const char * nodeGraphDBFile )
{
    PROFILE_FUNCTION

	// handle working dir
	if ( !FileIO::SetCurrentDir( m_Options.GetWorkingDir() ) )
	{
		FLOG_ERROR( "Failed to set working dir: '%s' (error: %u)", m_Options.GetWorkingDir().Get(), Env::GetLastErr() );
		return false;
	}

	const char * bffFile = m_Options.m_ConfigFile.IsEmpty() ? GetDefaultBFFFileName()
														    : m_Options.m_ConfigFile.Get();
	if ( m_DependencyGraph->Initialize( bffFile, nodeGraphDBFile ) == false )
	{
		return false;
	}

	// if the cache is enabled, make sure the path is set and accessible
	if ( m_Options.m_UseCacheRead || m_Options.m_UseCacheWrite )
	{
		if ( !m_CachePluginDLL.IsEmpty() )
		{
			m_Cache = FNEW( CachePlugin( m_CachePluginDLL ) );
		}
		else 
		{
			m_Cache = FNEW( Cache() );
		}

		if ( m_Cache->Init( m_CachePath ) == false )
		{
			m_Options.m_UseCacheRead = false;
			m_Options.m_UseCacheWrite = false;
		}
	}

	//
	// create the connection management system if we might need it
	if ( m_Options.m_AllowDistributed )
	{
		Array< AString > workers;
		if ( m_WorkerList.IsEmpty() )
		{
			// check for workers through brokerage
			// TODO:C This could be moved out of the main code path
			m_WorkerBrokerage.FindWorkers( workers );
		}
		else
		{
			workers = m_WorkerList;
		}

		if ( workers.IsEmpty() )
		{
			FLOG_WARN( "No workers available - Distributed compilation disabled" );
			m_Options.m_AllowDistributed = false;
		}
		else
		{
			OUTPUT( "Distributed Compilation : %u Workers in pool\n", workers.GetSize() );
			m_Client = FNEW( Client( workers ) );
		}
	}

	return true;
}

// Build
//------------------------------------------------------------------------------
bool FBuild::Build( const AString & target )
{
	ASSERT( !target.IsEmpty() );

	Array< AString > targets( 1, false );
	targets.Append( target );
	return Build( targets );
}

// Build
//------------------------------------------------------------------------------
bool FBuild::Build( const Array< AString > & targets )
{
	ASSERT( !targets.IsEmpty() );

	// Get the nodes for all the targets
	const size_t numTargets = targets.GetSize();
	Dependencies nodes( numTargets, 0 );
	for ( size_t i=0; i<numTargets; ++i )
	{
		const AString & target = targets[ i ];

		// get the node being requested (search for exact match, to find aliases etc first)
		Node * node = m_DependencyGraph->FindNodeInternal( target );
		if ( node == nullptr )
		{
			// failed to find the node, try looking for a fully pathed equivalent
			node = m_DependencyGraph->FindNode( target );
		}

		if ( node == nullptr )
		{
			FLOG_ERROR( "Unknown build target '%s'", target.Get() );
			return false;
		}
		nodes.Append( Dependency( node ) );
	}

	// create a temporary node, not hooked into the DB
	NodeProxy proxy( AStackString< 32 >( "*proxy*" ) );
	proxy.m_StaticDependencies = nodes;

	// build all targets in one sweep
	bool result = Build( &proxy );

	// output per-target results
	for ( size_t i=0; i<targets.GetSize(); ++i )
	{
		bool nodeResult = ( nodes[ i ].GetNode()->GetState() == Node::UP_TO_DATE );
		OUTPUT( "FBuild: %s: %s\n", nodeResult ? "OK" : "Error: BUILD FAILED", targets[ i ].Get() );
	}

	return result;
}

// SaveDependencyGraph
//------------------------------------------------------------------------------
bool FBuild::SaveDependencyGraph( const char * nodeGraphDBFile ) const
{
    PROFILE_FUNCTION

	nodeGraphDBFile = nodeGraphDBFile ? nodeGraphDBFile : GetDependencyGraphFileName();

	FLOG_INFO( "Saving DepGraph '%s'", nodeGraphDBFile );

	Timer t;

	// serialize into memory first
	MemoryStream memoryStream( 32 * 1024 * 1024, 8 * 1024 * 1024 );
	m_DependencyGraph->Save( memoryStream );

	// We'll save to a tmp file first
	AStackString<> tmpFileName( nodeGraphDBFile );
	tmpFileName += ".tmp";

	// try to open the file
	FileStream fileStream;
	if ( fileStream.Open( tmpFileName.Get(), FileStream::WRITE_ONLY ) == false )
	{
		// failing to open the dep graph for saving is a serious problem
		FLOG_ERROR( "Failed to open DepGraph for saving '%s'", nodeGraphDBFile );
		return false;
	}

	// write in-memory serialized data to disk
	if ( fileStream.Write( memoryStream.GetData(), memoryStream.GetSize() ) != memoryStream.GetSize() )
	{
		FLOG_ERROR( "Saving DepGraph FAILED!" );
		return false;
	}
	fileStream.Close();

	// rename tmp file
	if ( FileIO::FileMove( tmpFileName, AStackString<>( nodeGraphDBFile ) ) == false )
	{
		FLOG_ERROR( "Failed to rename temp DB file '%s' (%i)", tmpFileName.Get(), Env::GetLastErr() );
		return false;
	}

	FLOG_INFO( "Saving DepGraph Complete in %2.3fs", t.GetElapsed() );
	return true;
}

// Build
//------------------------------------------------------------------------------
bool FBuild::Build( Node * nodeToBuild )
{
	ASSERT( nodeToBuild );

	s_StopBuild = false; // allow multiple runs in same process

	// create worker threads
	m_JobQueue = FNEW( JobQueue( m_Options.m_NumWorkerThreads ) );

	m_Timer.Start();
	m_LastProgressOutputTime = 0.0f;
	m_LastProgressCalcTime = 0.0f;
	m_SmoothedProgressCurrent = 0.0f;
	m_SmoothedProgressTarget = 0.0f;
	FLog::StartBuild();

	// create worker dir for main thread build case
	if ( m_Options.m_NumWorkerThreads == 0 )
	{
		WorkerThread::CreateThreadLocalTmpDir();
	}

	bool stopping( false );

	// keep doing build passes until completed/failed
	for ( ;; )
	{
        // process completed jobs
        m_JobQueue->FinalizeCompletedJobs();

		if ( !stopping )
		{
			// do a sweep of the graph to create more jobs
			m_DependencyGraph->DoBuildPass( nodeToBuild );
		}

		if ( m_Options.m_NumWorkerThreads == 0 )
		{
			// no local threads - do build directly
			WorkerThread::Update();
		}

		bool complete = ( nodeToBuild->GetState() == Node::UP_TO_DATE ) ||
						( nodeToBuild->GetState() == Node::FAILED );

		if ( s_StopBuild || complete )
		{
			if ( stopping == false ) 
			{
				// free the network distribution system (if there is one)
				FDELETE m_Client;
				m_Client = nullptr;

				// wait for workers to exit.  Can still be building even though we've failed:
				//  - only 1 failed node propagating up to root while others are not yet complete
				//  - aborted build, so workers can be incomplete
				m_JobQueue->SignalStopWorkers();
				stopping = true;
			}
		}

		if ( !stopping )
		{
			if ( m_Options.m_WrapperChild )
			{
                SystemMutex wrapperMutex( m_Options.GetMainProcessMutexName().Get() );
				if ( wrapperMutex.TryLock() )
				{
					// parent process has terminated
					s_StopBuild = true;
				}
			}
		}

		// completely stopped?
		if ( stopping && m_JobQueue->HaveWorkersStopped() )
		{
			break;
		}

		// Wait until more work to process or time has elapsed
		m_JobQueue->MainThreadWait( 500 );

		// update progress
		UpdateBuildStatus( nodeToBuild );
	}

    // wrap up/free any jobs that come from the last build pass
    m_JobQueue->FinalizeCompletedJobs();

	FDELETE m_JobQueue;
	m_JobQueue = nullptr;

	FLog::StopBuild();

	// even if the build has failed, we can still save the graph.
	// This is desireable because:
	// - it will save parsing the bff next time
	// - it will record the items that did build, so they won't build again
	if ( m_Options.m_SaveDBOnCompletion )
	{
		SaveDependencyGraph();
	}

	// TODO:C Move this into BuildStats
	float timeTaken = m_Timer.GetElapsed();
	m_BuildStats.m_TotalBuildTime = timeTaken;

	m_BuildStats.OnBuildStop( nodeToBuild );

	return ( nodeToBuild->GetState() == Node::UP_TO_DATE );
}

// SetEnvironmentString
//------------------------------------------------------------------------------
void FBuild::SetEnvironmentString( const char * envString, uint32_t size, const AString & libEnvVar )
{
	FREE( m_EnvironmentString );
	m_EnvironmentString = (char *)ALLOC( size + 1 );
	m_EnvironmentStringSize = size;
	AString::Copy( envString, m_EnvironmentString, size );
	m_LibEnvVar = libEnvVar;
}

// ImportEnvironmentVar
//------------------------------------------------------------------------------
bool FBuild::ImportEnvironmentVar( const char * name, AString & value, uint32_t & hash )
{
	// check if system environment contains the variable
	if ( Env::GetEnvVariable( name, value ) == false )
	{
		FLOG_ERROR( "Could not import environment variable '%s'", name );
		return false;
	}

	// compute hash value for actual value
	hash = Murmur3::Calc32( value );

	// check if the environment var was already imported
	const EnvironmentVarAndHash * it = m_ImportedEnvironmentVars.Begin();
	const EnvironmentVarAndHash * const end = m_ImportedEnvironmentVars.End();
	while ( it < end )
	{
		if ( it->GetName() == name )
		{
			// check if imported environment changed since last import
			if ( it->GetHash() != hash )
			{
				FLOG_ERROR( "Overwriting imported environment variable '%s' with a different value = '%s'",
							name, value.Get() );
				return false;
			}

			// skip registration when already imported with same hash value
			return true;
		}
		it++;
	}

	// import new variable name with its hash value
	const EnvironmentVarAndHash var( name, hash );
	m_ImportedEnvironmentVars.Append( var );

	return true;
}

// GetLibEnvVar
//------------------------------------------------------------------------------
void FBuild::GetLibEnvVar( AString & value ) const
{
	// has environment been overridden in BFF?
	if ( m_EnvironmentString )
	{
		// use overridden LIB path (which maybe empty)
		value = m_LibEnvVar;
	}
	else
	{
		// use real environment LIB path
		Env::GetEnvVariable( "LIB", value );
	}
}

// OnBuildError
//------------------------------------------------------------------------------
/*static*/ void FBuild::OnBuildError()
{
	if ( FBuild::Get().GetOptions().m_StopOnFirstError )
	{
		s_StopBuild = true;
	}
}

// UpdateBuildStatus
//------------------------------------------------------------------------------
void FBuild::UpdateBuildStatus( const Node * node )
{
	PROFILE_FUNCTION

	if ( FBuild::Get().GetOptions().m_ShowProgress == false ) 
	{
		return;
	}

	const float OUTPUT_FREQUENCY( 1.0f );
	const float CALC_FREQUENCY( 5.0f );

	float timeNow = m_Timer.GetElapsed();

	bool doUpdate = ( ( timeNow - m_LastProgressOutputTime ) >= OUTPUT_FREQUENCY );	
	if ( doUpdate == false )
	{
		return;
	}

	// recalculate progress estimate?
	if ( ( timeNow - m_LastProgressCalcTime ) >= CALC_FREQUENCY )
	{
        PROFILE_SECTION( "CalcPogress" )

		FBuildStats & bs = m_BuildStats;
		bs.m_NodeTimeProgressms = 0;
		bs.m_NodeTimeTotalms = 0;
		m_DependencyGraph->UpdateBuildStatus( node, bs.m_NodeTimeProgressms, bs.m_NodeTimeTotalms );
		m_LastProgressCalcTime = m_Timer.GetElapsed();

		// calculate percentage
		float doneRatio = (float)( (double)bs.m_NodeTimeProgressms / (double)bs.m_NodeTimeTotalms );

		// don't allow it to reach 100% (handles rounding inaccuracies)
		float donePerc = Math::Min< float >( doneRatio * 100.0f, 99.9f );

		// don't allow progress to go backwards
		m_SmoothedProgressTarget = Math::Max< float >( donePerc, m_SmoothedProgressTarget );
	}

	m_SmoothedProgressCurrent = ( 0.5f * m_SmoothedProgressCurrent ) + ( m_SmoothedProgressTarget * 0.5f );

	// get nodes counts;
	uint32_t numJobs = 0;
	uint32_t numJobsActive = 0;
	uint32_t numJobsDist = 0;
	uint32_t numJobsDistActive = 0;
	if ( JobQueue::IsValid() )
	{
		JobQueue::Get().GetJobStats( numJobs, numJobsActive, numJobsDist, numJobsDistActive );
	}

	FLog::OutputProgress( timeNow, m_SmoothedProgressCurrent, numJobs, numJobsActive, numJobsDist, numJobsDistActive );
	m_LastProgressOutputTime = timeNow;
}

// GetDependencyGraphFileName
//------------------------------------------------------------------------------
/*static*/ const char * FBuild::GetDependencyGraphFileName()
{
	return "fbuild.fdb";
}

// GetDefaultBFFFileName
//------------------------------------------------------------------------------
/*static*/ const char * FBuild::GetDefaultBFFFileName()
{
	return "fbuild.bff";
}

// SetCachePath
//------------------------------------------------------------------------------
void FBuild::SetCachePath( const AString & path )
{ 
	m_CachePath = path;
}

// GetCacheFileName
//------------------------------------------------------------------------------
void FBuild::GetCacheFileName( uint64_t keyA, uint32_t keyB, uint64_t keyC, AString & path ) const
{
	// cache version - bump if cache format is changed
	static const int cacheVersion( 5 );

	// format example: 2377DE32AB045A2D_FED872A1_AB62FEAA23498AAC.4
	path.Format( "%016llX_%08X_%016llX.%u", keyA, keyB, keyC, cacheVersion );
}

//------------------------------------------------------------------------------
