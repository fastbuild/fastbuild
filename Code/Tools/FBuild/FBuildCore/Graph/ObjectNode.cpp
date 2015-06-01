// ObjectNode.cpp
//------------------------------------------------------------------------------

// Includes
//------------------------------------------------------------------------------
#include "Tools/FBuild/FBuildCore/PrecompiledHeader.h"

#include "ObjectNode.h"

#include "Tools/FBuild/FBuildCore/Cache/ICache.h"
#include "Tools/FBuild/FBuildCore/FBuild.h"
#include "Tools/FBuild/FBuildCore/FLog.h"
#include "Tools/FBuild/FBuildCore/Graph/CompilerNode.h"
#include "Tools/FBuild/FBuildCore/Graph/NodeGraph.h"
#include "Tools/FBuild/FBuildCore/Graph/NodeProxy.h"
#include "Tools/FBuild/FBuildCore/Helpers/CIncludeParser.h"
#include "Tools/FBuild/FBuildCore/Helpers/Compressor.h"
#include "Tools/FBuild/FBuildCore/Helpers/ResponseFile.h"
#include "Tools/FBuild/FBuildCore/Helpers/ToolManifest.h"
#include "Tools/FBuild/FBuildCore/WorkerPool/Job.h"
#include "Tools/FBuild/FBuildCore/WorkerPool/JobQueue.h"
#include "Tools/FBuild/FBuildCore/WorkerPool/WorkerThread.h"

// Core
#include "Core/Env/Env.h"
#include "Core/FileIO/FileIO.h"
#include "Core/FileIO/FileStream.h"
#include "Core/FileIO/PathUtils.h"
#include "Core/Math/Murmur3.h"
#include "Core/Process/Process.h"
#include "Core/Process/Thread.h"
#include "Core/Tracing/Tracing.h"
#include "Core/Strings/AStackString.h"

#include <string.h>
#if defined( __WINDOWS__ )
	#include <windows.h> // for FILETIME etc
#endif
#if defined( __OSX__ ) || defined( __LINUX__ )
    #include <sys/time.h>
#endif

// CONSTRUCTOR
//------------------------------------------------------------------------------
ObjectNode::ObjectNode( const AString & objectName,
					    Node * inputNode,
					    Node * compilerNode,
						const AString & compilerArgs,
						const AString & compilerArgsDeoptimized,
						Node * precompiledHeader,
						uint32_t flags,
						const Dependencies & compilerForceUsing,
						bool deoptimizeWritableFiles,
						bool deoptimizeWritableFilesWithToken,
                        Node * preprocessorNode,
                        const AString & preprocessorArgs,
                        uint32_t preprocessorFlags )
: FileNode( objectName, Node::FLAG_NONE )
, m_Includes( 0, true )
, m_Flags( flags )
, m_CompilerArgs( compilerArgs )
, m_CompilerArgsDeoptimized( compilerArgsDeoptimized )
, m_CompilerForceUsing( compilerForceUsing )
, m_DeoptimizeWritableFiles( deoptimizeWritableFiles )
, m_DeoptimizeWritableFilesWithToken( deoptimizeWritableFilesWithToken )
, m_Remote( false )
, m_PCHNode( precompiledHeader )
, m_PreprocessorNode( preprocessorNode )
, m_PreprocessorArgs( preprocessorArgs )
, m_PreprocessorFlags( preprocessorFlags )
{
	m_StaticDependencies.SetCapacity( 3 );

	ASSERT( compilerNode );
	m_StaticDependencies.Append( Dependency( compilerNode ) );

	ASSERT( inputNode );
	m_StaticDependencies.Append( Dependency( inputNode ) );

	if ( precompiledHeader )
	{
		m_StaticDependencies.Append( Dependency( precompiledHeader ) );
	}
    if ( preprocessorNode )
    {
        m_StaticDependencies.Append( Dependency( preprocessorNode ) );
    }

	m_StaticDependencies.Append( compilerForceUsing );

	m_Type = OBJECT_NODE;
	m_LastBuildTimeMs = 5000; // higher default than a file node
}

// CONSTRUCTOR (Remote)
//------------------------------------------------------------------------------
ObjectNode::ObjectNode( const AString & objectName,
						NodeProxy * srcFile,
						const AString & compilerArgs,
						uint32_t flags )
: FileNode( objectName, Node::FLAG_NONE )
, m_Includes( 0, true )
, m_Flags( flags )
, m_CompilerArgs( compilerArgs )
, m_DeoptimizeWritableFiles( false )
, m_DeoptimizeWritableFilesWithToken( false )
, m_Remote( true )
, m_PCHNode( nullptr )
, m_PreprocessorNode( nullptr )
, m_PreprocessorArgs()
, m_PreprocessorFlags( 0 )
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

