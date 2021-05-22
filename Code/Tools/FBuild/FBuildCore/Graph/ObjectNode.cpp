// ObjectNode.cpp
//------------------------------------------------------------------------------

// Includes
//------------------------------------------------------------------------------
#include "ObjectNode.h"

#include "Tools/FBuild/FBuildCore/BFF/Functions/FunctionObjectList.h"
#include "Tools/FBuild/FBuildCore/Cache/ICache.h"
#include "Tools/FBuild/FBuildCore/ExeDrivers/Compiler/CompilerDriverBase.h"
#include "Tools/FBuild/FBuildCore/ExeDrivers/Compiler/CompilerDriver_CL.h"
#include "Tools/FBuild/FBuildCore/ExeDrivers/Compiler/CompilerDriver_CodeWarriorWii.h"
#include "Tools/FBuild/FBuildCore/ExeDrivers/Compiler/CompilerDriver_CUDA.h"
#include "Tools/FBuild/FBuildCore/ExeDrivers/Compiler/CompilerDriver_GCCClang.h"
#include "Tools/FBuild/FBuildCore/ExeDrivers/Compiler/CompilerDriver_Generic.h"
#include "Tools/FBuild/FBuildCore/ExeDrivers/Compiler/CompilerDriver_GreenHillsWiiU.h"
#include "Tools/FBuild/FBuildCore/ExeDrivers/Compiler/CompilerDriver_OrbisWavePSSLC.h"
#include "Tools/FBuild/FBuildCore/ExeDrivers/Compiler/CompilerDriver_QtRCC.h"
#include "Tools/FBuild/FBuildCore/ExeDrivers/Compiler/CompilerDriver_SNC.h"
#include "Tools/FBuild/FBuildCore/ExeDrivers/Compiler/CompilerDriver_VBCC.h"
#include "Tools/FBuild/FBuildCore/FBuild.h"
#include "Tools/FBuild/FBuildCore/FLog.h"
#include "Tools/FBuild/FBuildCore/Cache/LightCache.h"
#include "Tools/FBuild/FBuildCore/Graph/CompilerNode.h"
#include "Tools/FBuild/FBuildCore/Graph/NodeGraph.h"
#include "Tools/FBuild/FBuildCore/Graph/NodeProxy.h"
#include "Tools/FBuild/FBuildCore/Graph/SettingsNode.h"
#include "Tools/FBuild/FBuildCore/Helpers/Args.h"
#include "Tools/FBuild/FBuildCore/Helpers/BuildProfiler.h"
#include "Tools/FBuild/FBuildCore/Helpers/CIncludeParser.h"
#include "Tools/FBuild/FBuildCore/Helpers/Compressor.h"
#include "Tools/FBuild/FBuildCore/Helpers/MultiBuffer.h"
#include "Tools/FBuild/FBuildCore/Helpers/ResponseFile.h"
#include "Tools/FBuild/FBuildCore/Helpers/ToolManifest.h"
#include "Tools/FBuild/FBuildCore/WorkerPool/Job.h"
#include "Tools/FBuild/FBuildCore/WorkerPool/JobQueue.h"
#include "Tools/FBuild/FBuildCore/WorkerPool/WorkerThread.h"

// Core
#include "Core/Containers/UniquePtr.h"
#include "Core/Env/Env.h"
#include "Core/Env/ErrorFormat.h"
#include "Core/FileIO/ConstMemoryStream.h"
#include "Core/FileIO/FileIO.h"
#include "Core/FileIO/FileStream.h"
#include "Core/FileIO/PathUtils.h"
#include "Core/Math/xxHash.h"
#include "Core/Process/Process.h"
#include "Core/Profile/Profile.h"
#include "Core/Process/Thread.h"
#include "Core/Time/Time.h"
#include "Core/Tracing/Tracing.h"
#include "Core/Strings/AStackString.h"

#include <string.h>
#if defined( __OSX__ ) || defined( __LINUX__ )
    #include <sys/time.h>
#endif

// Reflection
//------------------------------------------------------------------------------
REFLECT_NODE_BEGIN( ObjectNode, Node, MetaNone() )
    REFLECT( m_Compiler,                            "Compiler",                         MetaFile() + MetaAllowNonFile())
    REFLECT( m_CompilerOptions,                     "CompilerOptions",                  MetaNone() )
    REFLECT( m_CompilerOptionsDeoptimized,          "CompilerOptionsDeoptimized",       MetaOptional() )
    REFLECT( m_CompilerInputFile,                   "CompilerInputFile",                MetaFile() )
    REFLECT( m_CompilerOutputExtension,             "CompilerOutputExtension",          MetaOptional() )
    REFLECT( m_PCHObjectFileName,                   "PCHObjectFileName",                MetaOptional() + MetaFile() )
    REFLECT( m_DeoptimizeWritableFiles,             "DeoptimizeWritableFiles",          MetaOptional() )
    REFLECT( m_DeoptimizeWritableFilesWithToken,    "DeoptimizeWritableFilesWithToken", MetaOptional() )
    REFLECT( m_AllowDistribution,                   "AllowDistribution",                MetaOptional() )
    REFLECT( m_AllowCaching,                        "AllowCaching",                     MetaOptional() )
    REFLECT_ARRAY( m_CompilerForceUsing,            "CompilerForceUsing",               MetaOptional() + MetaFile() )

    // Preprocessor
    REFLECT( m_Preprocessor,                        "Preprocessor",                     MetaOptional() + MetaFile() + MetaAllowNonFile())
    REFLECT( m_PreprocessorOptions,                 "PreprocessorOptions",              MetaOptional() )

    REFLECT_ARRAY( m_PreBuildDependencyNames,       "PreBuildDependencies",             MetaOptional() + MetaFile() + MetaAllowNonFile() )

    // Internal State
    REFLECT( m_PrecompiledHeader,                   "PrecompiledHeader",                MetaHidden() )
    REFLECT( m_CompilerFlags.m_Flags,               "CompilerFlags",                    MetaHidden() )
    REFLECT( m_PreprocessorFlags.m_Flags,           "PreprocessorFlags",                MetaHidden() )
    REFLECT( m_PCHCacheKey,                         "PCHCacheKey",                      MetaHidden() + MetaIgnoreForComparison() )
    REFLECT( m_OwnerObjectList,                     "OwnerObjectList",                  MetaHidden() )
REFLECT_END( ObjectNode )

// CONSTRUCTOR
//------------------------------------------------------------------------------
ObjectNode::ObjectNode()
: FileNode( AString::GetEmpty(), Node::FLAG_NONE )
{
    m_Type = OBJECT_NODE;
    m_LastBuildTimeMs = 5000; // higher default than a file node
}

// Initialize
//------------------------------------------------------------------------------
/*virtual*/ bool ObjectNode::Initialize( NodeGraph & nodeGraph, const BFFToken * iter, const Function * function )
{
    ASSERT( m_OwnerObjectList.IsEmpty() == false ); // Must be set before we get here

    // .PreBuildDependencies
    if ( !InitializePreBuildDependencies( nodeGraph, iter, function, m_PreBuildDependencyNames ) )
    {
        return false; // InitializePreBuildDependencies will have emitted an error
    }

    // .Compiler
    CompilerNode * compiler( nullptr );
    if ( !Function::GetCompilerNode( nodeGraph, iter, function, m_Compiler, compiler ) )
    {
        return false; // GetCompilerNode will have emitted an error
    }

    // .CompilerInputFile
    Dependencies compilerInputFile;
    if ( !Function::GetFileNode( nodeGraph, iter, function, m_CompilerInputFile, ".CompilerInputFile", compilerInputFile ) )
    {
        return false; // GetFileNode will have emitted an error
    }
    ASSERT( compilerInputFile.GetSize() == 1 ); // Should not be possible to expand to > 1 thing

    // .Preprocessor
    CompilerNode * preprocessor( nullptr );
    if ( m_Preprocessor.IsEmpty() == false )
    {
        if ( !Function::GetCompilerNode( nodeGraph, iter, function, m_Preprocessor, preprocessor ) )
        {
            return false; // GetCompilerNode will have emitted an error
        }
    }

    // .CompilerForceUsing
    Dependencies compilerForceUsing;
    if ( !Function::GetFileNodes( nodeGraph, iter, function, m_CompilerForceUsing, ".CompilerForceUsing", compilerForceUsing ) )
    {
        return false; // GetFileNode will have emitted an error
    }

    // Precompiled Header
    Dependencies precompiledHeader;
    if ( m_PrecompiledHeader.IsEmpty() == false )
    {
        // m_PrecompiledHeader is only set if our associated ObjectList or Library created one
        VERIFY( Function::GetFileNode( nodeGraph, iter, function, m_PrecompiledHeader, ".PrecompiledHeader", precompiledHeader ) );
        ASSERT( precompiledHeader.GetSize() == 1 );
    }

    // Store Dependencies
    m_StaticDependencies.SetCapacity( 1 + 1 + precompiledHeader.GetSize() + ( preprocessor ? 1 : 0 ) + compilerForceUsing.GetSize() );
    m_StaticDependencies.EmplaceBack( compiler );
    m_StaticDependencies.Append( compilerInputFile );
    m_StaticDependencies.Append( precompiledHeader );
    if ( preprocessor )
    {
        m_StaticDependencies.EmplaceBack( preprocessor );
    }
    m_StaticDependencies.Append( compilerForceUsing );

    return true;
}

// CONSTRUCTOR (Remote)
//------------------------------------------------------------------------------
ObjectNode::ObjectNode( const AString & objectName,
                        NodeProxy * srcFile,
                        const AString & compilerOptions,
                        uint32_t flags )
: FileNode( objectName, Node::FLAG_NONE )
, m_CompilerOptions( compilerOptions )
, m_Remote( true )
{
    m_Type = OBJECT_NODE;
    m_LastBuildTimeMs = 5000; // higher default than a file node
    m_CompilerFlags.m_Flags = flags;

    m_StaticDependencies.SetCapacity( 2 );
    m_StaticDependencies.EmplaceBack( nullptr );
    m_StaticDependencies.EmplaceBack( srcFile );
}

// DESTRUCTOR
//------------------------------------------------------------------------------
ObjectNode::~ObjectNode()
{
    // remote worker owns the ProxyNode for the source file, so must free it
    if ( m_Remote )
    {
        Node * srcFile = GetSourceFile();
        ASSERT( srcFile->GetType() == Node::PROXY_NODE );
        FDELETE srcFile;
    }
}

// DoBuild
//------------------------------------------------------------------------------
/*virtual*/ Node::BuildResult ObjectNode::DoBuild( Job * job )
{
    // Set a sensible catch-all default name for compilation. This will be modified
    // for various cases
    job->GetBuildProfilerScope()->SetStepName( "Compile" );

    // Delete previous file(s) if doing a clean build
    if ( FBuild::Get().GetOptions().m_ForceCleanBuild )
    {
        if ( DoPreBuildFileDeletion( GetName() ) == false )
        {
            return NODE_RESULT_FAILED; // HandleFileDeletion will have emitted an error
        }
        if ( IsMSVC() && IsCreatingPCH() )
        {
            if ( DoPreBuildFileDeletion( m_PCHObjectFileName ) == false )
            {
                return NODE_RESULT_FAILED; // HandleFileDeletion will have emitted an error
            }
        }
    }

    // Reset PCH cache key - will be set correctly if we end up using the cache
    if ( IsMSVC() && IsCreatingPCH() )
    {
        m_PCHCacheKey = 0;
    }

    // using deoptimization?
    bool useDeoptimization = ShouldUseDeoptimization();

    bool useCache = ShouldUseCache();
    bool useDist = m_CompilerFlags.IsDistributable() && m_AllowDistribution && FBuild::Get().GetOptions().m_AllowDistributed;
    bool useSimpleDist = GetCompiler()->SimpleDistributionMode();
    bool usePreProcessor = !useSimpleDist && ( useCache || useDist || IsGCC() || IsSNC() || IsClang() || IsClangCl() || IsCodeWarriorWii() || IsGreenHillsWiiU() || IsVBCC() || IsOrbisWavePSSLC() );
    if ( GetDedicatedPreprocessor() )
    {
        usePreProcessor = true;
        useDeoptimization = false; // disable deoptimization
    }

    // Graphing the current amount of distributable jobs
    FLOG_MONITOR( "GRAPH FASTBuild \"Distributable Jobs MemUsage\" MB %f\n", (double)( (float)Job::GetTotalLocalDataMemoryUsage() / (float)MEGABYTE ) );

    if ( usePreProcessor || useSimpleDist )
    {
        return DoBuildWithPreProcessor( job, useDeoptimization, useCache, useSimpleDist );
    }

    if ( IsMSVC() )
    {
        return DoBuildMSCL_NoCache( job, useDeoptimization );
    }

    if ( IsQtRCC() )
    {
        return DoBuild_QtRCC( job );
    }

    return DoBuildOther( job, useDeoptimization );
}

