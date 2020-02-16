// VSProjectBaseNode.cpp
//------------------------------------------------------------------------------

// Includes
//------------------------------------------------------------------------------
#include "VSProjectBaseNode.h"

// Reflection
//------------------------------------------------------------------------------
REFLECT_NODE_BEGIN_ABSTRACT( VSProjectBaseNode, Node, MetaNone() )
    REFLECT( m_ProjectGuid,     "ProjectGuid",     MetaOptional() )
REFLECT_END( VSProjectBaseNode )

// CONSTRUCTOR
//------------------------------------------------------------------------------
VSProjectBaseNode::VSProjectBaseNode()
    : FileNode( AString::GetEmpty(), Node::FLAG_ALWAYS_BUILD )
{
    m_LastBuildTimeMs = 100; // higher default than a file node
}

// DESTRUCTOR
//------------------------------------------------------------------------------
VSProjectBaseNode::~VSProjectBaseNode() = default;

//------------------------------------------------------------------------------
