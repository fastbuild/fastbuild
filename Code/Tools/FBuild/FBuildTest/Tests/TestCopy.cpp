// TestCopy.cpp
//------------------------------------------------------------------------------

// Includes
//------------------------------------------------------------------------------
#include "FBuildTest.h"
#include "Tools/FBuild/FBuildCore/FBuild.h"
#include "Tools/FBuild/FBuildCore/Graph/CopyNode.h"
#include "Tools/FBuild/FBuildCore/Graph/NodeGraph.h"

#include "Core/FileIO/FileIO.h"
#include "Core/Strings/AStackString.h"

// TestCopy
//------------------------------------------------------------------------------
class TestCopy : public FBuildTest
{
private:
	DECLARE_TESTS

	// make sure the CopyNode works
	void SingleCopyNode() const;
	void MultipleCopyNodes() const;

	// make sure the FunctionCopy works
	void TestCopyFunction_FileToFile() const;
	void TestCopyFunction_FileToFile_NoRebuild() const;
	void TestCopyFunction_FileToDir() const;
	void TestCopyFunction_FileToDir_NoRebuild() const;
	void TestCopyFunction_MultiFileToDir() const;
	void TestCopyFunction_MultiFileToDir_NoRebuild() const;
};

// Register Tests
//------------------------------------------------------------------------------
REGISTER_TESTS_BEGIN( TestCopy )
	REGISTER_TEST( SingleCopyNode )
	REGISTER_TEST( MultipleCopyNodes )
	REGISTER_TEST( TestCopyFunction_FileToFile )
	REGISTER_TEST( TestCopyFunction_FileToFile_NoRebuild )
	REGISTER_TEST( TestCopyFunction_FileToDir )
	REGISTER_TEST( TestCopyFunction_FileToDir_NoRebuild )
	REGISTER_TEST( TestCopyFunction_MultiFileToDir )
	REGISTER_TEST( TestCopyFunction_MultiFileToDir_NoRebuild )
REGISTER_TESTS_END

// SingleCopyNode
//------------------------------------------------------------------------------
void TestCopy::SingleCopyNode() const
{
	const AStackString<> testFileName( "Data/TestGraph/library.cpp" );
	const AStackString<> testFileNameCopy( "../../../../ftmp/Test/Graph/library.copynode.cpp" );

	// check files are in expected states
	EnsureFileExists( testFileName );
	EnsureFileDoesNotExist( testFileNameCopy );

	FBuildOptions options;
	options.m_ShowSummary = true; // required to generate stats for node count checks

	// Build
	{
		FBuild fb( options );
		NodeGraph & ng = fb.GetDependencyGraph();

		// make a fileNode for the source
		FileNode * srcNode = ng.CreateFileNode( testFileName );

		// and an ObjectNode for the output
		Dependencies empty;
		Node * dstNode = ng.CreateCopyNode( testFileNameCopy, srcNode, empty );

		TEST_ASSERT( fb.Build( dstNode ) );
		TEST_ASSERT( fb.SaveDependencyGraph( "..\\..\\..\\..\\ftmp\\Test\\Graph\\singlecopynode.fdb" ) );

		EnsureFileExists( testFileNameCopy );

		// Check stats
		//				 Seen,	Built,	Type
		CheckStatsNode ( 1,		1,		Node::FILE_NODE );
		CheckStatsNode ( 1,		1,		Node::COPY_NODE );
		CheckStatsTotal( 2,		2 );
	}

	// check no rebuild
	{
		FBuild fb( options );
		fb.Initialize( "..\\..\\..\\..\\ftmp\\Test\\Graph\\singlecopynode.fdb" );

		TEST_ASSERT( fb.Build( testFileNameCopy ) );

		// Check stats
		//				 Seen,	Built,	Type
		CheckStatsNode ( 1,		1,		Node::FILE_NODE );
		CheckStatsNode ( 1,		0,		Node::COPY_NODE );
		CheckStatsTotal( 2,		1 );
	}
}

