// LibraryNode.cpp
//------------------------------------------------------------------------------

// Includes
//------------------------------------------------------------------------------
#include "Tools/FBuild/FBuildCore/PrecompiledHeader.h"

#include "LibraryNode.h"
#include "DirectoryListNode.h"
#include "UnityNode.h"

#include "Tools/FBuild/FBuildCore/FBuild.h"
#include "Tools/FBuild/FBuildCore/FLog.h"
#include "Tools/FBuild/FBuildCore/Graph/CompilerNode.h"
#include "Tools/FBuild/FBuildCore/Graph/NodeGraph.h"
#include "Tools/FBuild/FBuildCore/Graph/ObjectListNode.h"
#include "Tools/FBuild/FBuildCore/Graph/ObjectNode.h"
#include "Tools/FBuild/FBuildCore/Helpers/ResponseFile.h"

// Core
#include "Core/FileIO/FileIO.h"
#include "Core/FileIO/FileStream.h"
#include "Core/FileIO/PathUtils.h"
#include "Core/Process/Process.h"
#include "Core/Strings/AStackString.h"

// CONSTRUCTOR
//------------------------------------------------------------------------------
LibraryNode::LibraryNode( const AString & libraryName,
						  const Dependencies & inputNodes,
						  CompilerNode * compiler,
						  const AString & compilerArgs,
						  const AString & compilerArgsDeoptimized,
						  const AString & compilerOutputPath,
						  const AString & librarian,
						  const AString & librarianArgs,
						  uint32_t flags,
						  ObjectNode * precompiledHeader,
						  const Dependencies & compilerForceUsing,
						  const Dependencies & preBuildDependencies,
						  const Dependencies & additionalInputs,
						  bool deoptimizeWritableFiles,
						  bool deoptimizeWritableFilesWithToken,
                          CompilerNode * preprocessor,
                          const AString &preprocessorArgs )
: FileNode( libraryName, Node::FLAG_NONE )
, m_CompilerForceUsing( compilerForceUsing )
, m_AdditionalInputs( additionalInputs )
, m_DeoptimizeWritableFiles( deoptimizeWritableFiles )
, m_DeoptimizeWritableFilesWithToken( deoptimizeWritableFilesWithToken )
{
	m_Type = LIBRARY_NODE;
	m_LastBuildTimeMs = 1000; // higher default than a file node

	// depend on the input nodes
	m_StaticDependencies = inputNodes;

	// store precompiled headers if provided
	m_PrecompiledHeader = precompiledHeader;

	// store options we'll need to use dynamically
	m_Compiler = compiler;
	m_CompilerArgs = compilerArgs;
	m_CompilerArgsDeoptimized = compilerArgsDeoptimized;
	m_CompilerOutputPath = compilerOutputPath;

    m_Preprocessor     = preprocessor;
    m_PreprocessorArgs = preprocessorArgs;

	m_LibrarianPath = librarian; // TODO:C This should be a node
	m_LibrarianArgs = librarianArgs;
	m_Flags = flags;

	m_PreBuildDependencies = preBuildDependencies;
}

// DESTRUCTOR
//------------------------------------------------------------------------------
LibraryNode::~LibraryNode()
{
}

