// FunctionTest
//------------------------------------------------------------------------------

// Includes
//------------------------------------------------------------------------------
#include "Tools/FBuild/FBuildCore/PrecompiledHeader.h"

#include "FunctionTest.h"
#include "Tools/FBuild/FBuildCore/FBuild.h"
#include "Tools/FBuild/FBuildCore/Graph/NodeGraph.h"
#include "Tools/FBuild/FBuildCore/Graph/TestNode.h"

// CONSTRUCTOR
//------------------------------------------------------------------------------
FunctionTest::FunctionTest()
: Function( "Test" )
{
}

// AcceptsHeader
//------------------------------------------------------------------------------
/*virtual*/ bool FunctionTest::AcceptsHeader() const
{
	return true;
}

// Commit
//------------------------------------------------------------------------------
/*virtual*/ bool FunctionTest::Commit( const BFFIterator & funcStartIter ) const
{
	NodeGraph & ng = FBuild::Get().GetDependencyGraph();
	AStackString<> name;
	if ( GetNameForNode( funcStartIter, TestNode::GetReflectionInfoS(), name ) == false )
	{
		return false;
	}

	if ( ng.FindNode( name ) )
	{
		Error::Error_1100_AlreadyDefined( funcStartIter, this, name );
		return false;
	}

	TestNode * testNode = ng.CreateTestNode( name );

	if ( !PopulateProperties( funcStartIter, testNode ) )
	{
		return false;
	}

	if ( !testNode->Initialize( funcStartIter, this ) )
    {
        return false;
    }

	// handle alias creation
	return ProcessAlias( funcStartIter, testNode );
}

//------------------------------------------------------------------------------
