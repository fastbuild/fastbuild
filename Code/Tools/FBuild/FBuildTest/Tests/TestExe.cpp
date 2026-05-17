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

//------------------------------------------------------------------------------
TEST_GROUP( TestExe, FBuildTest )
{
public:
};

//------------------------------------------------------------------------------
TEST_CASE( TestExe, Build )
{
    FBuildTestOptions options;
    options.m_ConfigFile = "Tools/FBuild/FBuildTest/Data/TestExe/exe.bff";
    options.m_ForceCleanBuild = true;
    FBuildForTest fBuild( options );
    TEST_ASSERT( fBuild.Initialize() );

    const AStackString exe( "../tmp/Test/Exe/exe.exe" );

    // clean up anything left over from previous runs
    EnsureFileDoesNotExist( exe );

    // build (via alias)
    TEST_ASSERT( fBuild.Build( "Exe" ) );
    TEST_ASSERT( fBuild.SaveDependencyGraph( "../tmp/Test/Exe/exe.fdb" ) );

    // make sure all output is where it is expected
    EnsureFileExists( exe );

    // Check stats: Seen, Built, Type
    CheckStatsNode( 2, 2, Node::FILE_NODE ); // cpp + linker exe
    CheckStatsNode( 1, 1, Node::COMPILER_NODE );
    CheckStatsNode( 1, 1, Node::OBJECT_NODE );
    CheckStatsNode( 1, 1, Node::OBJECT_LIST_NODE );
    CheckStatsNode( 1, 1, Node::EXE_NODE );
    CheckStatsNode( 1, 1, Node::ALIAS_NODE );
    CheckStatsTotal( 7, 7 );
}

//------------------------------------------------------------------------------
TEST_CASE( TestExe, CheckValidExe )
{
    Process p;
    TEST_ASSERT( p.Spawn( "../tmp/Test/Exe/exe.exe", nullptr, nullptr, nullptr ) );
    const int ret = p.WaitForExit();
    TEST_ASSERT( ret == 99 ); // verify expected ret code
}

//------------------------------------------------------------------------------
TEST_CASE( TestExe, Build_NoRebuild )
{
    FBuildTestOptions options;
    options.m_ConfigFile = "Tools/FBuild/FBuildTest/Data/TestExe/exe.bff";
    FBuildForTest fBuild( options );
    TEST_ASSERT( fBuild.Initialize( "../tmp/Test/Exe/exe.fdb" ) );

    // build (via alias)
    TEST_ASSERT( fBuild.Build( "Exe" ) );

    // Check stats: Seen, Built, Type
    CheckStatsNode( 2, 2, Node::FILE_NODE ); // cpp + linker exe
    CheckStatsNode( 1, 0, Node::COMPILER_NODE );
    CheckStatsNode( 1, 0, Node::OBJECT_NODE );
    CheckStatsNode( 1, 0, Node::OBJECT_LIST_NODE );
    CheckStatsNode( 1, 0, Node::EXE_NODE );
    CheckStatsNode( 1, 1, Node::ALIAS_NODE );
    CheckStatsTotal( 7, 3 );
}

//------------------------------------------------------------------------------
