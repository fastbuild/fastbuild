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
//  REGISTER_TEST( TestMixedAssemblyWithCPP ) // TODO:A Enable
REGISTER_TESTS_END

// TestSingleFile
//------------------------------------------------------------------------------
void TestCSharp::TestSingleFile() const
{
    FBuildTestOptions options;
    options.m_ConfigFile = "Tools/FBuild/FBuildTest/Data/TestCSharp/csharp.bff";
    options.m_ForceCleanBuild = true;

    FBuild fBuild( options );
    TEST_ASSERT( fBuild.Initialize() );

    // delete files from previous runs
    EnsureFileDoesNotExist( "../tmp/Test/CSharp/csharpsingle.dll" );

    // Build it
    TEST_ASSERT( fBuild.Build( AStackString<>( "CSharp-Single-Target" ) ) );
    TEST_ASSERT( fBuild.SaveDependencyGraph( "../tmp/Test/CSharp/csharpsingle.fdb" ) );

    // Test output file
    EnsureFileExists( "../tmp/Test/CSharp/csharpsingle.dll" );

    // Check stats
    //               Seen,  Built,  Type
    CheckStatsNode ( 2,     2,      Node::FILE_NODE );  // compiler + 1 cs file
    CheckStatsNode ( 1,     1,      Node::CS_NODE );
    CheckStatsNode ( 1,     1,      Node::ALIAS_NODE );
    CheckStatsTotal( 4,     4 );
}

// TestSingleFile_NoRebuild
//------------------------------------------------------------------------------
void TestCSharp::TestSingleFile_NoRebuild() const
{
    FBuildTestOptions options;
    options.m_ConfigFile = "Tools/FBuild/FBuildTest/Data/TestCSharp/csharp.bff";
    options.m_ShowSummary = true; // required to generate stats for node count checks

    FBuild fBuild( options );
    TEST_ASSERT( fBuild.Initialize( "../tmp/Test/CSharp/csharpsingle.fdb" ) );

    // Build it
    TEST_ASSERT( fBuild.Build( AStackString<>( "CSharp-Single-Target" ) ) );

    // Check stats
    //               Seen,  Built,  Type
    CheckStatsNode ( 2,     2,      Node::FILE_NODE );  // compiler + 1 cs file
    CheckStatsNode ( 1,     0,      Node::CS_NODE );
    CheckStatsNode ( 1,     1,      Node::ALIAS_NODE );
    CheckStatsTotal( 4,     3 );
}

// TestMultipleFiles
//------------------------------------------------------------------------------
void TestCSharp::TestMultipleFiles() const
{
    FBuildTestOptions options;
    options.m_ConfigFile = "Tools/FBuild/FBuildTest/Data/TestCSharp/csharp.bff";
    options.m_ForceCleanBuild = true;

    FBuild fBuild( options );
    TEST_ASSERT( fBuild.Initialize() );

    // delete files from previous runs
    EnsureFileDoesNotExist( "../tmp/Test/CSharp/csharpmulti.dll" );

    // Build it
    TEST_ASSERT( fBuild.Build( AStackString<>( "CSharp-Multi-Target" ) ) );
    TEST_ASSERT( fBuild.SaveDependencyGraph( "../tmp/Test/CSharp/csharpmulti.fdb" ) );

    // Test output files
    EnsureFileExists( "../tmp/Test/CSharp/csharpmulti.dll" );

    // Check stats
    //               Seen,  Built,  Type
    CheckStatsNode ( 4,     4,      Node::FILE_NODE );  // compiler + 3x cs
    CheckStatsNode ( 1,     1,      Node::CS_NODE );
    CheckStatsNode ( 1,     1,      Node::ALIAS_NODE );
    CheckStatsNode ( 1,     1,      Node::DIRECTORY_LIST_NODE );
    CheckStatsTotal( 7,     7 );
}

