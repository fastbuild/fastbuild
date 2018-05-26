// FunctionAlias
//------------------------------------------------------------------------------

// Includes
//------------------------------------------------------------------------------
#include "Tools/FBuild/FBuildCore/PrecompiledHeader.h"

#include "FunctionCopyDir.h"
#include "Tools/FBuild/FBuildCore/FBuild.h"
#include "Tools/FBuild/FBuildCore/Graph/CopyDirNode.h"
#include "Tools/FBuild/FBuildCore/Graph/NodeGraph.h"

// Core
#include "Core/FileIO/PathUtils.h"

// CONSTRUCTOR
//------------------------------------------------------------------------------
FunctionCopyDir::FunctionCopyDir()
: Function( "CopyDir" )
{
}

// AcceptsHeader
//------------------------------------------------------------------------------
/*virtual*/ bool FunctionCopyDir::AcceptsHeader() const
{
    return true;
}

// Commit
//------------------------------------------------------------------------------
/*virtual*/ bool FunctionCopyDir::Commit( NodeGraph & nodeGraph, const BFFIterator & funcStartIter ) const
{
    if ( nodeGraph.FindNode( m_AliasForFunction ) )
    {
        Error::Error_1100_AlreadyDefined( funcStartIter, this, m_AliasForFunction );
        return false;
    }

    CopyDirNode * copyDirNode = nodeGraph.CreateCopyDirNode( m_AliasForFunction );

    if ( !PopulateProperties( nodeGraph, funcStartIter, copyDirNode ) )
    {
        return false;
    }

    if ( !copyDirNode->Initialize( nodeGraph, funcStartIter, this ) )
    {
        return false;
    }

    return true;
}

//------------------------------------------------------------------------------
