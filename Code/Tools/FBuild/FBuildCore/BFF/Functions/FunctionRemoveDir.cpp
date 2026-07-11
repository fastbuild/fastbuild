// FunctionRemoveDir
//------------------------------------------------------------------------------

// Includes
//------------------------------------------------------------------------------
#include "FunctionRemoveDir.h"
#include "Tools/FBuild/FBuildCore/FBuild.h"
#include "Tools/FBuild/FBuildCore/Graph/NodeGraph.h"
#include "Tools/FBuild/FBuildCore/Graph/RemoveDirNode.h"

// Core
#include "Core/FileIO/PathUtils.h"

// CONSTRUCTOR
//------------------------------------------------------------------------------
FunctionRemoveDir::FunctionRemoveDir()
    : Function( "RemoveDir" )
{
}

// AcceptsHeader
//------------------------------------------------------------------------------
/*virtual*/ bool FunctionRemoveDir::AcceptsHeader() const
{
    return true;
}

// NeedsHeader
//------------------------------------------------------------------------------
/*virtual*/ bool FunctionRemoveDir::NeedsHeader() const
{
    return true;
}

// Commit
//------------------------------------------------------------------------------
/*virtual*/ bool FunctionRemoveDir::Commit( NodeGraph & nodeGraph, const BFFToken * funcStartIter ) const
{
    if ( const Node * existingNode = nodeGraph.FindNode( m_AliasForFunction ) )
    {
        const BFFToken * existingToken = nodeGraph.FindNodeSourceToken( existingNode );
        Error::Error_1100_AlreadyDefined( funcStartIter, this, m_AliasForFunction, existingToken );
        return false;
    }

    Node * removeDirNode = nodeGraph.CreateNode<RemoveDirNode>( m_AliasForFunction, funcStartIter );

    if ( !PopulateProperties( nodeGraph, funcStartIter, removeDirNode ) )
    {
        return false;
    }

    if ( !removeDirNode->Initialize( nodeGraph, funcStartIter, this ) )
    {
        return false;
    }

    return true;
}

// CreateNode
//------------------------------------------------------------------------------
/*virtual*/ Node * FunctionRemoveDir::CreateNode() const
{
    return FNEW( RemoveDirNode );
}

//------------------------------------------------------------------------------