// DoBuild_Remote
//------------------------------------------------------------------------------
/*virtual*/ Node::BuildResult ObjectNode::DoBuild2( Job * job, bool racingRemoteJob = false )
{
    job->GetBuildProfilerScope()->SetStepName( racingRemoteJob ? "Compile (Race)" : "Compile" );

    // we may be using deoptimized options, but they are always
    // the "normal" args when remote compiling
    const bool useDeoptimization = job->IsLocal() && ShouldUseDeoptimization();
    const bool stealingRemoteJob = job->IsLocal(); // are we stealing a remote job?
    const bool isFollowingLightCacheMiss = false;
    return DoBuildWithPreProcessor2( job, useDeoptimization, stealingRemoteJob, racingRemoteJob, isFollowingLightCacheMiss );
}

// Finalize
//------------------------------------------------------------------------------
/*virtual*/ bool ObjectNode::Finalize( NodeGraph & nodeGraph )
{
    ASSERT( Thread::IsMainThread() );

    // convert includes to nodes
    m_DynamicDependencies.Clear();
    m_DynamicDependencies.SetCapacity( m_Includes.GetSize() );
    for ( Array< AString >::ConstIter it = m_Includes.Begin();
            it != m_Includes.End();
            it++ )
    {
        Node * fn = nodeGraph.FindNode( *it );
        if ( fn == nullptr )
        {
            fn = nodeGraph.CreateFileNode( *it );
        }
        else if ( fn->IsAFile() == false )
        {
            FLOG_ERROR( "'%s' is not a FileNode (type: %s)", fn->GetName().Get(), fn->GetTypeName() );
            return false;
        }

        // Ensure files that are seen for the first time here have their
        // mod time recorded in the database
        if ( ( fn->GetType() == Node::FILE_NODE ) &&
             ( fn->GetStamp() == 0 ) &&
             ( fn->GetStatFlag( Node::STATS_BUILT ) == false ) )
        {
            fn->CastTo< FileNode >()->DoBuild( nullptr );
        }

        m_DynamicDependencies.EmplaceBack( fn );
    }

    Node::Finalize( nodeGraph );

    return true;
}

// Migrate
//------------------------------------------------------------------------------
/*virtual*/ void ObjectNode::Migrate( const Node & oldNode )
{
    // Migrate Node level properties
    Node::Migrate( oldNode );

    // Migrate the PCHCacheKey if there is one. This special case property is
    // lazily determined during a build, but needs to persist across migrations
    // to prevent unnecessary rebuilds of object that depend on this one, if this
    // is a precompiled header object.
    m_PCHCacheKey = oldNode.CastTo< ObjectNode >()->m_PCHCacheKey;
}

// DoBuildMSCL_NoCache
//------------------------------------------------------------------------------
/*virtual*/ Node::BuildResult ObjectNode::DoBuildMSCL_NoCache( Job * job, bool useDeoptimization )
{
    // Format compiler args string
    Args fullArgs;
    const bool showIncludes( true );
    const bool useSourceMapping( true );
    const bool finalize( true );
    if ( !BuildArgs( job, fullArgs, PASS_COMPILE, useDeoptimization, showIncludes, useSourceMapping, finalize ) )
    {
        return NODE_RESULT_FAILED; // BuildArgs will have emitted an error
    }

    EmitCompilationMessage( fullArgs, useDeoptimization );

    // spawn the process
    CompileHelper ch;
    if ( !ch.SpawnCompiler( job, GetName(), GetCompiler(), GetCompiler()->GetExecutable(), fullArgs ) ) // use response file for MSVC
    {
        return NODE_RESULT_FAILED; // SpawnCompiler has logged error
    }

    // Handle MSCL warnings if not already a failure
    // If "warnings as errors" is enabled (/WX) we don't need to check
    // (since compilation will fail anyway, and the output will be shown)
    if ( ( ch.GetResult() == 0 ) && !m_CompilerFlags.IsWarningsAsErrorsMSVC() )
    {
        if ( IsClangCl() )
        {
            HandleWarningsClangCl( job, GetName(), ch.GetErr() );
            HandleWarningsClangCl( job, GetName(), ch.GetOut() );
        }
        else
        {
            HandleWarningsMSVC( job, GetName(), ch.GetOut() );
        }
    }

    const char *output = nullptr;
    uint32_t outputSize = 0;

    // MSVC will write /ShowIncludes output on stderr sometimes (ex: /Fi)
    if ( IsIncludesInStdErr() == true )
    {
        output = ch.GetErr().Get();
        outputSize = ch.GetErr().GetLength();
    }
    else
    {
        // but most of the time it will be on stdout
        output = ch.GetOut().Get();
        outputSize = ch.GetOut().GetLength();
    }

    // compiled ok, try to extract includes
    if ( ProcessIncludesMSCL( output, outputSize ) == false )
    {
        return NODE_RESULT_FAILED; // ProcessIncludesMSCL will have emitted an error
    }

    // record new file time
    RecordStampFromBuiltFile();

    return NODE_RESULT_OK;
}

// DoBuildWithPreProcessor
//------------------------------------------------------------------------------
Node::BuildResult ObjectNode::DoBuildWithPreProcessor( Job * job, bool useDeoptimization, bool useCache, bool useSimpleDist )
{
    job->GetBuildProfilerScope()->SetStepName( "Preprocess" );

    Args fullArgs;
    const bool showIncludes( false );
    const bool useSourceMapping( true );
    const bool finalize( true );
    Pass pass = useSimpleDist ? PASS_PREP_FOR_SIMPLE_DISTRIBUTION : PASS_PREPROCESSOR_ONLY;
    if ( !BuildArgs( job, fullArgs, pass, useDeoptimization, showIncludes, useSourceMapping, finalize ) )
    {
        return NODE_RESULT_FAILED; // BuildArgs will have emitted an error
    }

    // Try to use the light cache if enabled
    if ( useCache && GetCompiler()->GetUseLightCache() )
    {
        LightCache lc;
        if ( lc.Hash( this, fullArgs.GetFinalArgs(), m_LightCacheKey, m_Includes ) == false )
        {
            // Light cache could not be used (can't parse includes)
            if ( FBuild::Get().GetOptions().m_CacheVerbose )
            {
                FLOG_OUTPUT( "LightCache cannot be used for '%s'\n"
                             "%s",
                              GetName().Get(),
                              lc.GetErrors().Get() );
            }

            // Fall through to generate preprocessed output for old style cache and distribution....
        }
        else
        {
            // LightCache hashing was successful
            SetStatFlag( Node::STATS_LIGHT_CACHE ); // Light compatible

            // Try retrieve from cache
            GetCacheName( job ); // Prepare the cache key (always done here even if write only mode)
            if ( RetrieveFromCache( job ) )
            {
                return NODE_RESULT_OK_CACHE;
            }

            // Cache miss
            const bool belowMemoryLimit = ( ( Job::GetTotalLocalDataMemoryUsage() / MEGABYTE ) < FBuild::Get().GetSettings()->GetDistributableJobMemoryLimitMiB() );
            const bool canDistribute = belowMemoryLimit && m_CompilerFlags.IsDistributable() && m_AllowDistribution && FBuild::Get().GetOptions().m_AllowDistributed;
            if ( canDistribute == false )
            {
                // can't distribute, so generating preprocessed output is useless
                // so we directly compile from source as one-pass compilation is faster
                const bool stealingRemoteJob = false; // never queued
                const bool racingRemoteJob = false; // never queued
                const bool isFollowingLightCacheMiss = true;
                return DoBuildWithPreProcessor2( job, useDeoptimization, stealingRemoteJob, racingRemoteJob, isFollowingLightCacheMiss );
            }

            // Fall through to generate preprocessed output for distribution....
        }
    }

    if ( pass == PASS_PREPROCESSOR_ONLY )
    {
        if ( BuildPreprocessedOutput( fullArgs, job, useDeoptimization ) == false )
        {
            return NODE_RESULT_FAILED; // BuildPreprocessedOutput will have emitted an error
        }

        // preprocessed ok, try to extract includes
        if ( ProcessIncludesWithPreProcessor( job ) == false )
        {
            return NODE_RESULT_FAILED; // ProcessIncludesWithPreProcessor will have emitted an error
        }
    }

    if ( pass == PASS_PREP_FOR_SIMPLE_DISTRIBUTION )
    {
        if ( LoadStaticSourceFileForDistribution( fullArgs, job, useDeoptimization ) == false )
        {
            return NODE_RESULT_FAILED; // BuildPreprocessedOutput will have emitted an error
        }
    }

    // Do Clang unity fixup if needed
    if ( IsUnity() &&
         ( IsClang() || IsClangCl() ) &&
         GetCompiler()->IsClangUnityFixupEnabled() )
    {
        DoClangUnityFixup( job );
    }

    // calculate the cache entry lookup
    if ( useCache )
    {
        // try to get from cache
        GetCacheName( job ); // Prepare the cache key (always done here even if write only mode)
        if ( RetrieveFromCache( job ) )
        {
            return NODE_RESULT_OK_CACHE;
        }
    }

    // can we do the rest of the work remotely?
    const bool canDistribute = useSimpleDist || ( m_CompilerFlags.IsDistributable() && m_AllowDistribution && FBuild::Get().GetOptions().m_AllowDistributed );
    const bool belowMemoryLimit = ( ( Job::GetTotalLocalDataMemoryUsage() / MEGABYTE ) < FBuild::Get().GetSettings()->GetDistributableJobMemoryLimitMiB() );
    if ( canDistribute && belowMemoryLimit )
    {
        // compress job data
        Compressor c;
        c.Compress( job->GetData(), job->GetDataSize() );
        size_t compressedSize = c.GetResultSize();
        job->OwnData( c.ReleaseResult(), compressedSize, true );

        // yes... re-queue for secondary build
        return NODE_RESULT_NEED_SECOND_BUILD_PASS;
    }

    // can't do the work remotely, so do it right now
    const bool stealingRemoteJob = false; // never queued
    const bool racingRemoteJob = false;
    const bool isFollowingLightCacheMiss = false;
    const Node::BuildResult result = DoBuildWithPreProcessor2( job, useDeoptimization, stealingRemoteJob, racingRemoteJob, isFollowingLightCacheMiss );
    if ( result != Node::NODE_RESULT_OK )
    {
        return result;
    }

    return Node::NODE_RESULT_OK;
}

