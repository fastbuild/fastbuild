// ObjectNode.cpp
//------------------------------------------------------------------------------

// Includes
//------------------------------------------------------------------------------
#include "Tools/FBuild/FBuildCore/PrecompiledHeader.h"

#include "ObjectNode.h"

#include "Tools/FBuild/FBuildCore/BFF/Functions/FunctionObjectList.h"
#include "Tools/FBuild/FBuildCore/Cache/ICache.h"
#include "Tools/FBuild/FBuildCore/FBuild.h"
#include "Tools/FBuild/FBuildCore/FLog.h"
#include "Tools/FBuild/FBuildCore/Graph/CompilerNode.h"
#include "Tools/FBuild/FBuildCore/Graph/NodeGraph.h"
#include "Tools/FBuild/FBuildCore/Graph/NodeProxy.h"
#include "Tools/FBuild/FBuildCore/Graph/SettingsNode.h"
#include "Tools/FBuild/FBuildCore/Helpers/Args.h"
#include "Tools/FBuild/FBuildCore/Helpers/CIncludeParser.h"
#include "Tools/FBuild/FBuildCore/Helpers/Compressor.h"
#include "Tools/FBuild/FBuildCore/Helpers/MultiBuffer.h"
#include "Tools/FBuild/FBuildCore/Helpers/ResponseFile.h"
#include "Tools/FBuild/FBuildCore/Helpers/ToolManifest.h"
#include "Tools/FBuild/FBuildCore/WorkerPool/Job.h"
#include "Tools/FBuild/FBuildCore/WorkerPool/JobQueue.h"
#include "Tools/FBuild/FBuildCore/WorkerPool/WorkerThread.h"

