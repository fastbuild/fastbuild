// AliasNode.cpp
//------------------------------------------------------------------------------

// Includes
//------------------------------------------------------------------------------
#include "AliasNode.h"

#include "Tools/FBuild/FBuildCore/BFF/Functions/Function.h"
#include "Tools/FBuild/FBuildCore/FBuild.h"
#include "Tools/FBuild/FBuildCore/FLog.h"
#include "Tools/FBuild/FBuildCore/Graph/NodeGraph.h"

// Reflection
//------------------------------------------------------------------------------
REFLECT_NODE_BEGIN( AliasNode, Node, MetaNone() )
    REFLECT_ARRAY( m_Targets,   "Targets",          MetaFile() + MetaAllowNonFile() )
    REFLECT( m_Hidden,          "Hidden",           MetaOptional() )
REFLECT_END( AliasNode )

// CONSTRUCTOR
//------------------------------------------------------------------------------
AliasNode::AliasNode()
    : Node( Node::ALIAS_NODE )
{
    m_ControlFlags = Node::FLAG_ALWAYS_BUILD;
    m_LastBuildTimeMs = 1; // almost no work is done for this node
}

// Initialize
//------------------------------------------------------------------------------
/*virtual*/ bool AliasNode::Initialize( NodeGraph & nodeGraph, const BFFToken * iter, const Function * function )
{
    Dependencies targets( 32 );
    GetNodeListOptions options;
    options.m_AllowCopyDirNodes = true;
    options.m_AllowUnityNodes = true;
    options.m_AllowRemoveDirNodes = true;
    options.m_AllowCompilerNodes = true;
    if ( !Function::GetNodeList( nodeGraph, iter, function, ".Targets", m_Targets, targets, options ) )
    {
        return false; // GetNodeList will have emitted an error
    }

    m_StaticDependencies = targets;

    return true;
}

// DESTRUCTOR
//------------------------------------------------------------------------------
AliasNode::~AliasNode() = default;

// DoBuild
//------------------------------------------------------------------------------
/*virtual*/ Node::BuildResult AliasNode::DoBuild( Job * /*job*/ )
{
    for ( const Dependency & dep : m_StaticDependencies )
    {
        // If any nodes are file nodes ...
        const Node * n = dep.GetNode();
        if ( n->GetType() == Node::FILE_NODE )
        {
            // ... and the file is missing ...
            if ( n->GetStamp() == 0 )
            {
                // ... the build should fail
                FLOG_ERROR( "Alias: %s\nFailed due to missing file: %s\n", GetName().Get(), n->GetName().Get() );
                return BuildResult::eFailed;
            }
        }
    }
    return BuildResult::eOk;
}

//------------------------------------------------------------------------------