// DoBuildWithPreProcessor2
//------------------------------------------------------------------------------
Node::BuildResult ObjectNode::DoBuildWithPreProcessor2( Job * job, bool useDeoptimization, bool stealingRemoteJob, bool racingRemoteJob, bool isFollowingLightCacheMiss )
{
    job->GetBuildProfilerScope()->SetStepName( racingRemoteJob ? "Compile (Race)" : "Compile" );

    // should never use preprocessor if using CLR
    ASSERT( IsUsingCLR() == false );

    bool usePreProcessedOutput = true;
    if ( job->IsLocal() )
    {
        if ( IsClang() ||
             IsClangCl() ||
             IsGCC() ||
             IsSNC() )
        {
            // Using the PCH with Clang/SNC/GCC doesn't prevent storing to the cache
            // so we can use the PCH accelerated compilation
            if ( IsUsingPCH() )
            {
                usePreProcessedOutput = false;
            }

            // Creating a PCH must not use the preprocessed output, or we'll create
            // a PCH which cannot be used for acceleration
            if ( IsCreatingPCH() )
            {
                usePreProcessedOutput = false;
            }
        }

        if ( IsMSVC() )
        {
            // If using the PCH, we can't use the preprocessed output
            // as that's incompatible with the PCH
            if ( IsUsingPCH() )
            {
                usePreProcessedOutput = false;
            }

            // If creating the PCH, we can't use the preprocessed info
            // as this would prevent acceleration by users of the PCH
            if ( IsCreatingPCH() )
            {
                usePreProcessedOutput = false;
            }

            // Compiling with /analyze cannot use the preprocessed output
            // as it results in inconsistent behavior with the _PREFAST_ macro
            if ( IsUsingStaticAnalysisMSVC() )
            {
                usePreProcessedOutput = false;
            }
        }

        // The CUDA compiler seems to not be able to compile its own preprocessed output
        if ( IsCUDANVCC() )
        {
            usePreProcessedOutput = false;
        }

        if ( IsVBCC() )
        {
            usePreProcessedOutput = false;
        }

        // We might not have preprocessed data if using the LightCache
        if ( job->GetData() == nullptr )
        {
            usePreProcessedOutput = false;
        }
    }

    Args fullArgs;
    AStackString<> tmpDirectoryName;
    AStackString<> tmpFileName;
    if ( usePreProcessedOutput )
    {
        if ( WriteTmpFile( job, tmpDirectoryName, tmpFileName ) == false )
        {
            return NODE_RESULT_FAILED; // WriteTmpFile will have emitted an error
        }

        const bool showIncludes( false );
        const bool useSourceMapping( true );
        const bool finalize( true );
        if ( !BuildArgs( job, fullArgs, PASS_COMPILE_PREPROCESSED, useDeoptimization, showIncludes, useSourceMapping, finalize, tmpFileName ) )
        {
            return NODE_RESULT_FAILED; // BuildArgs will have emitted an error
        }
    }
    else
    {
        const bool showIncludes( false );
        const bool useSourceMapping( true );
        const bool finalize( true );
        if ( !BuildArgs( job, fullArgs, PASS_COMPILE, useDeoptimization, showIncludes, useSourceMapping, finalize ) )
        {
            return NODE_RESULT_FAILED; // BuildArgs will have emitted an error
        }
    }

    const bool verbose = FLog::ShowVerbose();
    const bool showCommands = ( FBuild::IsValid() && FBuild::Get().GetOptions().m_ShowCommandLines );
    const bool isRemote = ( job->IsLocal() == false );
    if ( stealingRemoteJob || racingRemoteJob || verbose || showCommands || isRemote || isFollowingLightCacheMiss )
    {
        // show that we are locally consuming a remote job
        EmitCompilationMessage( fullArgs, useDeoptimization, stealingRemoteJob, racingRemoteJob, false, isRemote );
    }

    bool result = BuildFinalOutput( job, fullArgs );

    // cleanup temp file
    if ( tmpFileName.IsEmpty() == false )
    {
        FileIO::FileDelete( tmpFileName.Get() );
    }

    // cleanup temp directory
    if ( tmpDirectoryName.IsEmpty() == false )
    {
        FileIO::DirectoryDelete( tmpDirectoryName );
    }

    if ( result == false )
    {
        // If the failure is forced due to a local cancellation, mark up the profiler
        // state so we can see that
        if ( job->GetDistributionState() == Job::DIST_RACE_WON_REMOTELY_CANCEL_LOCAL )
        {
            job->GetBuildProfilerScope()->SetStepName( "Compile (Race Lost)" );
        }

        return NODE_RESULT_FAILED; // BuildFinalOutput will have emitted error
    }

    // record new file time
    if ( job->IsLocal() )
    {
        // record new file time
        RecordStampFromBuiltFile();

        const bool useCache = ShouldUseCache();
        if ( m_Stamp && useCache )
        {
            WriteToCache( job );
        }
    }

    return NODE_RESULT_OK;
}

// DoBuild_QtRCC
//------------------------------------------------------------------------------
Node::BuildResult ObjectNode::DoBuild_QtRCC( Job * job )
{
    // spawn the process to gather dependencies
    {
        // Format compiler args string
        const bool useDeoptimization( false );
        const bool showIncludes( false );
        const bool useSourceMapping( true );
        const bool finalize( true );
        Args fullArgs;
        if ( !BuildArgs( job, fullArgs, PASS_PREPROCESSOR_ONLY, useDeoptimization, showIncludes, useSourceMapping, finalize ) )
        {
            return NODE_RESULT_FAILED; // BuildArgs will have emitted an error
        }

        EmitCompilationMessage( fullArgs, false );

        CompileHelper ch;
        if ( !ch.SpawnCompiler( job, GetName(), GetCompiler(), GetCompiler()->GetExecutable(), fullArgs ) )
        {
            return NODE_RESULT_FAILED; // compile has logged error
        }

        // get output
        AString output( ch.GetOut() );
        output.Replace( '\r', '\n' ); // Normalize all carriage line endings

        // split into lines
        Array< AString > lines( 256, true );
        output.Tokenize( lines, '\n' );

        m_Includes.Clear();

        // extract paths and store them as includes
        for ( const AString & line : lines )
        {
            if ( line.GetLength() > 0 )
            {
                AStackString<> cleanedInclude;
                NodeGraph::CleanPath( line, cleanedInclude );
                m_Includes.Append( cleanedInclude );
            }
        }
    }

    // spawn the process to compile
    {
        // Format compiler args string
        const bool useDeoptimization( false );
        const bool showIncludes( false );
        const bool useSourceMapping( true );
        const bool finalize( true );
        Args fullArgs;
        if ( !BuildArgs( job, fullArgs, PASS_COMPILE, useDeoptimization, showIncludes, useSourceMapping, finalize ) )
        {
            return NODE_RESULT_FAILED; // BuildArgs will have emitted an error
        }

        CompileHelper ch;
        if ( !ch.SpawnCompiler( job, GetName(), GetCompiler(), GetCompiler()->GetExecutable(), fullArgs ) )
        {
            return NODE_RESULT_FAILED; // compile has logged error
        }
    }

    // record new file time
    RecordStampFromBuiltFile();

    return NODE_RESULT_OK;
}

// DoBuildOther
//------------------------------------------------------------------------------
/*virtual*/ Node::BuildResult ObjectNode::DoBuildOther( Job * job, bool useDeoptimization )
{
    // Format compiler args string
    Args fullArgs;
    const bool showIncludes( false );
    const bool useSourceMapping( true );
    const bool finalize( true );
    if ( !BuildArgs( job, fullArgs, PASS_COMPILE, useDeoptimization, showIncludes, useSourceMapping, finalize ) )
    {
        return NODE_RESULT_FAILED; // BuildArgs will have emitted an error
    }

    EmitCompilationMessage( fullArgs, useDeoptimization );

    // spawn the process
    CompileHelper ch;
    if ( !ch.SpawnCompiler( job, GetName(), GetCompiler(), GetCompiler()->GetExecutable(), fullArgs ) )
    {
        return NODE_RESULT_FAILED; // compile has logged error
    }

    // record new file time
    RecordStampFromBuiltFile();

    return NODE_RESULT_OK;
}

// ProcessIncludesMSCL
//------------------------------------------------------------------------------
bool ObjectNode::ProcessIncludesMSCL( const char * output, uint32_t outputSize )
{
    Timer t;

    {
        CIncludeParser parser;

        // It's possible to have no output (Clang CL) in which case the file
        // includes nothing
        if ( output && outputSize )
        {
            const bool result = parser.ParseMSCL_Output( output, outputSize );
            if ( result == false )
            {
                FLOG_ERROR( "Failed to process includes for '%s'", GetName().Get() );
                return false;
            }
        }

        // record that we have a list of includes
        // (we need a flag because we can't use the array size
        // as a determinator, because the file might not include anything)
        m_Includes.Clear();
        parser.SwapIncludes( m_Includes );
    }

    FLOG_VERBOSE( "Process Includes:\n - File: %s\n - Time: %u ms\n - Num : %u", m_Name.Get(), uint32_t( t.GetElapsedMS() ), uint32_t( m_Includes.GetSize() ) );

    return true;
}

// ProcessIncludesWithPreProcessor
//------------------------------------------------------------------------------
bool ObjectNode::ProcessIncludesWithPreProcessor( Job * job )
{
    Timer t;

    {
        const char * output = (char *)job->GetData();
        size_t outputSize = job->GetDataSize();

        // Unlike most compilers, VBCC writes preprocessed output to a file
        ConstMemoryStream vbccMemoryStream;
        if ( IsVBCC() )
        {
            if ( !GetVBCCPreprocessedOutput( vbccMemoryStream ) )
            {
                return false; // GetVBCCPreprocessedOutput handles error output
            }
            output = (const char *)vbccMemoryStream.GetData();
            outputSize = vbccMemoryStream.GetSize();
        }

        ASSERT( output && outputSize );

        CIncludeParser parser;
        bool msvcStyle;
        if ( GetDedicatedPreprocessor() != nullptr )
        {
            msvcStyle = m_PreprocessorFlags.IsMSVC() || m_PreprocessorFlags.IsCUDANVCC();
        }
        else
        {
            msvcStyle = m_CompilerFlags.IsMSVC() || m_CompilerFlags.IsCUDANVCC();
        }
        bool result = msvcStyle ? parser.ParseMSCL_Preprocessed( output, outputSize )
                                : parser.ParseGCC_Preprocessed( output, outputSize );
        if ( result == false )
        {
            FLOG_ERROR( "Failed to process includes for '%s'", GetName().Get() );
            return false;
        }

        // record that we have a list of includes
        // (we need a flag because we can't use the array size
        // as a determinator, because the file might not include anything)
        m_Includes.Clear();
        parser.SwapIncludes( m_Includes );
    }

    FLOG_VERBOSE( "Process Includes:\n - File: %s\n - Time: %u ms\n - Num : %u", m_Name.Get(), uint32_t( t.GetElapsedMS() ), uint32_t( m_Includes.GetSize() ) );

    return true;
}

// LoadRemote
//------------------------------------------------------------------------------
/*static*/ Node * ObjectNode::LoadRemote( IOStream & stream )
{
    AStackString<> name;
    AStackString<> sourceFile;
    uint32_t flags;
    AStackString<> compilerArgs;
    if ( ( stream.Read( name ) == false ) ||
         ( stream.Read( sourceFile ) == false ) ||
         ( stream.Read( flags ) == false ) ||
         ( stream.Read( compilerArgs ) == false ) )
    {
        return nullptr;
    }

    NodeProxy * srcFile = FNEW( NodeProxy( sourceFile ) );

    return FNEW( ObjectNode( name, srcFile, compilerArgs, flags ) );
}