// Core
#include "Core/Env/Env.h"
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
    REFLECT( m_Compiler,                            "Compiler",                         MetaFile() )
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
    REFLECT( m_Preprocessor,                        "Preprocessor",                     MetaOptional() + MetaFile() )
    REFLECT( m_PreprocessorOptions,                 "PreprocessorOptions",              MetaOptional() )

    REFLECT_ARRAY( m_PreBuildDependencyNames,       "PreBuildDependencies",             MetaOptional() + MetaFile() + MetaAllowNonFile() )

    // Internal State
    REFLECT( m_Flags,                               "Flags",                            MetaHidden() )
    REFLECT( m_PreprocessorFlags,                   "PreprocessorFlags",                MetaHidden() )
    REFLECT( m_PCHCacheKey,                         "PCHCacheKey",                      MetaHidden() )
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
bool ObjectNode::Initialize( NodeGraph & nodeGraph, const BFFIterator & iter, const Function * function )
{
    // .PreBuildDependencies
    if ( !InitializePreBuildDependencies( nodeGraph, iter, function, m_PreBuildDependencyNames ) )
    {
        return false; // InitializePreBuildDependencies will have emitted an error
    }

    // .Compiler
    CompilerNode * compiler( nullptr );
    if ( !((FunctionObjectList *)function)->GetCompilerNode( nodeGraph, iter, m_Compiler, compiler ) )
    {
        return false; // GetCompilerNode will have emitted an error
    }

    // .CompilerInputFile
    Dependencies compilerInputFile;
    if ( !function->GetFileNode( nodeGraph, iter, m_CompilerInputFile, ".CompilerInputFile", compilerInputFile ) )
    {
        return false; // GetFileNode will have emitted an error
    }
    ASSERT( compilerInputFile.GetSize() == 1 ); // Should not be possible to expand to > 1 thing

    // .Preprocessor
    CompilerNode * preprocessor( nullptr );
    if ( m_Preprocessor.IsEmpty() == false )
    {
        if ( !((FunctionObjectList *)function)->GetCompilerNode( nodeGraph, iter, m_Preprocessor, preprocessor ) )
        {
            return false; // GetCompilerNode will have emitted an error
        }
    }

    // .CompilerForceUsing
    Dependencies compilerForceUsing;
    if ( !function->GetFileNodes( nodeGraph, iter, m_CompilerForceUsing, ".CompilerForceUsing", compilerForceUsing ) )
    {
        return false; // GetFileNode will have emitted an error
    }

    // Store Dependencies
    m_StaticDependencies.SetCapacity( 1 + 1 + ( m_PrecompiledHeader ? 1 : 0 ) + ( preprocessor ? 1 : 0 ) + compilerForceUsing.GetSize() );
    m_StaticDependencies.Append( Dependency( compiler ) );
    m_StaticDependencies.Append( compilerInputFile );
    if ( m_PrecompiledHeader )
    {
        m_StaticDependencies.Append( Dependency( m_PrecompiledHeader ) );
    }
    if ( preprocessor )
    {
        m_StaticDependencies.Append( Dependency( preprocessor ) );
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
, m_Flags( flags )
, m_Remote( true )
{
    m_Type = OBJECT_NODE;
    m_LastBuildTimeMs = 5000; // higher default than a file node

    m_StaticDependencies.SetCapacity( 2 );
    m_StaticDependencies.Append( Dependency( nullptr ) );
    m_StaticDependencies.Append( Dependency( srcFile ) );
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

// DoDynamicDependencies
//------------------------------------------------------------------------------
/*virtual*/ bool ObjectNode::DoDynamicDependencies( NodeGraph & /*nodeGraph*/, bool forceClean )
{
    // TODO:A Remove ObjectNode::DoDynamicDependencies
    // - Dependencies added in Finalize need to be cleared if StaticDependencies
    //   change. We should to that globally to remove the need for this code.

    if (forceClean)
    {
        m_DynamicDependencies.Clear(); // We will update deps in Finalize after DoBuild
        return true;
    }

    // If static deps would trigger a rebuild, invalidate dynamicdeps
    const uint64_t stamp = GetStamp();
    for ( const Dependency & dep : m_StaticDependencies )
    {
        if ( dep.GetNode()->GetStamp() > stamp )
        {
            m_DynamicDependencies.Clear(); // We will update deps in Finalize after DoBuild
            return true;
        }
    }
    return true;
}

// DoBuild
//------------------------------------------------------------------------------
/*virtual*/ Node::BuildResult ObjectNode::DoBuild( Job * job )
{
    // Delete previous file(s) if doing a clean build
    if ( FBuild::Get().GetOptions().m_ForceCleanBuild )
    {
        if ( FileIO::FileExists( GetName().Get() ) )
        {
            if ( FileIO::FileDelete( GetName().Get() ) == false )
            {
                FLOG_ERROR( "Failed to delete file before build '%s'", GetName().Get() );
                return NODE_RESULT_FAILED;
            }
        }
        if ( GetFlag( FLAG_MSVC ) && GetFlag( FLAG_CREATING_PCH ) )
        {
            if ( FileIO::FileExists( m_PCHObjectFileName.Get() ) )
            {
                if ( FileIO::FileDelete( m_PCHObjectFileName.Get() ) == false )
                {
                    FLOG_ERROR( "Failed to delete file before build '%s'", m_PCHObjectFileName.Get() );
                    return NODE_RESULT_FAILED;
                }
            }
        }
    }

    // Reset PCH cache key - will be set correctly if we end up using the cache
    if ( GetFlag( FLAG_MSVC ) && GetFlag( FLAG_CREATING_PCH ) )
    {
        m_PCHCacheKey = 0;
    }

    // using deoptimization?
    bool useDeoptimization = ShouldUseDeoptimization();

    bool useCache = ShouldUseCache();
    bool useDist = GetFlag( FLAG_CAN_BE_DISTRIBUTED ) && m_AllowDistribution && FBuild::Get().GetOptions().m_AllowDistributed;
    bool useSimpleDist = GetCompiler()->CastTo< CompilerNode >()->SimpleDistributionMode();
    bool usePreProcessor = !useSimpleDist && ( useCache || useDist || GetFlag( FLAG_GCC ) || GetFlag( FLAG_SNC ) || GetFlag( FLAG_CLANG ) || GetFlag( CODEWARRIOR_WII ) || GetFlag( GREENHILLS_WIIU ) || GetFlag( ObjectNode::FLAG_VBCC ) );
    if ( GetDedicatedPreprocessor() )
    {
        usePreProcessor = true;
        useDeoptimization = false; // disable deoptimization
    }

    // Graphing the current amount of distributable jobs
    FLOG_MONITOR( "GRAPH FASTBuild \"Distributable Jobs MemUsage\" MB %f\n", (float)Job::GetTotalLocalDataMemoryUsage() / (float)MEGABYTE );

    if ( usePreProcessor || useSimpleDist )
    {
        return DoBuildWithPreProcessor( job, useDeoptimization, useCache, useSimpleDist );
    }

    if ( GetFlag( FLAG_MSVC ) )
    {
        return DoBuildMSCL_NoCache( job, useDeoptimization );
    }

    if ( GetFlag( FLAG_QT_RCC ))
    {
        return DoBuild_QtRCC( job );
    }

    return DoBuildOther( job, useDeoptimization );
}

// DoBuild_Remote
//------------------------------------------------------------------------------
/*virtual*/ Node::BuildResult ObjectNode::DoBuild2( Job * job, bool racingRemoteJob = false )
{
    // we may be using deoptimized options, but they are always
    // the "normal" args when remote compiling
    bool useDeoptimization = job->IsLocal() && ShouldUseDeoptimization();
    bool stealingRemoteJob = job->IsLocal(); // are we stealing a remote job?
    return DoBuildWithPreProcessor2( job, useDeoptimization, stealingRemoteJob, racingRemoteJob );
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
        m_DynamicDependencies.Append( Dependency( fn ) );
    }

    return true;
}

// DoBuildMSCL_NoCache
//------------------------------------------------------------------------------
/*virtual*/ Node::BuildResult ObjectNode::DoBuildMSCL_NoCache( Job * job, bool useDeoptimization )
{
    // Format compiler args string
    Args fullArgs;
    const bool showIncludes( true );
    if ( !BuildArgs( job, fullArgs, PASS_COMPILE, useDeoptimization, showIncludes ) )
    {
        return NODE_RESULT_FAILED; // BuildArgs will have emitted an error
    }

    EmitCompilationMessage( fullArgs, useDeoptimization );

    // spawn the process
    CompileHelper ch;
    if ( !ch.SpawnCompiler( job, GetName(), GetCompiler()->GetName(), fullArgs ) ) // use response file for MSVC
    {
        return NODE_RESULT_FAILED; // SpawnCompiler has logged error
    }

    // Handle MSCL warnings if not already a failure
    // If "warnings as errors" is enabled (/WX) we don't need to check
    // (since compilation will fail anyway, and the output will be shown)
    if ( ( ch.GetResult() == 0 ) && !GetFlag( FLAG_WARNINGS_AS_ERRORS_MSVC ) )
    {
        HandleWarningsMSVC( job, GetName(), ch.GetOut().Get(), ch.GetOutSize() );
    }

    const char *output = nullptr;
    uint32_t outputSize = 0;

    // MSVC will write /ShowIncludes output on stderr sometimes (ex: /Fi)
    if ( GetFlag( FLAG_INCLUDES_IN_STDERR ) == true )
    {
        output = ch.GetErr().Get();
        outputSize = ch.GetErrSize();
    }
    else
    {
        // but most of the time it will be on stdout
        output = ch.GetOut().Get();
        outputSize = ch.GetOutSize();
    }

    // compiled ok, try to extract includes
    if ( ProcessIncludesMSCL( output, outputSize ) == false )
    {
        return NODE_RESULT_FAILED; // ProcessIncludesMSCL will have emitted an error
    }

    // record new file time
    m_Stamp = FileIO::GetFileLastWriteTime( m_Name );

    return NODE_RESULT_OK;
}

// DoBuildWithPreProcessor
//------------------------------------------------------------------------------
Node::BuildResult ObjectNode::DoBuildWithPreProcessor( Job * job, bool useDeoptimization, bool useCache, bool useSimpleDist )
{
    Args fullArgs;
    const bool showIncludes( false );
    Pass pass = useSimpleDist ? PASS_PREP_FOR_SIMPLE_DISTRIBUTION : PASS_PREPROCESSOR_ONLY;
    if ( !BuildArgs( job, fullArgs, pass, useDeoptimization, showIncludes ) )
    {
        return NODE_RESULT_FAILED; // BuildArgs will have emitted an error
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

    // calculate the cache entry lookup
    if ( useCache )
    {
        // try to get from cache
        if ( RetrieveFromCache( job ) )
        {
            return NODE_RESULT_OK_CACHE;
        }
    }

    // can we do the rest of the work remotely?
    const bool canDistribute = useSimpleDist || ( GetFlag( FLAG_CAN_BE_DISTRIBUTED ) && m_AllowDistribution && FBuild::Get().GetOptions().m_AllowDistributed );
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
    bool stealingRemoteJob = false; // never queued
    bool racingRemoteJob = false;
    Node::BuildResult result = DoBuildWithPreProcessor2( job, useDeoptimization, stealingRemoteJob, racingRemoteJob );
    if ( result != Node::NODE_RESULT_OK )
    {
        return result;
    }

    return Node::NODE_RESULT_OK;
}

// DoBuildWithPreProcessor2
//------------------------------------------------------------------------------
Node::BuildResult ObjectNode::DoBuildWithPreProcessor2( Job * job, bool useDeoptimization, bool stealingRemoteJob, bool racingRemoteJob )
{
    // should never use preprocessor if using CLR
    ASSERT( GetFlag( FLAG_USING_CLR ) == false );

    bool usePreProcessedOutput = true;
    if ( job->IsLocal() )
    {
        if ( GetFlag( FLAG_CLANG | FLAG_GCC | FLAG_SNC ) )
        {
            // Using the PCH with Clang/SNC/GCC doesn't prevent storing to the cache
            // so we can use the PCH accelerated compilation
            if ( GetFlag( FLAG_USING_PCH ) )
            {
                usePreProcessedOutput = false;
            }

            // Creating a PCH must not use the preprocessed output, or we'll create
            // a PCH which cannot be used for acceleration
            if ( GetFlag( FLAG_CREATING_PCH ) )
            {
                usePreProcessedOutput = false;
            }
        }

        if ( GetFlag( FLAG_MSVC ) )
        {
            // If using the PCH, we can't use the preprocessed output
            // as that's incompatible with the PCH
            if ( GetFlag( FLAG_USING_PCH ) )
            {
                usePreProcessedOutput = false;
            }

            // If creating the PCH, we can't use the preprocessed info
            // as this would prevent acceleration by users of the PCH
            if ( GetFlag( FLAG_CREATING_PCH ) )
            {
                usePreProcessedOutput = false;
            }
        }

        // The CUDA compiler seems to not be able to compile its own preprocessed output
        if ( GetFlag( FLAG_CUDA_NVCC ) )
        {
            usePreProcessedOutput = false;
        }

        if ( GetFlag( FLAG_VBCC ) )
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
        if ( !BuildArgs( job, fullArgs, PASS_COMPILE_PREPROCESSED, useDeoptimization, showIncludes, tmpFileName ) )
        {
            return NODE_RESULT_FAILED; // BuildArgs will have emitted an error
        }
    }
    else
    {
        const bool showIncludes( false );
        if ( !BuildArgs( job, fullArgs, PASS_COMPILE, useDeoptimization, showIncludes ) )
        {
            return NODE_RESULT_FAILED; // BuildArgs will have emitted an error
        }
    }

    const bool verbose = FLog::ShowInfo();
    const bool showCommands = ( FBuild::IsValid() && FBuild::Get().GetOptions().m_ShowCommandLines );
    const bool isRemote = ( job->IsLocal() == false );
    if ( stealingRemoteJob || racingRemoteJob || verbose || showCommands || isRemote )
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
        return NODE_RESULT_FAILED; // BuildFinalOutput will have emitted error
    }

    // record new file time
    if ( job->IsLocal() )
    {
        m_Stamp = FileIO::GetFileLastWriteTime( m_Name );

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
        Args fullArgs;
        if ( !BuildArgs( job, fullArgs, PASS_PREPROCESSOR_ONLY, useDeoptimization, showIncludes ) )
        {
            return NODE_RESULT_FAILED; // BuildArgs will have emitted an error
        }

        EmitCompilationMessage( fullArgs, false );

        CompileHelper ch;
        if ( !ch.SpawnCompiler( job, GetName(), GetCompiler()->GetName(), fullArgs ) )
        {
            return NODE_RESULT_FAILED; // compile has logged error
        }

        // get output
        const AutoPtr< char > & out = ch.GetOut();
        const uint32_t outSize = ch.GetOutSize();
        AStackString< 4096 > output( out.Get(), out.Get() + outSize );
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
        Args fullArgs;
        if ( !BuildArgs( job, fullArgs, PASS_COMPILE, useDeoptimization, showIncludes ) )
        {
            return NODE_RESULT_FAILED; // BuildArgs will have emitted an error
        }

        CompileHelper ch;
        if ( !ch.SpawnCompiler( job, GetName(), GetCompiler()->GetName(), fullArgs ) )
        {
            return NODE_RESULT_FAILED; // compile has logged error
        }
    }

    // record new file time
    m_Stamp = FileIO::GetFileLastWriteTime( m_Name );

    return NODE_RESULT_OK;
}

// DoBuildOther
//------------------------------------------------------------------------------
/*virtual*/ Node::BuildResult ObjectNode::DoBuildOther( Job * job, bool useDeoptimization )
{
    // Format compiler args string
    Args fullArgs;
    const bool showIncludes( false );
    if ( !BuildArgs( job, fullArgs, PASS_COMPILE, useDeoptimization, showIncludes ) )
    {
        return NODE_RESULT_FAILED; // BuildArgs will have emitted an error
    }

    EmitCompilationMessage( fullArgs, useDeoptimization );

    // spawn the process
    CompileHelper ch;
    if ( !ch.SpawnCompiler( job, GetName(), GetCompiler()->GetName(), fullArgs ) )
    {
        return NODE_RESULT_FAILED; // compile has logged error
    }

    // record new file time
    m_Stamp = FileIO::GetFileLastWriteTime( m_Name );

    return NODE_RESULT_OK;
}

// ProcessIncludesMSCL
//------------------------------------------------------------------------------
bool ObjectNode::ProcessIncludesMSCL( const char * output, uint32_t outputSize )
{
    Timer t;

    {
        CIncludeParser parser;
        bool result = ( output && outputSize ) ? parser.ParseMSCL_Output( output, outputSize )
                                               : false;

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

    FLOG_INFO( "Process Includes:\n - File: %s\n - Time: %u ms\n - Num : %u", m_Name.Get(), uint32_t( t.GetElapsedMS() ), uint32_t( m_Includes.GetSize() ) );

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
        if ( GetFlag( FLAG_VBCC ) )
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
            msvcStyle = GetPreprocessorFlag( FLAG_MSVC ) || GetPreprocessorFlag( FLAG_CUDA_NVCC );
        }
        else
        {
            msvcStyle = GetFlag( FLAG_MSVC ) || GetFlag( FLAG_CUDA_NVCC );
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

    FLOG_INFO( "Process Includes:\n - File: %s\n - Time: %u ms\n - Num : %u", m_Name.Get(), uint32_t( t.GetElapsedMS() ), uint32_t( m_Includes.GetSize() ) );

    return true;
}

// Load
//------------------------------------------------------------------------------
/*static*/ Node * ObjectNode::Load( NodeGraph & nodeGraph, IOStream & stream )
{
    NODE_LOAD( AStackString<>, name );

    ObjectNode * node = nodeGraph.CreateObjectNode( name );

    if ( node->Deserialize( nodeGraph, stream ) == false )
    {
        return nullptr;
    }

    // TODO:B Use normal serialization
    NODE_LOAD_NODE_LINK( Node, precompiledHeader );
    node->m_PrecompiledHeader = precompiledHeader ? precompiledHeader->CastTo< ObjectNode >() : nullptr;

    return node;
}

// LoadRemote
//------------------------------------------------------------------------------
/*static*/ Node * ObjectNode::LoadRemote( IOStream & stream )
{
    NODE_LOAD( AStackString<>,  name );
    NODE_LOAD( AStackString<>,  sourceFile );
    NODE_LOAD( uint32_t,        flags );
    NODE_LOAD( AStackString<>,  compilerArgs );

    NodeProxy * srcFile = FNEW( NodeProxy( sourceFile ) );

    return FNEW( ObjectNode( name, srcFile, compilerArgs, flags ) );
}

// DetermineFlags
//------------------------------------------------------------------------------
/*static*/ uint32_t ObjectNode::DetermineFlags( const CompilerNode * compilerNode,
                                                const AString & args,
                                                bool creatingPCH,
                                                bool usingPCH )
{
    uint32_t flags = 0;

    // set flags known from the context the args will be used in
    flags |= ( creatingPCH  ? ObjectNode::FLAG_CREATING_PCH : 0 );
    flags |= ( usingPCH     ? ObjectNode::FLAG_USING_PCH : 0 );

    const bool isDistributableCompiler = compilerNode->CanBeDistributed();

    // Compiler Type - TODO:C Eliminate duplication of these flags
    const CompilerNode::CompilerFamily compilerFamily = compilerNode->GetCompilerFamily();
    switch ( compilerFamily )
    {
        case CompilerNode::CompilerFamily::CUSTOM:          break; // Nothing to do
        case CompilerNode::CompilerFamily::MSVC:            flags |= FLAG_MSVC;        break;
        case CompilerNode::CompilerFamily::CLANG:           flags |= FLAG_CLANG;       break;
        case CompilerNode::CompilerFamily::GCC:             flags |= FLAG_GCC;         break;
        case CompilerNode::CompilerFamily::SNC:             flags |= FLAG_SNC;         break;
        case CompilerNode::CompilerFamily::CODEWARRIOR_WII: flags |= CODEWARRIOR_WII;  break;
        case CompilerNode::CompilerFamily::GREENHILLS_WIIU: flags |= GREENHILLS_WIIU;  break;
        case CompilerNode::CompilerFamily::CUDA_NVCC:       flags |= FLAG_CUDA_NVCC;   break;
        case CompilerNode::CompilerFamily::QT_RCC:          flags |= FLAG_QT_RCC;      break;
        case CompilerNode::CompilerFamily::VBCC:            flags |= FLAG_VBCC;        break;
    }

    // Check MS compiler options
    if ( flags & ObjectNode::FLAG_MSVC )
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
                flags |= ObjectNode::FLAG_USING_PDB;
            }
            else if ( IsCompilerArg_MSVC( token, "clr" ) )
            {
                usingCLR = true;
                flags |= ObjectNode::FLAG_USING_CLR;
            }
            else if ( IsCompilerArg_MSVC( token, "ZW" ) )
            {
                usingWinRT = true;
            }
            else if ( IsCompilerArg_MSVC( token, "P" ) )
            {
                usingPreprocessorOnly = true;
                flags |= ObjectNode::FLAG_INCLUDES_IN_STDERR;
            }
            else if ( IsCompilerArg_MSVC( token, "WX" ) )
            {
                flags |= ObjectNode::FLAG_WARNINGS_AS_ERRORS_MSVC;
            }
        }

        // 1) clr code cannot be distributed due to a compiler bug where the preprocessed using
        // statements are truncated
        // 2) code consuming the windows runtime cannot be distributed due to preprocessing weirdness
        // 3) pch files can't be built from preprocessed output (disabled acceleration), so can't be distributed
        // 4) user only wants preprocessor step executed
        if ( !usingCLR && !usingPreprocessorOnly )
        {
            if ( isDistributableCompiler && !usingWinRT && !( flags & ObjectNode::FLAG_CREATING_PCH ) )
            {
                flags |= ObjectNode::FLAG_CAN_BE_DISTRIBUTED;
            }

            // TODO:A Support caching of 7i format
            if ( ( flags & ObjectNode::FLAG_USING_PDB ) == 0 )
            {
                flags |= ObjectNode::FLAG_CAN_BE_CACHED;
            }
        }
    }

    // check for cacheability/distributability for non-MSVC
    if ( flags & ( ObjectNode::FLAG_CLANG | ObjectNode::FLAG_GCC | ObjectNode::FLAG_SNC | ObjectNode::CODEWARRIOR_WII | ObjectNode::GREENHILLS_WIIU ) )
    {
        // creation of the PCH must be done locally to generate a usable PCH
        if ( !( flags & ObjectNode::FLAG_CREATING_PCH ) )
        {
            if ( isDistributableCompiler )
            {
                flags |= ObjectNode::FLAG_CAN_BE_DISTRIBUTED;
            }
        }

        // all objects can be cached with GCC/SNC/Clang (including PCH files)
        flags |= ObjectNode::FLAG_CAN_BE_CACHED;
    }

    // CUDA Compiler
    if ( flags & ObjectNode::FLAG_CUDA_NVCC )
    {
        // Can cache objects
        flags |= ObjectNode::FLAG_CAN_BE_CACHED;
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

// Save
//------------------------------------------------------------------------------
/*virtual*/ void ObjectNode::Save( IOStream & stream ) const
{
    NODE_SAVE( m_Name );
    Node::Serialize( stream );

    // TODO:B Use normal serialization
    NODE_SAVE_NODE_LINK( m_PrecompiledHeader );
}

// SaveRemote
//------------------------------------------------------------------------------
/*virtual*/ void ObjectNode::SaveRemote( IOStream & stream ) const
{
    // Force using implies /clr which is not distributable
    ASSERT( m_CompilerForceUsing.IsEmpty() );

    // Save minimal information for the remote worker
    NODE_SAVE( m_Name );
    NODE_SAVE( GetSourceFile()->GetName() );
    NODE_SAVE( m_Flags );

    // TODO:B would be nice to make ShouldUseDeoptimization cache the result for this build
    // instead of opening the file again.
    const bool useDeoptimization = ShouldUseDeoptimization();
    if ( useDeoptimization )
    {
        NODE_SAVE( m_CompilerOptionsDeoptimized );
    }
    else
    {
        NODE_SAVE( m_CompilerOptions );
    }
}

// GetDedicatedPreprocessor
//------------------------------------------------------------------------------
Node * ObjectNode::GetDedicatedPreprocessor() const
{
    if ( m_Preprocessor.IsEmpty() )
    {
        return nullptr;
    }
    size_t preprocessorIndex = 2;
    if ( m_PrecompiledHeader )
    {
        ++preprocessorIndex;
    }
    return m_StaticDependencies[ preprocessorIndex ].GetNode();
}

// GetPDBName
//------------------------------------------------------------------------------
void ObjectNode::GetPDBName( AString & pdbName ) const
{
    ASSERT( IsUsingPDB() );
    pdbName = m_Name;
    pdbName += ".pdb";
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

    Timer t;

    // hash the pre-processed intput data
    ASSERT( job->GetData() );
    uint64_t a = xxHash::Calc64( job->GetData(), job->GetDataSize() );

    // hash the build "environment"
    // TODO:B Exclude preprocessor control defines (the preprocessed input has considered those already)
    uint32_t b = xxHash::Calc32( m_CompilerOptions.Get(), m_CompilerOptions.GetLength() );

    // ToolChain hash
    uint64_t c = GetCompiler()->CastTo< CompilerNode >()->GetManifest().GetToolId();

    // PCH dependency
    uint64_t d = 0;
    if ( GetFlag( FLAG_USING_PCH ) && GetFlag( FLAG_MSVC ) )
    {
        d = m_PrecompiledHeader->CastTo< ObjectNode >()->m_PCHCacheKey;
        ASSERT( d != 0 ); // Should not be in here if PCH is not cached
    }

    AStackString<> cacheName;
    FBuild::Get().GetCacheFileName( a, b, c, d, cacheName );
    job->SetCacheName(cacheName);

    FLOG_INFO( "Cache hash: %u ms - %u kb '%s'\n",
                uint32_t( t.GetElapsedMS() ),
                uint32_t( job->GetDataSize() / KILOBYTE ),
                cacheName.Get() );
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

    const AString & cacheFileName = GetCacheName(job);

    Timer t;

    ICache * cache = FBuild::Get().GetCache();
    ASSERT( cache );
    if ( cache )
    {
        void * cacheData( nullptr );
        size_t cacheDataSize( 0 );
        if ( cache->Retrieve( cacheFileName, cacheData, cacheDataSize ) )
        {
            // Hash the PCH result if we will need it later
            uint64_t pchKey = 0;
            if ( GetFlag( FLAG_CREATING_PCH ) && GetFlag( FLAG_MSVC ) )
            {
                pchKey = xxHash::Calc64( cacheData, cacheDataSize );
            }

            // do decompression
            Compressor c;
            if ( c.IsValidData( cacheData, cacheDataSize ) == false )
            {
                FLOG_WARN( "Cache returned invalid data for '%s'", m_Name.Get() );
                return false;
            }
            c.Decompress( cacheData );
            const void * data = c.GetResult();
            const size_t dataSize = c.GetResultSize();

            MultiBuffer buffer( data, dataSize );

            Array< AString > fileNames( 2, false );
            fileNames.Append( m_Name );

            AStackString<> extraFile;
            if ( GetExtraCacheFilePath( job, extraFile ) )
            {
                fileNames.Append( extraFile );
            }

            // Get current "system time" and convert to "file time"
            #if defined( __WINDOWS__ )
                const uint64_t fileTimeNow = Time::GetCurrentFileTime();
            #endif

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

                // Get current "system time" and convert to "file time"
                #if defined( __WINDOWS__ )
                    const bool timeSetOK = FileIO::SetFileLastWriteTime( fileNames[ i ], fileTimeNow );
                #elif defined( __APPLE__ ) || defined( __LINUX__ )
                    const bool timeSetOK = ( utimes( fileNames[ i ].Get(), nullptr ) == 0 );
                #endif

                // set the time on the local file
                if ( timeSetOK == false )
                {
                    cache->FreeMemory( cacheData, cacheDataSize );
                    FLOG_ERROR( "Failed to set timestamp on file after cache hit '%s' (%u)", fileNames[ i ].Get(), Env::GetLastErr() );
                    return false;
                }
            }

            cache->FreeMemory( cacheData, cacheDataSize );

            FileIO::WorkAroundForWindowsFilePermissionProblem( m_Name );

            // the file time we set and local file system might have different
            // granularity for timekeeping, so we need to update with the actual time written
            m_Stamp = FileIO::GetFileLastWriteTime( m_Name );

            FLOG_INFO( "Cache hit: %u ms '%s'\n", uint32_t( t.GetElapsedMS() ), cacheFileName.Get() );
            FLOG_BUILD( "Obj: %s <CACHE>\n", GetName().Get() );
            SetStatFlag( Node::STATS_CACHE_HIT );

            // Dependent objects need to know the PCH key to be able to pull from the cache
            if ( GetFlag( FLAG_CREATING_PCH ) && GetFlag( FLAG_MSVC ) )
            {
                m_PCHCacheKey = pchKey;
            }

            return true;
        }
    }

    FLOG_INFO( "Cache miss: %u ms '%s'\n", uint32_t( t.GetElapsedMS() ), cacheFileName.Get() );
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

    const AString & cacheFileName = GetCacheName(job);
    ASSERT(!cacheFileName.IsEmpty());

    Timer t;

    ICache * cache = FBuild::Get().GetCache();
    ASSERT( cache );
    if ( cache )
    {
        Array< AString > fileNames( 2, false );
        fileNames.Append( m_Name );

        AStackString<> extraFile;
        if ( GetExtraCacheFilePath( job, extraFile ) )
        {
            fileNames.Append( extraFile );
        }

        MultiBuffer buffer;
        if ( buffer.CreateFromFiles( fileNames ) )
        {
            // try to compress
            Compressor c;
            c.Compress( buffer.GetData(), (size_t)buffer.GetDataSize() );
            const void * data = c.GetResult();
            const size_t dataSize = c.GetResultSize();

            if ( cache->Publish( cacheFileName, data, dataSize ) )
            {
                // cache store complete
                FLOG_INFO( "Cache store: %u ms '%s'\n", uint32_t( t.GetElapsedMS() ), cacheFileName.Get() );
                SetStatFlag( Node::STATS_CACHE_STORE );

                // Dependent objects need to know the PCH key to be able to pull from the cache
                if ( GetFlag( FLAG_CREATING_PCH ) && GetFlag( FLAG_MSVC ) )
                {
                    m_PCHCacheKey = xxHash::Calc64( data, dataSize );
                }
                return;
            }
        }
    }

    FLOG_INFO( "Cache store fail: %u ms '%s'\n", uint32_t( t.GetElapsedMS() ), cacheFileName.Get() );
}

// GetExtraCacheFilePath
//------------------------------------------------------------------------------
bool ObjectNode::GetExtraCacheFilePath( const Job * job, AString & extraFileName ) const
{
    const Node * node = job->GetNode();
    if ( node->GetType() == Node::OBJECT_NODE )
    {
        const ObjectNode * objectNode = node->CastTo< ObjectNode >();
        if ( objectNode->GetFlag( ObjectNode::FLAG_MSVC ) &&
             objectNode->GetFlag( ObjectNode::FLAG_CREATING_PCH ) )
        {
            extraFileName = m_PCHObjectFileName;
            return true;
        }
    }
    return false;
}

// EmitCompilationMessage
//------------------------------------------------------------------------------
void ObjectNode::EmitCompilationMessage( const Args & fullArgs, bool useDeoptimization, bool stealingRemoteJob, bool racingRemoteJob, bool useDedicatedPreprocessor, bool isRemote ) const
{
    // print basic or detailed output, depending on options
    // we combine everything into one string to ensure it is contiguous in
    // the output
    AStackString<> output;
    output += "Obj: ";
    if ( useDeoptimization )
    {
        output += "**Deoptimized** ";
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
    if ( FLog::ShowInfo() || ( FBuild::IsValid() && FBuild::Get().GetOptions().m_ShowCommandLines ) || isRemote )
    {
        output += useDedicatedPreprocessor ? GetDedicatedPreprocessor()->GetName().Get() : GetCompiler() ? GetCompiler()->GetName().Get() : "";
        output += ' ';
        output += fullArgs.GetRawArgs();
        output += '\n';
    }
    FLOG_BUILD_DIRECT( output.Get() );
}

// StripTokenWithArg
//------------------------------------------------------------------------------
/*static*/ bool ObjectNode::StripTokenWithArg( const char * tokenToCheckFor, const AString & token, size_t & index )
{
    if ( token.BeginsWith( tokenToCheckFor ) )
    {
        if ( token == tokenToCheckFor )
        {
            ++index; // skip additional next token
        }
        return true; // found
    }
    return false; // not found
}

// StripTokenWithArg_MSVC
//------------------------------------------------------------------------------
/*static*/ bool ObjectNode::StripTokenWithArg_MSVC( const char * tokenToCheckFor, const AString & token, size_t & index )
{
    if ( IsStartOfCompilerArg_MSVC( token, tokenToCheckFor ) )
    {
        if ( IsCompilerArg_MSVC( token, tokenToCheckFor ) )
        {
            ++index; // skip additional next token
        }
        return true; // found
    }
    return false; // not found
}
// StripToken
//------------------------------------------------------------------------------
/*static*/ bool ObjectNode::StripToken( const char * tokenToCheckFor, const AString & token, bool allowStartsWith )
{
    if ( allowStartsWith )
    {
        return token.BeginsWith( tokenToCheckFor );
    }
    else
    {
        return ( token == tokenToCheckFor );
    }
}

// StripToken_MSVC
//------------------------------------------------------------------------------
/*static*/ bool ObjectNode::StripToken_MSVC( const char * tokenToCheckFor, const AString & token, bool allowStartsWith )
{
    if ( allowStartsWith )
    {
        return IsStartOfCompilerArg_MSVC( token, tokenToCheckFor );
    }
    else
    {
        return IsCompilerArg_MSVC( token, tokenToCheckFor );
    }
}


// BuildArgs
//------------------------------------------------------------------------------
bool ObjectNode::BuildArgs( const Job * job, Args & fullArgs, Pass pass, bool useDeoptimization, bool showIncludes, const AString & overrideSrcFile ) const
{
    PROFILE_FUNCTION

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

    const bool isMSVC   = ( useDedicatedPreprocessor ) ? GetPreprocessorFlag( FLAG_MSVC ) : GetFlag( FLAG_MSVC );
    const bool isClang  = ( useDedicatedPreprocessor ) ? GetPreprocessorFlag( FLAG_CLANG ) : GetFlag( FLAG_CLANG );
    const bool isGCC    = ( useDedicatedPreprocessor ) ? GetPreprocessorFlag( FLAG_GCC ) : GetFlag( FLAG_GCC );
    const bool isSNC    = ( useDedicatedPreprocessor ) ? GetPreprocessorFlag( FLAG_SNC ) : GetFlag( FLAG_SNC );
    const bool isCWWii  = ( useDedicatedPreprocessor ) ? GetPreprocessorFlag( CODEWARRIOR_WII ) : GetFlag( CODEWARRIOR_WII );
    const bool isGHWiiU = ( useDedicatedPreprocessor ) ? GetPreprocessorFlag( GREENHILLS_WIIU ) : GetFlag( GREENHILLS_WIIU );
    const bool isCUDA   = ( useDedicatedPreprocessor ) ? GetPreprocessorFlag( FLAG_CUDA_NVCC ) : GetFlag( FLAG_CUDA_NVCC );
    const bool isQtRCC  = ( useDedicatedPreprocessor ) ? GetPreprocessorFlag( FLAG_QT_RCC ) : GetFlag( FLAG_QT_RCC );
    const bool isVBCC   = ( useDedicatedPreprocessor ) ? GetPreprocessorFlag( FLAG_VBCC ) : GetFlag( FLAG_VBCC );

    const size_t numTokens = tokens.GetSize();
    for ( size_t i = 0; i < numTokens; ++i )
    {
        // current token
        const AString & token = tokens[ i ];

        // -o removal for preprocessor
        if ( pass == PASS_PREPROCESSOR_ONLY )
        {
            if ( isGCC || isSNC || isClang || isCWWii || isGHWiiU || isCUDA || isVBCC )
            {
                if ( StripTokenWithArg( "-o", token, i ) )
                {
                    continue;
                }

                if ( !isVBCC )
                {
                    if ( StripToken( "-c", token ) )
                    {
                        continue; // remove -c (compile) option when using preprocessor only
                    }
                }
            }
            else if ( isQtRCC )
            {
                // remove --output (or alias -o) so dependency list goes to stdout
                if ( StripTokenWithArg( "--output", token, i ) ||
                     StripTokenWithArg( "-o", token, i ) )
                {
                    continue;
                }
            }
        }

        if ( isClang || isGCC ) // Also check when invoked via gcc sym link
        {
            // The pch can only be utilized when doing a direct compilation
            //  - Can't be used to generate the preprocessed output
            //  - Can't be used to accelerate compilation of the preprocessed output
            if ( pass != PASS_COMPILE )
            {
                if ( StripTokenWithArg( "-include-pch", token, i ) )
                {
                    continue; // skip this token in both cases
                }
            }
        }

        if ( isMSVC )
        {
            if ( pass == PASS_COMPILE_PREPROCESSED )
            {
                // Can't use the precompiled header when compiling the preprocessed output
                // as this would prevent cacheing.
                if ( StripTokenWithArg_MSVC( "Yu", token, i ) )
                {
                    continue; // skip this token in both cases
                }
                if ( StripTokenWithArg_MSVC( "Fp", token, i ) )
                {
                    continue; // skip this token in both cases
                }

                // Remote compilation writes to a temp pdb
                if ( job->IsLocal() == false )
                {
                    if ( StripTokenWithArg_MSVC( "Fd", token, i ) )
                    {
                        continue; // skip this token in both cases
                    }
                }
            }
        }

        if ( isMSVC )
        {
            // FASTBuild handles the multiprocessor scheduling
            if ( StripToken_MSVC( "MP", token, true ) ) // true = strip '/MP' and starts with '/MP'
            {
                continue;
            }

            // "Minimal Rebuild" is not compatible with FASTBuild
            if ( StripToken_MSVC( "Gm", token ) )
            {
                continue;
            }
        }

        // remove includes for second pass
        if ( pass == PASS_COMPILE_PREPROCESSED )
        {
            if ( isClang )
            {
                // Clang requires -I options be stripped when compiling preprocessed code
                // (it raises an error if we don't remove these)
                if ( StripTokenWithArg( "-I", token, i ) )
                {
                    continue; // skip this token in both cases
                }
            }
            if ( isGCC || isClang || isVBCC )
            {
                // Remove forced includes so they aren't forced twice
                if ( StripTokenWithArg( "-include", token, i ) )
                {
                    continue; // skip this token in both cases
                }
            }
            if ( isMSVC )
            {
                // NOTE: Leave /I includes for compatibility with Recode
                // (unlike Clang, MSVC is ok with leaving the /I when compiling preprocessed code)

                // To prevent D8049 "command line is too long to fit in debug record"
                // we expand relative includes as they would be on the host (so the remote machine's
                // working dir is not used, which might be longer, causing this overflow an internal
                // limit of cl.exe)
                if ( ( job->IsLocal() == false ) && IsStartOfCompilerArg_MSVC( token, "I" ) )
                {
                    // Get include path part
                    const char * start = token.Get() + 2; // Skip /I or -I
                    const char * end = token.GetEnd();

                    // strip quotes if present
                    if ( *start == '"' )
                    {
                        ++start;
                    }
                    if ( end[ -1 ] == '"' )
                    {
                        --end;
                    }
                    AStackString<> includePath( start, end );
                    const bool isFullPath = PathUtils::IsFullPath( includePath );

                    // Replace relative paths and leave full paths alone
                    if ( isFullPath == false )
                    {
                        // Remove relative include
                        StripTokenWithArg_MSVC( "I", token, i );

                        // Add full path include
                        fullArgs.Append( token.Get(), start - token.Get() );
                        fullArgs += job->GetRemoteSourceRoot();
                        fullArgs += '\\';
                        fullArgs += includePath;
                        fullArgs.Append( end, token.GetEnd() - end );
                        fullArgs.AddDelimiter();

                        continue; // Include path has been replaced
                    }
                }

                // Strip "Force Includes" statements (as they are merged in now)
                if ( StripTokenWithArg_MSVC( "FI", token, i ) )
                {
                    continue; // skip this token in both cases
                }
            }
        }

        if ( isMSVC )
        {
            if ( pass == PASS_PREPROCESSOR_ONLY )
            {
                //Strip /ZW
                if ( StripToken_MSVC( "ZW", token ) )
                {
                    fullArgs += "/D__cplusplus_winrt ";
                    continue;
                }

                // Strip /Yc (pch creation)
                if ( StripTokenWithArg_MSVC( "Yc", token, i ) )
                {
                    continue;
                }
            }
        }

        // Remove static analyzer from clang preprocessor
        if ( pass == PASS_PREPROCESSOR_ONLY )
        {
            if ( isClang )
            {
                if ( StripToken( "--analyze", token ) )
                {
                    continue;
                }

                if ( StripTokenWithArg( "-Xanalyzer", token, i ) ||
                     StripTokenWithArg( "-analyzer-output", token, i ) ||
                     StripTokenWithArg( "-analyzer-config", token, i ) ||
                     StripTokenWithArg( "-analyzer-checker", token, i ) )
                {
                    continue;
                }
            }
        }

        // %1 -> InputFile
        const char * found = token.Find( "%1" );
        if ( found )
        {
            fullArgs += AStackString<>( token.Get(), found );
            if ( overrideSrcFile.IsEmpty() )
            {
                fullArgs += GetSourceFile()->GetName();
            }
            else
            {
                fullArgs += overrideSrcFile;
            }
            fullArgs += AStackString<>( found + 2, token.GetEnd() );
            fullArgs.AddDelimiter();
            continue;
        }

        // %2 -> OutputFile
        found = token.Find( "%2" );
        if ( found )
        {
            fullArgs += AStackString<>( token.Get(), found );
            fullArgs += m_Name;
            fullArgs += AStackString<>( found + 2, token.GetEnd() );
            fullArgs.AddDelimiter();
            continue;
        }

        // %3 -> PrecompiledHeader Obj
        if ( isMSVC )
        {
            found = token.Find( "%3" );
            if ( found )
            {
                // handle /Option:%3 -> /Option:A
                fullArgs += AStackString<>( token.Get(), found );
                ASSERT( m_PCHObjectFileName.IsEmpty() == false ); // Should have been populated
                fullArgs += m_PCHObjectFileName;
                fullArgs += AStackString<>( found + 2, token.GetEnd() );
                fullArgs.AddDelimiter();
                continue;
            }
        }

        // %4 -> CompilerForceUsing list
        if ( isMSVC )
        {
            found = token.Find( "%4" );
            if ( found )
            {
                AStackString<> pre( token.Get(), found );
                AStackString<> post( found + 2, token.GetEnd() );
                ExpandCompilerForceUsing( fullArgs, pre, post );
                fullArgs.AddDelimiter();
                continue;
            }
        }

        // cl.exe treats \" as an escaped quote
        // It's a common user error to terminate things (like include paths) with a quote
        // this way, messing up the rest of the args and causing bizarre failures.
        // Since " is not a valid character in a path, just strip the escape char
        if ( isMSVC )
        {
            // Is this invalid?
            //  bad: /I"directory\"  - TODO:B Handle other args with this problem
            //  ok : /I\"directory\"
            //  ok : /I"directory"
            if ( ObjectNode::IsStartOfCompilerArg_MSVC( token, "I\"" ) && token.EndsWith( "\\\"" ) )
            {
                fullArgs.Append( token.Get(), token.GetLength() - 2 );
                fullArgs += '"';
                fullArgs.AddDelimiter();
                continue;
            }
        }

        // untouched token
        fullArgs += token;
        fullArgs.AddDelimiter();
    }

    if ( pass == PASS_PREPROCESSOR_ONLY )
    {
        if ( isMSVC )
        {
            fullArgs += "/E"; // run pre-processor only

            // Ensure unused defines declared in the PCH but not used
            // in the PCH are accounted for (See TestPrecompiledHeaders/CacheUniqueness)
            if ( GetFlag( FLAG_CREATING_PCH ) )
            {
                fullArgs += " /d1PP"; // Must be after /E
            }
        }
        else if ( isQtRCC )
        {
            fullArgs += " --list"; // List used resources
        }
        else
        {
            ASSERT( isGCC || isSNC || isClang || isCWWii || isGHWiiU || isCUDA || isVBCC );
            fullArgs += "-E"; // run pre-processor only

            // Ensure unused defines declared in the PCH but not used
            // in the PCH are accounted for (See TestPrecompiledHeaders/CacheUniqueness)
            if ( GetFlag( FLAG_CREATING_PCH ) )
            {
                fullArgs += " -dD";
            }

            const bool clangRewriteIncludes = GetCompiler()->CastTo< CompilerNode >()->IsClangRewriteIncludesEnabled();
            if ( isClang && clangRewriteIncludes )
            {
                fullArgs += " -frewrite-includes";
            }
        }
    }

    if ( showIncludes )
    {
        fullArgs += " /showIncludes"; // we'll extract dependency information from this
    }

    // Remote compilation writes to a temp pdb
    if ( ( job->IsLocal() == false ) &&
         ( job->GetNode()->CastTo< ObjectNode >()->IsUsingPDB() ) )
    {
        AStackString<> pdbName, tmp;
        GetPDBName( pdbName );
        tmp.Format( " /Fd\"%s\"", pdbName.Get() );
        fullArgs += tmp;
    }

    // Handle all the special needs of args
    AStackString<> remoteCompiler;
    if ( job->IsLocal() == false )
    {
        job->GetToolManifest()->GetRemoteFilePath( 0, remoteCompiler );
    }
    const AString& compiler = job->IsLocal() ? GetCompiler()->GetName() : remoteCompiler;
    if ( fullArgs.Finalize( compiler, GetName(), CanUseResponseFile() ) == false )
    {
        return false; // Finalize will have emitted an error
    }

    return true;
}

// ExpandCompilerForceUsing
//------------------------------------------------------------------------------
void ObjectNode::ExpandCompilerForceUsing( Args & fullArgs, const AString & pre, const AString & post ) const
{
    const size_t startIndex = 2 + ( m_PrecompiledHeader ? 1 : 0 ) + ( !m_Preprocessor.IsEmpty() ? 1 : 0 ); // Skip Compiler, InputFile, PCH and Preprocessor
    const size_t endIndex = m_StaticDependencies.GetSize();
    for ( size_t i=startIndex; i<endIndex; ++i )
    {
        Node * n = m_StaticDependencies[ i ].GetNode();

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
         useDedicatedPreprocessor ? GetDedicatedPreprocessor()->GetName() : GetCompiler()->GetName(),
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
            if ( ch.GetErr().Get() )
            {
                DumpOutput( job, ch.GetErr().Get(), ch.GetErrSize(), GetName() );
            }
            else
            {
                DumpOutput( job, ch.GetOut().Get(), ch.GetOutSize(), GetName() );
            }
        }

        return false; // SpawnCompiler will have emitted error
    }

    // take a copy of the output because ReadAllData uses huge buffers to avoid re-sizing
    TransferPreprocessedData( ch.GetOut().Get(), ch.GetOutSize(), job );

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
    AutoPtr< void > mem( ALLOC( contentSize ) );
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
            CompilerNode* cn = GetCompiler()->CastTo< CompilerNode >();
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
            do
            {
                buggyEnum = strstr( workBuffer, BUGGY_CODE );
                if ( buggyEnum != nullptr )
                {
                    ++nbrEnumsFound;
                    enumsFound.Append( buggyEnum );
                    workBuffer = buggyEnum + sizeof( BUGGY_CODE ) - 1;
                }
            } while ( buggyEnum != nullptr );
            ASSERT( enumsFound.GetSize() == nbrEnumsFound );

            // Now allocate the new buffer with enough space to add a space after each enum found
            newBufferSize = outputBufferSize + nbrEnumsFound;
            bufferCopy = (char *)ALLOC( newBufferSize + 1 ); // null terminator for include parser

            uint32_t enumIndex = 0;
            workBuffer = outputBuffer;
            char * writeDest = bufferCopy;
            ptrdiff_t sizeLeftInSourceBuffer = outputBufferSize;
            buggyEnum = nullptr;
            do
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

                if ( buggyEnum != nullptr )
                {
                    // Copy what's before the enum + the enum.
                    ptrdiff_t sizeToCopy = buggyEnum - workBuffer;
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
                else
                {
                    // Copy the rest of the data.
                    memcpy( writeDest, workBuffer, sizeLeftInSourceBuffer );
                    break;
                }
            } while ( buggyEnum != nullptr );

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

    Node * sourceFile = GetSourceFile();
    uint32_t sourceNameHash = xxHash::Calc32( sourceFile->GetName().Get(), sourceFile->GetName().GetLength() );

    FileStream tmpFile;
    AStackString<> fileName( sourceFile->GetName().FindLast( NATIVE_SLASH ) + 1 );

    void const * dataToWrite = job->GetData();
    size_t dataToWriteSize = job->GetDataSize();

    // handle compressed data
    Compressor c; // scoped here so we can access decompression buffer
    if ( job->IsDataCompressed() )
    {
        c.Decompress( dataToWrite );
        dataToWrite = c.GetResult();
        dataToWriteSize = c.GetResultSize();
    }

    WorkerThread::GetTempFileDirectory( tmpDirectory );
    tmpDirectory.AppendFormat( "%08X%c", sourceNameHash, NATIVE_SLASH );
    if ( FileIO::DirectoryCreate( tmpDirectory ) == false )
    {
        job->Error( "Failed to create temp directory '%s' to build '%s' (error %u)", tmpDirectory.Get(), GetName().Get(), Env::GetLastErr() );
        job->OnSystemError();
        return NODE_RESULT_FAILED;
    }
    tmpFileName = tmpDirectory;
    tmpFileName += fileName;
    if ( WorkerThread::CreateTempFile( tmpFileName, tmpFile ) == false )
    {
        job->Error( "Failed to create temp file '%s' to build '%s' (error %u)", tmpFileName.Get(), GetName().Get(), Env::GetLastErr() );
        job->OnSystemError();
        return NODE_RESULT_FAILED;
    }
    if ( tmpFile.Write( dataToWrite, dataToWriteSize ) != dataToWriteSize )
    {
        job->Error( "Failed to write to temp file '%s' to build '%s' (error %u)", tmpFileName.Get(), GetName().Get(), Env::GetLastErr() );
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
        compiler = GetCompiler()->GetName();
    }
    else
    {
        ASSERT( job->GetToolManifest() );
        job->GetToolManifest()->GetRemoteFilePath( 0, compiler );
        job->GetToolManifest()->GetRemotePath( workingDir );
    }

    // spawn the process
    CompileHelper ch( true, job->GetAbortFlagPointer() );
    if ( !ch.SpawnCompiler( job, GetName(), compiler, fullArgs, workingDir.IsEmpty() ? nullptr : workingDir.Get() ) )
    {
        // did spawn fail, or did we spawn and fail to compile?
        if ( ch.GetResult() != 0 )
        {
            // failed to compile

            // for remote jobs, we must serialize the errors to return with the job
            if ( job->IsLocal() == false )
            {
                AutoPtr< char > mem( (char *)ALLOC( ch.GetOutSize() + ch.GetErrSize() ) );
                memcpy( mem.Get(), ch.GetOut().Get(), ch.GetOutSize() );
                memcpy( mem.Get() + ch.GetOutSize(), ch.GetErr().Get(), ch.GetErrSize() );
                job->OwnData( mem.Release(), ( ch.GetOutSize() + ch.GetErrSize() ) );
            }
        }

        return false; // compile has logged error
    }
    else
    {
        // Handle MSCL warnings if not already a failure
        if ( IsMSVC() && ( ch.GetResult() == 0 ) && !GetFlag( FLAG_WARNINGS_AS_ERRORS_MSVC ))
        {
            HandleWarningsMSVC( job, GetName(), ch.GetOut().Get(), ch.GetOutSize() );
        }
    }

    return true;
}

// CompileHelper::CONSTRUCTOR
//------------------------------------------------------------------------------
ObjectNode::CompileHelper::CompileHelper( bool handleOutput, const volatile bool * abortPointer )
    : m_HandleOutput( handleOutput )
    , m_Process( FBuild::GetAbortBuildPointer(), abortPointer )
    , m_OutSize( 0 )
    , m_ErrSize( 0 )
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
                                               const AString & compiler,
                                               const Args & fullArgs,
                                               const char * workingDir )
{
    const char * environmentString = ( FBuild::IsValid() ? FBuild::Get().GetEnvironmentString() : nullptr );
    if ( ( job->IsLocal() == false ) && ( job->GetToolManifest() ) )
    {
        environmentString = job->GetToolManifest()->GetRemoteEnvironmentString();
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

        job->Error( "Failed to spawn process (error 0x%x) to build '%s'\n", Env::GetLastErr(), name.Get() );
        job->OnSystemError();
        return false;
    }

    // capture all of the stdout and stderr
    m_Process.ReadAllData( m_Out, &m_OutSize, m_Err, &m_ErrSize );

    // Get result
    m_Result = m_Process.WaitForExit();
    if ( m_Process.HasAborted() )
    {
        return false;
    }

    // Handle special types of failures
    HandleSystemFailures( job, m_Result, m_Out.Get(), m_Err.Get() );

    // output any errors (even if succeeded, there might be warnings)
    if ( m_HandleOutput && m_Err.Get() )
    {
        const bool treatAsWarnings = true; // change msg formatting
        DumpOutput( job, m_Err.Get(), m_ErrSize, name, treatAsWarnings );
    }

    // failed?
    if ( m_Result != 0 )
    {
        // output 'stdout' which may contain errors for some compilers
        if ( m_HandleOutput )
        {
            DumpOutput( job, m_Out.Get(), m_OutSize, name );
        }

        job->Error( "Failed to build Object (error 0x%x) '%s'\n", m_Result, name.Get() );

        return false;
    }

    return true;
}

