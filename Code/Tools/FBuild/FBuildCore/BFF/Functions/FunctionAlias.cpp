// FunctionAlias
//------------------------------------------------------------------------------

// Includes
//------------------------------------------------------------------------------
#include "Tools/FBuild/FBuildCore/PrecompiledHeader.h"

#include "FunctionAlias.h"
#include "Tools/FBuild/FBuildCore/FBuild.h"
#include "Tools/FBuild/FBuildCore/Graph/NodeGraph.h"

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
/*virtual*/ bool FunctionAlias::Commit( const BFFIterator & funcStartIter ) const
{
	// make sure all required variables are defined
	Dependencies targetNodes( 16, true );
	const bool required = true;
	const bool allowCopyDirNodes = true;
	const bool allowUnityNodes = true;
	if ( !GetNodeList( funcStartIter, ".Targets", targetNodes, required, allowCopyDirNodes, allowUnityNodes ) )
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

//------------------------------------------------------------------------------
