// ObjectListNode.cpp
//------------------------------------------------------------------------------

// Includes
//------------------------------------------------------------------------------
#include "Tools/FBuild/FBuildCore/PrecompiledHeader.h"

#include "ObjectListNode.h"

#include "Tools/FBuild/FBuildCore/FBuild.h"
#include "Tools/FBuild/FBuildCore/FLog.h"
#include "Tools/FBuild/FBuildCore/Graph/CompilerNode.h"
#include "Tools/FBuild/FBuildCore/Graph/DirectoryListNode.h"
#include "Tools/FBuild/FBuildCore/Graph/NodeGraph.h"
#include "Tools/FBuild/FBuildCore/Graph/ObjectNode.h"
#include "Tools/FBuild/FBuildCore/Graph/UnityNode.h"
#include "Tools/FBuild/FBuildCore/Helpers/Args.h"
#include "Tools/FBuild/FBuildCore/BFF/BFFVariable.h"

// Core
#include "Core/FileIO/IOStream.h"
#include "Core/FileIO/PathUtils.h"
#include "Core/Strings/AStackString.h"

// CONSTRUCTOR
//------------------------------------------------------------------------------
ObjectListNode::ObjectListNode( const AString & listName,
						  const Dependencies & inputNodes,
						  CompilerNode * compiler,
						  const AString & compilerArgs,
						  const AString & compilerArgsDeoptimized,
						  const AString & compilerOutputPath,
						  ObjectNode * precompiledHeader,
						  const Dependencies & compilerForceUsing,
						  const Dependencies & preBuildDependencies,
						  bool deoptimizeWritableFiles,
						  bool deoptimizeWritableFilesWithToken,
						  bool allowDistribution,
						  bool allowCaching,
                          CompilerNode * preprocessor,
                          const AString & preprocessorArgs,
						  const AString & baseDirectory )
: Node( listName, Node::OBJECT_LIST_NODE, Node::FLAG_NONE )
, m_CompilerForceUsing( compilerForceUsing )
, m_DeoptimizeWritableFiles( deoptimizeWritableFiles )
, m_DeoptimizeWritableFilesWithToken( deoptimizeWritableFilesWithToken )
, m_AllowDistribution(allowDistribution)
, m_AllowCaching(allowCaching)
{
	m_LastBuildTimeMs = 10000; // TODO:C Reduce this when dynamic deps are saved

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

	m_PreBuildDependencies = preBuildDependencies;

	m_BaseDirectory = baseDirectory;
}

// DESTRUCTOR
//------------------------------------------------------------------------------
ObjectListNode::~ObjectListNode()
{
}

// IsAFile
//------------------------------------------------------------------------------
/*virtual*/ bool ObjectListNode::IsAFile() const
{
	return false;
}

