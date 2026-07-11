// TestCSharp.cpp
//------------------------------------------------------------------------------
#if defined( __WINDOWS__ )

// Includes
//------------------------------------------------------------------------------
    #include "FBuildTest.h"

// FBuildCore
    #include "Tools/FBuild/FBuildCore/BFF/BFFParser.h"
    #include "Tools/FBuild/FBuildCore/FBuild.h"

    #include "Core/FileIO/FileIO.h"
    #include "Core/Strings/AStackString.h"

//------------------------------------------------------------------------------
TEST_GROUP( TestCSharp, FBuildTest )
{
public:
};

//------------------------------------------------------------------------------
TEST_CASE( TestCSharp, TestSingleFile )
{
    FBuildTestOptions options;
    options.m_ConfigFile = "Tools/FBuild/FBuildTest/Data/TestCSharp/csharp.bff";
    options.m_ForceCleanBuild = true;

    FBuildForTest fBuild( options );
    TEST_ASSERT( fBuild.Initialize() );

    // delete files from previous runs
    EnsureFileDoesNotExist( "../tmp/Test/CSharp/csharpsingle.dll" );

    // Build it
    TEST_ASSERT( fBuild.Build( "CSharp-Single-Target" ) );
    TEST_ASSERT( fBuild.SaveDependencyGraph( "../tmp/Test/CSharp/csharpsingle.fdb" ) );

    // Test output file
    EnsureFileExists( "../tmp/Test/CSharp/csharpsingle.dll" );

    // Check stats: Seen, Built, Type
    CheckStatsNode( 1, 1, Node::COMPILER_NODE );
    CheckStatsNode( 1, 1, Node::FILE_NODE );  // 1 cs file
    CheckStatsNode( 1, 1, Node::CS_NODE );
    CheckStatsNode( 1, 1, Node::ALIAS_NODE );
    CheckStatsTotal( 4, 4 );
}

//------------------------------------------------------------------------------
TEST_CASE( TestCSharp, TestSingleFile_NoRebuild )
{
    FBuildTestOptions options;
    options.m_ConfigFile = "Tools/FBuild/FBuildTest/Data/TestCSharp/csharp.bff";
    options.m_ShowSummary = true; // required to generate stats for node count checks

    FBuildForTest fBuild( options );
    TEST_ASSERT( fBuild.Initialize( "../tmp/Test/CSharp/csharpsingle.fdb" ) );

    // Build it
    TEST_ASSERT( fBuild.Build( "CSharp-Single-Target" ) );

    // Check stats: Seen, Built, Type
    CheckStatsNode( 1, 0, Node::COMPILER_NODE );
    CheckStatsNode( 1, 1, Node::FILE_NODE );  // 1 cs file
    CheckStatsNode( 1, 0, Node::CS_NODE );
    CheckStatsNode( 1, 1, Node::ALIAS_NODE );
    CheckStatsTotal( 4, 2 );
}

//------------------------------------------------------------------------------
TEST_CASE( TestCSharp, TestSingleFile_NoRebuild_BFFChange )
{
    FBuildOptions options;
    options.m_ConfigFile = "Tools/FBuild/FBuildTest//Data/TestCSharp/csharp.bff";
    options.m_ShowSummary = true; // required to generate stats for node count checks
    options.m_ForceDBMigration_Debug = true;

    FBuildForTest fBuild( options );
    TEST_ASSERT( fBuild.Initialize( "../tmp/Test/CSharp/csharpsingle.fdb" ) );

    // Build it
    TEST_ASSERT( fBuild.Build( "CSharp-Single-Target" ) );

    // Check stats: Seen, Built, Type
    CheckStatsNode( 1, 0, Node::COMPILER_NODE );
    CheckStatsNode( 1, 1, Node::FILE_NODE );  // 1 cs file
    CheckStatsNode( 1, 0, Node::CS_NODE );
    CheckStatsNode( 1, 1, Node::ALIAS_NODE );
    CheckStatsTotal( 4, 2 );
}

