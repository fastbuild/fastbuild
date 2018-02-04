// LinkerNode.cpp
//------------------------------------------------------------------------------

// Includes
//------------------------------------------------------------------------------
#include "Tools/FBuild/FBuildCore/PrecompiledHeader.h"

#include "ExeNode.h"

#include "Tools/FBuild/FBuildCore/FBuild.h"
#include "Tools/FBuild/FBuildCore/Graph/NodeGraph.h"

#include "Core/FileIO/IOStream.h"
#include "Core/Strings/AStackString.h"

// Reflection
//------------------------------------------------------------------------------
REFLECT_NODE_BEGIN( ExeNode, LinkerNode, MetaNone() )
REFLECT_END( ExeNode )

// CONSTRUCTOR
//------------------------------------------------------------------------------
ExeNode::ExeNode()
    : LinkerNode()
{
    m_Type = EXE_NODE;
}

// DESTRUCTOR
//------------------------------------------------------------------------------
ExeNode::~ExeNode() = default;

// Load
//------------------------------------------------------------------------------
/*static*/ Node * ExeNode::Load( NodeGraph & nodeGraph, IOStream & stream )
{
    NODE_LOAD( AStackString<>, name );

    ExeNode * node = nodeGraph.CreateExeNode( name );

    if ( node->Deserialize( nodeGraph, stream ) == false )
    {
        return nullptr;
    }

    return node;
}

//------------------------------------------------------------------------------
