// TestCSharp.cpp
//------------------------------------------------------------------------------

// Includes
//------------------------------------------------------------------------------
#include "FBuildTest.h"

// FBuildCore
#include "Tools/FBuild/FBuildCore/BFF/BFFParser.h"
#include "Tools/FBuild/FBuildCore/FBuild.h"

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
    void TestSingleFile_NoRebuild_BFFChange() const;
    void TestMultipleFiles() const;
    void TestMultipleFiles_NoRebuild() const;
    void TestMultipleFiles_NoRebuild_BFFChange() const;
    void TestMultipleAssemblies() const;
    void TestMultipleAssemblies_NoRebuild() const;
    void TestMultipleAssemblies_NoRebuild_BFFChange() const;
    void TestMixedAssemblyWithCPP() const;
};

// Register Tests
//------------------------------------------------------------------------------
REGISTER_TESTS_BEGIN( TestCSharp )
    REGISTER_TEST( TestSingleFile )
    REGISTER_TEST( TestSingleFile_NoRebuild )
    REGISTER_TEST( TestSingleFile_NoRebuild_BFFChange )
    REGISTER_TEST( TestMultipleFiles )
    REGISTER_TEST( TestMultipleFiles_NoRebuild )
    REGISTER_TEST( TestMultipleFiles_NoRebuild_BFFChange )
    REGISTER_TEST( TestMultipleAssemblies )
    REGISTER_TEST( TestMultipleAssemblies_NoRebuild )
    REGISTER_TEST( TestMultipleAssemblies_NoRebuild_BFFChange )
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
    TEST_ASSERT( fBuild.Build( "CSharp-Single-Target" ) );
    TEST_ASSERT( fBuild.SaveDependencyGraph( "../tmp/Test/CSharp/csharpsingle.fdb" ) );

    // Test output file
    EnsureFileExists( "../tmp/Test/CSharp/csharpsingle.dll" );

    // Check stats
    //               Seen,  Built,  Type
    CheckStatsNode ( 1,     1,      Node::COMPILER_NODE );
    CheckStatsNode ( 1,     1,      Node::FILE_NODE );  // 1 cs file
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
    TEST_ASSERT( fBuild.Build( "CSharp-Single-Target" ) );

    // Check stats
    //               Seen,  Built,  Type
    CheckStatsNode ( 1,     0,      Node::COMPILER_NODE);
    CheckStatsNode ( 1,     1,      Node::FILE_NODE );  // 1 cs file
    CheckStatsNode ( 1,     0,      Node::CS_NODE );
    CheckStatsNode ( 1,     1,      Node::ALIAS_NODE );
    CheckStatsTotal( 4,     2 );
}

// TestSingleFile_NoRebuild_BFFChange
//------------------------------------------------------------------------------
void TestCSharp::TestSingleFile_NoRebuild_BFFChange() const
{
    FBuildOptions options;
    options.m_ConfigFile = "Tools/FBuild/FBuildTest//Data/TestCSharp/csharp.bff";
    options.m_ShowSummary = true; // required to generate stats for node count checks
    options.m_ForceDBMigration_Debug = true;

    FBuild fBuild( options );
    TEST_ASSERT( fBuild.Initialize( "../tmp/Test/CSharp/csharpsingle.fdb" ) );

    // Build it
    TEST_ASSERT( fBuild.Build( "CSharp-Single-Target" ) );

    // Check stats
    //               Seen,  Built,  Type
    CheckStatsNode ( 1,     0,      Node::COMPILER_NODE );
    CheckStatsNode ( 1,     1,      Node::FILE_NODE );  // 1 cs file
    CheckStatsNode ( 1,     0,      Node::CS_NODE );
    CheckStatsNode ( 1,     1,      Node::ALIAS_NODE );
    CheckStatsTotal( 4,     2 );
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
    TEST_ASSERT( fBuild.Build( "CSharp-Multi-Target" ) );
    TEST_ASSERT( fBuild.SaveDependencyGraph( "../tmp/Test/CSharp/csharpmulti.fdb" ) );

    // Test output files
    EnsureFileExists( "../tmp/Test/CSharp/csharpmulti.dll" );

    // Check stats
    //               Seen,  Built,  Type
    CheckStatsNode ( 1,     1,      Node::COMPILER_NODE );
    CheckStatsNode ( 3,     3,      Node::FILE_NODE );  // 3x cs
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
    TEST_ASSERT( fBuild.Build( "CSharp-Multi-Target" ) );

    // Check stats
    //               Seen,  Built,  Type
    CheckStatsNode ( 1,     0,      Node::COMPILER_NODE );
    CheckStatsNode ( 3,     3,      Node::FILE_NODE );  // 3x cs
    CheckStatsNode ( 1,     0,      Node::CS_NODE );
    CheckStatsNode ( 1,     1,      Node::ALIAS_NODE );
    CheckStatsNode ( 1,     1,      Node::DIRECTORY_LIST_NODE );
    CheckStatsTotal( 7,     5 );
}

