// FunctionTest
//------------------------------------------------------------------------------

// Includes
//------------------------------------------------------------------------------
#include "Tools/FBuild/FBuildCore/PrecompiledHeader.h"

#include "FunctionTest.h"
#include "Tools/FBuild/FBuildCore/FBuild.h"
#include "Tools/FBuild/FBuildCore/BFF/BFFVariable.h"
#include "Tools/FBuild/FBuildCore/Graph/AliasNode.h"
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
	// make sure all required variables are defined
	const BFFVariable * outputV;
	const BFFVariable * executableV;
	const BFFVariable * argsV;
	const BFFVariable * workingDirV;
	if ( !GetString( funcStartIter, outputV,		".TestOutput", true ) ||
		 !GetString( funcStartIter, executableV,	".TestExecutable", true ) ||
		 !GetString( funcStartIter, argsV,			".TestArguments" ) ||
		 !GetString( funcStartIter, workingDirV,	".TestWorkingDir" ) )
	{
		return false;
	}

	// check for duplicates
	NodeGraph & ng = FBuild::Get().GetDependencyGraph();
	if ( ng.FindNode( outputV->GetString() ) != nullptr )
	{
		Error::Error_1100_AlreadyDefined( funcStartIter, this, outputV->GetString() );
		return false;
	}

	Node * testExeNode = ng.FindNode( executableV->GetString() );
	if ( testExeNode == nullptr )
	{
		testExeNode = ng.CreateFileNode( executableV->GetString() );
	}
	if ( testExeNode->GetType() == Node::ALIAS_NODE )
	{
		AliasNode * an = testExeNode->CastTo< AliasNode >();
		testExeNode = an->GetAliasedNodes()[ 0 ].GetNode();
	}
	if ( testExeNode->IsAFile() == false )
	{
		Error::Error_1103_NotAFile( funcStartIter, this, "TestExecutable", testExeNode->GetName(), testExeNode->GetType() );
		return false;
	}

	// optional args
	const AString & arguments(	argsV ?			argsV->GetString()		: AString::GetEmpty() );
	const AString & workingDir( workingDirV ?	workingDirV->GetString(): AString::GetEmpty() );

	// create the TestNode
	Node * outputNode = ng.CreateTestNode( outputV->GetString(), 
										   (FileNode *)testExeNode,
										   arguments,
										   workingDir );
	
	return ProcessAlias( funcStartIter, outputNode );
}

//------------------------------------------------------------------------------