// DetermineFlags
//------------------------------------------------------------------------------
/*static*/ ObjectNode::CompilerFlags ObjectNode::DetermineFlags( const CompilerNode * compilerNode,
                                                                 const AString & args,
                                                                 bool creatingPCH,
                                                                 bool usingPCH )
{
    CompilerFlags flags;

    // set flags known from the context the args will be used in
    if ( creatingPCH )
    {
        flags .Set( CompilerFlags::FLAG_CREATING_PCH );
    }
    if ( usingPCH )
    {
        flags.Set( CompilerFlags::FLAG_USING_PCH );
    }

    const bool isDistributableCompiler = compilerNode->CanBeDistributed();

    // Compiler Type - TODO:C Eliminate duplication of these flags
    const CompilerNode::CompilerFamily compilerFamily = compilerNode->GetCompilerFamily();
    switch ( compilerFamily )
    {
        case CompilerNode::CompilerFamily::CUSTOM:          break; // Nothing to do
        case CompilerNode::CompilerFamily::MSVC:            flags.Set( CompilerFlags::FLAG_MSVC );              break;
        case CompilerNode::CompilerFamily::CLANG:           flags.Set( CompilerFlags::FLAG_CLANG );             break;
        case CompilerNode::CompilerFamily::CLANG_CL:        flags.Set( CompilerFlags::FLAG_CLANG_CL );          break;
        case CompilerNode::CompilerFamily::GCC:             flags.Set( CompilerFlags::FLAG_GCC );               break;
        case CompilerNode::CompilerFamily::SNC:             flags.Set( CompilerFlags::FLAG_SNC );               break;
        case CompilerNode::CompilerFamily::CODEWARRIOR_WII: flags.Set( CompilerFlags::CODEWARRIOR_WII );        break;
        case CompilerNode::CompilerFamily::GREENHILLS_WIIU: flags.Set( CompilerFlags::GREENHILLS_WIIU );        break;
        case CompilerNode::CompilerFamily::CUDA_NVCC:       flags.Set( CompilerFlags::FLAG_CUDA_NVCC );         break;
        case CompilerNode::CompilerFamily::QT_RCC:          flags.Set( CompilerFlags::FLAG_QT_RCC );            break;
        case CompilerNode::CompilerFamily::VBCC:            flags.Set( CompilerFlags::FLAG_VBCC );              break;
        case CompilerNode::CompilerFamily::ORBIS_WAVE_PSSLC:flags.Set( CompilerFlags::FLAG_ORBIS_WAVE_PSSLC );  break;
        case CompilerNode::CompilerFamily::CSHARP:          ASSERT( false ); break; // Guarded in ObjectListNode::Initialize
    }

    // Source mappings are not currently forwarded so can only compiled locally
    const bool hasSourceMapping = ( compilerNode->GetSourceMapping().IsEmpty() == false );

    // Check MS compiler options
    if ( flags.IsMSVC() || flags.IsClangCl() )
    {
        bool usingCLR = false;
        bool usingWinRT = false;
        bool usingPreprocessorOnly = false;

        Array< AString > tokens;
        args.Tokenize( tokens );
        const AString * const end = tokens.End();
        for ( const AString * it = tokens.Begin(); it != end; ++it )
        {
            const AString & token = *it;

            if ( IsCompilerArg_MSVC( token, "Zi" ) || IsCompilerArg_MSVC( token, "ZI" ) )
            {
                if ( !flags.IsClangCl() ) // with clang-cl, Zi is an alias for /Z7, it does not produce PDBs
                {
                    flags.Set( CompilerFlags::FLAG_USING_PDB );
                }
            }
            else if ( IsStartOfCompilerArg_MSVC( token, "clr" ) )
            {
                usingCLR = true;
                flags.Set( CompilerFlags::FLAG_USING_CLR );
            }
            else if ( IsCompilerArg_MSVC( token, "ZW" ) )
            {
                usingWinRT = true;
            }
            else if ( IsCompilerArg_MSVC( token, "P" ) )
            {
                usingPreprocessorOnly = true;
                flags.Set( CompilerFlags::FLAG_INCLUDES_IN_STDERR );
            }
            else if ( IsCompilerArg_MSVC( token, "WX" ) )
            {
                flags.Set( CompilerFlags::FLAG_WARNINGS_AS_ERRORS_MSVC );
            }
            else if ( IsStartOfCompilerArg_MSVC( token, "analyze" ) )
            {
                if ( IsCompilerArg_MSVC( token, "analyze-" ) )
                {
                    flags.Clear( CompilerFlags::FLAG_STATIC_ANALYSIS_MSVC );
                }
                else
                {
                    flags.Set( CompilerFlags::FLAG_STATIC_ANALYSIS_MSVC );
                }
            }
        }

        // 1) clr code cannot be distributed due to a compiler bug where the preprocessed using
        // statements are truncated
        // 2) code consuming the windows runtime cannot be distributed due to preprocessing weirdness
        // 3) pch files can't be built from preprocessed output (disabled acceleration), so can't be distributed
        // 4) user only wants preprocessor step executed
        // 5) Distribution of /analyze is not currently supported due to preprocessor/_PREFAST_ inconsistencies
        // 6) Source mappings are not currently forwarded so can only compiled locally
        if ( !usingCLR && !usingPreprocessorOnly )
        {
            if ( isDistributableCompiler &&
                 !usingWinRT &&
                 !( flags.IsCreatingPCH() )&&
                 !( flags.IsUsingStaticAnalysisMSVC() ) &&
                 !hasSourceMapping )
            {
                flags.Set( CompilerFlags::FLAG_CAN_BE_DISTRIBUTED );
            }

            // TODO:A Support caching of 7i format
            if ( flags.IsUsingPDB() == false )
            {
                flags.Set( CompilerFlags::FLAG_CAN_BE_CACHED );
            }
        }
    }

    // Check GCC/Clang options
    bool objectiveC = false;
    if ( flags.IsClang() || flags.IsGCC() )
    {
        // Clang supported -fdiagnostics-color option (and defaulted to =auto) since its first release
        if ( flags.IsClang() )
        {
            flags.Set( CompilerFlags::FLAG_DIAGNOSTICS_COLOR_AUTO );
        }

        Array< AString > tokens;
        args.Tokenize( tokens );
        const AString * const end = tokens.End();
        for ( const AString * it = tokens.Begin(); it != end; ++it )
        {
            const AString & token = *it;

            if ( token == "-fdiagnostics-color=auto" )
            {
                flags.Set( CompilerFlags::FLAG_DIAGNOSTICS_COLOR_AUTO );
            }
            else if ( token.BeginsWith( "-fdiagnostics-color" ) || token == "-fno-diagnostics-color" )
            {
                flags.Clear( CompilerFlags::FLAG_DIAGNOSTICS_COLOR_AUTO );
            }
            else if ( token.BeginsWith( "-werror" ) )
            {
                flags.Set( CompilerFlags::FLAG_WARNINGS_AS_ERRORS_CLANGGCC );
            }
            else if ( token == "-fobjc-arc" )
            {
                objectiveC = true;
            }
            else if ( token == "-x" )
            {
                if ( it < ( end - 1 ) )
                {
                    const AString & nextToken = *( it + 1);
                    if ( nextToken == "objective-c" )
                    {
                        objectiveC = true;
                    }
                }
            }
        }
    }

    // check for cacheability/distributability for non-MSVC
    if ( flags.IsClang() ||
         flags.IsGCC() ||
         flags.IsSNC() ||
         flags.IsCodeWarriorWii() ||
         flags.IsGreenHillsWiiU() )
    {
        // creation of the PCH must be done locally to generate a usable PCH
        // Objective C/C++ cannot be distributed
        // Source mappings are not currently forwarded so can only compiled locally
        if ( !creatingPCH && !objectiveC && !hasSourceMapping )
        {
            if ( isDistributableCompiler )
            {
                flags.Set( CompilerFlags::FLAG_CAN_BE_DISTRIBUTED );
            }
        }

        // all objects can be cached with GCC/SNC/Clang (including PCH files)
        flags.Set( CompilerFlags::FLAG_CAN_BE_CACHED );
    }

    // CUDA Compiler
    if ( flags.IsCUDANVCC() )
    {
        // Can cache objects
        flags.Set( CompilerFlags::FLAG_CAN_BE_CACHED );
    }

    if ( flags.IsOrbisWavePSSLC() )
    {
        if ( isDistributableCompiler )
        {
            flags.Set( CompilerFlags::FLAG_CAN_BE_DISTRIBUTED );
        }

        // Can cache objects
        flags.Set( CompilerFlags::FLAG_CAN_BE_CACHED );
    }

    return flags;
}

// IsCompilerArg_MSVC
//------------------------------------------------------------------------------
/*static*/ bool ObjectNode::IsCompilerArg_MSVC( const AString & token, const char * arg )
{
    ASSERT( token.IsEmpty() == false );

    // MSVC Compiler args can start with - or /
    if ( ( token[0] != '/' ) && ( token[0] != '-' ) )
    {
        return false;
    }

    // Length check to early out
    const size_t argLen = AString::StrLen( arg );
    if ( ( token.GetLength() - 1 ) != argLen )
    {
        return false; // token is too short or too long
    }

    // MSVC Compiler args are case-sensitive
    return token.EndsWith( arg );
}

// IsStartOfCompilerArg_MSVC
//------------------------------------------------------------------------------
/*static*/ bool ObjectNode::IsStartOfCompilerArg_MSVC( const AString & token, const char * arg )
{
    ASSERT( token.IsEmpty() == false );

    // MSVC Compiler args can start with - or /
    if ( ( token[0] != '/' ) && ( token[0] != '-' ) )
    {
        return false;
    }

    // Length check to early out
    const size_t argLen = AString::StrLen( arg );
    if ( ( token.GetLength() - 1 ) < argLen )
    {
        return false; // token is too short
    }

    // MSVC Compiler args are case-sensitive
    return ( AString::StrNCmp( token.Get() + 1, arg, argLen ) == 0 );
}

// SaveRemote
//------------------------------------------------------------------------------
/*virtual*/ void ObjectNode::SaveRemote( IOStream & stream ) const
{
    // Force using implies /clr which is not distributable
    ASSERT( m_CompilerForceUsing.IsEmpty() );

    // Save minimal information for the remote worker
    stream.Write( m_Name );
    stream.Write( GetSourceFile()->GetName() );
    stream.Write( m_CompilerFlags.m_Flags );

    // TODO:B would be nice to make ShouldUseDeoptimization cache the result for this build
    // instead of opening the file again.
    const AString & compilerOptions = ShouldUseDeoptimization() ? m_CompilerOptionsDeoptimized : m_CompilerOptions;

    // Prepare args for remote worker
    UniquePtr<CompilerDriverBase, DeleteDeletor> driver;
    CreateDriver( m_CompilerFlags, AString::GetEmpty(), driver );
    Array< AString > tokens( 1024, true );
    compilerOptions.Tokenize( tokens );
    Args fullArgs;

    // Adjust args for as needed for the given compiler
    const size_t numTokens = tokens.GetSize();
    for ( size_t i = 0; i < numTokens; ++i )
    {
        // current token
        const AString & token = tokens[ i ];
        const AString & nextToken = ( i < ( numTokens - 1 ) ) ? tokens[ i + 1 ] : AString::GetEmpty();

        // Handle compiling preprocessed output args adjustment
        if ( driver->ProcessArg_PreparePreprocessedForRemote( token, i, nextToken, fullArgs ) )
        {
            continue;
        }

        // untouched token
        fullArgs += token;
        fullArgs.AddDelimiter();
    }
    driver->AddAdditionalArgs_PreparePreprocessedForRemote( fullArgs );

    stream.Write( fullArgs.GetRawArgs() );
}

// GetCompiler
//-----------------------------------------------------------------------------
CompilerNode * ObjectNode::GetCompiler() const
{
    // node can be null if compiling remotely
    const Node * node = m_StaticDependencies[0].GetNode();
    return node ? node->CastTo< CompilerNode >() : nullptr;
}

// GetDedicatedPreprocessor
//------------------------------------------------------------------------------
CompilerNode * ObjectNode::GetDedicatedPreprocessor() const
{
    if ( m_Preprocessor.IsEmpty() )
    {
        return nullptr;
    }
    size_t preprocessorIndex = 2;
    if ( m_PrecompiledHeader.IsEmpty() == false )
    {
        ++preprocessorIndex;
    }
    return m_StaticDependencies[ preprocessorIndex ].GetNode()->CastTo< CompilerNode >();
}

// GetPrecompiledHeader()
//------------------------------------------------------------------------------
ObjectNode * ObjectNode::GetPrecompiledHeader() const
{
    ASSERT( m_PrecompiledHeader.IsEmpty() == false );
    return m_StaticDependencies[ 2 ].GetNode()->CastTo< ObjectNode >();
}

// GetPDBName
//------------------------------------------------------------------------------
void ObjectNode::GetPDBName( AString & pdbName ) const
{
    ASSERT( IsUsingPDB() );
    pdbName = m_Name;
    pdbName += ".pdb";
}

// GetNativeAnalysisXMLPath
//------------------------------------------------------------------------------
void ObjectNode::GetNativeAnalysisXMLPath( AString& outXMLFileName ) const
{
    ASSERT( IsUsingStaticAnalysisMSVC() );

    const AString & sourceName = m_PCHObjectFileName.IsEmpty() ? m_Name : m_PCHObjectFileName;

    // TODO:B The xml path can be manually specified with /analyze:log
    const char * extPos = sourceName.FindLast( '.' ); // Only last extension removed
    outXMLFileName.Assign( sourceName.Get(), extPos ? extPos : sourceName.GetEnd() );
    outXMLFileName += ".nativecodeanalysis.xml";
}

// GetObjExtension
//------------------------------------------------------------------------------
const char * ObjectNode::GetObjExtension() const
{
    if ( m_CompilerOutputExtension.IsEmpty() )
    {
        #if defined( __WINDOWS__ )
            return ".obj";
        #else
            return ".o";
        #endif
    }
    return m_CompilerOutputExtension.Get();
}

// GetCacheName
//------------------------------------------------------------------------------
const AString & ObjectNode::GetCacheName( Job * job ) const
{
    // use already determined cache name if available?
    if ( job->GetCacheName().IsEmpty() == false )
    {
        return job->GetCacheName();
    }

    PROFILE_FUNCTION;

    // hash the pre-processed input data
    ASSERT( m_LightCacheKey || job->GetData() );
    const uint64_t preprocessedSourceKey = m_LightCacheKey ? m_LightCacheKey : xxHash::Calc64( job->GetData(), job->GetDataSize() );
    ASSERT( preprocessedSourceKey );

    // hash the build "environment"
    // TODO:B Exclude preprocessor control defines (the preprocessed input has considered those already)
    uint32_t commandLineKey;
    {
        Args args;
        const bool useDeoptimization = false;
        const bool showIncludes = false;
        const bool useSourceMapping = false; // Source mapping compiler flags contain local paths, so we treat them specially
        const bool finalize = false; // Don't write args to response file
        BuildArgs( job, args, PASS_COMPILE_PREPROCESSED, useDeoptimization, showIncludes, useSourceMapping, finalize );

        if ( job->IsLocal() )
        {
            // Append the source mapping destination only, so different machines with different
            // working directory local paths compute consistent keys.
            const AString& sourceMapping = job->GetNode()->CastTo<ObjectNode>()->GetCompiler()->GetSourceMapping();
            args.AddDelimiter();
            args += sourceMapping;
        }

        commandLineKey = xxHash::Calc32( args.GetRawArgs().Get(), args.GetRawArgs().GetLength() );
    }
    ASSERT( commandLineKey );

    // ToolChain hash
    const uint64_t toolChainKey = GetCompiler()->CastTo< CompilerNode >()->GetManifest().GetToolId();
    ASSERT( toolChainKey );

    // PCH dependency
    uint64_t pchKey = 0;
    if ( IsUsingPCH() && IsMSVC() )
    {
        pchKey = GetPrecompiledHeader()->m_PCHCacheKey;
        ASSERT( pchKey != 0 ); // Should not be in here if PCH is not cached
    }

    AStackString<> cacheName;
    ICache::GetCacheId( preprocessedSourceKey, commandLineKey, toolChainKey, pchKey, cacheName );
    job->SetCacheName(cacheName);

    return job->GetCacheName();
}