// DoDynamicDependencies
//------------------------------------------------------------------------------
/*virtual*/ bool LibraryNode::DoDynamicDependencies( bool forceClean )
{
	(void)forceClean; // dynamic deps are always re-added here, so this is meaningless

	// clear dynamic deps from previous passes
	m_DynamicDependencies.Clear();

	//Timer t;

	Node * pchCPP = nullptr;
	if ( m_PrecompiledHeader )
	{
		ASSERT( m_PrecompiledHeader->GetType() == Node::OBJECT_NODE );
		pchCPP = m_PrecompiledHeader->GetPrecompiledHeaderCPPFile();
	}

	NodeGraph & ng = FBuild::Get().GetDependencyGraph();

	// if we depend on any directory lists, we need to use them to get our files
	for ( Dependencies::Iter i = m_StaticDependencies.Begin();
		  i != m_StaticDependencies.End();
		  i++ )
	{
		// is this a dir list?
		if ( i->GetNode()->GetType() == Node::DIRECTORY_LIST_NODE )
		{
			// get the list of files
			DirectoryListNode * dln = i->GetNode()->CastTo< DirectoryListNode >();
			const Array< FileIO::FileInfo > & files = dln->GetFiles();
			m_DynamicDependencies.SetCapacity( m_DynamicDependencies.GetSize() + files.GetSize() );
			for ( Array< FileIO::FileInfo >::Iter fIt = files.Begin();
					fIt != files.End();
					fIt++ )
			{
				// Create the file node (or find an existing one)
				Node * n = ng.FindNode( fIt->m_Name );
				if ( n == nullptr )
				{
					n = ng.CreateFileNode( fIt->m_Name );
				}
				else if ( n->IsAFile() == false )
				{
					FLOG_ERROR( "Library() .CompilerInputFile '%s' is not a FileNode (type: %s)", n->GetName().Get(), n->GetTypeName() );
					return false;
				}

				// ignore the precompiled header as a convenience for the user
				// so they don't have to exclude it explicitly
				if ( pchCPP && ( n == pchCPP ) )
				{
					continue;
				}

				// create the object that will compile the above file
				if ( CreateDynamicObjectNode( n, dln ) == false )
				{
					return false; // CreateDynamicObjectNode will have emitted error
				}
			}
		}
		else if ( i->GetNode()->GetType() == Node::FILE_NODE )
		{
			// a single file, create the object that will compile it
			if ( CreateDynamicObjectNode( i->GetNode(), nullptr ) == false )
			{
				return false; // CreateDynamicObjectNode will have emitted error
			}
		}
		else if ( i->GetNode()->GetType() == Node::UNITY_NODE )
		{
			// get the dir list from the unity node
			UnityNode * un = i->GetNode()->CastTo< UnityNode >();

			// unity files
			const Array< AString > & unityFiles = un->GetUnityFileNames();
			for ( Array< AString >::Iter it = unityFiles.Begin();
				  it != unityFiles.End();
				  it++ )
			{
				Node * n = ng.FindNode( *it );
				if ( n == nullptr )
				{
					n = ng.CreateFileNode( *it );
				}
				else if ( n->IsAFile() == false )
				{
					FLOG_ERROR( "Library() .CompilerInputUnity '%s' is not a FileNode (type: %s)", n->GetName().Get(), n->GetTypeName() );
					return false;
				}

				// create the object that will compile the above file
				if ( CreateDynamicObjectNode( n, nullptr, true ) == false )
				{
					return false; // CreateDynamicObjectNode will have emitted error
				}
			}
			
			// files from unity to build individually
			const Array< UnityNode::FileAndOrigin > & isolatedFiles = un->GetIsolatedFileNames();
			for ( Array< UnityNode::FileAndOrigin >::Iter it = isolatedFiles.Begin();
				  it != isolatedFiles.End();
				  it++ )
			{
				Node * n = ng.FindNode( it->GetName() );
				if ( n == nullptr )
				{
					n = ng.CreateFileNode( it->GetName() );
				}
				else if ( n->IsAFile() == false )
				{
					FLOG_ERROR( "Library() Isolated '%s' is not a FileNode (type: %s)", n->GetName().Get(), n->GetTypeName() );
					return false;
				}

				// create the object that will compile the above file
				if ( CreateDynamicObjectNode( n, it->GetDirListOrigin(), false, true ) == false )
				{
					return false; // CreateDynamicObjectNode will have emitted error
				}
			}
		}
		else
		{
			ASSERT( false ); // unexpected node type
		}
	}

	//float time = t.GetElapsed();
	//FLOG_WARN( "DynamicDeps: %2.3f\t%s", time, GetName().Get() );

	// If we have a precompiled header, add that to our dynamic deps so that
	// any symbols in the PCH's .obj are also linked, when either:
	// a) we are a static library
	// b) a DLL or executable links our .obj files
	if ( m_PrecompiledHeader )
	{
		m_DynamicDependencies.Append( Dependency( m_PrecompiledHeader ) );
	}

	// additional libs/objects
	m_DynamicDependencies.Append( m_AdditionalInputs );

	// make sure we have something to build!
	// (or we at least have some inputs)
	if ( m_DynamicDependencies.GetSize() == 0 )
	{
		FLOG_ERROR( "No files found to build '%s'", GetName().Get() );
		return false;
	}

	return true;
}

