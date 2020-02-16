// VSProjectBaseNode.cpp
//------------------------------------------------------------------------------

// Includes
//------------------------------------------------------------------------------
#include "VSProjectBaseNode.h"

// Reflection
//------------------------------------------------------------------------------
REFLECT_NODE_BEGIN( VSProjectBaseNode, Node, MetaName( "ProjectOutput" ) + MetaFile() )
REFLECT_END( VSProjectBaseNode )

// CONSTRUCTOR
//------------------------------------------------------------------------------
VSProjectBaseNode::VSProjectBaseNode()
    : FileNode( AString::GetEmpty(), Node::FLAG_ALWAYS_BUILD )
{
    m_Type = Node::PROXY_NODE;
    m_LastBuildTimeMs = 100; // higher default than a file node
}

// DESTRUCTOR
//------------------------------------------------------------------------------
VSProjectBaseNode::~VSProjectBaseNode() = default;

//------------------------------------------------------------------------------
