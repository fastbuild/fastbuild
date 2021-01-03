// FBuild - the main application
//------------------------------------------------------------------------------

// Includes
//------------------------------------------------------------------------------
#include "FBuild.h"

#include "FLog.h"
#include "BFF/BFFParser.h"
#include "BFF/Functions/Function.h"
#include "Cache/ICache.h"
#include "Cache/Cache.h"
#include "Cache/CachePlugin.h"
#include "Cache/LightCache.h"
#include "Graph/Node.h"
#include "Graph/NodeGraph.h"
#include "Graph/NodeProxy.h"
#include "Graph/SettingsNode.h"
#include "Helpers/BuildProfiler.h"
#include "Helpers/CompilationDatabase.h"
#include "Helpers/Report.h"
#include "Protocol/Client.h"
#include "Protocol/Protocol.h"
#include "WorkerPool/JobQueue.h"
#include "WorkerPool/WorkerThread.h"

#include "Core/Env/Assert.h"
#include "Core/Env/Env.h"
#include "Core/Env/ErrorFormat.h"
#include "Core/Env/Types.h"
#include "Core/FileIO/FileIO.h"
#include "Core/FileIO/FileStream.h"
#include "Core/FileIO/MemoryStream.h"
#include "Core/Math/xxHash.h"
#include "Core/Mem/SmallBlockAllocator.h"
#include "Core/Process/Atomic.h"
#include "Core/Process/SystemMutex.h"
#include "Core/Profile/Profile.h"
#include "Core/Strings/AStackString.h"
#include "Core/Tracing/Tracing.h"
#include "Core/Process/Process.h"

#include <stdio.h>
#include <time.h>

//#define DEBUG_CRT_MEMORY_USAGE // Uncomment this for (very slow) detailed mem checks
#ifdef DEBUG_CRT_MEMORY_USAGE
    #include <crtdbg.h>
#endif

// Static
//------------------------------------------------------------------------------
/*static*/ bool FBuild::s_StopBuild( false );
/*static*/ volatile bool FBuild::s_AbortBuild( false );

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

    // store all user provided options
    m_Options = options;

    // track the old working dir to restore if modified (mainly for unit tests)
    VERIFY( FileIO::GetCurrentDir( m_OldWorkingDir ) );

    // poke options where required
    FLog::SetShowVerbose( m_Options.m_ShowVerbose );
    FLog::SetShowBuildReason( m_Options.m_ShowBuildReason );
    FLog::SetShowErrors( m_Options.m_ShowErrors );
    FLog::SetShowProgress( m_Options.m_ShowProgress );
    FLog::SetMonitorEnabled( m_Options.m_EnableMonitor );

    if ( options.m_Profile )
    {
        FNEW( BuildProfiler );
    }

    Function::Create();

    NetworkStartupHelper::SetMainShutdownFlag( &s_AbortBuild );
}

// DESTRUCTOR
//------------------------------------------------------------------------------
FBuild::~FBuild()
{
    PROFILE_FUNCTION;

    Function::Destroy();

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
        FLOG_ERROR( "Failed to restore working dir. Error: %s Dir: '%s'", LAST_ERROR_STR, m_OldWorkingDir.Get() );
    }

    LightCache::ClearCachedFiles();

    if ( BuildProfiler::IsValid() )
    {
        FDELETE( &BuildProfiler::Get() );
    }
}

