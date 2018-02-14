// TestLinkerDependencies.cpp
//------------------------------------------------------------------------------

// Includes
//------------------------------------------------------------------------------
#include "FBuildTest.h"

#include "Tools/FBuild/FBuildCore/FBuild.h"

#include "Core/FileIO/FileIO.h"
#include "Core/Strings/AStackString.h"

// TestLinkerDependencies
//------------------------------------------------------------------------------
class TestLinkerDependencies : public FBuildTest
{
private:
    DECLARE_TESTS

    void TestParseBFF() const;
    void TestBuildExe() const;

    const char * GetBuildExeDBFileName() const { return "../tmp/Test/LinkerDependencies/exe.fdb"; }
};

// Register Tests
//------------------------------------------------------------------------------
REGISTER_TESTS_BEGIN( TestLinkerDependencies )
    REGISTER_TEST( TestParseBFF )
    REGISTER_TEST( TestBuildExe )
REGISTER_TESTS_END

// TestStackFramesEmpty
//------------------------------------------------------------------------------
void TestLinkerDependencies::TestParseBFF() const
{
    FBuildOptions options;
    options.m_ConfigFile = "Tools/FBuild/FBuildTest/Data/TestLinkerDependencies/fbuild.bff";

    FBuild fBuild( options );
    TEST_ASSERT( fBuild.Initialize() );
}

// TestBuildExe
//------------------------------------------------------------------------------
void TestLinkerDependencies::TestBuildExe() const
{
    FBuildOptions options;
    options.m_ShowSummary = true; // required to generate stats for node count checks
    options.m_ConfigFile = "Tools/FBuild/FBuildTest/Data/TestLinkerDependencies/fbuild.bff";

    FBuild fBuild( options );
    TEST_ASSERT( fBuild.Initialize() );

    const AStackString<> exe( "../tmp/Test/LinkerDependencies/exe.exe" );
    #if defined( __WINDOWS__ )
        const AStackString<> obj1( "../tmp/Test/LinkerDependencies/a.obj" );
    #else
        const AStackString<> obj1( "../tmp/Test/LinkerDependencies/a.o" );
    #endif

    // clean up anything left over from previous runs
    EnsureFileDoesNotExist( exe );
    EnsureFileDoesNotExist( obj1 );

    // Build
    TEST_ASSERT( fBuild.Build( exe ) );
    TEST_ASSERT( fBuild.SaveDependencyGraph( GetBuildExeDBFileName() ) );

    // make sure all output files are as expected
    EnsureFileExists( exe );
    EnsureFileExists( obj1 );

    // Check stats
    //               Seen,  Built,  Type
    CheckStatsNode ( 5,     4,      Node::FILE_NODE ); // 2 cpps + exe
    CheckStatsNode ( 1,     1,      Node::COMPILER_NODE );
    CheckStatsNode ( 2,     2,      Node::OBJECT_NODE );
    CheckStatsNode ( 1,     1,      Node::LIBRARY_NODE );
    CheckStatsNode ( 1,     1,      Node::EXE_NODE );
    CheckStatsTotal( 11,    10 );
}
