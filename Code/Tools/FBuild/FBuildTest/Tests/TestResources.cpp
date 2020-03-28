// TestResources.cpp
//------------------------------------------------------------------------------

// Includes
//------------------------------------------------------------------------------
#include "Tools/FBuild/FBuildTest/Tests/FBuildTest.h"

#include "Tools/FBuild/FBuildCore/FBuild.h"

#include "Core/FileIO/FileIO.h"
#include "Core/Process/Process.h"
#include "Core/Strings/AStackString.h"

// TestResources
//------------------------------------------------------------------------------
class TestResources : public FBuildTest
{
private:
    DECLARE_TESTS

    void BuildResource() const;
    void BuildResource_NoRebuild() const;
};

// Register Tests
//------------------------------------------------------------------------------
REGISTER_TESTS_BEGIN( TestResources )
    REGISTER_TEST( BuildResource )
    REGISTER_TEST( BuildResource_NoRebuild )
REGISTER_TESTS_END

// BuildResource
//------------------------------------------------------------------------------
void TestResources::BuildResource() const
{
    FBuildTestOptions options;
    options.m_ConfigFile = "Tools/FBuild/FBuildTest/Data/TestResources/fbuild.bff";
    options.m_ForceCleanBuild = true;

    FBuild fBuild( options );
    TEST_ASSERT( fBuild.Initialize() );

    const AStackString<> binRes( "../tmp/Test/Resources/resource.res" );

    // clean up anything left over from previous runs
    EnsureFileDoesNotExist( "binRes" );

    TEST_ASSERT( fBuild.Build( "exe" ) );
    TEST_ASSERT( fBuild.SaveDependencyGraph( "../tmp/Test/Resources/resource.fdb" ) );

    // make sure all output files are as expected
    EnsureFileExists( binRes );

    // spawn exe which does a runtime check that the resource is availble
    Process p;
    p.Spawn( "../tmp/Test/Resources/exe.exe", nullptr, nullptr, nullptr );
    int ret = p.WaitForExit();
    TEST_ASSERT( ret == 1 ); // verify expected ret code

    // Check stats
    //               Seen,  Built,  Type
    // NOTE: Don't test file nodes since test used windows.h
    CheckStatsNode ( 2,     2,      Node::OBJECT_NODE );
    CheckStatsNode ( 1,     1,      Node::OBJECT_LIST_NODE );
    CheckStatsNode ( 1,     1,      Node::LIBRARY_NODE );
    CheckStatsNode ( 1,     1,      Node::ALIAS_NODE );
    CheckStatsNode ( 1,     1,      Node::EXE_NODE );
}

// BuildResource_NoRebuild
//------------------------------------------------------------------------------
void TestResources::BuildResource_NoRebuild() const
{
    FBuildTestOptions options;
    options.m_ConfigFile = "Tools/FBuild/FBuildTest/Data/TestResources/fbuild.bff";

    FBuild fBuild( options );
    TEST_ASSERT( fBuild.Initialize( "../tmp/Test/Resources/resource.fdb" ) );

    TEST_ASSERT( fBuild.Build( "exe" ) );

    // Check stats
    //               Seen,  Built,  Type
    // NOTE: Don't test file nodes since test used windows.h
    CheckStatsNode ( 2,     0,      Node::OBJECT_NODE );
    CheckStatsNode ( 1,     0,      Node::OBJECT_LIST_NODE );
    CheckStatsNode ( 1,     0,      Node::LIBRARY_NODE );
    CheckStatsNode ( 1,     1,      Node::ALIAS_NODE );
    CheckStatsNode ( 1,     0,      Node::EXE_NODE );
}

//------------------------------------------------------------------------------
