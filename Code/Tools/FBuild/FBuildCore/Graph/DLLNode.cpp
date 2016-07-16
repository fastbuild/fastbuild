// LinkerNode.cpp
//------------------------------------------------------------------------------

// Includes
//------------------------------------------------------------------------------
#include "Tools/FBuild/FBuildCore/PrecompiledHeader.h"

#include "DLLNode.h"

#include "Tools/FBuild/FBuildCore/FBuild.h"
#include "Tools/FBuild/FBuildCore/Graph/NodeGraph.h"

#include "Core/FileIO/IOStream.h"
#include "Core/Strings/AStackString.h"

// CONSTRUCTOR
//------------------------------------------------------------------------------
DLLNode::DLLNode( const AString & linkerOutputName,
				  const Dependencies & inputLibraries,
				  const Dependencies & otherLibraries,
				  const AString & linkerType,
				  const AString & linker,
				  const AString & linkerArgs,
				  uint32_t flags,
				  const Dependencies & assemblyResources,
				  const AString & importLibName,
				  Node * linkerStampExe,
				  const AString & linkerStampExeArgs )
: LinkerNode( linkerOutputName, inputLibraries, otherLibraries, linkerType, linker, linkerArgs, flags, assemblyResources, importLibName, linkerStampExe, linkerStampExeArgs )
{
	m_Type = DLL_NODE;
}

// DESTRUCTOR
//------------------------------------------------------------------------------
DLLNode::~DLLNode()
{
}

// GetImportLibName
//------------------------------------------------------------------------------
void DLLNode::GetImportLibName( AString & importLibName ) const
{
	// if the name was explicitly set by the user, use that
	if ( m_ImportLibName.IsEmpty() == false )
	{
		importLibName = m_ImportLibName;
		return;
	}

    // with msvc, we need to link the import lib that matches the dll
    if (GetFlag(LinkerNode::LINK_FLAG_MSVC))
    {
	    // get name minus extension (handle no extension gracefully)
	    const char * lastDot = GetName().FindLast( '.' );
	    lastDot = lastDot ? lastDot : GetName().GetEnd();
	    importLibName.Assign( GetName().Get(), lastDot );

	    // assume .lib extension for import
	    importLibName += ".lib";
        return;
    }

    // for other platforms, use the object directly (e.g. .so or .dylib)
    importLibName = GetName();
}

// Load
//------------------------------------------------------------------------------
/*static*/ Node * DLLNode::Load( NodeGraph & nodeGraph, IOStream & stream )
{
    // common Linker properties
	NODE_LOAD( AStackString<>,	name );
	NODE_LOAD( AStackString<>,	linkerType );
	NODE_LOAD( AStackString<>,	linker );
	NODE_LOAD( AStackString<>,	linkerArgs );
	NODE_LOAD_DEPS( 0,			inputLibs);
	NODE_LOAD( uint32_t,		flags );
	NODE_LOAD_DEPS( 0,  		assemblyResources );
	NODE_LOAD_DEPS( 0,			otherLibs );
	NODE_LOAD( AStackString<>,	importLibName );
    NODE_LOAD_NODE( Node,		linkerStampExe );
    NODE_LOAD( AStackString<>,  linkerStampExeArgs );

	DLLNode * dn = nodeGraph.CreateDLLNode( name, inputLibs, otherLibs, linkerType, linker, linkerArgs, flags, assemblyResources, importLibName, linkerStampExe, linkerStampExeArgs );
	return dn;
}

//------------------------------------------------------------------------------