// RetrieveFromCache
//------------------------------------------------------------------------------
bool ObjectNode::RetrieveFromCache( Job * job )
{
    if ( FBuild::Get().GetOptions().m_UseCacheRead == false )
    {
        return false;
    }

    PROFILE_FUNCTION;

    const AString & cacheFileName = GetCacheName(job);

    Timer t;

    ICache * cache = FBuild::Get().GetCache();
    ASSERT( cache );

    void * cacheData( nullptr );
    size_t cacheDataSize( 0 );
    if ( cache->Retrieve( cacheFileName, cacheData, cacheDataSize ) )
    {
        const uint32_t retrieveTime = uint32_t( t.GetElapsedMS() );

        // Hash the PCH result if we will need it later
        uint64_t pchKey = 0;
        if ( IsCreatingPCH() && IsMSVC() )
        {
            pchKey = xxHash::Calc64( cacheData, cacheDataSize );
        }

        const uint32_t startDecompress = uint32_t( t.GetElapsedMS() );

        // do decompression
        Compressor c;
        if ( c.IsValidData( cacheData, cacheDataSize ) == false )
        {
            FLOG_WARN( "Cache returned invalid data (header)\n"
                       " - File: '%s'\n"
                       " - Key : %s\n",
                       m_Name.Get(), cacheFileName.Get() );
            return false;
        }
        if ( c.Decompress( cacheData ) == false )
        {
            FLOG_WARN( "Cache returned invalid data (payload)\n"
                       " - File: '%s'\n"
                       " - Key : %s\n",
                       m_Name.Get(), cacheFileName.Get() );
            return false;
        }
        const void * data = c.GetResult();
        const size_t dataSize = c.GetResultSize();

        const uint32_t stopDecompress = uint32_t( t.GetElapsedMS() );

        MultiBuffer buffer( data, dataSize );

        Array< AString > fileNames( 4, false );
        fileNames.Append( m_Name );

        GetExtraCacheFilePaths( job, fileNames );

        // Extract the files
        const size_t numFiles = fileNames.GetSize();
        for ( size_t i=0; i<numFiles; ++i )
        {
            if ( !buffer.ExtractFile( i, fileNames[ i ] ) )
            {
                cache->FreeMemory( cacheData, cacheDataSize );
                FLOG_ERROR( "Failed to write local file during cache retrieval '%s'", fileNames[ i ].Get() );
                return false;
            }

            // Update file modification time
            const bool timeSetOK = FileIO::SetFileLastWriteTimeToNow( fileNames[ i ] );

            // set the time on the local file
            if ( timeSetOK == false )
            {
                cache->FreeMemory( cacheData, cacheDataSize );
                FLOG_ERROR( "Failed to set timestamp after cache hit. Error: %s Target: '%s'", LAST_ERROR_STR, fileNames[ i ].Get() );
                return false;
            }
        }

        cache->FreeMemory( cacheData, cacheDataSize );

        FileIO::WorkAroundForWindowsFilePermissionProblem( m_Name );

        // record new file time (note that time may differ from what we set above due to
        // file system precision)
        RecordStampFromBuiltFile();

        // Output
        if ( FBuild::Get().GetOptions().m_ShowCommandSummary ||
             FBuild::Get().GetOptions().m_CacheVerbose )
        {
            AStackString<> output;
            output.Format( "Obj: %s <CACHE>\n", GetName().Get() );
            if ( FBuild::Get().GetOptions().m_CacheVerbose )
            {
                output.AppendFormat( " - Cache Hit: %u ms (Retrieve: %u ms - Decompress: %u ms) (Compressed: %zu - Uncompressed: %zu) '%s'\n", uint32_t( t.GetElapsedMS() ), retrieveTime, stopDecompress - startDecompress, cacheDataSize, dataSize, cacheFileName.Get() );
            }
            FLOG_OUTPUT( output );
        }

        SetStatFlag( Node::STATS_CACHE_HIT );

        // Dependent objects need to know the PCH key to be able to pull from the cache
        if ( IsCreatingPCH() && IsMSVC() )
        {
            m_PCHCacheKey = pchKey;
        }

        job->GetBuildProfilerScope()->SetStepName( "Cache Hit" );

        return true;
    }

    // Output
    if ( FBuild::Get().GetOptions().m_CacheVerbose )
    {
        FLOG_OUTPUT( "Obj: %s\n"
                     " - Cache Miss: %u ms '%s'\n",
                     GetName().Get(), uint32_t( t.GetElapsedMS() ), cacheFileName.Get() );
    }

    SetStatFlag( Node::STATS_CACHE_MISS );
    return false;
}

// WriteToCache
//------------------------------------------------------------------------------
void ObjectNode::WriteToCache( Job * job )
{
    if (FBuild::Get().GetOptions().m_UseCacheWrite == false)
    {
        return;
    }

    PROFILE_FUNCTION;

    const AString & cacheFileName = GetCacheName(job);
    ASSERT(!cacheFileName.IsEmpty());

    Timer t;

    ICache * cache = FBuild::Get().GetCache();
    ASSERT( cache );

    Array< AString > fileNames( 4, false );
    fileNames.Append( m_Name );

    GetExtraCacheFilePaths( job, fileNames );

    MultiBuffer buffer;
    if ( buffer.CreateFromFiles( fileNames ) )
    {
        // try to compress
        const uint32_t startCompress( (uint32_t)t.GetElapsedMS() );
        Compressor c;
        c.Compress( buffer.GetData(), (size_t)buffer.GetDataSize(), FBuild::Get().GetOptions().m_CacheCompressionLevel );
        const void * data = c.GetResult();
        const size_t dataSize = c.GetResultSize();
        const uint32_t stopCompress( (uint32_t)t.GetElapsedMS() );

        const uint32_t startPublish( stopCompress );
        if ( cache->Publish( cacheFileName, data, dataSize ) )
        {
            // cache store complete
            const uint32_t stopPublish( (uint32_t)t.GetElapsedMS() );

            SetStatFlag( Node::STATS_CACHE_STORE );

            // Dependent objects need to know the PCH key to be able to pull from the cache
            if ( IsCreatingPCH() && IsMSVC() )
            {
                m_PCHCacheKey = xxHash::Calc64( data, dataSize );
            }

            const uint32_t cachingTime = uint32_t( t.GetElapsedMS() );
            AddCachingTime( cachingTime );

            // Output
            if ( FBuild::Get().GetOptions().m_CacheVerbose )
            {
                AStackString<> output;
                output.Format( "Obj: %s\n"
                                " - Cache Store: %u ms (Store: %u ms - Compress: %u ms) (Compressed: %zu - Uncompressed: %zu) '%s'\n",
                                GetName().Get(), cachingTime, ( stopPublish - startPublish ), ( stopCompress - startCompress ), dataSize, (size_t)buffer.GetDataSize(), cacheFileName.Get() );
                if ( m_PCHCacheKey != 0 )
                {
                    output.AppendFormat( " - PCH Key: %" PRIx64 "\n", m_PCHCacheKey );
                }
                FLOG_OUTPUT( output );
            }

            return;
        }
    }

    // Output
    if ( FBuild::Get().GetOptions().m_CacheVerbose )
    {
        FLOG_OUTPUT( "Obj: %s\n"
                     " - Cache Store Fail: %u ms '%s'\n",
                     GetName().Get(), uint32_t( t.GetElapsedMS() ), cacheFileName.Get() );
    }
}

// GetExtraCacheFilePaths
//------------------------------------------------------------------------------
void ObjectNode::GetExtraCacheFilePaths( const Job * job, Array< AString > & outFileNames ) const
{
    const Node * node = job->GetNode();
    if ( node->GetType() != Node::OBJECT_NODE )
    {
        return;
    }

    // Only the MSVC compiler creates extra objects
    const ObjectNode * objectNode = node->CastTo< ObjectNode >();
    if ( objectNode->m_CompilerFlags.IsMSVC() == false )
    {
        return;
    }

    // Precompiled Headers have an extra file
    if ( objectNode->m_CompilerFlags.IsCreatingPCH() )
    {
        // .pch.obj
        outFileNames.Append( m_PCHObjectFileName );
    }

    // Static analysis adds extra files
    if ( objectNode->m_CompilerFlags.IsUsingStaticAnalysisMSVC() )
    {
        if ( objectNode->m_CompilerFlags.IsCreatingPCH() )
        {
            // .pchast (precompiled headers only)

            // Get file name start
            ASSERT( PathUtils::IsFullPath( m_PCHObjectFileName ) ); // Something is terribly wrong

            AStackString<> pchASTFileName( m_PCHObjectFileName );
            if ( pchASTFileName.EndsWithI( ".obj" ) )
            {
                pchASTFileName.SetLength( pchASTFileName.GetLength() - 4 );
            }

            pchASTFileName += "ast";
            outFileNames.Append( pchASTFileName );
        }

        // .nativecodeanalysis.xml (all files)
        AStackString<> xmlFileName;
        GetNativeAnalysisXMLPath( xmlFileName );
        outFileNames.Append( xmlFileName );
    }
}

// EmitCompilationMessage
//------------------------------------------------------------------------------
void ObjectNode::EmitCompilationMessage( const Args & fullArgs, bool useDeoptimization, bool stealingRemoteJob, bool racingRemoteJob, bool useDedicatedPreprocessor, bool isRemote ) const
{
    // print basic or detailed output, depending on options
    // we combine everything into one string to ensure it is contiguous in
    // the output
    AStackString<> output;
    if ( FBuild::IsValid()  && FBuild::Get().GetOptions().m_ShowCommandSummary )
    {
        output += "Obj: ";
        if ( useDeoptimization )
        {
            output += "**Deoptimized** ";
        }
        if ( IsIsolatedFromUnity() )
        {
            output += "**Isolated** ";
        }
        output += GetName();
        if ( racingRemoteJob )
        {
            output += " <LOCAL RACE>";
        }
        else if ( stealingRemoteJob )
        {
            output += " <LOCAL>";
        }
        output += '\n';
    }
    if ( ( FBuild::IsValid() && FBuild::Get().GetOptions().m_ShowCommandLines ) || isRemote )
    {
        output += useDedicatedPreprocessor ? GetDedicatedPreprocessor()->GetExecutable().Get() : GetCompiler() ? GetCompiler()->GetExecutable().Get() : "";
        output += ' ';
        output += fullArgs.GetRawArgs();
        output += '\n';
    }
    FLOG_OUTPUT( output );
}

