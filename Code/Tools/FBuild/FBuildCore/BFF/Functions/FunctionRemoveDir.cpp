// FunctionRemoveDir
//------------------------------------------------------------------------------

// Includes
//------------------------------------------------------------------------------
#include "Tools/FBuild/FBuildCore/PrecompiledHeader.h"

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

// Commit
//------------------------------------------------------------------------------
/*virtual*/ bool FunctionRemoveDir::Commit( NodeGraph & nodeGraph, const BFFIterator & funcStartIter ) const
{
    RemoveDirNode * removeDirNode = nodeGraph.CreateRemoveDirNode( m_AliasForFunction );

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

//------------------------------------------------------------------------------