//------------------------------------------------------------------------------
TEST_CASE( TestCSharp, TestMultipleFiles )
{
    FBuildTestOptions options;
    options.m_ConfigFile = "Tools/FBuild/FBuildTest/Data/TestCSharp/csharp.bff";
    options.m_ForceCleanBuild = true;

    FBuildForTest fBuild( options );
    TEST_ASSERT( fBuild.Initialize() );

    // delete files from previous runs
    EnsureFileDoesNotExist( "../tmp/Test/CSharp/csharpmulti.dll" );

    // Build it
    TEST_ASSERT( fBuild.Build( "CSharp-Multi-Target" ) );
    TEST_ASSERT( fBuild.SaveDependencyGraph( "../tmp/Test/CSharp/csharpmulti.fdb" ) );

    // Test output files
    EnsureFileExists( "../tmp/Test/CSharp/csharpmulti.dll" );

    // Check stats: Seen, Built, Type
    CheckStatsNode( 1, 1, Node::COMPILER_NODE );
    CheckStatsNode( 3, 3, Node::FILE_NODE );  // 3x cs
    CheckStatsNode( 1, 1, Node::CS_NODE );
    CheckStatsNode( 1, 1, Node::ALIAS_NODE );
    CheckStatsNode( 1, 1, Node::DIRECTORY_LIST_NODE );
    CheckStatsTotal( 7, 7 );
}

//------------------------------------------------------------------------------
TEST_CASE( TestCSharp, TestMultipleFiles_NoRebuild )
{
    FBuildTestOptions options;
    options.m_ConfigFile = "Tools/FBuild/FBuildTest/Data/TestCSharp/csharp.bff";

    FBuildForTest fBuild( options );
    TEST_ASSERT( fBuild.Initialize( "../tmp/Test/CSharp/csharpmulti.fdb" ) );

    // Build it
    TEST_ASSERT( fBuild.Build( "CSharp-Multi-Target" ) );

    // Check stats: Seen, Built, Type
    CheckStatsNode( 1, 0, Node::COMPILER_NODE );
    CheckStatsNode( 3, 3, Node::FILE_NODE );  // 3x cs
    CheckStatsNode( 1, 0, Node::CS_NODE );
    CheckStatsNode( 1, 1, Node::ALIAS_NODE );
    CheckStatsNode( 1, 1, Node::DIRECTORY_LIST_NODE );
    CheckStatsTotal( 7, 5 );
}

//------------------------------------------------------------------------------
TEST_CASE( TestCSharp, TestMultipleFiles_NoRebuild_BFFChange )
{
    FBuildOptions options;
    options.m_ConfigFile = "Tools/FBuild/FBuildTest/Data/TestCSharp/csharp.bff";
    options.m_ShowSummary = true; // required to generate stats for node count checks
    options.m_ForceDBMigration_Debug = true;

    FBuildForTest fBuild( options );
    TEST_ASSERT( fBuild.Initialize( "../tmp/Test/CSharp/csharpmulti.fdb" ) );

    // Build it
    TEST_ASSERT( fBuild.Build( "CSharp-Multi-Target" ) );

    // Check stats: Seen, Built, Type
    CheckStatsNode( 1, 0, Node::COMPILER_NODE );
    CheckStatsNode( 3, 3, Node::FILE_NODE );  // 3x cs
    CheckStatsNode( 1, 0, Node::CS_NODE );
    CheckStatsNode( 1, 1, Node::ALIAS_NODE );
    CheckStatsNode( 1, 1, Node::DIRECTORY_LIST_NODE );
    CheckStatsTotal( 7, 5 );
}

//------------------------------------------------------------------------------
TEST_CASE( TestCSharp, TestMultipleAssemblies )
{
    FBuildTestOptions options;
    options.m_ConfigFile = "Tools/FBuild/FBuildTest/Data/TestCSharp/csharp.bff";
    options.m_ForceCleanBuild = true;

    FBuildForTest fBuild( options );
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

    // Check stats: Seen, Built, Type
    CheckStatsNode( 1, 1, Node::COMPILER_NODE );
    CheckStatsNode( 3, 3, Node::FILE_NODE );  // 2x cs
    CheckStatsNode( 3, 3, Node::CS_NODE );
    CheckStatsNode( 1, 1, Node::ALIAS_NODE );
    CheckStatsTotal( 8, 8 );
}

//------------------------------------------------------------------------------
TEST_CASE( TestCSharp, TestMultipleAssemblies_NoRebuild )
{
    FBuildTestOptions options;
    options.m_ConfigFile = "Tools/FBuild/FBuildTest/Data/TestCSharp/csharp.bff";

    FBuildForTest fBuild( options );
    TEST_ASSERT( fBuild.Initialize( "../tmp/Test/CSharp/csharpmultipleassemblies.fdb" ) );

    // Build it
    TEST_ASSERT( fBuild.Build( "CSharp-AssemblyC" ) );

    // Check stats: Seen, Built, Type
    CheckStatsNode( 1, 0, Node::COMPILER_NODE );
    CheckStatsNode( 3, 3, Node::FILE_NODE );  // 3x cs
    CheckStatsNode( 3, 0, Node::CS_NODE );
    CheckStatsNode( 1, 1, Node::ALIAS_NODE );
    CheckStatsTotal( 8, 4 );
}