// BuildArgs
//------------------------------------------------------------------------------
bool ObjectNode::BuildArgs( const Job * job, Args & fullArgs, Pass pass, bool useDeoptimization, bool showIncludes, bool useSourceMapping, bool finalize, const AString & overrideSrcFile ) const
{
    PROFILE_FUNCTION;

    Array< AString > tokens( 1024, true );

    const bool useDedicatedPreprocessor = ( ( pass == PASS_PREPROCESSOR_ONLY ) && GetDedicatedPreprocessor() );
    if ( useDedicatedPreprocessor )
    {
        m_PreprocessorOptions.Tokenize( tokens );
    }
    else if ( useDeoptimization )
    {
        ASSERT( !m_CompilerOptionsDeoptimized.IsEmpty() );
        m_CompilerOptionsDeoptimized.Tokenize( tokens );
    }
    else
    {
        m_CompilerOptions.Tokenize( tokens );
    }
    fullArgs.Clear();

    // Get base path if needed
    AStackString<> basePath;
    const bool useRelativePaths = job->IsLocal() && // TODO:C We'd want to support this remotely as well
                                  job->GetNode()->CastTo<ObjectNode>()->GetCompiler()->GetUseRelativePaths();
    if ( useRelativePaths )
    {
        basePath = FBuild::Get().GetOptions().GetWorkingDir(); // NOTE: FBuild only valid locally
        PathUtils::EnsureTrailingSlash( basePath );
    }

    const CompilerFlags & flags = useDedicatedPreprocessor ? m_PreprocessorFlags : m_CompilerFlags;

    const bool forceColoredDiagnostics = ( flags.IsDiagnosticsColorAuto() && ( Env::IsStdOutRedirected() == false ) );

    UniquePtr<CompilerDriverBase, DeleteDeletor> driver;
    CreateDriver( flags, job->GetRemoteSourceRoot(), driver );

    driver->SetOverrideSourceFile( overrideSrcFile );
    driver->SetRelativeBasePath( basePath );
    driver->SetForceColoredDiagnostics( forceColoredDiagnostics );
    driver->SetUseSourceMapping( ( useSourceMapping && job->IsLocal() ) ? GetCompiler()->GetSourceMapping() : AString::GetEmpty() );

    // Adjust args for as needed for the given compiler
    const size_t numTokens = tokens.GetSize();
    for ( size_t i = 0; i < numTokens; ++i )
    {
        // current token
        const AString & token = tokens[ i ];
        const AString & nextToken = ( i < ( numTokens - 1 ) ) ? tokens[ i + 1 ] : AString::GetEmpty();

        // Handle Preprocessor args adjustment
        if ( ( pass == PASS_PREPROCESSOR_ONLY ) && driver->ProcessArg_PreprocessorOnly( token, i, nextToken, fullArgs ) )
        {
            continue;
        }

        // Handle compiling preprocessed output args adjustment
        if ( ( pass == PASS_COMPILE_PREPROCESSED ) && driver->ProcessArg_CompilePreprocessed( token, i, nextToken, job->IsLocal(), fullArgs ) )
        {
            continue;
        }

        // Handle general args adjustment
        if ( driver->ProcessArg_Common( token, i, fullArgs ) )
        {
            continue;
        }

        // Handle build-time substitutions
        if ( driver->ProcessArg_BuildTimeSubstitution( token, i, fullArgs ) )
        {
            continue;
        }

        // untouched token
        fullArgs += token;
        fullArgs.AddDelimiter();
    }

    // Add additional compiler-specific args
    if ( pass == PASS_PREPROCESSOR_ONLY )
    {
        driver->AddAdditionalArgs_Preprocessor( fullArgs );
    }
    driver->AddAdditionalArgs_Common( job->IsLocal(), fullArgs );

    if ( showIncludes )
    {
        fullArgs += " /showIncludes"; // we'll extract dependency information from this
    }

    // Skip finalization?
    if ( finalize == false )
    {
        return true;
    }

    // Handle all the special needs of args
    AStackString<> remoteCompiler;
    if ( job->IsLocal() == false )
    {
        job->GetToolManifest()->GetRemoteFilePath( 0, remoteCompiler );
    }
    const AString& compiler = job->IsLocal() ? GetCompiler()->GetExecutable() : remoteCompiler;
    if ( fullArgs.Finalize( compiler, GetName(), GetResponseFileMode() ) == false )
    {
        return false; // Finalize will have emitted an error
    }

    return true;
}

// ExpandCompilerForceUsing
//------------------------------------------------------------------------------
void ObjectNode::ExpandCompilerForceUsing( Args & fullArgs, const AString & pre, const AString & post ) const
{
    const size_t startIndex = 2 + ( !m_PrecompiledHeader.IsEmpty() ? 1u : 0u ) + ( !m_Preprocessor.IsEmpty() ? 1u : 0u ); // Skip Compiler, InputFile, PCH and Preprocessor
    const size_t endIndex = m_StaticDependencies.GetSize();
    for ( size_t i=startIndex; i<endIndex; ++i )
    {
        const Node * n = m_StaticDependencies[ i ].GetNode();

        fullArgs += pre;
        fullArgs += n->GetName();
        fullArgs += post;
        fullArgs.AddDelimiter();
    }
}

// BuildPreprocessedOutput
//------------------------------------------------------------------------------
bool ObjectNode::BuildPreprocessedOutput( const Args & fullArgs, Job * job, bool useDeoptimization ) const
{
    const bool useDedicatedPreprocessor = ( GetDedicatedPreprocessor() != nullptr );
    EmitCompilationMessage( fullArgs, useDeoptimization, false, false, useDedicatedPreprocessor );

    // spawn the process
    CompileHelper ch( false ); // don't handle output (we'll do that)
    // TODO:A Add checks in BuildArgs for length of dedicated preprocessor
    if ( !ch.SpawnCompiler( job, GetName(),
         useDedicatedPreprocessor ? GetDedicatedPreprocessor() : GetCompiler(),
         useDedicatedPreprocessor ? GetDedicatedPreprocessor()->GetExecutable() : GetCompiler()->GetExecutable(),
         fullArgs ) )
    {
        // only output errors in failure case
        // (as preprocessed output goes to stdout, normal logging is pushed to
        // stderr, and we don't want to see that unless there is a problem)
        // NOTE: Output is omitted in case the compiler has been aborted because we don't care about the errors
        // caused by the manual process abortion (process killed)
        if ( ( ch.GetResult() != 0 ) && !ch.HasAborted() )
        {
            // Use the error text, but if it's empty, use the output
            if ( ch.GetErr().IsEmpty() == false )
            {
                DumpOutput( job, GetName(), ch.GetErr() );
            }
            else
            {
                DumpOutput( job, GetName(), ch.GetOut() );
            }
        }

        return false; // SpawnCompiler will have emitted error
    }

    // take a copy of the output because ReadAllData uses huge buffers to avoid re-sizing
    TransferPreprocessedData( ch.GetOut().Get(), ch.GetOut().GetLength(), job );

    return true;
}

// LoadStaticSourceFileForDistribution
//------------------------------------------------------------------------------
bool ObjectNode::LoadStaticSourceFileForDistribution( const Args & fullArgs, Job * job, bool useDeoptimization ) const
{
    // PreProcessing for SimpleDistribution is just loading the source file

    const bool useDedicatedPreprocessor = ( GetDedicatedPreprocessor() != nullptr );
    EmitCompilationMessage(fullArgs, useDeoptimization, false, false, useDedicatedPreprocessor);

    const AString & fileName = job->GetNode()->CastTo<ObjectNode>()->GetSourceFile()->CastTo<FileNode>()->GetName();

    // read the file into memory
    FileStream fs;
    if ( fs.Open( fileName.Get(), FileStream::READ_ONLY ) == false )
    {
        FLOG_ERROR( "Error: opening file '%s' while loading source file for transport\n", fileName.Get() );
        return false;
    }
    uint32_t contentSize = (uint32_t)fs.GetFileSize();
    UniquePtr< void > mem( ALLOC( contentSize ) );
    if ( fs.Read( mem.Get(), contentSize ) != contentSize )
    {
        FLOG_ERROR( "Error: reading file '%s' in Compiler ToolManifest\n", fileName.Get() );
        return false;
    }

    job->OwnData( mem.Release(), contentSize );

    return true;
}

// TransferPreprocessedData
//------------------------------------------------------------------------------
void ObjectNode::TransferPreprocessedData( const char * data, size_t dataSize, Job * job ) const
{
    // We will trim the buffer
    const char* outputBuffer = data;
    size_t outputBufferSize = dataSize;
    size_t newBufferSize = outputBufferSize;
    char * bufferCopy = nullptr;

    #if defined( __WINDOWS__ )
        // VS 2012 sometimes generates corrupted code when preprocessing an already preprocessed file when it encounters
        // enum definitions.
        // Example:
        //      enum dateorder
        //      {
        //          no_order, dmy, mdy, ymd, ydm
        //      };
        // Becomes:
        //      enummdateorder
        //      {
        //          no_order, dmy, mdy, ymd, ydm
        //      };
        // And then compilation fails.
        //
        // Adding a space between the enum keyword and the name avoids the bug.
        // This bug is fixed in VS2013 or later.
        bool doVS2012Fixup = false;
        if ( GetCompiler()->GetType() == Node::COMPILER_NODE )
        {
            const CompilerNode* cn = GetCompiler();
            doVS2012Fixup = cn->IsVS2012EnumBugFixEnabled();
        }

        if ( doVS2012Fixup )
        {
            Array< const char * > enumsFound( 2048, true );
            const char BUGGY_CODE[] = "enum ";
            const char* workBuffer = outputBuffer;
            ASSERT( workBuffer[ outputBufferSize ] == '\0' );

            // First scan the buffer to find all occurences of the enums.
            // This will then let us resize the buffer to the appropriate size.
            // Keeping the found enums let us avoid searching for them twice.
            uint32_t nbrEnumsFound = 0;
            const char * buggyEnum = nullptr;
            for (;;)
            {
                buggyEnum = strstr( workBuffer, BUGGY_CODE );
                if ( buggyEnum == nullptr )
                {
                    break;
                }
                ++nbrEnumsFound;
                enumsFound.Append( buggyEnum );
                workBuffer = buggyEnum + sizeof( BUGGY_CODE ) - 1;
            }
            ASSERT( enumsFound.GetSize() == nbrEnumsFound );

            // Now allocate the new buffer with enough space to add a space after each enum found
            newBufferSize = outputBufferSize + nbrEnumsFound;
            bufferCopy = (char *)ALLOC( newBufferSize + 1 ); // null terminator for include parser

            uint32_t enumIndex = 0;
            workBuffer = outputBuffer;
            char * writeDest = bufferCopy;
            size_t sizeLeftInSourceBuffer = outputBufferSize;
            buggyEnum = nullptr;
            for (;;)
            {
                if ( enumIndex < nbrEnumsFound )
                {
                    buggyEnum = enumsFound[ enumIndex ];
                    ++enumIndex;
                }
                else
                {
                    buggyEnum = nullptr;
                }

                if ( buggyEnum == nullptr )
                {
                    // Copy the rest of the data.
                    memcpy( writeDest, workBuffer, sizeLeftInSourceBuffer );
                    break;
                }

                // Copy what's before the enum + the enum.
                size_t sizeToCopy = (size_t)( buggyEnum - workBuffer );
                sizeToCopy += ( sizeof( BUGGY_CODE ) - 1 );
                memcpy( writeDest, workBuffer, sizeToCopy );
                writeDest += sizeToCopy;

                // Add a space
                *writeDest = ' ';
                ++writeDest;

                ASSERT( sizeLeftInSourceBuffer >= sizeToCopy );
                sizeLeftInSourceBuffer -= sizeToCopy;
                workBuffer += sizeToCopy;
            }

            bufferCopy[ newBufferSize ] = 0; // null terminator for include parser
        }
        else
    #endif
    {
        bufferCopy = (char *)ALLOC( newBufferSize + 1 ); // null terminator for include parser
        memcpy( bufferCopy, outputBuffer, newBufferSize );
        bufferCopy[ newBufferSize ] = 0; // null terminator for include parser
    }

    job->OwnData( bufferCopy, newBufferSize );
}