// GatherDynamicDependencies
//------------------------------------------------------------------------------
/*virtual*/ bool ObjectListNode::GatherDynamicDependencies( NodeGraph & nodeGraph, bool forceClean )
{
	(void)forceClean; // dynamic deps are always re-added here, so this is meaningless

	// clear dynamic deps from previous passes
	m_DynamicDependencies.Clear();

	//Timer t;

    #if defined( __WINDOWS__ )
		// On Windows, with MSVC we compile a cpp file to generate the PCH
		// Filter here to ensure that doesn't get compiled twice
        Node * pchCPP = nullptr;
        if ( m_PrecompiledHeader && m_PrecompiledHeader->IsMSVC() )
        {
            pchCPP = m_PrecompiledHeader->GetPrecompiledHeaderCPPFile();
        }
    #endif

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
				Node * n = nodeGraph.FindNode( fIt->m_Name );
				if ( n == nullptr )
				{
					n = nodeGraph.CreateFileNode( fIt->m_Name );
				}
				else if ( n->IsAFile() == false )
				{
					FLOG_ERROR( "Library() .CompilerInputFile '%s' is not a FileNode (type: %s)", n->GetName().Get(), n->GetTypeName() );
					return false;
				}

				// ignore the precompiled header as a convenience for the user
				// so they don't have to exclude it explicitly
                #if defined( __WINDOWS__ )
                    if ( n == pchCPP )
                    {
                        continue;
                    }
                #endif

				// create the object that will compile the above file
				if ( CreateDynamicObjectNode( nodeGraph, n, dln->GetPath() ) == false )
				{
					return false; // CreateDynamicObjectNode will have emitted error
				}
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
				Node * n = nodeGraph.FindNode( *it );
				if ( n == nullptr )
				{
					n = nodeGraph.CreateFileNode( *it );
				}
				else if ( n->IsAFile() == false )
				{
					FLOG_ERROR( "Library() .CompilerInputUnity '%s' is not a FileNode (type: %s)", n->GetName().Get(), n->GetTypeName() );
					return false;
				}

				// create the object that will compile the above file
				if ( CreateDynamicObjectNode( nodeGraph, n, AString::GetEmpty(), true ) == false )
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
				Node * n = nodeGraph.FindNode( it->GetName() );
				if ( n == nullptr )
				{
					n = nodeGraph.CreateFileNode( it->GetName() );
				}
				else if ( n->IsAFile() == false )
				{
					FLOG_ERROR( "Library() Isolated '%s' is not a FileNode (type: %s)", n->GetName().Get(), n->GetTypeName() );
					return false;
				}

				// create the object that will compile the above file
				if ( CreateDynamicObjectNode( nodeGraph, n, it->GetDirListOrigin()->GetPath(), false, true ) == false )
				{
					return false; // CreateDynamicObjectNode will have emitted error
				}
			}
		}
        else if ( i->GetNode()->IsAFile() )
		{
			// a single file, create the object that will compile it
			if ( CreateDynamicObjectNode( nodeGraph, i->GetNode(), AString::GetEmpty() ) == false )
			{
				return false; // CreateDynamicObjectNode will have emitted error
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

    return true;
}

// DoDynamicDependencies
//------------------------------------------------------------------------------
/*virtual*/ bool ObjectListNode::DoDynamicDependencies( NodeGraph & nodeGraph, bool forceClean )
{
    if ( GatherDynamicDependencies( nodeGraph, forceClean ) == false )
    {
        return false; // GatherDynamicDependencies will have emitted error
    }

	// make sure we have something to build!
	if ( m_DynamicDependencies.GetSize() == 0 )
	{
		FLOG_ERROR( "No files found to build '%s'", GetName().Get() );
		return false;
	}

	if ( m_ExtraASMPath.IsEmpty() == false )
	{
		if ( !FileIO::EnsurePathExists( m_ExtraASMPath ) )
		{
			FLOG_ERROR( "Failed to create folder for .asm file '%s'", m_ExtraASMPath.Get() );
			return false;
		}
	}

	if ( m_ExtraPDBPath.IsEmpty() == false )
	{
		if ( !FileIO::EnsurePathExists( m_ExtraPDBPath ) )
		{
			FLOG_ERROR( "Failed to create folder for .pdb file '%s'", m_ExtraPDBPath.Get() );
			return false;
		}
	}

	return true;
}

// DoBuild
//------------------------------------------------------------------------------
/*virtual*/ Node::BuildResult ObjectListNode::DoBuild( Job * UNUSED( job ) )
{
	// consider ourselves to be as recent as the newest file
	uint64_t timeStamp = 0;
	const Dependency * const end = m_DynamicDependencies.End();
	for ( const Dependency * it = m_DynamicDependencies.Begin(); it != end; ++it )
	{
		ObjectNode * on = it->GetNode()->CastTo< ObjectNode >();
		timeStamp = Math::Max< uint64_t >( timeStamp, on->GetStamp() );
	}
	m_Stamp = timeStamp;

	return NODE_RESULT_OK;
}

// GetInputFiles
//------------------------------------------------------------------------------
void ObjectListNode::GetInputFiles( Args & fullArgs, const AString & pre, const AString & post ) const
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
					fullArgs.AddDelimiter();
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
            ASSERT( GetType() == Node::LIBRARY_NODE ); // should only be possible for a LibraryNode

			// insert all the objects in the object list
			ObjectListNode * oln = n->CastTo< ObjectListNode >();
			oln->GetInputFiles( fullArgs, pre, post );
			continue;
		}

		// normal object
		fullArgs += pre;
		fullArgs += n->GetName();
		fullArgs += post;
		fullArgs.AddDelimiter();
	}
}