//------------------------------------------------------------------------------
TEST_CASE( TestCSharp, TestMultipleAssemblies_NoRebuild_BFFChange )
{
    FBuildOptions options;
    options.m_ConfigFile = "Tools/FBuild/FBuildTest/Data/TestCSharp/csharp.bff";
    options.m_ShowSummary = true; // required to generate stats for node count checks
    options.m_ForceDBMigration_Debug = true;

    FBuildForTest fBuild( options );
    TEST_ASSERT( fBuild.Initialize( "../tmp/Test/CSharp/csharpmultipleassemblies.fdb" ) );

    // Build it
    TEST_ASSERT( fBuild.Build( "CSharp-AssemblyC" ) );

    // Check stats: Seen, Built, Type
    CheckStatsNode( 1, 0, Node::COMPILER_NODE );
    CheckStatsNode( 3, 3, Node::FILE_NODE );  // 3x cs
    CheckStatsNode( 3, 0, Node::CS_NODE );
    CheckStatsNode( 1, 1, Node::ALIAS_NODE );
    CheckStatsTotal( 8, 4 );
}

//------------------------------------------------------------------------------
TEST_CASE( TestCSharp, TestMixedAssemblyWithCPP )
{
    // TODO:A Implement functionality and tests
}

//------------------------------------------------------------------------------
TEST_CASE( TestCSharp, CSharpWithObjectListFails )
{
    //
    // The C# compiler should only be used with CSAssembly
    //
    FBuildOptions options;
    options.m_ConfigFile = "Tools/FBuild/FBuildTest/Data/TestCSharp/ObjectListFails/fbuild.bff";

    // Expect failure
    FBuildForTest fBuild( options );
    TEST_ASSERT( fBuild.Initialize() == false );

    // Check for the expected failure
    TEST_ASSERT( GetRecordedOutput().Find( "#1503 - ObjectList() - C# compiler should use CSAssembly." ) );
}

//------------------------------------------------------------------------------
TEST_CASE( TestCSharp, UsingNonCSharpCompilerFails )
{
    //
    // CSAssembly should only use the C# Compiler
    //
    FBuildOptions options;
    options.m_ConfigFile = "Tools/FBuild/FBuildTest/Data/TestCSharp/UsingNonCSharpCompilerFails/fbuild.bff";

    // Expect failure
    FBuildForTest fBuild( options );
    TEST_ASSERT( fBuild.Initialize() == false );

    // Check for the expected failure
    TEST_ASSERT( GetRecordedOutput().Find( "#1504 - CSAssembly() - CSAssembly requires a C# Compiler." ) );
}

//------------------------------------------------------------------------------
TEST_CASE( TestCSharp, Exclusions )
{
    FBuildTestOptions options;
    options.m_ConfigFile = "Tools/FBuild/FBuildTest/Data/TestCSharp/Exclusions/fbuild.bff";
    FBuildForTest fBuild( options );
    TEST_ASSERT( fBuild.Initialize() );

    // build (via alias)
    TEST_ASSERT( fBuild.Build( "Test" ) );

    // Check all the exclusion methods worked as expected
    // clang-format off
    const char * const aliasesToCheck[] =
    {
        "ExcludePath-ForwardSlash",
        "ExcludePath-Backslash",
        "ExcludedFiles-File",
        "ExcludedFiles-Path-ForwardSlash",
        "ExcludedFiles-Path-Backslash",
        "ExcludePattern-ForwardSlash",
        "ExcludePattern-Backslash",
    };
    // clang-format on
    for ( const char * const aliasToCheck : aliasesToCheck )
    {
        // Get the TestNode (via the Alias)
        const Node * aliasNode = fBuild.GetNode( aliasToCheck );
        TEST_ASSERT( aliasNode );
        const Node * testNode = aliasNode->GetStaticDependencies()[ 0 ].GetNode();
        TEST_ASSERT( testNode );

        // Check that it has one dynamic dependency, and that it's the 'B' file
        TEST_ASSERT( testNode->GetDynamicDependencies().GetSize() == 1 );
        TEST_ASSERT( testNode->GetDynamicDependencies()[ 0 ].GetNode()->GetName().EndsWithI( "FileB.cs" ) );
    }
}

//------------------------------------------------------------------------------
#endif