// TestMultipleFiles_NoRebuild_BFFChange
//------------------------------------------------------------------------------
void TestCSharp::TestMultipleFiles_NoRebuild_BFFChange() const
{
    FBuildOptions options;
    options.m_ConfigFile = "Tools/FBuild/FBuildTest/Data/TestCSharp/csharp.bff";
    options.m_ShowSummary = true; // required to generate stats for node count checks
    options.m_ForceDBMigration_Debug = true;

    FBuild fBuild( options );
    TEST_ASSERT( fBuild.Initialize( "../tmp/Test/CSharp/csharpmulti.fdb" ) );

    // Build it
    TEST_ASSERT( fBuild.Build( "CSharp-Multi-Target" ) );

    // Check stats
    //               Seen,  Built,  Type
    CheckStatsNode ( 1,     0,      Node::COMPILER_NODE );
    CheckStatsNode ( 3,     3,      Node::FILE_NODE );  // 3x cs
    CheckStatsNode ( 1,     0,      Node::CS_NODE );
    CheckStatsNode ( 1,     1,      Node::ALIAS_NODE );
    CheckStatsNode ( 1,     1,      Node::DIRECTORY_LIST_NODE );
    CheckStatsTotal( 7,     5 );
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
    TEST_ASSERT( fBuild.Build( "CSharp-AssemblyC" ) );
    TEST_ASSERT( fBuild.SaveDependencyGraph( "../tmp/Test/CSharp/csharpmultipleassemblies.fdb" ) );

    // Test output files
    EnsureFileExists( "../tmp/Test/CSharp/csharpassemblya.dll" );
    EnsureFileExists( "../tmp/Test/CSharp/csharpassemblyb.dll" );
    EnsureFileExists( "../tmp/Test/CSharp/csharpassemblyc.dll" );

    // Check stats
    //               Seen,  Built,  Type
    CheckStatsNode ( 1,     1,      Node::COMPILER_NODE );
    CheckStatsNode ( 3,     3,      Node::FILE_NODE );  // 2x cs
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
    TEST_ASSERT( fBuild.Build( "CSharp-AssemblyC" ) );

    // Check stats
    //               Seen,  Built,  Type
    CheckStatsNode ( 1,     0,      Node::COMPILER_NODE );
    CheckStatsNode ( 3,     3,      Node::FILE_NODE );  // 3x cs
    CheckStatsNode ( 3,     0,      Node::CS_NODE );
    CheckStatsNode ( 1,     1,      Node::ALIAS_NODE );
    CheckStatsTotal( 8,     4 );
}

// TestMultipleAssemblies_NoRebuild_BFFChange
//------------------------------------------------------------------------------
void TestCSharp::TestMultipleAssemblies_NoRebuild_BFFChange() const
{
    FBuildOptions options;
    options.m_ConfigFile = "Tools/FBuild/FBuildTest/Data/TestCSharp/csharp.bff";
    options.m_ShowSummary = true; // required to generate stats for node count checks
    options.m_ForceDBMigration_Debug = true;

    FBuild fBuild( options );
    TEST_ASSERT( fBuild.Initialize( "../tmp/Test/CSharp/csharpmultipleassemblies.fdb" ) );

    // Build it
    TEST_ASSERT( fBuild.Build( "CSharp-AssemblyC" ) );

    // Check stats
    //               Seen,  Built,  Type
    CheckStatsNode ( 1,     0,      Node::COMPILER_NODE );
    CheckStatsNode ( 3,     3,      Node::FILE_NODE );  // 3x cs
    CheckStatsNode ( 3,     0,      Node::CS_NODE );
    CheckStatsNode ( 1,     1,      Node::ALIAS_NODE );
    CheckStatsTotal( 8,     4 );
}

// TestMixedAssemblyWithCPP
//------------------------------------------------------------------------------
void TestCSharp::TestMixedAssemblyWithCPP() const
{
    // TODO:A Implement functionality and tests
}

//------------------------------------------------------------------------------