// DoBuild
//------------------------------------------------------------------------------
/*virtual*/ Node::BuildResult LibraryNode::DoBuild( Job * UNUSED( job ) )
{
	// delete library before creation (so ar.exe will not merge old symbols)
	if ( FileIO::FileExists( GetName().Get() ) )
	{
		FileIO::FileDelete( GetName().Get() );
	}

	// Format compiler args string
	AStackString< 4 * KILOBYTE > fullArgs;
	GetFullArgs( fullArgs );

	// use the exe launch dir as the working dir
	const char * workingDir = nullptr;

	const char * environment = FBuild::Get().GetEnvironmentString();

	EmitCompilationMessage( fullArgs );

	// use response file?
	ResponseFile rf;
	AStackString<> responseFileArgs;
    #if defined( __APPLE__ ) || defined( __LINUX__ )
        const bool useResponseFile = false; // OSX/Linux ar doesn't support response files
    #else
        const bool useResponseFile = GetFlag( LIB_FLAG_LIB ) || GetFlag( LIB_FLAG_AR ) || GetFlag( LIB_FLAG_ORBIS_AR ) || GetFlag( LIB_FLAG_GREENHILLS_AX );
    #endif
	if ( useResponseFile )
	{
		// orbis-ar.exe requires escaped slashes inside response file
		if ( GetFlag( LIB_FLAG_ORBIS_AR ) )
		{
			rf.SetEscapeSlashes();
		}

		// write args to response file
		if ( !rf.Create( fullArgs ) )
		{
			return NODE_RESULT_FAILED; // Create will have emitted error
		}

		// override args to use response file
		responseFileArgs.Format( "@\"%s\"", rf.GetResponseFilePath().Get() );
	}

	// spawn the process
	Process p;
	bool spawnOK = p.Spawn( m_LibrarianPath.Get(),
							useResponseFile ? responseFileArgs.Get() : fullArgs.Get(),
							workingDir,
							environment );

	if ( !spawnOK )
	{
		FLOG_ERROR( "Failed to spawn process for Library creation for '%s'", GetName().Get() );
		return NODE_RESULT_FAILED;
	}

	// capture all of the stdout and stderr
	AutoPtr< char > memOut;
	AutoPtr< char > memErr;
	uint32_t memOutSize = 0;
	uint32_t memErrSize = 0;
	p.ReadAllData( memOut, &memOutSize, memErr, &memErrSize );

	ASSERT( !p.IsRunning() );
	// Get result
	int result = p.WaitForExit();

	if ( result != 0 )
	{
		if ( memOut.Get() ) { FLOG_ERROR_DIRECT( memOut.Get() ); }
		if ( memErr.Get() ) { FLOG_ERROR_DIRECT( memErr.Get() ); }
	}

	// did the executable fail?
	if ( result != 0 )
	{
		FLOG_ERROR( "Failed to build Library (error %i) '%s'", result, GetName().Get() );
		return NODE_RESULT_FAILED;
	}

	// record time stamp for next time
	m_Stamp = FileIO::GetFileLastWriteTime( m_Name );
	ASSERT( m_Stamp );

	return NODE_RESULT_OK;
}