// WriteTmpFile
//------------------------------------------------------------------------------
bool ObjectNode::WriteTmpFile( Job * job, AString & tmpDirectory, AString & tmpFileName ) const
{
    ASSERT( job->GetData() && job->GetDataSize() );

    const Node * sourceFile = GetSourceFile();
    uint32_t sourceNameHash = xxHash::Calc32( sourceFile->GetName().Get(), sourceFile->GetName().GetLength() );

    FileStream tmpFile;
    AStackString<> fileName( sourceFile->GetName().FindLast( NATIVE_SLASH ) + 1 );
    if ( IsGCC() )
    {
        // Add extension to the name of the temporary file that corresponds to
        // a preprocessed variant of the original extension.
        // That way GCC will be able to deduce the same language from the name
        // of temporary file as it would do from the original name.
        const char * lastDot = fileName.FindLast( '.' );
        if ( ( lastDot != nullptr ) && ( lastDot[1] != '\0' ) )
        {
            AStackString<> extension( lastDot + 1 );
            if ( extension == "c" )
            {
                fileName += ".i";
            }
            else if ( ( extension == "cpp" ) || ( extension == "cc" ) || ( extension == "cxx" ) || ( extension == "c++" || extension == "cp" || extension == "CPP" || extension == "C" ) )
            {
                fileName += ".ii";
            }
            else if ( extension == "m" )
            {
                fileName += ".mi";
            }
            else if ( ( extension == "mm" ) || ( extension == "M" ) )
            {
                fileName += ".mii";
            }
            else if ( ( extension == "F" ) || ( extension == "fpp" ) || ( extension == "FPP" ) )
            {
                fileName += ".f";
            }
            else if ( extension == "FOR" )
            {
                fileName += ".for";
            }
            else if ( extension == "FTN" )
            {
                fileName += ".ftn";
            }
            else if ( extension == "F90" )
            {
                fileName += ".f90";
            }
            else if ( extension == "F95" )
            {
                fileName += ".f95";
            }
            else if ( extension == "F03" )
            {
                fileName += ".f03";
            }
            else if ( extension == "F08" )
            {
                fileName += ".f08";
            }
            else if ( ( extension == "S" ) || ( extension == "sx" ) )
            {
                fileName += ".s";
            }
        }
    }

    void const * dataToWrite = job->GetData();
    size_t dataToWriteSize = job->GetDataSize();

    // handle compressed data
    Compressor c; // scoped here so we can access decompression buffer
    if ( job->IsDataCompressed() )
    {
        VERIFY( c.Decompress( dataToWrite ) );
        dataToWrite = c.GetResult();
        dataToWriteSize = c.GetResultSize();
    }

    WorkerThread::GetTempFileDirectory( tmpDirectory );
    tmpDirectory.AppendFormat( "%08X%c", sourceNameHash, NATIVE_SLASH );
    if ( FileIO::DirectoryCreate( tmpDirectory ) == false )
    {
        job->Error( "Failed to create temp directory. Error: %s TmpDir: '%s' Target: '%s'", LAST_ERROR_STR, tmpDirectory.Get(), GetName().Get() );
        job->OnSystemError();
        return NODE_RESULT_FAILED;
    }
    tmpFileName = tmpDirectory;
    tmpFileName += fileName;
    if ( WorkerThread::CreateTempFile( tmpFileName, tmpFile ) == false )
    {
        FileIO::WorkAroundForWindowsFilePermissionProblem( tmpFileName, FileStream::WRITE_ONLY, 10 ); // 10s max wait

        // Try again
        if ( WorkerThread::CreateTempFile( tmpFileName, tmpFile ) == false )
        {
            job->Error( "Failed to create temp file. Error: %s TmpFile: '%s' Target: '%s'", LAST_ERROR_STR, tmpFileName.Get(), GetName().Get() );
            job->OnSystemError();
            return NODE_RESULT_FAILED;
        }
    }
    if ( tmpFile.Write( dataToWrite, dataToWriteSize ) != dataToWriteSize )
    {
        job->Error( "Failed to write to temp file. Error: %s TmpFile: '%s' Target: '%s'", LAST_ERROR_STR, tmpFileName.Get(), GetName().Get() );
        job->OnSystemError();
        return NODE_RESULT_FAILED;
    }
    tmpFile.Close();

    FileIO::WorkAroundForWindowsFilePermissionProblem( tmpFileName );

    // On remote workers, free compressed buffer as we don't need it anymore
    // This reduces memory consumed on the remote worker.
    if ( job->IsLocal() == false )
    {
        job->OwnData( nullptr, 0, false ); // Free compressed buffer
    }

    return true;
}

// BuildFinalOutput
//------------------------------------------------------------------------------
bool ObjectNode::BuildFinalOutput( Job * job, const Args & fullArgs ) const
{
    // Use the remotely synchronized compiler if building remotely
    AStackString<> compiler;
    AStackString<> workingDir;
    if ( job->IsLocal() )
    {
        compiler = GetCompiler()->GetExecutable();
    }
    else
    {
        ASSERT( job->GetToolManifest() );
        job->GetToolManifest()->GetRemoteFilePath( 0, compiler );
        job->GetToolManifest()->GetRemotePath( workingDir );
    }

    // spawn the process
    CompileHelper ch( true, job->GetAbortFlagPointer() );
    if ( !ch.SpawnCompiler( job, GetName(), GetCompiler(), compiler, fullArgs, workingDir.IsEmpty() ? nullptr : workingDir.Get() ) )
    {
        // did spawn fail, or did we spawn and fail to compile?
        if ( ch.GetResult() != 0 )
        {
            // failed to compile

            // for remote jobs, we must serialize the errors to return with the job
            if ( job->IsLocal() == false )
            {
                UniquePtr< char > mem( (char *)ALLOC( ch.GetOut().GetLength() + ch.GetErr().GetLength() ) );
                memcpy( mem.Get(), ch.GetOut().Get(), ch.GetOut().GetLength() );
                memcpy( mem.Get() + ch.GetOut().GetLength(), ch.GetErr().Get(), ch.GetErr().GetLength() );
                job->OwnData( mem.Release(), ( ch.GetOut().GetLength() + ch.GetErr().GetLength() ) );
            }
        }

        return false; // compile has logged error
    }
    else
    {
        // Handle warnings for compilation that passed
        if ( ch.GetResult() == 0 )
        {
            if ( IsMSVC() )
            {
                if ( IsWarningsAsErrorsMSVC() == false )
                {
                    HandleWarningsMSVC( job, GetName(), ch.GetOut() );
                }
            }
            else if ( IsClangCl() )
            {
                if ( IsWarningsAsErrorsMSVC() == false )
                {
                    HandleWarningsClangCl( job, GetName(), ch.GetErr() );
                    HandleWarningsClangCl( job, GetName(), ch.GetOut() );
                }
            }
            else if ( IsClang() || IsGCC() )
            {
                if ( IsWarningsAsErrorsClangGCC() == false )
                {
                    HandleWarningsClangGCC( job, GetName(), ch.GetOut() );
                }
            }
        }

        // Special case for clang static analysis which doesn't write any
        // output file to disk. In that case, we write the static analysis
        // results as the output. This avoids a "file missing despite success"
        // error
        if ( ch.GetResult() == 0 )
        {
            if ( IsClang() || IsClangCl() )
            {
                if ( FileIO::FileExists( GetName().Get() ) == false )
                {
                    FileStream f;
                    if ( ( f.Open( GetName().Get(), FileStream::WRITE_ONLY ) == false ) ||
                         ( f.WriteBuffer( ch.GetErr().Get(), ch.GetErr().GetLength() ) != ch.GetErr().GetLength() ) )
                    {
                        FLOG_ERROR( "Error %s writing analysis results: %s", LAST_ERROR_STR, GetName().Get() );
                    }
                }
            }
        }
    }

    return true;
}

// CompileHelper::CONSTRUCTOR
//------------------------------------------------------------------------------
ObjectNode::CompileHelper::CompileHelper( bool handleOutput, const volatile bool * abortPointer )
    : m_HandleOutput( handleOutput )
    , m_Process( FBuild::GetAbortBuildPointer(), abortPointer )
    , m_Result( 0 )
{
}

// CompileHelper::DESTRUCTOR
//------------------------------------------------------------------------------
ObjectNode::CompileHelper::~CompileHelper() = default;

// CompilHelper::SpawnCompiler
//------------------------------------------------------------------------------
bool ObjectNode::CompileHelper::SpawnCompiler( Job * job,
                                               const AString & name,
                                               const CompilerNode * compilerNode,
                                               const AString & compiler,
                                               const Args & fullArgs,
                                               const char * workingDir )
{
    const char * environmentString = nullptr;
    if ( ( job->IsLocal() == false ) && ( job->GetToolManifest() ) )
    {
        environmentString = job->GetToolManifest()->GetRemoteEnvironmentString();
    }
    else
    {
        environmentString = compilerNode->GetEnvironmentString();
    }

    // spawn the process
    if ( false == m_Process.Spawn( compiler.Get(),
                                   fullArgs.GetFinalArgs().Get(),
                                   workingDir,
                                   environmentString ) )
    {
        if ( m_Process.HasAborted() )
        {
            return false;
        }

        job->Error( "Failed to spawn process. Error: %s Target: '%s'\n", LAST_ERROR_STR, name.Get() );
        job->OnSystemError();
        return false;
    }

    // capture all of the stdout and stderr
    m_Process.ReadAllData( m_Out, m_Err );

    // Get result
    m_Result = m_Process.WaitForExit();
    if ( m_Process.HasAborted() )
    {
        return false;
    }

    // Handle special types of failures
    HandleSystemFailures( job, m_Result, m_Out, m_Err );

    if ( m_HandleOutput )
    {
        if ( job->IsLocal() && FBuild::Get().GetOptions().m_ShowCommandOutput )
        {
            // Suppress /showIncludes - TODO:C leave in if user specified it
            StackArray< AString > exclusions;
            if ( ( compilerNode->GetCompilerFamily() == CompilerNode::CompilerFamily::MSVC ) &&
                ( fullArgs.GetFinalArgs().Find( " /showIncludes" ) ) )
            {
                exclusions.EmplaceBack( "Note: including file:" );
            }

            Node::DumpOutput( job, m_Out, &exclusions );
            Node::DumpOutput( job, m_Err, &exclusions );
        }
        else
        {
            // output any errors (even if succeeded, there might be warnings)
            if ( m_Err.IsEmpty() == false )
            {
                const bool treatAsWarnings = true; // change msg formatting
                DumpOutput( job, name, m_Err, treatAsWarnings );
            }
        }
    }

    // failed?
    if ( m_Result != 0 )
    {
        // output 'stdout' which may contain errors for some compilers
        if ( m_HandleOutput )
        {
            DumpOutput( job, name, m_Out );
        }

        job->Error( "Failed to build Object. Error: %s Target: '%s'\n", ERROR_STR( m_Result ), name.Get() );

        return false;
    }

    return true;
}

// HandleSystemFailures
//------------------------------------------------------------------------------
/*static*/ void ObjectNode::HandleSystemFailures( Job * job, int result, const AString & stdOut, const AString & stdErr )
{
    // Only remote compilation has special cases. We don't touch local failures.
    if ( job->IsLocal() )
    {
        return;
    }

    // Nothing to check if everything is ok
    if ( result == 0 )
    {
        return;
    }

    const ObjectNode * objectNode = job->GetNode()->CastTo< ObjectNode >();

    // General process failures
    #if defined( __WINDOWS__ )
        // If remote PC is shutdown by user, compiler can be terminated
        if ( ( (uint32_t)result == 0x40010004 ) || // DBG_TERMINATE_PROCESS
             ( (uint32_t)result == 0xC0000142 ) )  // STATUS_DLL_INIT_FAILED - Occurs if remote PC is stuck on force reboot dialog during shutdown
        {
            job->OnSystemError(); // task will be retried on another worker
            return;
        }

        // When a process is terminated by a user on Windows, the return code can
        // be 1. There seems to be no definitive way to differentiate this from
        // a process exiting with return code 1, so we default to considering it a
        // system failure, unless we can detect some specific situations.
        if ( result == 0x1 )
        {
            bool treatAsSystemError = true;

            // Some compilers (like Clang) return 1 when there are compile errors
            // and write those errors to stderr
            // don't consider this a system failure if there are compile errors
            if ( stdErr.IsEmpty() == false )
            {
                treatAsSystemError = false;
            }

            if ( treatAsSystemError )
            {
                job->OnSystemError(); // task will be retried on another worker
                return;
            }
        }

        // If DLLs are not correctly sync'd, add an extra message to help the user
        if ( (uint32_t)result == 0xC000007B ) // STATUS_INVALID_IMAGE_FORMAT
        {
            job->Error( "Remote failure: STATUS_INVALID_IMAGE_FORMAT (0xC000007B) - Check Compiler() settings!\n" );
            return;
        }
        if ( (uint32_t)result == 0xC0000135 ) // STATUS_DLL_NOT_FOUND
        {
            job->Error( "Remote failure: STATUS_DLL_NOT_FOUND (0xC0000135) - Check Compiler() settings!\n" );
            return;
        }
    #endif

    // Compiler specific failures

    #if defined( __WINDOWS__ )
        // MSVC errors
        if ( objectNode->IsMSVC() )
        {
            // But we need to determine if it's actually an out of space
            // (rather than some compile error with missing file(s))
            // These error codes have been observed in the wild
            if ( stdOut.Find( "C1082" ) ||
                 stdOut.Find( "C1085" ) ||
                 stdOut.Find( "C1088" ) )
            {
                job->OnSystemError();
                return;
            }

            // Windows temp directories can have problems failing to open temp files
            // resulting in 'C1083: Cannot open compiler intermediate file:'
            // It uses the same C1083 error as a mising include C1083, but since we flatten
            // includes on the host this should never occur remotely other than in this context.
            if ( stdOut.Find( "C1083" ) )
            {
                job->OnSystemError();
                return;
            }

            // The MSVC compiler can fail with "compiler is out of heap space" even if
            // using the 64bit toolchain. This failure can be intermittent and not
            // repeatable with the same code on a different machine, so we don't want it
            // to fail the build.
            if ( stdOut.Find( "C1060" ) )
            {
                // If either of these are present
                //  - C1076 : compiler limit : internal heap limit reached; use /Zm to specify a higher limit
                //  - C3859 : virtual memory range for PCH exceeded; please recompile with a command line option of '-Zmvalue' or greater
                // then the issue is related to compiler settings.
                // If they are not present, it's a system error, possibly caused by system resource
                // exhaustion on the remote machine
                if ( ( stdOut.Find( "C1076" ) == nullptr ) &&
                     ( stdOut.Find( "C3859" ) == nullptr ) )
                {
                    job->OnSystemError();
                    return;
                }
            }

            // If the compiler crashed (Internal Compiler Error), treat this
            // as a system error so it will be retried, since it can alse be
            // the result of faulty hardware.
            if ( stdOut.Find( "C1001" ) )
            {
                job->OnSystemError();
                return;
            }

            // Error messages above also contains this text
            // (We check for this message additionally to handle other error codes
            //  that may not have been observed yet)
            // TODO:C Should we check for localized msg?
            if ( stdOut.Find( "No space left on device" ) )
            {
                job->OnSystemError();
                return;
            }
        }
    #endif

    // Clang
    if ( objectNode->IsClang() || objectNode->IsClangCl() )
    {
        // When clang fails due to low disk space
        // TODO:C Should we check for localized msg?
        if ( stdErr.Find( "IO failure on output stream" ) )
        {
            job->OnSystemError();
            return;
        }
    }

    // GCC
    if ( objectNode->IsGCC() )
    {
        // When gcc fails due to low disk space
        // TODO:C Should we check for localized msg?
        if ( stdErr.Find( "No space left on device" ) )
        {
            job->OnSystemError();
            return;
        }
    }

    #if !defined( __WINDOWS__) 
        (void)stdOut; // No checks use stdOut outside of Windows right now
    #endif
}