// TestMultipleFiles_NoRebuild
//------------------------------------------------------------------------------
void TestCSharp::TestMultipleFiles_NoRebuild() const
{
    FBuildTestOptions options;
    options.m_ConfigFile = "Tools/FBuild/FBuildTest/Data/TestCSharp/csharp.bff";

    FBuild fBuild( options );
    TEST_ASSERT( fBuild.Initialize( "../tmp/Test/CSharp/csharpmulti.fdb" ) );

    // Build it
    TEST_ASSERT( fBuild.Build( AStackString<>( "CSharp-Multi-Target" ) ) );

    // Check stats
    //               Seen,  Built,  Type
    CheckStatsNode ( 4,     4,      Node::FILE_NODE );  // compiler + 3x cs
    CheckStatsNode ( 1,     0,      Node::CS_NODE );
    CheckStatsNode ( 1,     1,      Node::ALIAS_NODE );
    CheckStatsNode ( 1,     1,      Node::DIRECTORY_LIST_NODE );
    CheckStatsTotal( 7,     6 );
}

// TestMultipleAssemblies
//------------------------------------------------------------------------------
void TestCSharp::TestMultipleAssemblies() const
{
    FBuildTestOptions options;
    options.m_ConfigFile = "Tools/FBuild/FBuildTest/Data/TestCSharp/csharp.bff";
    options.m_ForceCleanBuild = true;

    FBuild fBuild( options );
    TEST_ASSERT( fBuild.Initialize() );

    // delete files from previous runs
    EnsureFileDoesNotExist( "../tmp/Test/CSharp/csharpassemblya.dll" );
    EnsureFileDoesNotExist( "../tmp/Test/CSharp/csharpassemblyb.dll" );
    EnsureFileDoesNotExist( "../tmp/Test/CSharp/csharpassemblyc.dll" );

    // Build it
    TEST_ASSERT( fBuild.Build( AStackString<>( "CSharp-AssemblyC" ) ) );
    TEST_ASSERT( fBuild.SaveDependencyGraph( "../tmp/Test/CSharp/csharpmultipleassemblies.fdb" ) );

    // Test output files
    EnsureFileExists( "../tmp/Test/CSharp/csharpassemblya.dll" );
    EnsureFileExists( "../tmp/Test/CSharp/csharpassemblyb.dll" );
    EnsureFileExists( "../tmp/Test/CSharp/csharpassemblyc.dll" );

    // Check stats
    //               Seen,  Built,  Type
    CheckStatsNode ( 4,     4,      Node::FILE_NODE );  // compiler + 2x cs
    CheckStatsNode ( 3,     3,      Node::CS_NODE );
    CheckStatsNode ( 1,     1,      Node::ALIAS_NODE );
    CheckStatsTotal( 8,     8 );
}


// TestMultipleAssemblies_NoRebuild
//------------------------------------------------------------------------------
void TestCSharp::TestMultipleAssemblies_NoRebuild() const
{
    FBuildTestOptions options;
    options.m_ConfigFile = "Tools/FBuild/FBuildTest/Data/TestCSharp/csharp.bff";

    FBuild fBuild( options );
    TEST_ASSERT( fBuild.Initialize( "../tmp/Test/CSharp/csharpmultipleassemblies.fdb" ) );

    // Build it
    TEST_ASSERT( fBuild.Build( AStackString<>( "CSharp-AssemblyC" ) ) );

    // Check stats
    //               Seen,  Built,  Type
    CheckStatsNode ( 4,     4,      Node::FILE_NODE );  // compiler + 2x cs
    CheckStatsNode ( 3,     0,      Node::CS_NODE );
    CheckStatsNode ( 1,     1,      Node::ALIAS_NODE );
    CheckStatsTotal( 8,     5 );
}


// TestMixedAssemblyWithCPP
//------------------------------------------------------------------------------
void TestCSharp::TestMixedAssemblyWithCPP() const
{
    // TODO:A Implement functionality and tests
}

//------------------------------------------------------------------------------
