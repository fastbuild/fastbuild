// TestTextFile.cpp
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

// TestTextFile
//------------------------------------------------------------------------------
class TestTextFile : public FBuildTest
{
private:
    DECLARE_TESTS

    void Build() const;
    void Build_NoRebuild() const;
    void Build_NoRebuild_BFFChange() const;
};

// Register Tests
//------------------------------------------------------------------------------
REGISTER_TESTS_BEGIN( TestTextFile )
    REGISTER_TEST( Build )
    REGISTER_TEST( Build_NoRebuild )
    REGISTER_TEST( Build_NoRebuild_BFFChange )
REGISTER_TESTS_END

// Build
//------------------------------------------------------------------------------
void TestTextFile::Build() const
{
    FBuildTestOptions options;
    options.m_ConfigFile = "Tools/FBuild/FBuildTest/Data/TestTextFile/fbuild.bff";
    options.m_ForceCleanBuild = true;
    FBuild fBuild( options );
    TEST_ASSERT( fBuild.Initialize() );

    // build (via alias)
    TEST_ASSERT( fBuild.Build( "TextFile" ) );
    TEST_ASSERT( fBuild.SaveDependencyGraph( "../tmp/Test/TextFile/fbuild.fdb" ) );

    // Check stats
    //               Seen,  Built,  Type
    CheckStatsNode ( 1,     1,      Node::TEXT_FILE_NODE );
    CheckStatsNode ( 1,     1,      Node::ALIAS_NODE );
    CheckStatsTotal( 2,     2 );
}

// Build_NoRebuild
//------------------------------------------------------------------------------
void TestTextFile::Build_NoRebuild() const
{
    FBuildTestOptions options;
    options.m_ConfigFile = "Tools/FBuild/FBuildTest/Data/TestTextFile/fbuild.bff";
    FBuild fBuild( options );
    TEST_ASSERT( fBuild.Initialize( "../tmp/Test/TextFile/fbuild.fdb" ) );

    // build (via alias)
    TEST_ASSERT( fBuild.Build( "TextFile" ) );

    // Check stats
    //               Seen,  Built,  Type
    CheckStatsNode ( 1,     0,      Node::TEXT_FILE_NODE );
    CheckStatsNode ( 1,     1,      Node::ALIAS_NODE );
    CheckStatsTotal( 2,     1 );
}

// Build_NoRebuild_BFFChange
//------------------------------------------------------------------------------
void TestTextFile::Build_NoRebuild_BFFChange() const
{
    FBuildTestOptions options;
    options.m_ConfigFile = "Tools/FBuild/FBuildTest/Data/TestTextFile/fbuild.bff";
    options.m_ForceDBMigration_Debug = true; // Force bff modified re-parse

    FBuild fBuild( options );
    TEST_ASSERT( fBuild.Initialize( "../tmp/Test/TextFile/fbuild.fdb" ) );

    // build (via alias)
    TEST_ASSERT( fBuild.Build( "TextFile" ) );

    // Check stats
    //               Seen,  Built,  Type
    CheckStatsNode ( 1,     0,      Node::TEXT_FILE_NODE );
    CheckStatsNode ( 1,     1,      Node::ALIAS_NODE );
    CheckStatsTotal( 2,     1 );
}

//------------------------------------------------------------------------------