// Initialize
//------------------------------------------------------------------------------
bool FBuild::Initialize( const char * nodeGraphDBFile )
{
    PROFILE_FUNCTION;
    BuildProfilerScope buildProfileScope( "Initialize" );

    // handle working dir
    if ( !FileIO::SetCurrentDir( m_Options.GetWorkingDir() ) )
    {
        FLOG_ERROR( "Failed to set working dir. Error: %s Dir: '%s'", LAST_ERROR_STR, m_Options.GetWorkingDir().Get() );
        return false;
    }

    const char * bffFile = m_Options.m_ConfigFile.IsEmpty() ? GetDefaultBFFFileName()
                                                            : m_Options.m_ConfigFile.Get();

    if ( nodeGraphDBFile != nullptr )
    {
        m_DependencyGraphFile = nodeGraphDBFile;
    }
    else
    {
        m_DependencyGraphFile = bffFile;
        if ( m_DependencyGraphFile.EndsWithI( ".bff" ) )
        {
            m_DependencyGraphFile.SetLength( m_DependencyGraphFile.GetLength() - 4 );
        }
        #if defined( __WINDOWS__ )
            m_DependencyGraphFile += ".windows.fdb";
        #elif defined( __OSX__ )
            m_DependencyGraphFile += ".osx.fdb";
        #elif defined( __LINUX__ )
            m_DependencyGraphFile += ".linux.fdb";
        #endif
    }

    SmallBlockAllocator::SetSingleThreadedMode( true );

    m_DependencyGraph = NodeGraph::Initialize( bffFile, m_DependencyGraphFile.Get(), m_Options.m_ForceDBMigration_Debug );

    SmallBlockAllocator::SetSingleThreadedMode( false );

    if ( m_DependencyGraph == nullptr )
    {
        return false;
    }

    const SettingsNode * settings = m_DependencyGraph->GetSettings();

    // if the cache is enabled, make sure the path is set and accessible
    if ( m_Options.m_UseCacheRead || m_Options.m_UseCacheWrite || m_Options.m_CacheInfo || m_Options.m_CacheTrim )
    {
        if ( !settings->GetCachePluginDLL().IsEmpty() )
        {
            m_Cache = FNEW( CachePlugin( settings->GetCachePluginDLL() ) );
        }
        else
        {
            m_Cache = FNEW( Cache() );
        }

        if ( m_Cache->Init( settings->GetCachePath(),
                            settings->GetCachePathMountPoint(),
                            m_Options.m_UseCacheRead,
                            m_Options.m_UseCacheWrite,
                            m_Options.m_CacheVerbose,
                            settings->GetCachePluginDLLConfig() ) == false )
        {
            m_Options.m_UseCacheRead = false;
            m_Options.m_UseCacheWrite = false;
            FDELETE m_Cache;
            m_Cache = nullptr;
        }
    }

    return true;
}

// Build
//------------------------------------------------------------------------------
bool FBuild::Build( const char* target )
{
    return Build( AStackString<>( target ) );
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

// GetTargets
//------------------------------------------------------------------------------
bool FBuild::GetTargets( const Array< AString > & targets, Dependencies & outDeps ) const
{
    ASSERT( !targets.IsEmpty() );

    // Get the nodes for all the targets
    const size_t numTargets = targets.GetSize();
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

            // Gets the 5 targets with minimal distance to user input
            Array< NodeGraph::NodeWithDistance > nearestNodes( 5, false );
            m_DependencyGraph->FindNearestNodesInternal( target, nearestNodes, 0xFFFFFFFF );

            if ( false == nearestNodes.IsEmpty() )
            {
                FLOG_WARN( "Did you mean one of these ?" );
                const size_t count = nearestNodes.GetSize();
                for ( size_t j = 0 ; j < count ; ++j )
                {
                    FLOG_WARN( "    %s", nearestNodes[j].m_Node->GetName().Get() );
                }
            }

            return false;
        }
        outDeps.EmplaceBack( node );
    }

    return true;
}

// Build
//------------------------------------------------------------------------------
bool FBuild::Build( const Array< AString > & targets )
{
    // create a temporary node, not hooked into the DB
    NodeProxy proxy( AStackString< 32 >( "*proxy*" ) );
    Dependencies deps( targets.GetSize(), 0 );
    if ( !GetTargets( targets, deps ) )
    {
        return false; // GetTargets will have emitted an error
    }
    proxy.m_StaticDependencies = deps;

    // build all targets in one sweep
    const bool result = Build( &proxy );

    // output per-target results
    for ( size_t i=0; i<targets.GetSize(); ++i )
    {
        const bool nodeResult = ( deps[ i ].GetNode()->GetState() == Node::UP_TO_DATE );
        OUTPUT( "FBuild: %s: %s\n", nodeResult ? "OK" : "Error: BUILD FAILED", targets[ i ].Get() );
    }

    return result;
}

