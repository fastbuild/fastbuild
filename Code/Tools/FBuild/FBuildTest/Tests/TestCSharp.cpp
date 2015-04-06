// TestCSharp.cpp
//------------------------------------------------------------------------------

// Includes
//------------------------------------------------------------------------------
#include "FBuildTest.h"

#include "Tools/FBuild/FBuildCore/FBuild.h"
#include "Tools/FBuild/FBuildCore/BFF/BFFParser.h"

#include "Core/FileIO/FileIO.h"
#include "Core/Strings/AStackString.h"

// TestCSharp
//------------------------------------------------------------------------------
class TestCSharp : public FBuildTest
{
private:
	DECLARE_TESTS

	// Tests
	void TestSingleFile() const;
	void TestSingleFile_NoRebuild() const;
	void TestMultipleFiles() const;
	void TestMultipleFiles_NoRebuild() const;
	void TestMultipleAssemblies() const;
	void TestMultipleAssemblies_NoRebuild() const;
	void TestMixedAssemblyWithCPP() const;
};

// Register Tests
//------------------------------------------------------------------------------
REGISTER_TESTS_BEGIN( TestCSharp )
	REGISTER_TEST( TestSingleFile )
	REGISTER_TEST( TestSingleFile_NoRebuild )
	REGISTER_TEST( TestMultipleFiles )
	REGISTER_TEST( TestMultipleFiles_NoRebuild )
	REGISTER_TEST( TestMultipleAssemblies )
	REGISTER_TEST( TestMultipleAssemblies_NoRebuild )
//	REGISTER_TEST( TestMixedAssemblyWithCPP ) // TODO:A Enable
REGISTER_TESTS_END

// TestSingleFile
//------------------------------------------------------------------------------
void TestCSharp::TestSingleFile() const
{
	FBuildOptions options;
	options.m_ConfigFile = "Data/TestCSharp/csharp.bff";
	options.m_ForceCleanBuild = true;
	options.m_ShowSummary = true; // required to generate stats for node count checks

	FBuild fBuild( options );
	TEST_ASSERT( fBuild.Initialize() );

	// delete files from previous runs
	EnsureFileDoesNotExist( "../../../../ftmp/Test/CSharp/csharpsingle.dll" );
	
	// Build it
	TEST_ASSERT( fBuild.Build( AStackString<>( "CSharp-Single-Target" ) ) );
	TEST_ASSERT( fBuild.SaveDependencyGraph( "../../../../ftmp/Test/CSharp/csharpsingle.fdb" ) );

	// Test output files
	EnsureFileExists( "../../../../ftmp/Test/CSharp/csharpsingle.dll" );

	// Check stats
	//				 Seen,	Built,	Type
	CheckStatsNode ( 1,		1,		Node::FILE_NODE );	// cs
	CheckStatsNode ( 1,		1,		Node::CS_NODE );
	CheckStatsNode ( 1,		1,		Node::ALIAS_NODE );
	CheckStatsTotal( 3,		3 );
}

// TestSingleFile_NoRebuild
//------------------------------------------------------------------------------
void TestCSharp::TestSingleFile_NoRebuild() const
{
	FBuildOptions options;
	options.m_ConfigFile = "Data/TestCSharp/csharp.bff";
	options.m_ShowSummary = true; // required to generate stats for node count checks

	FBuild fBuild( options );
	TEST_ASSERT( fBuild.Initialize( "../../../../ftmp/Test/CSharp/csharpsingle.fdb" ) );
	
	// Build it
	TEST_ASSERT( fBuild.Build( AStackString<>( "CSharp-Single-Target" ) ) );

	// Check stats
	//				 Seen,	Built,	Type
	CheckStatsNode ( 1,		1,		Node::FILE_NODE );	// cs
	CheckStatsNode ( 1,		0,		Node::CS_NODE );
	CheckStatsNode ( 1,		1,		Node::ALIAS_NODE );
	CheckStatsTotal( 3,		2 );
}

