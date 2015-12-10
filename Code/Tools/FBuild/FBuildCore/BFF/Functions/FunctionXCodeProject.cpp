// FunctionXCodeProject
//------------------------------------------------------------------------------

// Includes
//------------------------------------------------------------------------------
#include "Tools/FBuild/FBuildCore/PrecompiledHeader.h"

// FBuild
#include "FunctionXCodeProject.h"
#include "Tools/FBuild/FBuildCore/FBuild.h"
#include "Tools/FBuild/FBuildCore/Graph/NodeGraph.h"
#include "Tools/FBuild/FBuildCore/Graph/XCodeProjectNode.h"

// Core
#include "Core/Strings/AStackString.h"

// CONSTRUCTOR
//------------------------------------------------------------------------------
FunctionXCodeProject::FunctionXCodeProject()
: Function( "XCodeProject" )
{
}

// AcceptsHeader
//------------------------------------------------------------------------------
/*virtual*/ bool FunctionXCodeProject::AcceptsHeader() const
{
	return true;
}

// Commit
//------------------------------------------------------------------------------
/*virtual*/ bool FunctionXCodeProject::Commit( const BFFIterator & funcStartIter ) const
{
	NodeGraph & ng = FBuild::Get().GetDependencyGraph();
	AStackString<> name;
	if ( GetNameForNode( funcStartIter, XCodeProjectNode::GetReflectionInfoS(), name ) == false )
	{
		return false;
	}

	if ( ng.FindNode( name ) )
	{
		Error::Error_1100_AlreadyDefined( funcStartIter, this, name );
		return false;
	}

	auto * xcodeProjNode = ng.CreateXCodeProjectNode( name );

	if ( !PopulateProperties( funcStartIter, xcodeProjNode ) )
	{
		return false;
	}

	if ( !xcodeProjNode->Initialize( funcStartIter, this ) )
    {
        return false;
    }

	// handle alias creation
	return ProcessAlias( funcStartIter, xcodeProjNode );
}

//------------------------------------------------------------------------------
