// LinkerNode.cpp
//------------------------------------------------------------------------------

// Includes
//------------------------------------------------------------------------------
#include "Tools/FBuild/FBuildCore/PrecompiledHeader.h"

#include "DLLNode.h"

#include "Tools/FBuild/FBuildCore/FBuild.h"
#include "Tools/FBuild/FBuildCore/Graph/NodeGraph.h"

#include "Core/FileIO/IOStream.h"
#include "Core/FileIO/PathUtils.h"
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
DLLNode::~DLLNode() = default;

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

    // for other platforms, use the object directly (e.g. .so or .dylib)
    importLibName = GetName();

    // with msvc, we need to link the import lib that matches the dll
    if ( GetFlag( LinkerNode::LINK_FLAG_MSVC ) )
    {
        PathUtils::StripFileExtension( importLibName );

        // assume .lib extension for import
        importLibName += ".lib";
    }
    else if ( GetFlag( LinkerNode::LINK_FLAG_ORBIS_LD ) )
    {
        PathUtils::StripFileExtension( importLibName );

        // Assume we link with stub_weak library (loose linking)
        importLibName += "_stub_weak.a";
    }
}

// Load
//------------------------------------------------------------------------------
/*static*/ Node * DLLNode::Load( NodeGraph & nodeGraph, IOStream & stream )
{
    // common Linker properties
    NODE_LOAD( AStackString<>,  name );
    NODE_LOAD( AStackString<>,  linkerType );
    NODE_LOAD( AStackString<>,  linker );
    NODE_LOAD( AStackString<>,  linkerArgs );
    NODE_LOAD_DEPS( 0,          inputLibs);
    NODE_LOAD( uint32_t,        flags );
    NODE_LOAD_DEPS( 0,          assemblyResources );
    NODE_LOAD_DEPS( 0,          otherLibs );
    NODE_LOAD( AStackString<>,  importLibName );
    NODE_LOAD_NODE_LINK( Node,  linkerStampExe );
    NODE_LOAD( AStackString<>,  linkerStampExeArgs );

    DLLNode * dn = nodeGraph.CreateDLLNode( name, inputLibs, otherLibs, linkerType, linker, linkerArgs, flags, assemblyResources, importLibName, linkerStampExe, linkerStampExeArgs );
    return dn;
}

//------------------------------------------------------------------------------
