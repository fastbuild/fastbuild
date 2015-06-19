// TestTest.cpp
//------------------------------------------------------------------------------

// Includes
//------------------------------------------------------------------------------
#include "FBuildTest.h"
#include "Tools/FBuild/FBuildCore/FBuild.h"
#include "Tools/FBuild/FBuildCore/Graph/TestNode.h"
#include "Tools/FBuild/FBuildCore/Graph/NodeGraph.h"

#include "Core/FileIO/FileIO.h"
#include "Core/Strings/AStackString.h"

// TestTest
//------------------------------------------------------------------------------
class TestTest : public FBuildTest
{
private:
	DECLARE_TESTS

	void CreateNode() const;
	void Build() const;
	void Build_NoRebuild() const;
};

// Register Tests
//------------------------------------------------------------------------------
REGISTER_TESTS_BEGIN( TestTest )
	REGISTER_TEST( CreateNode )
	REGISTER_TEST( Build )
	REGISTER_TEST( Build_NoRebuild )
REGISTER_TESTS_END

// CreateNode
//------------------------------------------------------------------------------
void TestTest::CreateNode() const
{
	FBuild fb;
	NodeGraph & ng = fb.GetDependencyGraph();

	FileNode * fileNode = ng.CreateFileNode( AStackString<>( "test.exe" ) );

	TestNode * testNode = ng.CreateTestNode( AStackString<>( "output.txt" ), 
											 fileNode, 
											 AString::GetEmpty(), 
											 AString::GetEmpty() );

	TEST_ASSERT( testNode->GetType() == Node::TEST_NODE );
	TEST_ASSERT( TestNode::GetTypeS() == Node::TEST_NODE );
	TEST_ASSERT( AStackString<>( "Test" ) == testNode->GetTypeName() );
}

// Build
//------------------------------------------------------------------------------
void TestTest::Build() const
{
	FBuildOptions options;
	options.m_ConfigFile = "Data/TestTest/test.bff";
	options.m_ForceCleanBuild = true;
	options.m_ShowSummary = true; // required to generate stats for node count checks
	FBuild fBuild( options );
	TEST_ASSERT( fBuild.Initialize() );

	const AStackString<> testExe( "../../../../ftmp/Test/Test/test.exe" );

	// clean up anything left over from previous runs
	EnsureFileDoesNotExist( testExe );

	// build (via alias)
	TEST_ASSERT( fBuild.Build( AStackString<>( "Test" ) ) );
	TEST_ASSERT( fBuild.SaveDependencyGraph( "../../../../ftmp/Test/Test/test.fdb" ) );

	// make sure all output is where it is expected
	EnsureFileExists( testExe );

	// Check stats
	//				 Seen,	Built,	Type
	CheckStatsNode ( 1,		1,		Node::FILE_NODE ); // cpp 
	CheckStatsNode ( 1,		1,		Node::COMPILER_NODE );
	CheckStatsNode ( 1,		1,		Node::OBJECT_NODE );
	CheckStatsNode ( 1,		1,		Node::LIBRARY_NODE );
	CheckStatsNode ( 1,		1,		Node::EXE_NODE );
	CheckStatsNode ( 1,		1,		Node::TEST_NODE );
	CheckStatsNode ( 1,		1,		Node::ALIAS_NODE );
	CheckStatsTotal( 7,		7 );
}

// Build_NoRebuild
//------------------------------------------------------------------------------
void TestTest::Build_NoRebuild() const
{
	FBuildOptions options;
	options.m_ConfigFile = "Data/TestCopy/copy.bff";
	options.m_ShowSummary = true; // required to generate stats for node count checks
	FBuild fBuild( options );
	TEST_ASSERT( fBuild.Initialize( "../../../../ftmp/Test/Test/test.fdb" ) );

	// build (via alias)
	TEST_ASSERT( fBuild.Build( AStackString<>( "Test" ) ) );

	// Check stats
	//				 Seen,	Built,	Type
	CheckStatsNode ( 1,		1,		Node::FILE_NODE ); // cpp
	CheckStatsNode ( 1,		0,		Node::COMPILER_NODE );
	CheckStatsNode ( 1,		0,		Node::OBJECT_NODE );
	CheckStatsNode ( 1,		0,		Node::LIBRARY_NODE );
	CheckStatsNode ( 1,		0,		Node::EXE_NODE );
	CheckStatsNode ( 1,		0,		Node::TEST_NODE );
	CheckStatsNode ( 1,		1,		Node::ALIAS_NODE );
	CheckStatsTotal( 7,		2 );

}

//------------------------------------------------------------------------------