// SaveDependencyGraph
//------------------------------------------------------------------------------
bool FBuild::SaveDependencyGraph( const char * nodeGraphDBFile ) const
{
    PROFILE_FUNCTION;
    BuildProfilerScope buildProfileScope( "SaveDB" );

    ASSERT( nodeGraphDBFile != nullptr );

    FLOG_VERBOSE( "Saving DepGraph '%s'", nodeGraphDBFile );

    const Timer t;

    // serialize into memory first
    MemoryStream memoryStream( 32 * 1024 * 1024, 8 * 1024 * 1024 );
    m_DependencyGraph->Save( memoryStream, nodeGraphDBFile );

    // We'll save to a tmp file first
    AStackString<> tmpFileName( nodeGraphDBFile );
    tmpFileName += ".tmp";

    // Ensure output dir exists where we'll save the DB
    const char * lastSlash = tmpFileName.FindLast( '/' );
    lastSlash = lastSlash ? lastSlash : tmpFileName.FindLast( '\\' );
    if ( lastSlash )
    {
        AStackString<> pathOnly( tmpFileName.Get(), lastSlash );
        if ( FileIO::EnsurePathExists( pathOnly ) == false )
        {
            FLOG_ERROR( "Failed to create directory for DepGraph saving '%s'", pathOnly.Get() );
            return false;
        }
    }

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
        FLOG_ERROR( "Failed to rename temp DB file. Error: %s TmpFile: '%s'", LAST_ERROR_STR, tmpFileName.Get() );
        return false;
    }

    FLOG_VERBOSE( "Saving DepGraph Complete in %2.3fs", (double)t.GetElapsed() );
    return true;
}

// SaveDependencyGraph
//------------------------------------------------------------------------------
void FBuild::SaveDependencyGraph( IOStream & stream, const char* nodeGraphDBFile ) const
{
    m_DependencyGraph->Save( stream, nodeGraphDBFile );
}