// GetFullArgs
//------------------------------------------------------------------------------
void LibraryNode::GetFullArgs( AString & fullArgs ) const
{
	Array< AString > tokens( 1024, true );
	m_LibrarianArgs.Tokenize( tokens );

	const AString * const end = tokens.End();
	for ( const AString * it = tokens.Begin(); it!=end; ++it )
	{
		const AString & token = *it;
		if ( token.EndsWith( "%1" ) )
		{
			// handle /Option:%1 -> /Option:A /Option:B /Option:C
			AStackString<> pre;
			if ( token.GetLength() > 2 )
			{
				pre.Assign( token.Get(), token.GetEnd() - 2 );
			}

			// concatenate files, unquoted
			GetInputFiles( fullArgs, pre, AString::GetEmpty() );
		}
		else if ( token.EndsWith( "\"%1\"" ) )
		{
			// handle /Option:"%1" -> /Option:"A" /Option:"B" /Option:"C"
			AStackString<> pre( token.Get(), token.GetEnd() - 3 ); // 3 instead of 4 to include quote
			AStackString<> post( "\"" );

			// concatenate files, quoted
			GetInputFiles( fullArgs, pre, post );
		}
		else if ( token.EndsWith( "%2" ) )
		{
			// handle /Option:%2 -> /Option:A
			if ( token.GetLength() > 2 )
			{
				fullArgs += AStackString<>( token.Get(), token.GetEnd() - 2 );
			}
			fullArgs += m_Name;
		}
		else if ( token.EndsWith( "\"%2\"" ) )
		{
			// handle /Option:"%2" -> /Option:"A"
			AStackString<> pre( token.Get(), token.GetEnd() - 3 ); // 3 instead of 4 to include quote
			fullArgs += pre;
			fullArgs += m_Name;
			fullArgs += '"'; // post
		}
		else
		{
			fullArgs += token;
		}

		fullArgs += ' ';
	}
}

// GetInputFiles
//------------------------------------------------------------------------------
void LibraryNode::GetInputFiles( AString & fullArgs, const AString & pre, const AString & post ) const
{
	for ( Dependencies::Iter i = m_DynamicDependencies.Begin();
		  i != m_DynamicDependencies.End();
		  i++ )
	{
		const Node * n = i->GetNode();

		// handle pch files - get path to object
		if ( n->GetType() == Node::OBJECT_NODE )
		{
			// handle pch files - get path to matching object
			const ObjectNode * on = n->CastTo< ObjectNode >();
			if ( on->IsCreatingPCH() )
			{
				if ( on->IsMSVC() )
				{
					fullArgs += pre;
					fullArgs += on->GetName();
					fullArgs += on->GetObjExtension();
					fullArgs += post;
					fullArgs += ' ';
					continue;
				}
				else
				{
					// Clang/GCC/SNC don't have an object to link for a pch
					continue; 
				}
			}
		}

		// extract objects from additional lists
		if ( n->GetType() == Node::OBJECT_LIST_NODE )
		{
			// insert all the objects in the object list
			ObjectListNode * oln = n->CastTo< ObjectListNode >();
			oln->GetInputFiles( fullArgs, pre, post );
			continue;
		}

		// normal object or library
		fullArgs += pre;
		fullArgs += n->GetName();
		fullArgs += post;
		fullArgs += ' ';
	}
}

// DetermineFlags
//------------------------------------------------------------------------------
/*static*/ uint32_t LibraryNode::DetermineFlags( const AString & librarianName )
{
	uint32_t flags = 0;
	if ( librarianName.EndsWithI("lib.exe") ||
		 librarianName.EndsWithI("lib") )
	{
		flags |= LIB_FLAG_LIB;
	}
	else if ( librarianName.EndsWithI("ar.exe") ||
		 librarianName.EndsWithI("ar") )
	{
		if ( librarianName.FindI( "orbis-ar" ) )
		{
			flags |= LIB_FLAG_ORBIS_AR;
		}
		else
		{
			flags |= LIB_FLAG_AR;
		}
	}
	else if ( librarianName.EndsWithI( "\\ax.exe" ) ||
			  librarianName.EndsWithI( "\\ax" ) )
	{
		flags |= LIB_FLAG_GREENHILLS_AX;
	}
	return flags;
}

