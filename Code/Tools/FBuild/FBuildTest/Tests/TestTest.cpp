// TestTest.cpp
//------------------------------------------------------------------------------

// Includes
//------------------------------------------------------------------------------
#include "FBuildTest.h"

// FBuildCore
#include "Tools/FBuild/FBuildCore/FBuild.h"
#include "Tools/FBuild/FBuildCore/Graph/NodeGraph.h"
#include "Tools/FBuild/FBuildCore/Graph/TestNode.h"

#include "Core/FileIO/FileIO.h"
#include "Core/Strings/AStackString.h"

//------------------------------------------------------------------------------
TEST_GROUP( TestTest, FBuildTest )
{
public:
};

//------------------------------------------------------------------------------
TEST_CASE( TestTest, Build )
{
    FBuildTestOptions options;
    options.m_ConfigFile = "Tools/FBuild/FBuildTest/Data/TestTest/test.bff";
    options.m_ForceCleanBuild = true;
    options.m_ShowSummary = true; // required to generate stats for node count checks
    FBuildForTest fBuild( options );
    TEST_ASSERT( fBuild.Initialize() );

    const AStackString testExe( "../tmp/Test/Test/test.exe" );

    // clean up anything left over from previous runs
    EnsureFileDoesNotExist( testExe );

    // build (via alias)
    TEST_ASSERT( fBuild.Build( "Test" ) );
    TEST_ASSERT( fBuild.SaveDependencyGraph( "../tmp/Test/Test/test.fdb" ) );

    // make sure all output is where it is expected
    EnsureFileExists( testExe );

    // Check stats: Seen, Built, Type
    CheckStatsNode( 2, 2, Node::FILE_NODE ); // cpp / linker exe
    CheckStatsNode( 1, 1, Node::COMPILER_NODE );
    CheckStatsNode( 1, 1, Node::OBJECT_NODE );
    CheckStatsNode( 1, 1, Node::OBJECT_LIST_NODE );
    CheckStatsNode( 1, 1, Node::EXE_NODE );
    CheckStatsNode( 1, 1, Node::TEST_NODE );
    CheckStatsNode( 1, 1, Node::ALIAS_NODE );
    CheckStatsTotal( 8, 8 );
}

//------------------------------------------------------------------------------
TEST_CASE( TestTest, Build_NoRebuild )
{
    FBuildTestOptions options;
    options.m_ConfigFile = "Tools/FBuild/FBuildTest/Data/TestTest/test.bff";
    options.m_ShowSummary = true; // required to generate stats for node count checks
    FBuildForTest fBuild( options );
    TEST_ASSERT( fBuild.Initialize( "../tmp/Test/Test/test.fdb" ) );

    // build (via alias)
    TEST_ASSERT( fBuild.Build( "Test" ) );

    // Check stats: Seen, Built, Type
    CheckStatsNode( 2, 2, Node::FILE_NODE ); // cpp  / linker exe
    CheckStatsNode( 1, 0, Node::COMPILER_NODE );
    CheckStatsNode( 1, 0, Node::OBJECT_NODE );
    CheckStatsNode( 1, 0, Node::OBJECT_LIST_NODE );
    CheckStatsNode( 1, 0, Node::EXE_NODE );
    CheckStatsNode( 1, 0, Node::TEST_NODE );
    CheckStatsNode( 1, 1, Node::ALIAS_NODE );
    CheckStatsTotal( 8, 3 );
}

//------------------------------------------------------------------------------
TEST_CASE( TestTest, Fail_ReturnCode )
{
    FBuildTestOptions options;
    options.m_ConfigFile = "Tools/FBuild/FBuildTest/Data/TestTest/Fail_ReturnCode/fbuild.bff";
    FBuildForTest fBuild( options );
    TEST_ASSERT( fBuild.Initialize() );

    // Build and run test, expecting failure
    TEST_ASSERT( fBuild.Build( "Fail_ReturnCode" ) == false );

    // Ensure failure was of the test
    TEST_ASSERT( GetRecordedOutput().Find( "Error: 1 (0x01)" ) );
}

//------------------------------------------------------------------------------
TEST_CASE( TestTest, Fail_Crash )
{
    FBuildTestOptions options;
    options.m_ConfigFile = "Tools/FBuild/FBuildTest/Data/TestTest/Fail_Crash/fbuild.bff";
    FBuildForTest fBuild( options );
    TEST_ASSERT( fBuild.Initialize() );

    // Build and run test, expecting failure
    TEST_ASSERT( fBuild.Build( "Fail_Crash" ) == false );

    // Ensure failure was of the test
    TEST_ASSERT( GetRecordedOutput().Find( "Test failed" ) );
}

//------------------------------------------------------------------------------
TEST_CASE( TestTest, TimeOut )
{
    FBuildTestOptions options;
    options.m_ConfigFile = "Tools/FBuild/FBuildTest/Data/TestTest/test_timeout.bff";
    FBuildForTest fBuild( options );
    TEST_ASSERT( fBuild.Initialize() );

    // build (via alias)
    TEST_ASSERT( fBuild.Build( "Test" ) == false );

    // Ensure failure was of the test timing out
    TEST_ASSERT( GetRecordedOutput().Find( "Test timed out after" ) );
}

//------------------------------------------------------------------------------
TEST_CASE( TestTest, Exclusions )
{
    FBuildTestOptions options;
    options.m_ConfigFile = "Tools/FBuild/FBuildTest/Data/TestTest/Exclusions/fbuild.bff";
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
        TEST_ASSERT( testNode->GetDynamicDependencies()[ 0 ].GetNode()->GetName().EndsWithI( "FileB.txt" ) );
    }
}

//------------------------------------------------------------------------------