// HandleSystemFailures
//------------------------------------------------------------------------------
/*static*/ void ObjectNode::HandleSystemFailures( Job * job, int result, const char * stdOut, const char * stdErr )
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

    #if defined( __WINDOWS__ )
        // If remote PC is shutdown by user, compiler can be terminated
        if ( ( (uint32_t)result == 0x40010004 ) || // DBG_TERMINATE_PROCESS
             ( (uint32_t)result == 0xC0000142 ) )  // STATUS_DLL_INIT_FAILED - Occurs if remote PC is stuck on force reboot dialog during shutdown
        {
            job->OnSystemError(); // task will be retried on another worker
            return;
        }

        // If DLLs are not correctly sync'd, add an extra message to help the user
        if ( (uint32_t)result == 0xC000007B ) // STATUS_INVALID_IMAGE_FORMAT
        {
            job->Error( "Remote failure: STATUS_INVALID_IMAGE_FORMAT (0xC000007B) - Check Compiler() settings!\n" );
            return;
        }

        const ObjectNode * objectNode = job->GetNode()->CastTo< ObjectNode >();

        // MSVC errors
        if ( objectNode->IsMSVC() )
        {
            // But we need to determine if it's actually an out of space
            // (rather than some compile error with missing file(s))
            // These error code have been observed in the wild
            if ( stdOut )
            {
                if ( strstr( stdOut, "C1082" ) ||
                     strstr( stdOut, "C1085" ) ||
                     strstr( stdOut, "C1088" ) )
                {
                    job->OnSystemError();
                    return;
                }
            }

            // If the compiler crashed (Internal Compiler Error), treat this
            // as a system error so it will be retried, since it can alse be
            // the result of faulty hardware.
            if ( stdOut && strstr( stdOut, "C1001" ) )
            {
                job->OnSystemError();
                return;
            }

            // Error messages above also contains this text
            // (We check for this message additionally to handle other error codes
            //  that may not have been observed yet)
            // TODO:C Should we check for localized msg?
            if ( stdOut && strstr( stdOut, "No space left on device" ) )
            {
                job->OnSystemError();
                return;
            }
        }

        // Clang
        if ( objectNode->GetFlag( ObjectNode::FLAG_CLANG ) )
        {
            // When clang fails due to low disk space
            if ( result == 0x01 )
            {
                // TODO:C Should we check for localized msg?
                if ( stdErr && ( strstr( stdErr, "IO failure on output stream" ) ) )
                {
                    job->OnSystemError();
                    return;
                }
            }
        }

        // GCC
        if ( objectNode->GetFlag( ObjectNode::FLAG_GCC ) )
        {
            // When gcc fails due to low disk space
            if ( result == 0x01 )
            {
                // TODO:C Should we check for localized msg?
                if ( stdErr && ( strstr( stdErr, "No space left on device" ) ) )
                {
                    job->OnSystemError();
                    return;
                }
            }
        }
    #else
        // TODO:LINUX TODO:MAC Implement remote system failure checks
        (void)stdOut;
        (void)stdErr;
    #endif
}