// TestMultipleFiles
//------------------------------------------------------------------------------
void TestCSharp::TestMultipleFiles() const
{
	FBuildOptions options;
	options.m_ConfigFile = "Data/TestCSharp/csharp.bff";
	options.m_ForceCleanBuild = true;
	options.m_ShowSummary = true; // required to generate stats for node count checks

	FBuild fBuild( options );
	TEST_ASSERT( fBuild.Initialize() );

	// delete files from previous runs
	EnsureFileDoesNotExist( "../../../../ftmp/Test/CSharp/csharpmulti.dll" );
	
	// Build it
	TEST_ASSERT( fBuild.Build( AStackString<>( "CSharp-Multi-Target" ) ) );
	TEST_ASSERT( fBuild.SaveDependencyGraph( "../../../../ftmp/Test/CSharp/csharpmulti.fdb" ) );

	// Test output files
	EnsureFileExists( "../../../../ftmp/Test/CSharp/csharpmulti.dll" );

	// Check stats
	//				 Seen,	Built,	Type
	CheckStatsNode ( 3,		3,		Node::FILE_NODE );	// 3x cs
	CheckStatsNode ( 1,		1,		Node::CS_NODE );
	CheckStatsNode ( 1,		1,		Node::ALIAS_NODE );
	CheckStatsNode ( 1,		1,		Node::DIRECTORY_LIST_NODE );
	CheckStatsTotal( 6,		6 );
}

// TestMultipleFiles_NoRebuild
//------------------------------------------------------------------------------
void TestCSharp::TestMultipleFiles_NoRebuild() const
{
	FBuildOptions options;
	options.m_ConfigFile = "Data/TestCSharp/csharp.bff";
	options.m_ShowSummary = true; // required to generate stats for node count checks

	FBuild fBuild( options );
	TEST_ASSERT( fBuild.Initialize( "../../../../ftmp/Test/CSharp/csharpmulti.fdb" ) );
	
	// Build it
	TEST_ASSERT( fBuild.Build( AStackString<>( "CSharp-Multi-Target" ) ) );

	// Check stats
	//				 Seen,	Built,	Type
	CheckStatsNode ( 3,		3,		Node::FILE_NODE );	// 3x cs
	CheckStatsNode ( 1,		0,		Node::CS_NODE );
	CheckStatsNode ( 1,		1,		Node::ALIAS_NODE );
	CheckStatsNode ( 1,		1,		Node::DIRECTORY_LIST_NODE );
	CheckStatsTotal( 6,		5 );
}

// TestMultipleAssemblies
//------------------------------------------------------------------------------
void TestCSharp::TestMultipleAssemblies() const
{
	FBuildOptions options;
	options.m_ConfigFile = "Data/TestCSharp/csharp.bff";
	options.m_ForceCleanBuild = true;
	options.m_ShowSummary = true; // required to generate stats for node count checks

	FBuild fBuild( options );
	TEST_ASSERT( fBuild.Initialize() );

	// delete files from previous runs
	EnsureFileDoesNotExist( "../../../../ftmp/Test/CSharp/csharpassemblya.dll" );
	EnsureFileDoesNotExist( "../../../../ftmp/Test/CSharp/csharpassemblyb.dll" );
	
	// Build it
	TEST_ASSERT( fBuild.Build( AStackString<>( "CSharp-AssemblyB" ) ) );
	TEST_ASSERT( fBuild.SaveDependencyGraph( "../../../../ftmp/Test/CSharp/csharpmultipleassemblies.fdb" ) );

	// Test output files
	EnsureFileExists( "../../../../ftmp/Test/CSharp/csharpassemblya.dll" );
	EnsureFileExists( "../../../../ftmp/Test/CSharp/csharpassemblyb.dll" );

	// Check stats
	//				 Seen,	Built,	Type
	CheckStatsNode ( 2,		2,		Node::FILE_NODE );	// 2x cs
	CheckStatsNode ( 2,		2,		Node::CS_NODE );
	CheckStatsNode ( 1,		1,		Node::ALIAS_NODE );
	CheckStatsTotal( 5,		5 );
}


// TestMultipleAssemblies_NoRebuild
//------------------------------------------------------------------------------
void TestCSharp::TestMultipleAssemblies_NoRebuild() const
{
	FBuildOptions options;
	options.m_ConfigFile = "Data/TestCSharp/csharp.bff";
	options.m_ShowSummary = true; // required to generate stats for node count checks

	FBuild fBuild( options );
	TEST_ASSERT( fBuild.Initialize( "../../../../ftmp/Test/CSharp/csharpmultipleassemblies.fdb" ) );
	
	// Build it
	TEST_ASSERT( fBuild.Build( AStackString<>( "CSharp-AssemblyB" ) ) );

	// Check stats
	//				 Seen,	Built,	Type
	CheckStatsNode ( 2,		2,		Node::FILE_NODE );	// 2x cs
	CheckStatsNode ( 2,		0,		Node::CS_NODE );
	CheckStatsNode ( 1,		1,		Node::ALIAS_NODE );
	CheckStatsTotal( 5,		3 );
}


// TestMixedAssemblyWithCPP
//------------------------------------------------------------------------------
void TestCSharp::TestMixedAssemblyWithCPP() const
{
	// TODO:A Implement functionality and tests
}

//------------------------------------------------------------------------------
