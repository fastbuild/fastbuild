// LinkerNode.cpp
//------------------------------------------------------------------------------

// Includes
//------------------------------------------------------------------------------
#include "DLLNode.h"

#include "Tools/FBuild/FBuildCore/FBuild.h"
#include "Tools/FBuild/FBuildCore/Graph/NodeGraph.h"

#include "Core/FileIO/IOStream.h"
#include "Core/FileIO/PathUtils.h"
#include "Core/Strings/AStackString.h"

// Reflection
//------------------------------------------------------------------------------
REFLECT_NODE_BEGIN( DLLNode, LinkerNode, MetaNone() )
REFLECT_END( DLLNode )

// CONSTRUCTOR
//------------------------------------------------------------------------------
DLLNode::DLLNode()
    : LinkerNode()
{
    m_Type = DLL_NODE;
    m_LinkerLinkObjects = true; // Override LinkerNode default
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

//------------------------------------------------------------------------------