// CreateDynamicObjectNode
//------------------------------------------------------------------------------
bool LibraryNode::CreateDynamicObjectNode( Node * inputFile, const DirectoryListNode * dirNode, bool isUnityNode, bool isIsolatedFromUnityNode )
{
	const AString & fileName = inputFile->GetName();

	// Transform src file to dst object path
	// get file name only (no path, no ext)
	const char * lastSlash = fileName.FindLast( NATIVE_SLASH );
	lastSlash = lastSlash ? ( lastSlash + 1 ) : fileName.Get();
	const char * lastDot = fileName.FindLast( '.' );
	lastDot = lastDot && ( lastDot > lastSlash ) ? lastDot : fileName.GetEnd();

    // if source comes from a directory listing, use path relative to dirlist base
    // to replicate the folder hierearchy in the output
    if ( dirNode )
    {
        const AString & baseDir = dirNode->GetPath();
        ASSERT( PathUtils::PathBeginsWith( fileName, baseDir ) );
        if ( PathUtils::PathBeginsWith( fileName, baseDir ) )
        {
            // ... use everything after that
            lastSlash = fileName.Get() + baseDir.GetLength();
        }
    }

	AStackString<> fileNameOnly( lastSlash, lastDot );
	AStackString<> objFile( m_CompilerOutputPath );
	objFile += fileNameOnly;
	objFile += GetObjExtension();

	// Create an ObjectNode to compile the above file
	// and depend on that
	NodeGraph & ng = FBuild::Get().GetDependencyGraph();
	Node * on = ng.FindNode( objFile );
	if ( on == nullptr )
	{
		// determine flags - TODO:B Move DetermineFlags call out of build-time
		uint32_t flags = ObjectNode::DetermineFlags( m_Compiler, m_CompilerArgs );
		if ( isUnityNode )
		{
			flags |= ObjectNode::FLAG_UNITY;
		}
		if ( isIsolatedFromUnityNode )
		{
			flags |= ObjectNode::FLAG_ISOLATED_FROM_UNITY;
		}
		uint32_t preprocessorFlags = 0;
		if ( m_Preprocessor )
		{
			// determine flags - TODO:B Move DetermineFlags call out of build-time
			preprocessorFlags = ObjectNode::DetermineFlags( m_Preprocessor, m_PreprocessorArgs );
        }

		on = ng.CreateObjectNode( objFile, inputFile, m_Compiler, m_CompilerArgs, m_CompilerArgsDeoptimized, m_PrecompiledHeader, flags, m_CompilerForceUsing, m_DeoptimizeWritableFiles, m_DeoptimizeWritableFilesWithToken, m_Preprocessor, m_PreprocessorArgs, preprocessorFlags );
	}
	else if ( on->GetType() != Node::OBJECT_NODE )
	{
		FLOG_ERROR( "Node '%s' is not an ObjectNode (type: %s)", on->GetName().Get(), on->GetTypeName() );
		return false;
	}
	else 
	{
		ObjectNode * other = on->CastTo< ObjectNode >();
		if ( inputFile != other->GetSourceFile() )
		{
			FLOG_ERROR( "Conflicting objects found:\n"
						" File A: %s\n"
						" File B: %s\n"
						" Both compile to: %s\n", 
						inputFile->GetName().Get(),
						other->GetSourceFile()->GetName().Get(),
						objFile.Get() );
			return false;
		}
	}
	m_DynamicDependencies.Append( Dependency( on ) );
	return true;
}

// EmitCompilationMessage
//------------------------------------------------------------------------------
void LibraryNode::EmitCompilationMessage( const AString & fullArgs ) const
{
	AStackString<> output;
	output += "Lib: ";
	output += GetName();
	output += '\n';
	if ( FLog::ShowInfo() || FBuild::Get().GetOptions().m_ShowCommandLines )
	{
		output += m_LibrarianPath;
		output += ' ';
		output += fullArgs;
		output += '\n';
	}
	FLOG_BUILD( "%s", output.Get() );
}