// Build
//------------------------------------------------------------------------------
bool FBuild::Build( Node * nodeToBuild )
{
    ASSERT( nodeToBuild );

    AtomicStoreRelaxed( &s_StopBuild, false ); // allow multiple runs in same process
    AtomicStoreRelaxed( &s_AbortBuild, false ); // allow multiple runs in same process

    // create worker threads
    m_JobQueue = FNEW( JobQueue( m_Options.m_NumWorkerThreads ) );

    // create the connection management system if needed
    // (must be after JobQueue is created)
    if ( m_Options.m_AllowDistributed )
    {
        const SettingsNode * settings = m_DependencyGraph->GetSettings();

        // Worker list from Settings takes priority
        Array< AString > workers( settings->GetWorkerList() );
        if ( workers.IsEmpty() )
        {
            // check for workers through brokerage or environment
            m_WorkerBrokerage.FindWorkers( workers );
        }

        if ( workers.IsEmpty() )
        {
            FLOG_WARN( "No workers available - Distributed compilation disabled" );
            m_Options.m_AllowDistributed = false;
        }
        else
        {
            OUTPUT( "Distributed Compilation : %u Workers in pool '%s'\n", (uint32_t)workers.GetSize(), m_WorkerBrokerage.GetBrokerageRootPaths().Get() );
            m_Client = FNEW( Client( workers, m_Options.m_DistributionPort, settings->GetWorkerConnectionLimit(), m_Options.m_DistVerbose ) );
        }
    }

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

    if ( BuildProfiler::IsValid() )
    {
        BuildProfiler::Get().StartMetricsGathering();
    }

    bool stopping( false );

    // keep doing build passes until completed/failed
    {
        BuildProfilerScope buildProfileScope( "Build" );
        for ( ;; )
        {
            // process completed jobs
            m_JobQueue->FinalizeCompletedJobs( *m_DependencyGraph );

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

            const bool complete = ( nodeToBuild->GetState() == Node::UP_TO_DATE ) ||
                                  ( nodeToBuild->GetState() == Node::FAILED );

            if ( AtomicLoadRelaxed( &s_StopBuild ) || complete )
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
                    if ( m_Options.m_FastCancel )
                    {
                    // Notify the system that the main process has been killed and that it can kill its process.
                        AtomicStoreRelaxed( &s_AbortBuild, true );
                    }
                }
            }

            if ( !stopping )
            {
                if ( m_Options.m_WrapperMode == FBuildOptions::WRAPPER_MODE_FINAL_PROCESS )
                {
                    SystemMutex wrapperMutex( m_Options.GetMainProcessMutexName().Get() );
                    if ( wrapperMutex.TryLock() )
                    {
                        // parent process has terminated
                        AbortBuild();
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
        m_JobQueue->FinalizeCompletedJobs( *m_DependencyGraph );

        FDELETE m_JobQueue;
        m_JobQueue = nullptr;

        FLog::StopBuild();
    }

    if ( BuildProfiler::IsValid() )
    {
        BuildProfiler::Get().StopMetricsGathering();
    }

    // even if the build has failed, we can still save the graph.
    // This is desireable because:
    // - it will save parsing the bff next time
    // - it will record the items that did build, so they won't build again
    if ( m_Options.m_SaveDBOnCompletion )
    {
        SaveDependencyGraph( m_DependencyGraphFile.Get() );
    }

    // TODO:C Move this into BuildStats
    const float timeTaken = m_Timer.GetElapsed();
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
bool FBuild::ImportEnvironmentVar( const char * name, bool optional, AString & value, uint32_t & hash )
{
    // check if system environment contains the variable
    if ( Env::GetEnvVariable( name, value ) == false )
    {
        if ( !optional )
        {
            FLOG_ERROR( "Could not import environment variable '%s'", name );
            return false;
        }

        // set the hash to the "missing variable" value of 0
        hash = 0;
    }
    else
    {
        // compute hash value for actual value
        hash = xxHash::Calc32( value );
    }

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
    m_ImportedEnvironmentVars.EmplaceBack( name, hash );

    return true;
}

// AddFileExistsCheck
//------------------------------------------------------------------------------
bool FBuild::AddFileExistsCheck( const AString & fileName )
{
    return m_FileExistsInfo.CheckFile( fileName );
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

// AbortBuild
//------------------------------------------------------------------------------
void FBuild::AbortBuild()
{
    AtomicStoreRelaxed( &s_StopBuild, true );
    if ( FBuild::IsValid() && FBuild::Get().m_Options.m_FastCancel )
    {
        // Notify the system that the main process has been killed and that it can kill its process.
        AtomicStoreRelaxed( &s_AbortBuild, true );
    }
}

// OnBuildError
//------------------------------------------------------------------------------
/*static*/ void FBuild::OnBuildError()
{
    if ( FBuild::Get().GetOptions().m_StopOnFirstError )
    {
        AbortBuild();
    }
}

// GetStopBuild
//------------------------------------------------------------------------------
/*static*/ bool FBuild::GetStopBuild()
{
    return AtomicLoadRelaxed( &s_StopBuild );
}

// UpdateBuildStatus
//------------------------------------------------------------------------------
void FBuild::UpdateBuildStatus( const Node * node )
{
    PROFILE_FUNCTION;

    if ( FBuild::Get().GetOptions().m_ShowProgress == false )
    {
        if ( FBuild::Get().GetOptions().m_EnableMonitor == false )
        {
            return;
        }
    }

    const float OUTPUT_FREQUENCY( 1.0f );
    const float CALC_FREQUENCY( 5.0f );

    const float timeNow = m_Timer.GetElapsed();

    const bool doUpdate = ( ( timeNow - m_LastProgressOutputTime ) >= OUTPUT_FREQUENCY );
    if ( doUpdate == false )
    {
        return;
    }

    // recalculate progress estimate?
    if ( ( timeNow - m_LastProgressCalcTime ) >= CALC_FREQUENCY )
    {
        PROFILE_SECTION( "CalcPogress" );

        FBuildStats & bs = m_BuildStats;
        bs.m_NodeTimeProgressms = 0;
        bs.m_NodeTimeTotalms = 0;
        m_DependencyGraph->UpdateBuildStatus( node, bs.m_NodeTimeProgressms, bs.m_NodeTimeTotalms );
        m_LastProgressCalcTime = m_Timer.GetElapsed();

        // calculate percentage
        const float doneRatio = (float)( (double)bs.m_NodeTimeProgressms / (double)bs.m_NodeTimeTotalms );

        // don't allow it to reach 100% (handles rounding inaccuracies)
        const float donePerc = Math::Min< float >( doneRatio * 100.0f, 99.9f );

        // don't allow progress to go backwards
        m_SmoothedProgressTarget = Math::Max< float >( donePerc, m_SmoothedProgressTarget );
    }

    m_SmoothedProgressCurrent = ( 0.5f * m_SmoothedProgressCurrent ) + ( m_SmoothedProgressTarget * 0.5f );

    // get node counts
    uint32_t numJobs = 0;
    uint32_t numJobsActive = 0;
    uint32_t numJobsDist = 0;
    uint32_t numJobsDistActive = 0;
    if ( JobQueue::IsValid() )
    {
        JobQueue::Get().GetJobStats( numJobs, numJobsActive, numJobsDist, numJobsDistActive );
    }

    if ( FBuild::Get().GetOptions().m_ShowProgress )
    {
        FLog::OutputProgress( timeNow, m_SmoothedProgressCurrent, numJobs, numJobsActive, numJobsDist, numJobsDistActive );
    }

    FLOG_MONITOR( "PROGRESS_STATUS %f \n", (double)m_SmoothedProgressCurrent );

    m_LastProgressOutputTime = timeNow;
}

// GetDefaultBFFFileName
//------------------------------------------------------------------------------
/*static*/ const char * FBuild::GetDefaultBFFFileName()
{
    return "fbuild.bff";
}

// DisplayTargetList
//------------------------------------------------------------------------------
void FBuild::DisplayTargetList( bool showHidden ) const
{
    OUTPUT( "FBuild: List of available targets\n" );
    const size_t totalNodes = m_DependencyGraph->GetNodeCount();
    for ( size_t i = 0; i < totalNodes; ++i )
    {
        const Node * node = m_DependencyGraph->GetNodeByIndex( i );
        bool displayName = false;
        bool hidden = node->IsHidden();
        switch ( node->GetType() )
        {
            case Node::PROXY_NODE:          ASSERT( false ); break;
            case Node::COPY_FILE_NODE:      break;
            case Node::DIRECTORY_LIST_NODE: break;
            case Node::EXEC_NODE:           break;
            case Node::FILE_NODE:           break;
            case Node::LIBRARY_NODE:        break;
            case Node::OBJECT_NODE:         break;
            case Node::ALIAS_NODE:          displayName = true; hidden = node->IsHidden(); break;
            case Node::EXE_NODE:            break;
            case Node::CS_NODE:             break;
            case Node::UNITY_NODE:          displayName = true; hidden = node->IsHidden(); break;
            case Node::TEST_NODE:           break;
            case Node::COMPILER_NODE:       break;
            case Node::DLL_NODE:            break;
            case Node::VCXPROJECT_NODE:     break;
            case Node::VSPROJEXTERNAL_NODE: break;
            case Node::OBJECT_LIST_NODE:    displayName = true; hidden = node->IsHidden(); break;
            case Node::COPY_DIR_NODE:       break;
            case Node::SLN_NODE:            break;
            case Node::REMOVE_DIR_NODE:     break;
            case Node::XCODEPROJECT_NODE:   break;
            case Node::SETTINGS_NODE:       break;
            case Node::TEXT_FILE_NODE:      displayName = true; hidden = node->IsHidden(); break;
            case Node::LIST_DEPENDENCIES_NODE: break;
            case Node::NUM_NODE_TYPES:      ASSERT( false );                        break;
        }
        if ( displayName && ( !hidden || showHidden ) )
        {
            OUTPUT( "\t%s\n", node->GetName().Get() );
        }
    }
}

// DisplayDependencyDB
//------------------------------------------------------------------------------
bool FBuild::DisplayDependencyDB( const Array< AString > & targets ) const
{
    AString buffer( 10 * 1024 * 1024 );

    // Get the nodes for the targets, or leave empty to display everything
    Dependencies deps;
    if ( targets.IsEmpty() == false )
    {
        if ( !GetTargets( targets, deps ) )
        {
            return false; // GetTargets will have emitted an error
        }
    }

    OUTPUT( "FBuild: Dependency database\n" );
    m_DependencyGraph->SerializeToText( deps, buffer );
    OUTPUT( "%s", buffer.Get() );
    return true;
}

// GenerateDotGraph
//------------------------------------------------------------------------------
bool FBuild::GenerateDotGraph( const Array< AString > & targets, const bool fullGraph ) const
{
    // Get the nodes for the targets, or leave empty to get everything
    Dependencies deps;
    if ( targets.IsEmpty() == false )
    {
        if ( !GetTargets( targets, deps ) )
        {
            return false; // GetTargets will have emitted an error
        }
    }

    const char * const dotFileName = "fbuild.gv";
    OUTPUT( "Saving DOT graph file to '%s'\n", dotFileName );

    // Generate
    AString buffer( 10 * 1024 * 1024 );    
    m_DependencyGraph->SerializeToDotFormat( deps, fullGraph, buffer );

    // Write to disk
    FileStream f;
    if ( f.Open( dotFileName, FileStream::WRITE_ONLY ) &&
         ( f.WriteBuffer( buffer.Get(), buffer.GetLength() ) == buffer.GetLength() ) )
    {
        return true;
    }
    FLOG_ERROR( "Failed to DOT file '%s'\n", dotFileName );
    return false;
}

// GenerateCompilationDatabase
//------------------------------------------------------------------------------
bool FBuild::GenerateCompilationDatabase( const Array< AString > & targets ) const
{
    Dependencies deps;
    if ( !GetTargets( targets, deps ) )
    {
        return false; // GetTargets will have emitted an error
    }

    CompilationDatabase compdb;
    const AString & result = compdb.Generate( *m_DependencyGraph, deps );

    FileStream fs;
    if ( fs.Open( "compile_commands.json", FileStream::WRITE_ONLY ) == false )
    {
        FLOG_ERROR( "Failed to open compile_commands.json" );
        return false;
    }
    if ( fs.Write( result.Get(), result.GetLength() ) != result.GetLength() )
    {
        FLOG_ERROR( "Failed to write to compile_commands.json" );
        return false;
    }
    fs.Close();

    return true;
}

// GetTempDir
//------------------------------------------------------------------------------
/*static*/ bool FBuild::GetTempDir( AString & outTempDir )
{
    #if defined( __WINDOWS__ ) || defined( __LINUX__ ) || defined( __APPLE__ )
        // Check for override environment variable
        if ( Env::GetEnvVariable( "FASTBUILD_TEMP_PATH", outTempDir ) )
        {
            // Ensure env var was slash terminated
            #if defined( __WINDOWS__ )
                const bool slashTerminated = ( outTempDir.EndsWith( '/' ) || outTempDir.EndsWith( '\\' ) );
                if ( !slashTerminated )
                {
                    outTempDir += '\\';
                }
            #else
                const bool slashTerminated = outTempDir.EndsWith( '/' );
                if ( !slashTerminated )
                {
                    outTempDir += '/';
                }
            #endif

            return true;
        }
    #endif

    // Use regular system temp path
    return FileIO::GetTempDir( outTempDir );
}

// CacheOutputInfo
//------------------------------------------------------------------------------
bool FBuild::CacheOutputInfo() const
{
    OUTPUT( "CacheInfo:\n" );
    if ( m_Cache )
    {
        return m_Cache->OutputInfo( m_Options.m_ShowProgress );
    }

    OUTPUT( "- Cache not configured\n" );
    return false;
}

// CacheTrim
//------------------------------------------------------------------------------
bool FBuild::CacheTrim() const
{
    OUTPUT( "CacheTrim:\n" );
    if ( m_Cache )
    {
        return m_Cache->Trim( m_Options.m_ShowProgress, m_Options.m_CacheTrim );
    }

    OUTPUT( "- Cache not configured\n" );
    return false;
}

//------------------------------------------------------------------------------
