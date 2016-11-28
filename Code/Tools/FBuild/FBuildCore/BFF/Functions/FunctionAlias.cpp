// FunctionAlias
//------------------------------------------------------------------------------

// Includes
//------------------------------------------------------------------------------
#include "Tools/FBuild/FBuildCore/PrecompiledHeader.h"

#include "FunctionAlias.h"
#include "Tools/FBuild/FBuildCore/FBuild.h"
#include "Tools/FBuild/FBuildCore/Graph/NodeGraph.h"
#include "Tools/FBuild/FBuildCore/Graph/AliasNode.h"

// CONSTRUCTOR
//------------------------------------------------------------------------------
FunctionAlias::FunctionAlias()
: Function( "Alias" )
{
}

// AcceptsHeader
//------------------------------------------------------------------------------
/*virtual*/ bool FunctionAlias::AcceptsHeader() const
{
    return true;
}

// NeedsHeader
//------------------------------------------------------------------------------
/*virtual*/ bool FunctionAlias::NeedsHeader() const
{
    return true;
}

// Commit
//------------------------------------------------------------------------------
/*virtual*/ bool FunctionAlias::Commit( NodeGraph & nodeGraph, const BFFIterator & funcStartIter ) const
{
    if ( nodeGraph.FindNode( m_AliasForFunction ) )
    {
        Error::Error_1100_AlreadyDefined( funcStartIter, this, m_AliasForFunction );
        return false;
    }
    AliasNode * aliasNode = nodeGraph.CreateAliasNode( m_AliasForFunction );

    if ( !PopulateProperties( nodeGraph, funcStartIter, aliasNode ) )
    {
        return false;
    }

    return aliasNode->Initialize( nodeGraph, funcStartIter, this );
}

//------------------------------------------------------------------------------
