// AliasNode.cpp
//------------------------------------------------------------------------------

// Includes
//------------------------------------------------------------------------------
#include "Tools/FBuild/FBuildCore/PrecompiledHeader.h"

#include "AliasNode.h"

#include "Tools/FBuild/FBuildCore/BFF/Functions/Function.h"
#include "Tools/FBuild/FBuildCore/FBuild.h"
#include "Tools/FBuild/FBuildCore/FLog.h"
#include "Tools/FBuild/FBuildCore/Graph/NodeGraph.h"

#include "Core/FileIO/FileStream.h"

// Reflection
//------------------------------------------------------------------------------
REFLECT_NODE_BEGIN( AliasNode, Node, MetaNone() )
    REFLECT_ARRAY( m_Targets,   "Targets",          MetaFile() + MetaAllowNonFile() )
REFLECT_END( AliasNode )

// CONSTRUCTOR
//------------------------------------------------------------------------------
AliasNode::AliasNode()
: Node( AString::GetEmpty(), Node::ALIAS_NODE, Node::FLAG_TRIVIAL_BUILD )
{
    m_LastBuildTimeMs = 1; // almost no work is done for this node
}

// Initialize
//------------------------------------------------------------------------------
bool AliasNode::Initialize( NodeGraph & nodeGraph, const BFFIterator & iter, const Function * function )
{
    // TODO:B make this use m_Targets
    Dependencies targets( 32, true );
    const bool required = true;
    const bool allowCopyDirNodes = true;
    const bool allowUnityNodes = true;
    const bool allowRemoveDirNodes = true;
    if ( !function->GetNodeList( nodeGraph, iter, ".Targets", targets, required, allowCopyDirNodes, allowUnityNodes, allowRemoveDirNodes ) )
    {
        return false; // GetNodeList will have emitted an error
    }

    m_StaticDependencies = targets;

    return true;
}

// DESTRUCTOR
//------------------------------------------------------------------------------
AliasNode::~AliasNode() = default;

// DetermineNeedToBuild
//------------------------------------------------------------------------------
/*virtual*/ bool AliasNode::DetermineNeedToBuild( bool forceClean ) const
{
    (void)forceClean;
    return true;
}

// DoBuild
//------------------------------------------------------------------------------
/*virtual*/ Node::BuildResult AliasNode::DoBuild( Job * UNUSED( job ) )
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

// Load
//------------------------------------------------------------------------------
/*static*/ Node * AliasNode::Load( NodeGraph & nodeGraph, IOStream & stream )
{
    NODE_LOAD( AStackString<>, name );

    AliasNode * an = nodeGraph.CreateAliasNode( name );

    if ( an->Deserialize( nodeGraph, stream ) == false )
    {
        return nullptr;
    }
    return an;
}

// Save
//------------------------------------------------------------------------------
/*virtual*/ void AliasNode::Save( IOStream & stream ) const
{
    NODE_SAVE( m_Name );
    Node::Serialize( stream );
}

//------------------------------------------------------------------------------
