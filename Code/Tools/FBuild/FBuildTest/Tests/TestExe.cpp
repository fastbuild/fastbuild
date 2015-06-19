// TestExe.cpp
//------------------------------------------------------------------------------

// Includes
//------------------------------------------------------------------------------
#include "FBuildTest.h"
#include "Tools/FBuild/FBuildCore/FBuild.h"
#include "Tools/FBuild/FBuildCore/Graph/ExeNode.h"
#include "Tools/FBuild/FBuildCore/Graph/NodeGraph.h"

#include "Core/FileIO/FileIO.h"
#include "Core/Process/Process.h"
#include "Core/Strings/AStackString.h"

// TestExe
//------------------------------------------------------------------------------
class TestExe : public FBuildTest
{
private:
	DECLARE_TESTS

	void CreateNode() const;
	void Build() const;
	void CheckValidExe() const;
	void Build_NoRebuild() const;
};

// Register Tests
//------------------------------------------------------------------------------
REGISTER_TESTS_BEGIN( TestExe )
	REGISTER_TEST( CreateNode )
	REGISTER_TEST( Build )
	REGISTER_TEST( CheckValidExe )
	REGISTER_TEST( Build_NoRebuild )
REGISTER_TESTS_END

// CreateNode
//------------------------------------------------------------------------------
void TestExe::CreateNode() const
{
	FBuild fb;
	NodeGraph & ng = fb.GetDependencyGraph();

	Dependencies inputLibraries( 1, false );
	inputLibraries.Append( Dependency( ng.CreateFileNode( AStackString<>( "dummy.lib" ) ) ) );

	ExeNode * exeNode = ng.CreateExeNode( AStackString<>( "exe.exe" ), 
											inputLibraries,
											Dependencies(),
											AStackString<>( "linker.exe" ),
											AString::GetEmpty(), // args
											0, // flags
											Dependencies(),
											nullptr,
											AString::GetEmpty() ); // assembly resources

	TEST_ASSERT( exeNode->GetType() == Node::EXE_NODE );
	TEST_ASSERT( ExeNode::GetTypeS() == Node::EXE_NODE );
	TEST_ASSERT( AStackString<>( "Exe" ) == exeNode->GetTypeName() );
}

// Build
//------------------------------------------------------------------------------
void TestExe::Build() const
{
	FBuildOptions options;
	options.m_ConfigFile = "Data/TestExe/exe.bff";
	options.m_ForceCleanBuild = true;
	options.m_ShowSummary = true; // required to generate stats for node count checks
	FBuild fBuild( options );
	TEST_ASSERT( fBuild.Initialize() );

	const AStackString<> exe( "..\\..\\..\\..\\ftmp\\Test\\Exe\\exe.exe" );

	// clean up anything left over from previous runs
	EnsureFileDoesNotExist( exe );

	// build (via alias)
	TEST_ASSERT( fBuild.Build( AStackString<>( "Exe" ) ) );
	TEST_ASSERT( fBuild.SaveDependencyGraph( "..\\..\\..\\..\\ftmp\\Test\\Exe\\exe.fdb" ) );

	// make sure all output is where it is expected
	EnsureFileExists( exe );

	// Check stats
	//				 Seen,	Built,	Type
	CheckStatsNode ( 1,		1,		Node::FILE_NODE ); // cpp
	CheckStatsNode ( 1,		1,		Node::COMPILER_NODE );
	CheckStatsNode ( 1,		1,		Node::OBJECT_NODE );
	CheckStatsNode ( 1,		1,		Node::LIBRARY_NODE );
	CheckStatsNode ( 1,		1,		Node::EXE_NODE );
	CheckStatsNode ( 1,		1,		Node::ALIAS_NODE );
	CheckStatsTotal( 6,		6 );
}

// CheckValidExe
//------------------------------------------------------------------------------
void TestExe::CheckValidExe() const
{
	Process p;
	p.Spawn( "..\\..\\..\\..\\ftmp\\Test\\Exe\\exe.exe", nullptr, nullptr, nullptr );
	int ret = p.WaitForExit();
	TEST_ASSERT( ret == 10203040 ); // verify expected ret code
}

// Build_NoRebuild
//------------------------------------------------------------------------------
void TestExe::Build_NoRebuild() const
{
	FBuildOptions options;
	options.m_ConfigFile = "Data/TestExe/exe.bff";
	options.m_ShowSummary = true; // required to generate stats for node count checks
	FBuild fBuild( options );
	TEST_ASSERT( fBuild.Initialize( "..\\..\\..\\..\\ftmp\\Test\\Exe\\exe.fdb" ) );

	// build (via alias)
	TEST_ASSERT( fBuild.Build( AStackString<>( "Exe" ) ) );

	// Check stats
	//				 Seen,	Built,	Type
	CheckStatsNode ( 1,		1,		Node::FILE_NODE ); // cpp
	CheckStatsNode ( 1,		0,		Node::COMPILER_NODE );
	CheckStatsNode ( 1,		0,		Node::OBJECT_NODE );
	CheckStatsNode ( 1,		0,		Node::LIBRARY_NODE );
	CheckStatsNode ( 1,		0,		Node::EXE_NODE );
	CheckStatsNode ( 1,		1,		Node::ALIAS_NODE );
	CheckStatsTotal( 6,		2 );

}

//------------------------------------------------------------------------------
