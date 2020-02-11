// VSProjectBaseNode.cpp
//------------------------------------------------------------------------------

// Includes
//------------------------------------------------------------------------------
#include "VSProjectBaseNode.h"

// Reflection
//------------------------------------------------------------------------------

REFLECT_NODE_BEGIN( VSProjectBaseNode, Node, MetaName("ProjectOutput") + MetaFile() )
REFLECT_END( VSProjectBaseNode )


// CONSTRUCTOR
//------------------------------------------------------------------------------
VSProjectBaseNode::VSProjectBaseNode()
    : FileNode( AString::GetEmpty(), Node::FLAG_ALWAYS_BUILD )
{
    m_Type = Node::PROXY_NODE;
    m_LastBuildTimeMs = 100; // higher default than a file node
}

//REMOVELUCIAN 
//// Initialize
////------------------------------------------------------------------------------
///*virtual*/ bool VSProjectBaseNode::Initialize(NodeGraph& /*nodeGraph*/, BFFToken* /*iter*/, const Function* /*function*/)
//{
//    // Generate default platform / config tuples if not specified
//    if (m_ProjectPlatformConfigTuples.IsEmpty())
//    {
//        VSProjectPlatformConfigTuple PlatCfgTuple;
//        m_ProjectPlatformConfigTuples.SetCapacity(4);
//        PlatCfgTuple.m_Platform = "Win32";
//        PlatCfgTuple.m_Config = "Debug";
//        m_ProjectPlatformConfigTuples.Append(PlatCfgTuple);
//        PlatCfgTuple.m_Config = "Release";
//        m_ProjectPlatformConfigTuples.Append(PlatCfgTuple);
//        PlatCfgTuple.m_Platform = "x64";
//        m_ProjectPlatformConfigTuples.Append(PlatCfgTuple);
//        PlatCfgTuple.m_Config = "Debug";
//        m_ProjectPlatformConfigTuples.Append(PlatCfgTuple);
//    }
//
//    return true;
//}

// DESTRUCTOR
//------------------------------------------------------------------------------
VSProjectBaseNode::~VSProjectBaseNode() = default;


//------------------------------------------------------------------------------
