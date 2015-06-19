// FunctionUnity
//------------------------------------------------------------------------------

// Includes
//------------------------------------------------------------------------------
#include "Tools/FBuild/FBuildCore/PrecompiledHeader.h"

#include "FunctionUnity.h"
#include "Tools/FBuild/FBuildCore/FBuild.h"
#include "Tools/FBuild/FBuildCore/BFF/BFFIterator.h"
#include "Tools/FBuild/FBuildCore/BFF/BFFStackFrame.h"
#include "Tools/FBuild/FBuildCore/BFF/BFFVariable.h"
#include "Tools/FBuild/FBuildCore/Graph/NodeGraph.h"
#include "Tools/FBuild/FBuildCore/Graph/UnityNode.h"
#include "Tools/FBuild/FBuildCore/Graph/DirectoryListNode.h"

// Core
#include "Core/Strings/AStackString.h"
#include "Core/FileIO/PathUtils.h"

// UnityNode
//------------------------------------------------------------------------------
class UnityNode;

// CONSTRUCTOR
//------------------------------------------------------------------------------
FunctionUnity::FunctionUnity()
: Function( "Unity" )
{
}

// AcceptsHeader
//------------------------------------------------------------------------------
/*virtual*/ bool FunctionUnity::AcceptsHeader() const
{
	return true;
}

// NeedsHeader
//------------------------------------------------------------------------------
/*virtual*/ bool FunctionUnity::NeedsHeader() const
{
	return true;
}

// Commit
//------------------------------------------------------------------------------
/*virtual*/ bool FunctionUnity::Commit( const BFFIterator & funcStartIter ) const
{
	// parsing logic should guarantee we have a string for our name
	ASSERT( m_AliasForFunction.IsEmpty() == false );

	// Check for existing node
	NodeGraph & ng = FBuild::Get().GetDependencyGraph();
	if ( ng.FindNode( m_AliasForFunction ) )
	{
		Error::Error_1100_AlreadyDefined( funcStartIter, this, m_AliasForFunction );
		return false;
	}

	UnityNode * un = ng.CreateUnityNode( m_AliasForFunction );

	if ( !PopulateProperties( funcStartIter, un ) )
	{
		return false;
	}

	return un->Initialize( funcStartIter, this );
}

//------------------------------------------------------------------------------
