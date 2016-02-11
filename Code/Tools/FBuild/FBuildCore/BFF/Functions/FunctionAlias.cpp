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
#ifndef USE_NODE_REFLECTION
/*virtual*/ bool FunctionAlias::Commit( const BFFIterator & funcStartIter ) const
{
	// make sure all required variables are defined
	Dependencies targetNodes( 16, true );
	const bool required = true;
	const bool allowCopyDirNodes = true;
	const bool allowUnityNodes = true;
	const bool allowRemoveDirNodes = true;
	if ( !GetNodeList( funcStartIter, ".Targets", targetNodes, required, allowCopyDirNodes, allowUnityNodes, allowRemoveDirNodes ) )
	{
		return false; // GetNodeList will have emitted an error
	}

	if ( targetNodes.IsEmpty() )
	{
		Error::Error_1006_NothingToBuild( funcStartIter, this );
		return false;
	}

	NodeGraph & ng = FBuild::Get().GetDependencyGraph();
	if ( ng.FindNode( m_AliasForFunction ) != nullptr )
	{
		Error::Error_1100_AlreadyDefined( funcStartIter, this, m_AliasForFunction );
		return false;
	}

	ng.CreateAliasNode( m_AliasForFunction, targetNodes );

	return true;
}
#endif

// Commit
//------------------------------------------------------------------------------
#ifdef USE_NODE_REFLECTION
/*virtual*/ bool FunctionAlias::Commit( const BFFIterator & funcStartIter ) const
{
	NodeGraph & ng = FBuild::Get().GetDependencyGraph();
	if ( ng.FindNode( m_AliasForFunction ) )
	{
		Error::Error_1100_AlreadyDefined( funcStartIter, this, m_AliasForFunction );
		return false;
	}
	AliasNode * aliasNode = ng.CreateAliasNode( m_AliasForFunction );

	if ( !PopulateProperties( funcStartIter, aliasNode ) )
	{
		return false;
	}

	return aliasNode->Initialize( funcStartIter, this );
}
#endif

//------------------------------------------------------------------------------
