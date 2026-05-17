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

//------------------------------------------------------------------------------
TEST_GROUP( TestTextFile, FBuildTest )
{
public:
};

//------------------------------------------------------------------------------
TEST_CASE( TestTextFile, Build )
{
    FBuildTestOptions options;
    options.m_ConfigFile = "Tools/FBuild/FBuildTest/Data/TestTextFile/fbuild.bff";
    options.m_ForceCleanBuild = true;
    FBuildForTest fBuild( options );
    TEST_ASSERT( fBuild.Initialize() );

    // build (via alias)
    TEST_ASSERT( fBuild.Build( "TextFile" ) );
    TEST_ASSERT( fBuild.SaveDependencyGraph( "../tmp/Test/TextFile/fbuild.fdb" ) );

    // Check stats: Seen, Built, Type
    CheckStatsNode( 1, 1, Node::TEXT_FILE_NODE );
    CheckStatsNode( 1, 1, Node::ALIAS_NODE );
    CheckStatsTotal( 2, 2 );
}

//------------------------------------------------------------------------------
TEST_CASE( TestTextFile, Build_NoRebuild )
{
    FBuildTestOptions options;
    options.m_ConfigFile = "Tools/FBuild/FBuildTest/Data/TestTextFile/fbuild.bff";
    FBuildForTest fBuild( options );
    TEST_ASSERT( fBuild.Initialize( "../tmp/Test/TextFile/fbuild.fdb" ) );

    // build (via alias)
    TEST_ASSERT( fBuild.Build( "TextFile" ) );

    // Check stats: Seen, Built, Type
    CheckStatsNode( 1, 0, Node::TEXT_FILE_NODE );
    CheckStatsNode( 1, 1, Node::ALIAS_NODE );
    CheckStatsTotal( 2, 1 );
}

//------------------------------------------------------------------------------
TEST_CASE( TestTextFile, Build_NoRebuild_BFFChange )
{
    FBuildTestOptions options;
    options.m_ConfigFile = "Tools/FBuild/FBuildTest/Data/TestTextFile/fbuild.bff";
    options.m_ForceDBMigration_Debug = true; // Force bff modified re-parse

    FBuildForTest fBuild( options );
    TEST_ASSERT( fBuild.Initialize( "../tmp/Test/TextFile/fbuild.fdb" ) );

    // build (via alias)
    TEST_ASSERT( fBuild.Build( "TextFile" ) );

    // Check stats: Seen, Built, Type
    CheckStatsNode( 1, 0, Node::TEXT_FILE_NODE );
    CheckStatsNode( 1, 1, Node::ALIAS_NODE );
    CheckStatsTotal( 2, 1 );
}

//------------------------------------------------------------------------------
