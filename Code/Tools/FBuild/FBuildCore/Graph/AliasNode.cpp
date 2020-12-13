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
    : Node( AString::GetEmpty(), Node::ALIAS_NODE, Node::FLAG_ALWAYS_BUILD )
{
    m_LastBuildTimeMs = 1; // almost no work is done for this node
}

// Initialize
//------------------------------------------------------------------------------
/*virtual*/ bool AliasNode::Initialize( NodeGraph & nodeGraph, const BFFToken * iter, const Function * function )
{
    Dependencies targets( 32, true );
    const bool allowCopyDirNodes = true;
    const bool allowUnityNodes = true;
    const bool allowRemoveDirNodes = true;
    const bool allowCompilerNodes = true;
    if ( !Function::GetNodeList( nodeGraph, iter, function, ".Targets", m_Targets, targets, allowCopyDirNodes, allowUnityNodes, allowRemoveDirNodes, allowCompilerNodes ) )
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
    const Dependencies::Iter end = m_StaticDependencies.End();
    for ( Dependencies::Iter it = m_StaticDependencies.Begin();
          it != end;
          ++it )
    {
        // If any nodes are file nodes ...
        const Node * n = it->GetNode();
        if ( n->GetType() == Node::FILE_NODE )
        {
            // ... and the file is missing ...
            if ( n->GetStamp() == 0 )
            {
                // ... the build should fail
                FLOG_ERROR( "Alias: %s\nFailed due to missing file: %s\n", GetName().Get(), n->GetName().Get() );
                return Node::NODE_RESULT_FAILED;
            }
        }
    }
    return NODE_RESULT_OK;
}

//------------------------------------------------------------------------------