// DoBuild
//------------------------------------------------------------------------------
/*virtual*/ Node::BuildResult ObjectNode::DoBuild( Job * job )
{
	// delete previous file
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
		AStackString<> pchObj( GetName() );
		pchObj += GetObjExtension();
		if ( FileIO::FileExists( pchObj.Get() ) )
		{
			if ( FileIO::FileDelete( pchObj.Get() ) == false )
			{
				FLOG_ERROR( "Failed to delete file before build '%s'", GetName().Get() );
				return NODE_RESULT_FAILED;
			}
		}
	}

	// using deoptimization?
	bool useDeoptimization = ShouldUseDeoptimization();

	bool useCache = ShouldUseCache();
	bool useDist = GetFlag( FLAG_CAN_BE_DISTRIBUTED ) && FBuild::Get().GetOptions().m_AllowDistributed;
	bool usePreProcessor = ( useCache || useDist || GetFlag( FLAG_GCC ) || GetFlag( FLAG_SNC ) || GetFlag( FLAG_CLANG ) || GetFlag( CODEWARRIOR_WII ) || GetFlag( GREENHILLS_WIIU ) );
    if ( GetDedicatedPreprocessor() )
    {
        usePreProcessor = true;
        useDeoptimization = false; // disable deoptimization
    }

	if ( usePreProcessor )
	{
		return DoBuildWithPreProcessor( job, useDeoptimization, useCache );
	}

	if ( GetFlag( FLAG_MSVC ) )
	{
		return DoBuildMSCL_NoCache( job, useDeoptimization );
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
/*virtual*/ bool ObjectNode::Finalize()
{
	ASSERT( Thread::IsMainThread() );

	NodeGraph & ng = FBuild::Get().GetDependencyGraph();

	// convert includes to nodes
	m_DynamicDependencies.Clear();
	m_DynamicDependencies.SetCapacity( m_Includes.GetSize() );
	for ( Array< AString >::ConstIter it = m_Includes.Begin();
			it != m_Includes.End();
			it++ )
	{
		Node * fn = FBuild::Get().GetDependencyGraph().FindNode( *it );
		if ( fn == nullptr )
		{
			fn = ng.CreateFileNode( *it );
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
	AStackString< 8 * KILOBYTE > fullArgs;
	BuildFullArgs( job, fullArgs, PASS_COMPILE, useDeoptimization );
	fullArgs += " /showIncludes"; // we'll extract dependency information from this

	EmitCompilationMessage( fullArgs, useDeoptimization );

	// spawn the process
	CompileHelper ch;
	if ( !ch.SpawnCompiler( job, GetName(), GetCompiler()->GetName(), fullArgs, true ) ) // use response file for MSVC
	{
		return NODE_RESULT_FAILED; // SpawnCompiler has logged error
	}

	// compiled ok, try to extract includes
	if ( ProcessIncludesMSCL( ch.GetOut().Get(), ch.GetOutSize() ) == false )
	{
		return NODE_RESULT_FAILED; // ProcessIncludesMSCL will have emitted an error
	}

	// record new file time
	m_Stamp = FileIO::GetFileLastWriteTime( m_Name );

	return NODE_RESULT_OK;
}

// DoBuildWithPreProcessor
//------------------------------------------------------------------------------
Node::BuildResult ObjectNode::DoBuildWithPreProcessor( Job * job, bool useDeoptimization, bool useCache )
{
	AStackString< 8 * KILOBYTE > fullArgs;
	BuildFullArgs( job, fullArgs, PASS_PREPROCESSOR_ONLY, useDeoptimization );

	if ( BuildPreprocessedOutput( fullArgs, job, useDeoptimization ) == false )
	{
		return NODE_RESULT_FAILED; // BuildPreprocessedOutput will have emitted an error
	}

	// preprocessed ok, try to extract includes
	if ( ProcessIncludesWithPreProcessor( job ) == false )
	{
		return NODE_RESULT_FAILED; // ProcessIncludesWithPreProcessor will have emitted an error
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
	if ( GetFlag( FLAG_CAN_BE_DISTRIBUTED ) &&
		 FBuild::Get().GetOptions().m_AllowDistributed &&
		 JobQueue::Get().GetDistributableJobsMemUsage() < ( 512 * MEGABYTE ) )
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
			// If building a distributable/cacheable job locally, and
			// we are not going to write it to the cache, then we should
			// use the PCH as it will be much faster
			if ( GetFlag( FLAG_USING_PCH ) &&
				 ( FBuild::Get().GetOptions().m_UseCacheWrite == false ) )
			{
				usePreProcessedOutput = false;
			}
		}

		// The CUDA compiler seems to not be able to compile its own preprocessed output
		if ( GetFlag( FLAG_CUDA_NVCC ) )
		{
			usePreProcessedOutput = false;
		}
	}

	AStackString< 8 * KILOBYTE > fullArgs;
	AStackString<> tmpFileName;
	if ( usePreProcessedOutput )
	{
		if ( WriteTmpFile( job, tmpFileName ) == false )
		{
			return NODE_RESULT_FAILED; // WriteTmpFile will have emitted an error
		}

		BuildFullArgs( job, fullArgs, PASS_COMPILE_PREPROCESSED, useDeoptimization );

		// use preprocesed output path as input
		Node * sourceFile = GetSourceFile();
		fullArgs.Replace( sourceFile->GetName().Get(), tmpFileName.Get() );
	}
	else
	{
		BuildFullArgs( job, fullArgs, PASS_COMPILE, useDeoptimization );
	}

	if ( stealingRemoteJob || racingRemoteJob )
	{
		// show that we are locally consuming a remote job
		EmitCompilationMessage( fullArgs, useDeoptimization, stealingRemoteJob, racingRemoteJob );
	}

	bool result = BuildFinalOutput( job, fullArgs );

	// cleanup temp file
	if ( tmpFileName.IsEmpty() == false )
	{
		FileIO::FileDelete( tmpFileName.Get() );
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

// DoBuildOther
//------------------------------------------------------------------------------
/*virtual*/ Node::BuildResult ObjectNode::DoBuildOther( Job * job, bool useDeoptimization )
{
	// Format compiler args string
	AStackString< 8 * KILOBYTE > fullArgs;
	BuildFullArgs( job, fullArgs, PASS_COMPILE, useDeoptimization );

	EmitCompilationMessage( fullArgs, useDeoptimization );

	// spawn the process
	CompileHelper ch;
	if ( !ch.SpawnCompiler( job, GetName(), GetCompiler()->GetName(), fullArgs, false ) ) // don't use response file
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
		ASSERT( output && outputSize );

		CIncludeParser parser;
		bool result = parser.ParseMSCL_Output( output, outputSize );
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
		const char  * output = (char *)job->GetData();
		const size_t outputSize = job->GetDataSize();

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
/*static*/ Node * ObjectNode::Load( IOStream & stream )
{
	NODE_LOAD( AStackString<>,	name );
	NODE_LOAD_DEPS( 3,			staticDeps );
	NODE_LOAD_DEPS( 0,			dynamicDeps );
	NODE_LOAD( uint32_t,		flags );
	NODE_LOAD( AStackString<>,	compilerArgs );
	NODE_LOAD( AStackString<>,  compilerArgsDeoptimized )
	NODE_LOAD( AStackString<>,	objExtensionOverride );
	NODE_LOAD_DEPS( 0,			compilerForceUsing );
	NODE_LOAD( bool,			deoptimizeWritableFiles );
	NODE_LOAD( bool,			deoptimizeWritableFilesWithToken );
    NODE_LOAD_NODE( Node,       m_PCHNode )
	NODE_LOAD_NODE( CompilerNode, preprocessor );
    NODE_LOAD( AStackString<>, 	preprocessorArgs );
    NODE_LOAD( uint32_t, 		preprocessorFlags );

	// we are making inferences from the size of the staticDeps
	// ensure we catch if those asumptions break
    #if defined( ASSERTS_ENABLED )
	    size_t numStaticDepsExcludingForceUsing = staticDeps.GetSize() - compilerForceUsing.GetSize();
        // compiler + source file + (optional)precompiledHeader + (optional)preprocessor
	    ASSERT( ( numStaticDepsExcludingForceUsing >= 2 ) && ( numStaticDepsExcludingForceUsing <= 4 ) );
    #endif

	ASSERT( staticDeps.GetSize() >= 2 );
	Node * compiler = staticDeps[ 0 ].GetNode();
	Node * staticDepNode = staticDeps[ 1 ].GetNode();

	NodeGraph & ng = FBuild::Get().GetDependencyGraph();
	Node * on = ng.CreateObjectNode( name, staticDepNode, compiler, compilerArgs, compilerArgsDeoptimized, m_PCHNode, flags, compilerForceUsing, deoptimizeWritableFiles, deoptimizeWritableFilesWithToken, preprocessor, preprocessorArgs, preprocessorFlags );

	ObjectNode * objNode = on->CastTo< ObjectNode >();
	objNode->m_DynamicDependencies.Swap( dynamicDeps );
	objNode->m_ObjExtensionOverride = objExtensionOverride;

	return objNode;
}

// LoadRemote
//------------------------------------------------------------------------------
/*static*/ Node * ObjectNode::LoadRemote( IOStream & stream )
{
	NODE_LOAD( AStackString<>,	name );
	NODE_LOAD( AStackString<>,	sourceFile );
	NODE_LOAD( uint32_t,		flags );
	NODE_LOAD( AStackString<>,	compilerArgs );

	NodeProxy * srcFile = FNEW( NodeProxy( sourceFile ) );

	return FNEW( ObjectNode( name, srcFile, compilerArgs, flags ) );
}

// DetermineFlags
//------------------------------------------------------------------------------
/*static*/ uint32_t ObjectNode::DetermineFlags( const Node * compilerNode, const AString & args )
{
	uint32_t flags = 0;

	const AString & compiler = compilerNode->GetName();
	const bool isDistributableCompiler = ( compilerNode->GetType() == Node::COMPILER_NODE ) && 
										 ( compilerNode->CastTo< CompilerNode >()->CanBeDistributed() );

	// Compiler Type
	if ( compiler.EndsWithI( "\\cl.exe" ) ||
		 compiler.EndsWithI( "\\cl" ) )
    {
		flags |= ObjectNode::FLAG_MSVC;
	}
	else if ( compiler.EndsWithI( "clang++.exe" ) ||
			  compiler.EndsWithI( "clang++" ) ||
			  compiler.EndsWithI( "clang.exe" ) ||
			  compiler.EndsWithI( "clang" ) ||
              compiler.EndsWithI( "clang-cl.exe" ) ||
		      compiler.EndsWithI( "clang-cl" ) )
	{
		flags |= ObjectNode::FLAG_CLANG;
	}
	else if ( compiler.EndsWithI( "gcc.exe" ) || 
			  compiler.EndsWithI( "gcc" ) ||
			  compiler.EndsWithI( "g++.exe" ) ||
			  compiler.EndsWithI( "g++" ) )
	{
		flags |= ObjectNode::FLAG_GCC;
	}
	else if ( compiler.EndsWithI( "\\ps3ppusnc.exe" ) ||
			  compiler.EndsWithI( "\\ps3ppusnc" ) )
	{
		flags |= ObjectNode::FLAG_SNC;
	}
	else if ( compiler.EndsWithI( "\\mwcceppc.exe" ) ||
			  compiler.EndsWithI( "\\mwcceppc" ) )
	{
		flags |= ObjectNode::CODEWARRIOR_WII;
	}
	else if ( compiler.EndsWithI( "\\cxppc.exe" ) ||
			  compiler.EndsWithI( "\\cxppc" ) ||
			  compiler.EndsWithI( "\\ccppc.exe" ) ||
			  compiler.EndsWithI( "\\ccppc" ) )
	{
		flags |= ObjectNode::GREENHILLS_WIIU;
	}
	else if ( compiler.EndsWithI( "\\nvcc.exe" ) ||
			  compiler.EndsWithI( "\\nvcc" ) )
	{
		flags |= ObjectNode::FLAG_CUDA_NVCC;
	}

	// Check MS compiler options
	if ( flags & ObjectNode::FLAG_MSVC )
	{
		bool usingCLR = false;
		bool usingWinRT = false;

		Array< AString > tokens;
		args.Tokenize( tokens );
		const AString * const end = tokens.End();
		for ( const AString * it = tokens.Begin(); it != end; ++it )
		{
			const AString & token = *it;

			if ( ( token == "/Zi" ) || ( token == "/ZI" ) )
			{
				flags |= ObjectNode::FLAG_USING_PDB;
			}
			else if ( token == "/clr" )
			{
				usingCLR = true;
				flags |= ObjectNode::FLAG_USING_CLR;
			}
			else if ( token == "/ZW" )
			{
				usingWinRT = true;
			}
			else if ( token.BeginsWith( "/Yu" ) )
			{
				flags |= ObjectNode::FLAG_USING_PCH;
			}
			else if ( token.BeginsWith( "/Yc" ) )
			{
				flags |= ObjectNode::FLAG_CREATING_PCH;
			}
		}

		// 1) clr code cannot be distributed due to a compiler bug where the preprocessed using
		// statements are truncated
		// 2) code consuming the windows runtime cannot be cached due to preprocessing weirdness
		// 3) pch files are machine specific
		if ( !usingWinRT && !usingCLR && !( flags & ObjectNode::FLAG_CREATING_PCH ) )
		{
			if ( isDistributableCompiler )
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

	// Check Clang compiler options
	if ( flags & ObjectNode::FLAG_CLANG )
	{
		Array< AString > tokens;
		args.Tokenize( tokens );
		const AString * const end = tokens.End();
		for ( const AString * it = tokens.Begin(); it != end; ++it )
		{
			const AString & token = *it;
			if ( token == "-emit-pch" )
			{
				flags |= ObjectNode::FLAG_CREATING_PCH;
			}
			if ( token == "-x" )
			{
				++it;
				if ( it != tokens.End() )
				{
					if ( ( *it == "c++-header" ) || ( *it == "c-header" ) )
					{
						flags |= ObjectNode::FLAG_CREATING_PCH;
					}
				}
			}
			else if ( token == "-include-pch" )
			{
				flags |= ObjectNode::FLAG_USING_PCH;
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

// Save
//------------------------------------------------------------------------------
/*virtual*/ void ObjectNode::Save( IOStream & stream ) const
{
	NODE_SAVE( m_Name );
	NODE_SAVE_DEPS( m_StaticDependencies );
	NODE_SAVE_DEPS( m_DynamicDependencies );
	NODE_SAVE( m_Flags );
	NODE_SAVE( m_CompilerArgs );
	NODE_SAVE( m_CompilerArgsDeoptimized );
	NODE_SAVE( m_ObjExtensionOverride );
	NODE_SAVE_DEPS( m_CompilerForceUsing );
	NODE_SAVE( m_DeoptimizeWritableFiles );
	NODE_SAVE( m_DeoptimizeWritableFilesWithToken );
    NODE_SAVE_NODE( m_PCHNode )
	NODE_SAVE_NODE( m_PreprocessorNode );
    NODE_SAVE( m_PreprocessorArgs );
    NODE_SAVE( m_PreprocessorFlags );
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
		NODE_SAVE( m_CompilerArgsDeoptimized );
	}
	else
	{
		NODE_SAVE( m_CompilerArgs );
	}
}

// GetPDBName
//------------------------------------------------------------------------------
void ObjectNode::GetPDBName( AString & pdbName ) const
{
	ASSERT( IsUsingPDB() );
	pdbName = m_Name;
	pdbName += ".pdb";
}

// GetPriority
//------------------------------------------------------------------------------
/*virtual*/ Node::Priority ObjectNode::GetPriority() const
{
	return IsCreatingPCH() ? Node::PRIORITY_HIGH : Node::PRIORITY_NORMAL;
}

// GetObjExtension
//------------------------------------------------------------------------------
const char * ObjectNode::GetObjExtension() const
{
	if ( m_ObjExtensionOverride.IsEmpty() )
	{
		return ".obj";
	}
	return m_ObjExtensionOverride.Get();
}

// DumpOutput
//------------------------------------------------------------------------------
/*static*/ void ObjectNode::DumpOutput( Job * job, const char * data, uint32_t dataSize, const AString & name, bool treatAsWarnings )
{
	if ( ( data != nullptr ) && ( dataSize > 0 ) )
	{
		Array< AString > exclusions( 2, false );
		exclusions.Append( AString( "Note: including file:" ) );
		exclusions.Append( AString( "#line" ) );

		AStackString<> msg;
		msg.Format( "%s: %s\n", treatAsWarnings ? "WARNING" : "PROBLEM", name.Get() );

		AutoPtr< char > mem( (char *)Alloc( dataSize + msg.GetLength() ) );
		memcpy( mem.Get(), msg.Get(), msg.GetLength() );
		memcpy( mem.Get() + msg.GetLength(), data, dataSize );

		Node::DumpOutput( job, mem.Get(), dataSize + msg.GetLength(), &exclusions );
	}
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
	uint64_t a1, a2;
	a1 = Murmur3::Calc128( job->GetData(), job->GetDataSize(), a2 );
	uint64_t a = a1 ^ a2; // xor merge

	// hash the build "environment"
	// TODO:B Exclude preprocessor control defines (the preprocessed input has considered those already)
	uint32_t b = Murmur3::Calc32( m_CompilerArgs.Get(), m_CompilerArgs.GetLength() );

	// ToolChain hash
	uint64_t c = GetCompiler()->CastTo< CompilerNode >()->GetManifest().GetToolId();

	AStackString<> cacheName;
	FBuild::Get().GetCacheFileName( a, b, c, cacheName );
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

			FileStream objFile;
			if ( !objFile.Open( m_Name.Get(), FileStream::WRITE_ONLY ) )
			{
				cache->FreeMemory( cacheData, cacheDataSize );
				FLOG_ERROR( "Failed to open local file during cache retrieval '%s'", m_Name.Get() );
				return false;
			}

			if ( objFile.Write( data, dataSize ) != dataSize )
			{
				cache->FreeMemory( cacheData, cacheDataSize );
				FLOG_ERROR( "Failed to write to local file during cache retrieval '%s'", m_Name.Get() );
				return false;
			}

			cache->FreeMemory( cacheData, cacheDataSize );

			// Get current "system time" and convert to "file time"
			#if defined( __WINDOWS__ )
				SYSTEMTIME st;
				FILETIME ft;
				GetSystemTime( &st );
				if ( FALSE == SystemTimeToFileTime( &st, &ft ) )
				{
					FLOG_ERROR( "Failed to convert file time after cache hit '%s' (%u)", m_Name.Get(), Env::GetLastErr() );
					return false;
				}
				uint64_t fileTimeNow = ( (uint64_t)ft.dwLowDateTime | ( (uint64_t)ft.dwHighDateTime << 32 ) );
                const bool timeSetOK = objFile.SetLastWriteTime( fileTimeNow );
			#elif defined( __APPLE__ ) || defined( __LINUX__ )
                const bool timeSetOK = ( utimes( m_Name.Get(), nullptr ) == 0 );
			#endif
	
			// set the time on the local file
            if ( timeSetOK == false )
			{
				FLOG_ERROR( "Failed to set timestamp on file after cache hit '%s' (%u)", m_Name.Get(), Env::GetLastErr() );
				return false;
			}

			objFile.Close();

			FileIO::WorkAroundForWindowsFilePermissionProblem( m_Name );

			// the file time we set and local file system might have different 
			// granularity for timekeeping, so we need to update with the actual time written
			m_Stamp = FileIO::GetFileLastWriteTime( m_Name );

			FLOG_INFO( "Cache hit: %u ms '%s'\n", uint32_t( t.GetElapsedMS() ), cacheFileName.Get() );
			FLOG_BUILD( "Obj: %s <CACHE>\n", GetName().Get() );
			SetStatFlag( Node::STATS_CACHE_HIT );
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
		// open the compiled object which we want to store to the cache
		FileStream objFile;
		if ( objFile.Open( m_Name.Get(), FileStream::READ_ONLY ) )
		{
			// read obj all into memory
			const size_t objFileSize = (size_t)objFile.GetFileSize();
			AutoPtr< char > mem( (char *)ALLOC( objFileSize ) );
			if ( objFile.Read( mem.Get(), objFileSize ) == objFileSize )
			{
				// try to compress
				Compressor c;
				c.Compress( mem.Get(), objFileSize );
				const void * data = c.GetResult();
				const size_t dataSize = c.GetResultSize();

				if ( cache->Publish( cacheFileName, data, dataSize ) )
				{
					// cache store complete
					FLOG_INFO( "Cache store: %u ms '%s'\n", uint32_t( t.GetElapsedMS() ), cacheFileName.Get() );
					SetStatFlag( Node::STATS_CACHE_STORE );
					return;
				}
			}
		}
	}

	FLOG_INFO( "Cache store fail: %u ms '%s'\n", uint32_t( t.GetElapsedMS() ), cacheFileName.Get() );
}

// EmitCompilationMessage
//------------------------------------------------------------------------------
void ObjectNode::EmitCompilationMessage( const AString & fullArgs, bool useDeoptimization, bool stealingRemoteJob, bool racingRemoteJob, bool useDedicatedPreprocessor ) const
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
	if ( FLog::ShowInfo() || FBuild::Get().GetOptions().m_ShowCommandLines )
	{
		output += useDedicatedPreprocessor ? GetDedicatedPreprocessor()->GetName() : GetCompiler()->GetName();
		output += ' ';
		output += fullArgs;
		output += '\n';
	}
	FLOG_BUILD( "%s", output.Get() );
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

// BuildFullArgs
//------------------------------------------------------------------------------
void ObjectNode::BuildFullArgs( const Job * job, AString & fullArgs, Pass pass, bool useDeoptimization ) const
{
	Array< AString > tokens( 1024, true );

	const bool useDedicatedPreprocessor = ( ( pass == PASS_PREPROCESSOR_ONLY ) && GetDedicatedPreprocessor() );
	if ( useDedicatedPreprocessor )
	{
		m_PreprocessorArgs.Tokenize( tokens );
	}
	else if ( useDeoptimization )
	{
		ASSERT( !m_CompilerArgsDeoptimized.IsEmpty() );
		m_CompilerArgsDeoptimized.Tokenize( tokens );
	}
	else
	{
		m_CompilerArgs.Tokenize( tokens );
	}
	fullArgs.Clear();

	const bool isMSVC 	= ( useDedicatedPreprocessor ) ? GetPreprocessorFlag( FLAG_MSVC ) : GetFlag( FLAG_MSVC );
	const bool isClang	= ( useDedicatedPreprocessor ) ? GetPreprocessorFlag( FLAG_CLANG ) : GetFlag( FLAG_CLANG );
	const bool isGCC	= ( useDedicatedPreprocessor ) ? GetPreprocessorFlag( FLAG_GCC ) : GetFlag( FLAG_GCC );
	const bool isSNC	= ( useDedicatedPreprocessor ) ? GetPreprocessorFlag( FLAG_SNC ) : GetFlag( FLAG_SNC );
	const bool isCWWii	= ( useDedicatedPreprocessor ) ? GetPreprocessorFlag( CODEWARRIOR_WII ) : GetFlag( CODEWARRIOR_WII );
	const bool isGHWiiU	= ( useDedicatedPreprocessor ) ? GetPreprocessorFlag( GREENHILLS_WIIU ) : GetFlag( GREENHILLS_WIIU );
	const bool isCUDA	= ( useDedicatedPreprocessor ) ? GetPreprocessorFlag( FLAG_CUDA_NVCC ) : GetFlag( FLAG_CUDA_NVCC );

	const size_t numTokens = tokens.GetSize();
	for ( size_t i = 0; i < numTokens; ++i )
	{
		// current token
		const AString & token = tokens[ i ];

		// -o removal for preprocessor
		if ( pass == PASS_PREPROCESSOR_ONLY )
		{
			if ( isGCC || isSNC || isClang || isCWWii || isGHWiiU || isCUDA )
			{
				if ( StripTokenWithArg( "-o", token, i ) )
				{
					continue;
				}

				if ( StripToken( "-c", token ) )
				{
					continue; // remove -c (compile) option when using preprocessor only
				}
			}
		}

		if ( isClang )
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
				if ( StripTokenWithArg( "/Yu", token, i ) )
				{
					continue; // skip this token in both cases 
				}
				if ( StripTokenWithArg( "/Fp", token, i ) )
				{
					continue; // skip this token in both cases 
				}

				// Remote compilation writes to a temp pdb
				if ( job->IsLocal() == false )
				{
					if ( StripTokenWithArg( "/Fd", token, i ) )
					{
						continue; // skip this token in both cases 
					}
				}
			}
		}

		if ( isMSVC )
		{
			// FASTBuild handles the multiprocessor scheduling
			if ( StripToken( "/MP", token, true ) ) // true = strip '/MP' and starts with '/MP'
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
			if ( isMSVC )
			{
				// NOTE: Leave /I includes for compatibility with Recode
				// (unlike Clang, MSVC is ok with leaving the /I when compiling preprocessed code)

				// Strip "Force Includes" statements (as they are merged in now)
				if ( StripTokenWithArg( "/FI", token, i ) )
				{
					continue; // skip this token in both cases
				}
			}
		}

		// %1 -> InputFile
		const char * found = token.Find( "%1" );
		if ( found )
		{
			fullArgs += AStackString<>( token.Get(), found );
			fullArgs += GetSourceFile()->GetName();
			fullArgs += AStackString<>( found + 2, token.GetEnd() );
			fullArgs += ' ';
			continue;
		}

		// %2 -> OutputFile
		found = token.Find( "%2" );
		if ( found )
		{
			fullArgs += AStackString<>( token.Get(), found );
			fullArgs += m_Name;
			fullArgs += AStackString<>( found + 2, token.GetEnd() );
			fullArgs += ' ';
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
				fullArgs += m_Name;
				fullArgs += GetObjExtension(); // convert 'PrecompiledHeader.pch' to 'PrecompiledHeader.pch.obj'
				fullArgs += AStackString<>( found + 2, token.GetEnd() );
				fullArgs += ' ';
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
				ExpandTokenList( m_CompilerForceUsing, fullArgs, pre, post );
				fullArgs += ' ';
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
            if ( token.BeginsWith( "/I\"" ) && token.EndsWith( "\\\"" ) )
            {
				fullArgs.Append( token.Get(), token.GetLength() - 2 );
				fullArgs += '"';
		        fullArgs += ' ';
                continue;
            }
        }
				
		// untouched token
		fullArgs += token;
		fullArgs += ' ';
	}

	if ( pass == PASS_PREPROCESSOR_ONLY )
	{
		if ( isMSVC )
		{
			fullArgs += "/E"; // run pre-processor only
		}
		else
		{
			ASSERT( isGCC || isSNC || isClang || isCWWii || isGHWiiU || isCUDA );
			fullArgs += "-E"; // run pre-processor only
		}
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
}

// ExpandTokenList
//------------------------------------------------------------------------------
void ObjectNode::ExpandTokenList( const Dependencies & nodes, AString & fullArgs, const AString & pre, const AString & post ) const
{
	const Dependency * const end = nodes.End();
	for ( const Dependency * it = nodes.Begin(); it != end; ++it )
	{
		Node * n = it->GetNode();

		fullArgs += pre;
		fullArgs += n->GetName();
		fullArgs += post;
		fullArgs += ' ';
	}
}

// BuildPreprocessedOutput
//------------------------------------------------------------------------------
bool ObjectNode::BuildPreprocessedOutput( const AString & fullArgs, Job * job, bool useDeoptimization ) const
{
	const bool useDedicatedPreprocessor = ( GetDedicatedPreprocessor() != nullptr );
	EmitCompilationMessage( fullArgs, useDeoptimization, false, false, useDedicatedPreprocessor );

	// spawn the process
	CompileHelper ch( false ); // don't handle output (we'll do that)
    #if defined( __WINDOWS__ )
        const bool useResponseFile = GetFlag( FLAG_MSVC ) || GetFlag( FLAG_GCC ) || GetFlag( FLAG_SNC ) || GetFlag( FLAG_CLANG ) || GetFlag( CODEWARRIOR_WII ) || GetFlag( GREENHILLS_WIIU );
    #else
        const bool useResponseFile = false;
    #endif

	if ( !ch.SpawnCompiler( job, GetName(),
        useDedicatedPreprocessor ? GetDedicatedPreprocessor()->GetName() : GetCompiler()->GetName(),
        fullArgs, useResponseFile ) )
	{
		// only output errors in failure case
		// (as preprocessed output goes to stdout, normal logging is pushed to
		// stderr, and we don't want to see that unless there is a problem)
		if ( ch.GetResult() != 0 )
		{
			DumpOutput( job, ch.GetErr().Get(), ch.GetErrSize(), GetName() );
		}

		return false; // SpawnCompiler will have emitted error
	}

	// take a copy of the output because ReadAllData uses huge buffers to avoid re-sizing
	char * memCopy = (char *)ALLOC( ch.GetOutSize() + 1 ); // null terminator for include parser
	memcpy( memCopy, ch.GetOut().Get(), ch.GetOutSize() );
	memCopy[ ch.GetOutSize() ] = 0; // null terminator for include parser

	job->OwnData( memCopy, ch.GetOutSize() );

	return true;
}

// WriteTmpFile
//------------------------------------------------------------------------------
bool ObjectNode::WriteTmpFile( Job * job, AString & tmpFileName ) const
{
	ASSERT( job->GetData() && job->GetDataSize() );

	Node * sourceFile = GetSourceFile();

	FileStream tmpFile;
	AStackString<> fileName( sourceFile->GetName().FindLast( NATIVE_SLASH ) + 1 );

	// TODO:B This code doesn't seem to do anything.  Looks like the extension
    // logic was broken, but GCC support is still working, so perhaps this
	// can be removed
	if ( GetFlag( FLAG_GCC ) || GetFlag( CODEWARRIOR_WII ) || GetFlag( GREENHILLS_WIIU ) )
	{
		// GCC requires preprocessed output to be named a certain way
		// SNC & MSVC like the extension left alone
		// C code (.c) -> .i (NOTE: lower case only)
		// C++ code (.C, .cc, .cp, .cpp, .cxx) -> .ii
		const char * tmpFileExt = fileName.FindLast( '.' ); 
		tmpFileExt = tmpFileExt ? ( tmpFileExt + 1 ) : fileName.Get();
		tmpFileExt = ( strcmp( tmpFileExt, "c" ) == 0 ) ? "i" : "ii";
	}

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

	WorkerThread::CreateTempFilePath( fileName.Get(), tmpFileName );
	if ( WorkerThread::CreateTempFile( tmpFileName, tmpFile ) == false ) 
	{
		job->Error( "Failed to create temp file '%s' to build '%s' (error %u)", tmpFileName.Get(), GetName().Get(), Env::GetLastErr );
		job->OnSystemError();
		return NODE_RESULT_FAILED;
	}
	if ( tmpFile.Write( dataToWrite, dataToWriteSize ) != dataToWriteSize )
	{
		job->Error( "Failed to write to temp file '%s' to build '%s' (error %u)", tmpFileName.Get(), GetName().Get(), Env::GetLastErr );
		job->OnSystemError();
		return NODE_RESULT_FAILED;
	}
	tmpFile.Close();

	FileIO::WorkAroundForWindowsFilePermissionProblem( tmpFileName );

	return true;
}

// BuildFinalOutput
//------------------------------------------------------------------------------
bool ObjectNode::BuildFinalOutput( Job * job, const AString & fullArgs ) const
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
	CompileHelper ch;
    #if defined( __WINDOWS__ )
        const bool useResponseFile = GetFlag( FLAG_MSVC ) || GetFlag( FLAG_GCC ) || GetFlag( FLAG_SNC ) || GetFlag( FLAG_CLANG ) || GetFlag( CODEWARRIOR_WII ) || GetFlag( GREENHILLS_WIIU );
    #else
        const bool useResponseFile = false;
    #endif
	if ( !ch.SpawnCompiler( job, GetName(), compiler, fullArgs, useResponseFile, workingDir.IsEmpty() ? nullptr : workingDir.Get() ) )
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

	return true;
}

// CompileHelper::CONSTRUCTOR
//------------------------------------------------------------------------------
ObjectNode::CompileHelper::CompileHelper( bool handleOutput )
	: m_HandleOutput( handleOutput )
	, m_OutSize( 0 )
	, m_ErrSize( 0 )
	, m_Result( 0 )
{
}

// CompileHelper::DESTRUCTOR
//------------------------------------------------------------------------------
ObjectNode::CompileHelper::~CompileHelper()
{
}

// CompilHelper::SpawnCompiler
//------------------------------------------------------------------------------
bool ObjectNode::CompileHelper::SpawnCompiler( Job * job,
											   const AString & name, 
											   const AString & compiler,
											   const AString & fullArgs,
											   bool useResponseFile,
											   const char * workingDir )
{
	// using response file?
	ResponseFile rf;
	AStackString<> responseFileArgs;
	if ( useResponseFile )
	{
		// write args to response file
		if ( !rf.Create( fullArgs ) )
		{
			return false; // Create will have emitted error
		}

		// override args to use response file
		responseFileArgs.Format( "@\"%s\"", rf.GetResponseFilePath().Get() );
	}
	
	const char * environmentString = ( FBuild::IsValid() ? FBuild::Get().GetEnvironmentString() : nullptr );
	if ( ( job->IsLocal() == false ) && ( job->GetToolManifest() ) )
	{
		environmentString = job->GetToolManifest()->GetRemoteEnvironmentString();
	}

	// spawn the process
	if ( false == m_Process.Spawn( compiler.Get(), 
								   useResponseFile ? responseFileArgs.Get() : fullArgs.Get(),
								   workingDir,
								   environmentString ) )
	{
		job->Error( "Failed to spawn process (error 0x%x) to build '%s'\n", Env::GetLastErr(), name.Get() );
		job->OnSystemError();
		return false;
	}

	// capture all of the stdout and stderr
	m_Process.ReadAllData( m_Out, &m_OutSize, m_Err, &m_ErrSize );

	// Get result
	ASSERT( !m_Process.IsRunning() );
	m_Result = m_Process.WaitForExit();

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
		if ( result == DBG_TERMINATE_PROCESS )
		{
			job->OnSystemError(); // task will be retried on another worker
			return;
		}

		// If DLLs are not correctly sync'd, add an extra message to help the user
		if ( result == 0xC000007B ) // STATUS_INVALID_IMAGE_FORMAT
		{
			job->Error( "Remote failure: STATUS_INVALID_IMAGE_FORMAT (0xC000007B) - Check Compiler() settings!\n" );
			return;
		}

		const ObjectNode * objectNode = job->GetNode()->CastTo< ObjectNode >();

		// MSVC errors
		if ( objectNode->IsMSVC() )
		{
			// When out of space, MSVC returns this
			if ( result == ERROR_FILE_NOT_FOUND ) // 2 (0x2)
			{
				// But we need to determine if it's actually an out of space
				// (rather than some compile error with missing file(s))
				// These error code have been observed in the wild
				if ( stdOut )
				{
					if ( strstr( stdOut, "C1082" ) ||
						 strstr( stdOut, "C1088" ) )
					{
						job->OnSystemError();
						return;
					}
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
		}

		// Clang
		if ( objectNode->GetFlag( ObjectNode::FLAG_CLANG ) )
		{
			// When clang fails due to low disk space
			if ( result == 0x01 )
			{
				// TODO:C Should we check for localized msg?
				if ( stdErr && ( strcmp( stdErr, "IO failure on output stream" ) ) )
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
				if ( stdErr && ( strcmp( stdErr, "No space left on device" ) ) )
				{
					job->OnSystemError();
					return;
				}
			}
		}
	#else
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
					( FBuild::Get().GetOptions().m_UseCacheRead ||
					 FBuild::Get().GetOptions().m_UseCacheWrite );
	if ( GetFlag( FLAG_ISOLATED_FROM_UNITY ) )
	{
		// disable caching for locally modified files (since they will rarely if ever get a hit)
		useCache = false;
	}
	return useCache;
}

//------------------------------------------------------------------------------