// MultipleCopyNodes
//------------------------------------------------------------------------------
void TestCopy::MultipleCopyNodes() const
{
	const AStackString<> srcFile( "Data/TestGraph/library.cpp" );
	const AStackString<> dstFileA( "../../../../ftmp/Test/Graph/library.multiplecopynodes1.cpp" );
	const AStackString<> dstFileB( "../../../../ftmp/Test/Graph/library.multiplecopynodes2.cpp" );
	const AStackString<> dstFileC( "../../../../ftmp/Test/Graph/library.multiplecopynodes3.cpp" );

	// check files are as expected before starting test
	EnsureFileDoesNotExist( dstFileA );
	EnsureFileDoesNotExist( dstFileB );
	EnsureFileDoesNotExist( dstFileC );
	EnsureFileExists( srcFile );

	FBuildOptions options;
	options.m_ShowSummary = true; // required to generate stats for node count checks

	// build
	{
		FBuild fb( options );
		NodeGraph & ng = fb.GetDependencyGraph();

		// make a fileNode for the source
		FileNode * srcNode = ng.CreateFileNode( srcFile );

		Dependencies empty;
		Node * copyA = ng.CreateCopyNode( dstFileA, srcNode, empty );
		Node * copyB = ng.CreateCopyNode( dstFileB, (FileNode *)copyA, empty );
		Node * copyC = ng.CreateCopyNode( dstFileC, (FileNode *)copyB, empty );

		TEST_ASSERT( fb.Build( copyC ) );
		TEST_ASSERT( fb.SaveDependencyGraph( "..\\..\\..\\..\\ftmp\\Test\\Graph\\multiplecopynode.fdb" ) );

		EnsureFileExists( dstFileA );
		EnsureFileExists( dstFileB );
		EnsureFileExists( dstFileC );

		// Check stats
		//				 Seen,	Built,	Type
		CheckStatsNode ( 1,		1,		Node::FILE_NODE );
		CheckStatsNode ( 3,		3,		Node::COPY_NODE );
		CheckStatsTotal( 4,		4 );
	}

	// check no rebuild
	{
		FBuild fb( options );
		fb.Initialize( "..\\..\\..\\..\\ftmp\\Test\\Graph\\multiplecopynode.fdb" );

		TEST_ASSERT( fb.Build( dstFileC ) );

		// Check stats
		//				 Seen,	Built,	Type
		CheckStatsNode ( 1,		1,		Node::FILE_NODE );
		CheckStatsNode ( 3,		0,		Node::COPY_NODE );
		CheckStatsTotal( 4,		1 );
	}
}

// TestCopyFunction_FileToFile
//------------------------------------------------------------------------------
void TestCopy::TestCopyFunction_FileToFile() const
{
	FBuildOptions options;
	options.m_ConfigFile = "Data/TestCopy/copy.bff";
	options.m_ShowSummary = true; // required to generate stats for node count checks
	FBuild fBuild( options );
	TEST_ASSERT( fBuild.Initialize() );

	AStackString<> dst( "../../../../ftmp/Test/Copy/copy.bff.copy" );

	// clean up anything left over from previous runs
	EnsureFileDoesNotExist( dst );

	// build (via alias)
	TEST_ASSERT( fBuild.Build( AStackString<>( "TestCopyFileToFile" ) ) );
	TEST_ASSERT( fBuild.SaveDependencyGraph( "../../../../ftmp/Test/Copy/filetofile.fdb" ) );

	// make sure all output is where it is expected
	EnsureFileExists( dst );

	// Check stats
	//				 Seen,	Built,	Type
	CheckStatsNode ( 1,		1,		Node::FILE_NODE );
	CheckStatsNode ( 1,		1,		Node::COPY_NODE );
	CheckStatsNode ( 1,		1,		Node::ALIAS_NODE );
	CheckStatsTotal( 3,		3 );
}

// TestCopyFunction_FileToFile_NoRebuild
//------------------------------------------------------------------------------
void TestCopy::TestCopyFunction_FileToFile_NoRebuild() const
{
	FBuildOptions options;
	options.m_ConfigFile = "Data/TestCopy/copy.bff";
	options.m_ShowSummary = true; // required to generate stats for node count checks
	FBuild fBuild( options );
	TEST_ASSERT( fBuild.Initialize( "../../../../ftmp/Test/Copy/filetofile.fdb" ) );

	// build (via alias)
	TEST_ASSERT( fBuild.Build( AStackString<>( "TestCopyFileToFile" ) ) );

	// Check stats
	//				 Seen,	Built,	Type
	CheckStatsNode ( 1,		1,		Node::FILE_NODE );
	CheckStatsNode ( 1,		0,		Node::COPY_NODE );
	CheckStatsNode ( 1,		1,		Node::ALIAS_NODE );
	CheckStatsTotal( 3,		2 );
}