// GetInputFiles
//------------------------------------------------------------------------------
void ObjectListNode::GetInputFiles( Array< AString > & files ) const
{
    // only valid to call on ObjectListNode (not LibraryNode)
    ASSERT( GetType() == Node::OBJECT_LIST_NODE );

    files.SetCapacity( files.GetCapacity() + m_DynamicDependencies.GetSize() );
    for ( Dependencies::Iter i = m_DynamicDependencies.Begin();
		  i != m_DynamicDependencies.End();
		  i++ )
	{
		const Node * n = i->GetNode();
        files.Append( n->GetName() );
	}
}

// CreateDynamicObjectNode
//------------------------------------------------------------------------------
bool ObjectListNode::CreateDynamicObjectNode( NodeGraph & nodeGraph, Node * inputFile, const AString & baseDir, bool isUnityNode, bool isIsolatedFromUnityNode )
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
    AStackString<> subPath;
    if ( baseDir.IsEmpty() == false )
    {
		ASSERT( NodeGraph::IsCleanPath( baseDir ) );
		if ( PathUtils::PathBeginsWith( fileName, baseDir ) )
        {
            // ... use everything after that
			subPath.Assign( fileName.Get() + baseDir.GetLength(), lastSlash ); // includes last slash
        }
	}
	else
	{
		if ( !m_BaseDirectory.IsEmpty() && PathUtils::PathBeginsWith( fileName, m_BaseDirectory ) )
		{
			// ... use everything after that
			subPath.Assign( fileName.Get() + m_BaseDirectory.GetLength(), lastSlash ); // includes last slash
		}
	}

	AStackString<> fileNameOnly( lastSlash, lastDot );
	AStackString<> objFile( m_CompilerOutputPath );
    objFile += subPath;
    objFile += m_CompilerOutputPrefix;
	objFile += fileNameOnly;
	objFile += GetObjExtension();

	// Create an ObjectNode to compile the above file
	// and depend on that
	Node * on = nodeGraph.FindNode( objFile );
	if ( on == nullptr )
	{
		// determine flags - TODO:B Move DetermineFlags call out of build-time
		const bool usingPCH = ( m_PrecompiledHeader != nullptr );
		uint32_t flags = ObjectNode::DetermineFlags( m_Compiler, m_CompilerArgs, false, usingPCH );
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
			preprocessorFlags = ObjectNode::DetermineFlags( m_Preprocessor, m_PreprocessorArgs, false, usingPCH );
        }

		on = nodeGraph.CreateObjectNode( objFile, inputFile, m_Compiler, m_CompilerArgs, m_CompilerArgsDeoptimized, m_PrecompiledHeader, flags, m_CompilerForceUsing, m_DeoptimizeWritableFiles, m_DeoptimizeWritableFilesWithToken, m_AllowDistribution, m_AllowCaching, m_Preprocessor, m_PreprocessorArgs, preprocessorFlags );
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