// ShouldUseDeoptimization
//------------------------------------------------------------------------------
bool ObjectNode::ShouldUseDeoptimization() const
{
    if ( IsUnity() )
    {
        return false; // disabled for Unity files (which are always writable)
    }

    if ( ( m_DeoptimizeWritableFilesWithToken == false ) &&
         ( m_DeoptimizeWritableFiles == false ) )
    {
        return false; // feature not enabled
    }

    // Neither flag should be set for the creation of Precompiled Headers
    ASSERT( IsCreatingPCH() == false );

    if ( FileIO::GetReadOnly( GetSourceFile()->GetName() ) )
    {
        return false; // file is read only (not modified)
    }

    if ( m_DeoptimizeWritableFiles )
    {
        return true; // file modified - deoptimize with or without token
    }

    // does file contain token?
    FileStream fs;
    if ( fs.Open( GetSourceFile()->GetName().Get(), FileStream::READ_ONLY ) )
    {
        const size_t bytesToRead = Math::Min< size_t >( 1024, (size_t)fs.GetFileSize() );
        char buffer[ 1025 ];
        if ( fs.Read( buffer, bytesToRead ) == bytesToRead )
        {
            buffer[ bytesToRead ] = 0;
            if ( strstr( buffer, "FASTBUILD_DEOPTIMIZE_OBJECT" ) )
            {
                return true;
            }
            return false; // token not present
        }
    }

    // problem reading file - TODO:B emit an error?
    return false;
}

// ShouldUseCache
//------------------------------------------------------------------------------
bool ObjectNode::ShouldUseCache() const
{
    bool useCache = IsCacheable() &&
                    m_AllowCaching &&
                    ( FBuild::Get().GetOptions().m_UseCacheRead ||
                     FBuild::Get().GetOptions().m_UseCacheWrite );
    if ( IsIsolatedFromUnity() )
    {
        // disable caching for locally modified files (since they will rarely if ever get a hit)
        useCache = false;
    }
    if ( IsMSVC() && IsUsingPCH() )
    {
        // If the PCH is not in the cache, then no point looking there
        // for objects and also no point storing them
        if ( GetPrecompiledHeader()->m_PCHCacheKey == 0 )
        {
            return false;
        }
    }
    return useCache;
}

// GetResponseFileMode
//------------------------------------------------------------------------------
ArgsResponseFileMode ObjectNode::GetResponseFileMode() const
{
    // User forces response files to be used, regardless of args length?
    if ( GetCompiler() && GetCompiler()->ShouldForceResponseFileUse() )
    {
        return ArgsResponseFileMode::ALWAYS;
    }

    // User explicitly says we can use response file if needed?
    if ( GetCompiler() && GetCompiler()->CanUseResponseFile() )
    {
        return ArgsResponseFileMode::IF_NEEDED;
    }

    // Detect a compiler that supports response file args?
    #if defined( __WINDOWS__ )
        // Generally only windows applications support response files (to overcome Windows command line limits)
        // TODO:C This logic is Windows only as that's how it was originally implemented. It seems we
        // probably want this for other platforms as well though.
        if ( IsMSVC() ||
             IsGCC() ||
             IsSNC() ||
             IsClang() ||
             IsClangCl() ||
             IsCodeWarriorWii() ||
             IsGreenHillsWiiU() )
        {
            return ArgsResponseFileMode::IF_NEEDED;
        }
    #endif

    // Cannot use response files
    return ArgsResponseFileMode::NEVER;
}

// GetVBCCPreprocessedOutput
//------------------------------------------------------------------------------
bool ObjectNode::GetVBCCPreprocessedOutput( ConstMemoryStream & outStream ) const
{
    // Filename matches the source file, but with extension replaced
    const AString & sourceFileName = GetSourceFile()->GetName();
    const char * lastDot = sourceFileName.FindLast( '.' );
    lastDot = lastDot ? lastDot : sourceFileName.GetEnd();
    AStackString<> preprocessedFile( sourceFileName.Get(), lastDot );
    preprocessedFile += ".i";

    // Try to open the file
    FileStream f;
    if ( !f.Open( preprocessedFile.Get(), FileStream::READ_ONLY ) )
    {
        FLOG_ERROR( "Failed to open preprocessed file '%s'", preprocessedFile.Get() );
        return false;
    }

    // Allocate memory
    const size_t memSize = (size_t)f.GetFileSize();
    char * mem = (char *)ALLOC( memSize + 1 ); // +1 so we can null terminate
    outStream.Replace( mem, memSize, true ); // true = outStream now owns memory

    // Read contents
    if ( (size_t)f.ReadBuffer( mem, memSize ) != memSize )
    {
        FLOG_ERROR( "Failed to read preprocessed file '%s'", preprocessedFile.Get() );
        return false;
    }
    mem[ memSize ] = 0; // null terminate text buffer for parsing convenience

    return true;
}

// DoClangUnityFixup
//------------------------------------------------------------------------------
void ObjectNode::DoClangUnityFixup( Job * job ) const
{
    // Fixup preprocessed output so static analysis works well with Unity
    //
    // Static analysis only analyzes the "topmost" file, which in Unity is the
    // Unity.cpp which contains no code. The compiler know which file is the topmost
    // file by checking the directives left by the preprocessor, as described here:
    //  - https://gcc.gnu.org/onlinedocs/cpp/Preprocessor-Output.html
    //
    // Be removing the "push" flags the preprocesser adds (1) when leaving the main cpp
    // file and entering an inluded file only for the Unity, the compiler thinks
    // the included files are the top-level files and applies analysis the same way
    // as when they are built outside of Unity.
    //
    // We remove the "pop" flags (2) when returning to the unity as these must match the
    // "pop"s
    //
    // We do this fixup even if not using -Xanalyze, so that:
    // a) the behaviour is consistent, avoiding any problems that might show up only
    //    with static analysis
    // b) this behaviour may be useful for other types of warnings for similar reasons
    //    (i.e. if Clang suppresses then because it thinks a file is not the "main" file)
    //

    // Sanity checks
    ASSERT( IsClang() || IsClangCl() ); // Only necessary for Clang
    ASSERT( IsUnity() ); // Only makes sense to for Unity
    ASSERT( job->IsDataCompressed() == false ); // Can't fixup compressed data
    ASSERT( job->IsLocal() ); // Assuming we're doing this on the local machine (using FBuild singleton lower)

    // We'll walk the output and fix it up in-place

    AStackString<> srcFileName( GetSourceFile()->GetName() );
    #if defined( __WINDOWS__ )
        // Clang escapes backslashes, so we must do the same
        srcFileName.Replace( "\\", "\\\\" );
    #endif

    // Build the string used to find "pop" directives when returning to this file
    AStackString<> popDirectiveString;
    popDirectiveString = " \"";
    popDirectiveString += srcFileName;
    popDirectiveString += "\" 2"; // 2 is "pop" flag

    // Find the first instance of the primary filename (this ensured we ignore any
    // injected "-include" stuff before that)
    char * pos = strstr( (char *)job->GetData(), srcFileName.Get() );
    if ( pos == nullptr )
    {
        // If not found, try relative path
        AStackString<> basePath( FBuild::Get().GetOptions().GetWorkingDir() ); // NOTE: FBuild only valid locally
        PathUtils::EnsureTrailingSlash( basePath );

        AStackString<> relativeFileName;
        PathUtils::GetRelativePath( basePath, GetSourceFile()->GetName(), relativeFileName );
        #if defined( __WINDOWS__ )
            // Clang escapes backslashes, so we must do the same
            relativeFileName.Replace( "\\", "\\\\" );
        #endif
        pos = strstr( (char *)job->GetData(), relativeFileName.Get() );

        // Update pop directive string
        popDirectiveString = " \"";
        popDirectiveString += relativeFileName;
        popDirectiveString += "\" 2"; // 2 is "pop" flag
    }

    ASSERT( pos ); // TODO:A This fails because the path is now relative

    // Find top-level push/pop pairs. We don't want mess with nested directives
    // as these can be meaningful in other ways.
    while ( pos )
    {
        // Searching for a "push" directive (i.e. leaving the unity file and pushing
        // a file included by the Unity cpp)
        pos = strstr( pos, "\n# 1 \"" );
        if ( pos == nullptr )
        {
            return; // no more
        }
        pos += 6; // skip to string inside quotes

        // Ignore special directives like <built-in>
        if ( *pos == '<' )
        {
            continue; // Keep searching
        }

        // Find closing quote
        pos = strchr( pos, '"' );
        if ( pos == nullptr )
        {
            ASSERT( false ); // Unexpected/malformed
            return;
        }

        // Found it?
        if ( AString::StrNCmp( pos, "\" 1", 3 ) != 0 ) // Note: '1' flag for "push"
        {
            continue; // Keep searching
        }

        // Patch out "push" flag in-place
        pos[ 2 ] = ' ';
        pos += 3; // Skip closing quote, space and flag

        // Find "pop" directive that returns us to the Unity file
        char * popDirective = strstr( pos, popDirectiveString.Get() );
        if ( popDirective == nullptr )
        {
            ASSERT( false ); // Unexpected/malformed
            return;
        }

        // Pathc out "pop" flag to match the "push"
        pos = ( popDirective + popDirectiveString.GetLength() - 1 );
        ASSERT( *pos == '2' );
        *pos = ' ';
    }
}

// CreateDriver
//------------------------------------------------------------------------------
void ObjectNode::CreateDriver( ObjectNode::CompilerFlags flags,
                               const AString & remoteSourceRoot,
                               UniquePtr<CompilerDriverBase, DeleteDeletor> & outDriver ) const
{
    if      ( flags.IsMSVC() || flags.IsClangCl() ) { outDriver = FNEW( CompilerDriver_CL( flags.IsClangCl() ) ); }
    else if ( flags.IsClang() || flags.IsGCC() )    { outDriver = FNEW( CompilerDriver_GCCClang( flags.IsClang() ) ); }
    else if ( flags.IsVBCC() )                      { outDriver = FNEW( CompilerDriver_VBCC() ); }
    else if ( flags.IsQtRCC() )                     { outDriver = FNEW( CompilerDriver_QtRCC() ); }
    else if ( flags.IsOrbisWavePSSLC() )            { outDriver = FNEW( CompilerDriver_OrbisWavePSSLC() ); }
    else if ( flags.IsSNC() )                       { outDriver = FNEW( CompilerDriver_SNC() ); }
    else if ( flags.IsCUDANVCC() )                  { outDriver = FNEW( CompilerDriver_CUDA() ); }
    else if ( flags.IsCodeWarriorWii() )            { outDriver = FNEW( CompilerDriver_CodeWarriorWii() ); }
    else if ( flags.IsGreenHillsWiiU() )            { outDriver = FNEW( CompilerDriver_GreenHillsWiiU() ); }
    else                                            { outDriver = FNEW( CompilerDriver_Generic() ); }

    outDriver->Init( this, remoteSourceRoot );
}

//------------------------------------------------------------------------------
