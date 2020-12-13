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
    NodeGraph ng;

    #if defined( __WINDOWS__ )
        AStackString<> exeName( "c:\\exe.exe" );
    #else
        AStackString<> exeName( "/tmp/exe.exe" );
    #endif
    const ExeNode * exeNode = ng.CreateExeNode( exeName );
    TEST_ASSERT( exeNode->GetType() == Node::EXE_NODE );
    TEST_ASSERT( ExeNode::GetTypeS() == Node::EXE_NODE );
    TEST_ASSERT( AStackString<>( "Exe" ) == exeNode->GetTypeName() );
}

// Build
//------------------------------------------------------------------------------
void TestExe::Build() const
{
    FBuildTestOptions options;
    options.m_ConfigFile = "Tools/FBuild/FBuildTest/Data/TestExe/exe.bff";
    options.m_ForceCleanBuild = true;
    FBuild fBuild( options );
    TEST_ASSERT( fBuild.Initialize() );

    const AStackString<> exe( "../tmp/Test/Exe/exe.exe" );

    // clean up anything left over from previous runs
    EnsureFileDoesNotExist( exe );

    // build (via alias)
    TEST_ASSERT( fBuild.Build( "Exe" ) );
    TEST_ASSERT( fBuild.SaveDependencyGraph( "../tmp/Test/Exe/exe.fdb" ) );

    // make sure all output is where it is expected
    EnsureFileExists( exe );

    // Check stats
    //               Seen,  Built,  Type
    CheckStatsNode ( 2,     2,      Node::FILE_NODE ); // cpp + linker exe
    CheckStatsNode ( 1,     1,      Node::COMPILER_NODE );
    CheckStatsNode ( 1,     1,      Node::OBJECT_NODE );
    CheckStatsNode ( 1,     1,      Node::OBJECT_LIST_NODE );
    CheckStatsNode ( 1,     1,      Node::EXE_NODE );
    CheckStatsNode ( 1,     1,      Node::ALIAS_NODE );
    CheckStatsTotal( 7,     7 );
}

// CheckValidExe
//------------------------------------------------------------------------------
void TestExe::CheckValidExe() const
{
    Process p;
    p.Spawn( "../tmp/Test/Exe/exe.exe", nullptr, nullptr, nullptr );
    int ret = p.WaitForExit();
    TEST_ASSERT( ret == 99 ); // verify expected ret code
}

// Build_NoRebuild
//------------------------------------------------------------------------------
void TestExe::Build_NoRebuild() const
{
    FBuildTestOptions options;
    options.m_ConfigFile = "Tools/FBuild/FBuildTest/Data/TestExe/exe.bff";
    FBuild fBuild( options );
    TEST_ASSERT( fBuild.Initialize( "../tmp/Test/Exe/exe.fdb" ) );

    // build (via alias)
    TEST_ASSERT( fBuild.Build( "Exe" ) );

    // Check stats
    //               Seen,  Built,  Type
    CheckStatsNode ( 2,     2,      Node::FILE_NODE ); // cpp + linker exe
    CheckStatsNode ( 1,     0,      Node::COMPILER_NODE );
    CheckStatsNode ( 1,     0,      Node::OBJECT_NODE );
    CheckStatsNode ( 1,     0,      Node::OBJECT_LIST_NODE );
    CheckStatsNode ( 1,     0,      Node::EXE_NODE );
    CheckStatsNode ( 1,     1,      Node::ALIAS_NODE );
    CheckStatsTotal( 7,     3 );

}

//------------------------------------------------------------------------------