// Load
//------------------------------------------------------------------------------
/*static*/ Node * ObjectListNode::Load( NodeGraph & nodeGraph, IOStream & stream )
{
	NODE_LOAD( AStackString<>,	name );
	NODE_LOAD_NODE( CompilerNode,	compilerNode );
	NODE_LOAD( AStackString<>,	compilerArgs );
	NODE_LOAD( AStackString<>,	compilerArgsDeoptimized );
	NODE_LOAD( AStackString<>,	compilerOutputPath );
	NODE_LOAD_DEPS( 16,			staticDeps );
	NODE_LOAD_NODE( Node,		precompiledHeader );
	NODE_LOAD( AStackString<>,	objExtensionOverride );
    NODE_LOAD( AStackString<>,	compilerOutputPrefix );
	NODE_LOAD_DEPS( 0,			compilerForceUsing );
	NODE_LOAD_DEPS( 0,			preBuildDependencies );
	NODE_LOAD( bool,			deoptimizeWritableFiles );
	NODE_LOAD( bool,			deoptimizeWritableFilesWithToken );
	NODE_LOAD( bool,			allowDistribution );
	NODE_LOAD( bool,			allowCaching );
	NODE_LOAD_NODE( CompilerNode, preprocessorNode );
	NODE_LOAD( AStackString<>,	preprocessorArgs );
	NODE_LOAD( AStackString<>,  baseDirectory );
	NODE_LOAD( AStackString<>,	extraPDBPath );
	NODE_LOAD( AStackString<>,  extraASMPath );

	ObjectListNode * n = nodeGraph.CreateObjectListNode( name, 
								staticDeps, 
								compilerNode, 
								compilerArgs,
								compilerArgsDeoptimized,
								compilerOutputPath, 
								precompiledHeader ? precompiledHeader->CastTo< ObjectNode >() : nullptr,
								compilerForceUsing,
								preBuildDependencies,
								deoptimizeWritableFiles,
								deoptimizeWritableFilesWithToken,
								allowDistribution, 
								allowCaching,
								preprocessorNode,
								preprocessorArgs,
								baseDirectory );
	n->m_ObjExtensionOverride = objExtensionOverride;
    n->m_CompilerOutputPrefix = compilerOutputPrefix;
    n->m_ExtraPDBPath = extraPDBPath;
    n->m_ExtraASMPath = extraASMPath;

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
/*virtual*/ void ObjectListNode::Save( IOStream & stream ) const
{
	NODE_SAVE( m_Name );
	NODE_SAVE_NODE( m_Compiler );
	NODE_SAVE( m_CompilerArgs );
	NODE_SAVE( m_CompilerArgsDeoptimized );
	NODE_SAVE( m_CompilerOutputPath );
	NODE_SAVE_DEPS( m_StaticDependencies );
	NODE_SAVE_NODE( m_PrecompiledHeader );
	NODE_SAVE( m_ObjExtensionOverride );
    NODE_SAVE( m_CompilerOutputPrefix );
	NODE_SAVE_DEPS( m_CompilerForceUsing );
	NODE_SAVE_DEPS( m_PreBuildDependencies );
	NODE_SAVE( m_DeoptimizeWritableFiles );
	NODE_SAVE( m_DeoptimizeWritableFilesWithToken );
	NODE_SAVE( m_AllowDistribution );
	NODE_SAVE( m_AllowCaching );
	NODE_SAVE_NODE( m_Preprocessor );
	NODE_SAVE( m_PreprocessorArgs );
	NODE_SAVE( m_BaseDirectory );
	NODE_SAVE( m_ExtraPDBPath );
	NODE_SAVE( m_ExtraASMPath );

	// TODO:B Need to save the dynamic deps, for better progress estimates
	// but we can't right now because we rely on the nodes we depend on 
	// being saved before us which isn't the case for dynamic deps.
	// dynamic deps are saved for more accurate progress estimates in future builds
	//Node::SaveDepArray( fileStream, m_DynamicDependencies );
}

// GetObjExtension
//------------------------------------------------------------------------------
const char * ObjectListNode::GetObjExtension() const
{
	if ( m_ObjExtensionOverride.IsEmpty() )
	{
		#if defined( __WINDOWS__ )
			return ".obj";
		#else
			return ".o";
		#endif
	}
	return m_ObjExtensionOverride.Get();
}

//------------------------------------------------------------------------------