// ShouldUseDeoptimization
//------------------------------------------------------------------------------
bool ObjectNode::ShouldUseDeoptimization() const
{
    if ( GetFlag( FLAG_UNITY ) )
    {
        return false; // disabled for Unity files (which are always writable)
    }

    if ( ( m_DeoptimizeWritableFilesWithToken == false ) &&
         ( m_DeoptimizeWritableFiles == false ) )
    {
        return false; // feature not enabled
    }

    // Neither flag should be set for the creation of Precompiled Headers
    ASSERT( GetFlag( FLAG_CREATING_PCH ) == false );

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
    bool useCache = GetFlag( FLAG_CAN_BE_CACHED ) &&
                    m_AllowCaching &&
                    ( FBuild::Get().GetOptions().m_UseCacheRead ||
                     FBuild::Get().GetOptions().m_UseCacheWrite );
    if ( GetFlag( FLAG_ISOLATED_FROM_UNITY ) )
    {
        // disable caching for locally modified files (since they will rarely if ever get a hit)
        useCache = false;
    }
    if ( GetFlag( FLAG_MSVC ) && GetFlag( FLAG_USING_PCH ) )
    {
        // If the PCH is not in the cache, then no point looking there
        // for objects and also no point storing them
        if ( m_PrecompiledHeader->CastTo< ObjectNode >()->m_PCHCacheKey == 0 )
        {
            return false;
        }
    }
    return useCache;
}

// CanUseResponseFile
//------------------------------------------------------------------------------
bool ObjectNode::CanUseResponseFile() const
{
    #if defined( __WINDOWS__ )
        // Generally only windows applications support response files (to overcome Windows command line limits)
        return ( GetFlag( FLAG_MSVC ) || GetFlag( FLAG_GCC ) || GetFlag( FLAG_SNC ) || GetFlag( FLAG_CLANG ) || GetFlag( CODEWARRIOR_WII ) || GetFlag( GREENHILLS_WIIU ) );
    #else
        return false;
    #endif
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

//------------------------------------------------------------------------------