// TestCopyFunction_FileToDir
//------------------------------------------------------------------------------
void TestCopy::TestCopyFunction_FileToDir() const
{
	FBuildOptions options;
	options.m_ConfigFile = "Data/TestCopy/copy.bff";
	options.m_ShowSummary = true; // required to generate stats for node count checks
	FBuild fBuild( options );
	TEST_ASSERT( fBuild.Initialize() );

	const AStackString<> dst( "../../../../ftmp/Test/Copy/copy.bff" );

	// clean up anything left over from previous runs
	EnsureFileDoesNotExist( dst );

	// build (via alias)
	TEST_ASSERT( fBuild.Build( AStackString<>( "TestCopyFileToDir" ) ) );
	TEST_ASSERT( fBuild.SaveDependencyGraph( "../../../../ftmp/Test/Copy/filetodir.fdb" ) );

	EnsureFileExists( dst );

	// Check stats
	//				 Seen,	Built,	Type
	CheckStatsNode ( 1,		1,		Node::FILE_NODE );
	CheckStatsNode ( 1,		1,		Node::COPY_NODE );
	CheckStatsNode ( 1,		1,		Node::ALIAS_NODE );
	CheckStatsTotal( 3,		3 );
}

// TestCopyFunction_FileToDir_NoRebuild
//------------------------------------------------------------------------------
void TestCopy::TestCopyFunction_FileToDir_NoRebuild() const
{
	FBuildOptions options;
	options.m_ConfigFile = "Data/TestCopy/copy.bff";
	options.m_ShowSummary = true; // required to generate stats for node count checks
	FBuild fBuild( options );
	TEST_ASSERT( fBuild.Initialize( "../../../../ftmp/Test/Copy/filetodir.fdb" ) );

	// build (via alias)
	TEST_ASSERT( fBuild.Build( AStackString<>( "TestCopyFileToDir" ) ) );

	// Check stats
	//				 Seen,	Built,	Type
	CheckStatsNode ( 1,		1,		Node::FILE_NODE );
	CheckStatsNode ( 1,		0,		Node::COPY_NODE );
	CheckStatsNode ( 1,		1,		Node::ALIAS_NODE );
	CheckStatsTotal( 3,		2 );
}

// TestCopyFunction_MultiFileToDir
//------------------------------------------------------------------------------
void TestCopy::TestCopyFunction_MultiFileToDir() const
{
	FBuildOptions options;
	options.m_ConfigFile = "Data/TestCopy/copy.bff";
	options.m_ShowSummary = true; // required to generate stats for node count checks
	FBuild fBuild( options );
	TEST_ASSERT( fBuild.Initialize() );

	const AStackString<> dst1( "../../../../ftmp/Test/Copy/a.txt" );
	const AStackString<> dst2( "../../../../ftmp/Test/Copy/b.txt" );

	// clean up anything left over from previous runs
	EnsureFileDoesNotExist( dst1 );
	EnsureFileDoesNotExist( dst2 );

	// build (via alias)
	TEST_ASSERT( fBuild.Build( AStackString<>( "TestMultiCopyToDir" ) ) );
	TEST_ASSERT( fBuild.SaveDependencyGraph( "../../../../ftmp/Test/Copy/multifiletodir.fdb" ) );

	EnsureFileExists( dst1 );
	EnsureFileExists( dst2 );

	// Check stats
	//				 Seen,	Built,	Type
	CheckStatsNode ( 2,		2,		Node::FILE_NODE );
	CheckStatsNode ( 2,		2,		Node::COPY_NODE );
	CheckStatsNode ( 1,		1,		Node::ALIAS_NODE );
	CheckStatsTotal( 5,		5 );
}

// TestCopyFunction_MultiFileToDir_NoRebuild
//------------------------------------------------------------------------------
void TestCopy::TestCopyFunction_MultiFileToDir_NoRebuild() const
{
	FBuildOptions options;
	options.m_ConfigFile = "Data/TestCopy/copy.bff";
	options.m_ShowSummary = true; // required to generate stats for node count checks
	FBuild fBuild( options );
	TEST_ASSERT( fBuild.Initialize( "../../../../ftmp/Test/Copy/multifiletodir.fdb" ) );

	// build (via alias)
	TEST_ASSERT( fBuild.Build( AStackString<>( "TestMultiCopyToDir" ) ) );

	// Check stats
	//				 Seen,	Built,	Type
	CheckStatsNode ( 2,		2,		Node::FILE_NODE );
	CheckStatsNode ( 2,		0,		Node::COPY_NODE );
	CheckStatsNode ( 1,		1,		Node::ALIAS_NODE );
	CheckStatsTotal( 5,		3 );
}

//------------------------------------------------------------------------------