// Load
//------------------------------------------------------------------------------
/*static*/ Node * LibraryNode::Load( IOStream & stream )
{
	NODE_LOAD( AStackString<>,	name );
	NODE_LOAD_NODE( CompilerNode,	compilerNode );
	NODE_LOAD( AStackString<>,	compilerArgs );
	NODE_LOAD( AStackString<>,	compilerArgsDeoptimized );
	NODE_LOAD( AStackString<>,	compilerOutputPath );
	NODE_LOAD( AStackString<>,	librarianPath );
	NODE_LOAD( AStackString<>,	librarianArgs );
	NODE_LOAD( uint32_t,		flags );
	NODE_LOAD_DEPS( 16,			staticDeps );
	NODE_LOAD_NODE( Node,		precompiledHeader );
	NODE_LOAD( AStackString<>,	objExtensionOverride );
	NODE_LOAD_DEPS( 0,			compilerForceUsing );
	NODE_LOAD_DEPS( 0,			preBuildDependencies );
	NODE_LOAD_DEPS( 0,			additionalInputs );
	NODE_LOAD( bool,			deoptimizeWritableFiles );
	NODE_LOAD( bool,			deoptimizeWritableFilesWithToken );
	NODE_LOAD_NODE( CompilerNode, preprocessorNode );
	NODE_LOAD( AStackString<>,	preprocessorArgs );

	NodeGraph & ng = FBuild::Get().GetDependencyGraph();
	LibraryNode * n = ng.CreateLibraryNode( name, 
								 staticDeps, 
								 compilerNode, 
								 compilerArgs,
								 compilerArgsDeoptimized,
								 compilerOutputPath, 
								 librarianPath, 
								 librarianArgs,
								 flags,
								 precompiledHeader ? precompiledHeader->CastTo< ObjectNode >() : nullptr,
								 compilerForceUsing,
								 preBuildDependencies,
								 additionalInputs,
								 deoptimizeWritableFiles,
								 deoptimizeWritableFilesWithToken,
								 preprocessorNode,
								 preprocessorArgs );
	n->m_ObjExtensionOverride = objExtensionOverride;

	// TODO:B Need to save the dynamic deps, for better progress estimates
	// but we can't right now because we rely on the nodes we depend on 
	// being saved before us which isn't the case for dynamic deps.
	//if ( Node::LoadDepArray( fileStream, n->m_DynamicDependencies ) == false )
	//{
	//	FDELETE n;
	//	return nullptr;
	//}
	return n;
}

// Save
//------------------------------------------------------------------------------
/*virtual*/ void LibraryNode::Save( IOStream & stream ) const
{
	NODE_SAVE( m_Name );
	NODE_SAVE_NODE( m_Compiler );
	NODE_SAVE( m_CompilerArgs );
	NODE_SAVE( m_CompilerArgsDeoptimized );
	NODE_SAVE( m_CompilerOutputPath );
	NODE_SAVE( m_LibrarianPath );
	NODE_SAVE( m_LibrarianArgs );
	NODE_SAVE( m_Flags );
	NODE_SAVE_DEPS( m_StaticDependencies );
	NODE_SAVE_NODE( m_PrecompiledHeader );
	NODE_SAVE( m_ObjExtensionOverride );
	NODE_SAVE_DEPS( m_CompilerForceUsing );
	NODE_SAVE_DEPS( m_PreBuildDependencies );
	NODE_SAVE_DEPS( m_AdditionalInputs );
	NODE_SAVE( m_DeoptimizeWritableFiles );
	NODE_SAVE( m_DeoptimizeWritableFilesWithToken );
	NODE_SAVE_NODE( m_Preprocessor );
	NODE_SAVE( m_PreprocessorArgs );

	// TODO:B Need to save the dynamic deps, for better progress estimates
	// but we can't right now because we rely on the nodes we depend on 
	// being saved before us which isn't the case for dynamic deps.
	// dynamic deps are saved for more accurate progress estimates in future builds
	//Node::SaveDepArray( fileStream, m_DynamicDependencies );
}

// GetObjExtension
//------------------------------------------------------------------------------
const char * LibraryNode::GetObjExtension() const
{
	if ( m_ObjExtensionOverride.IsEmpty() )
	{
		return ".obj";
	}
	return m_ObjExtensionOverride.Get();
}

//------------------------------------------------------------------------------
