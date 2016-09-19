// TestCLR.cpp
//------------------------------------------------------------------------------

// Includes
//------------------------------------------------------------------------------
#include "FBuildTest.h"

#include "Tools/FBuild/FBuildCore/FBuild.h"
#include "Tools/FBuild/FBuildCore/BFF/BFFParser.h"

#include "Core/FileIO/FileIO.h"
#include "Core/Process/Process.h"
#include "Core/Strings/AStackString.h"

// TestCLR
//------------------------------------------------------------------------------
class TestCLR : public FBuildTest
{
private:
    DECLARE_TESTS

    // Helpers
    FBuildStats Build( FBuildOptions options, bool useDB, const char * target ) const;
    const char * GetTestDBFileName() const { return "../../../../tmp/Test/CLR/test.fdb"; }

    // Tests
    void Test() const;
    void Test_NoBuild() const;
    void TestCache() const;

    void TestParallelBuild() const;
    void TestParallelBuild_NoBuild() const;

    void TestCLRToCPPBridge() const;
};

// Register Tests
//------------------------------------------------------------------------------
REGISTER_TESTS_BEGIN( TestCLR )
    REGISTER_TEST( Test )           // clean build, populate cache
    REGISTER_TEST( Test_NoBuild )   // check nothing rebuilds
    REGISTER_TEST( TestCache )      // clean build, read from cache
    REGISTER_TEST( Test_NoBuild )   // check nothing rebuilds (again)

    REGISTER_TEST( TestParallelBuild ) // build several clr files in parallel
    REGISTER_TEST( TestParallelBuild_NoBuild ) // check nothing rebuilds

    REGISTER_TEST( TestCLRToCPPBridge ) // Linking C++ and CLR
REGISTER_TESTS_END

// Test
//------------------------------------------------------------------------------
FBuildStats TestCLR::Build( FBuildOptions options, bool useDB, const char * target ) const
{
    options.m_ConfigFile = "Data/TestCLR/clr.bff";

    FBuild fBuild( options );
    TEST_ASSERT( fBuild.Initialize( useDB ? GetTestDBFileName() : nullptr ) );

    // Build it
    TEST_ASSERT( fBuild.Build( AStackString<>( target ) ) );
    TEST_ASSERT( fBuild.SaveDependencyGraph( GetTestDBFileName() ) );

    return fBuild.GetStats();
}

// Test
//------------------------------------------------------------------------------
void TestCLR::Test() const
{
    FBuildOptions options;
    options.m_ForceCleanBuild = true;
    options.m_UseCacheWrite = true;
    options.m_ShowSummary = true; // required to generate stats for node count checks

    EnsureFileDoesNotExist( "../../../../tmp/Test/CLR/clr.lib" );

    FBuildStats stats = Build( options, false, "CLR-Target" ); // dont' use DB

    EnsureFileExists( "../../../../tmp/Test/CLR/clr.lib" );

    // Check stats
    //               Seen,  Built,  Type
    CheckStatsNode ( stats, 3,      1,      Node::FILE_NODE );  // cpp
    CheckStatsNode ( stats, 1,      1,      Node::COMPILER_NODE );
    CheckStatsNode ( stats, 1,      1,      Node::OBJECT_NODE );
    CheckStatsNode ( stats, 1,      1,      Node::LIBRARY_NODE );
    CheckStatsNode ( stats, 1,      1,      Node::ALIAS_NODE );
    CheckStatsTotal( stats, 7,      5 );

    TEST_ASSERT( stats.GetCacheStores() == 0 ); // cache not supported due to compiler bug
}

// Test_NoBuild
//------------------------------------------------------------------------------
void TestCLR::Test_NoBuild() const
{
    FBuildOptions options;
    options.m_ShowSummary = true; // required to generate stats for node count checks
    FBuildStats stats = Build( options, true, "CLR-Target" );

    // Check stats
    //                      Seen,   Built,  Type
    CheckStatsNode ( stats, 3,      3,      Node::FILE_NODE );  // cpp + h + mscorlib
    CheckStatsNode ( stats, 1,      0,      Node::COMPILER_NODE );
    CheckStatsNode ( stats, 1,      0,      Node::OBJECT_NODE );
    CheckStatsNode ( stats, 1,      0,      Node::LIBRARY_NODE );
    CheckStatsNode ( stats, 1,      1,      Node::ALIAS_NODE );
    CheckStatsTotal( stats, 7,      4 );
}

// TestCache
//------------------------------------------------------------------------------
void TestCLR::TestCache() const
{
    FBuildOptions options;
    options.m_ForceCleanBuild = true;
    options.m_UseCacheRead = true;
    options.m_ShowSummary = true; // required to generate stats for node count checks

    EnsureFileDoesNotExist( "../../../../tmp/Test/CLR/clr.lib" );

    FBuildStats stats = Build( options, false, "CLR-Target" ); // dont' use DB

    EnsureFileExists( "../../../../tmp/Test/CLR/clr.lib" );

    // Check stats
    //               Seen,  Built,  Type
    CheckStatsNode ( stats, 3,      1,      Node::FILE_NODE );  // cpp
    CheckStatsNode ( stats, 1,      1,      Node::COMPILER_NODE );
    CheckStatsNode ( stats, 1,      1,      Node::OBJECT_NODE );
    CheckStatsNode ( stats, 1,      1,      Node::LIBRARY_NODE );
    CheckStatsNode ( stats, 1,      1,      Node::ALIAS_NODE );
    CheckStatsTotal( stats, 7,      5 );

    TEST_ASSERT( stats.GetCacheHits() == 0 ); // cache not supported dur to compiler bug
}

// TestParallelBuild
//------------------------------------------------------------------------------
void TestCLR::TestParallelBuild() const
{
    FBuildOptions options;
    options.m_ForceCleanBuild = true;
    options.m_ShowSummary = true; // required to generate stats for node count checks

    EnsureFileDoesNotExist( "../../../../tmp/Test/CLR/clrmulti.lib" );

    FBuildStats stats = Build( options, false, "CLR-Parallel-Target" ); // dont' use DB

    EnsureFileExists( "../../../../tmp/Test/CLR/clrmulti.lib" );

    // Check stats
    //               Seen,  Built,  Type
    CheckStatsNode ( stats, 1,      1,      Node::DIRECTORY_LIST_NODE );
    CheckStatsNode ( stats, 5,      3,      Node::FILE_NODE );  // 3xcpp + .h
    CheckStatsNode ( stats, 1,      1,      Node::COMPILER_NODE );
    CheckStatsNode ( stats, 3,      3,      Node::OBJECT_NODE );
    CheckStatsNode ( stats, 1,      1,      Node::LIBRARY_NODE );
    CheckStatsNode ( stats, 1,      1,      Node::ALIAS_NODE );
    CheckStatsTotal( stats, 12,     10 );
}

// TestParallelBuild_NoBuild
//------------------------------------------------------------------------------
void TestCLR::TestParallelBuild_NoBuild() const
{
    FBuildOptions options;
    options.m_ShowSummary = true; // required to generate stats for node count checks

    FBuildStats stats = Build( options, true, "CLR-Parallel-Target" );

    // Check stats
    //                      Seen,   Built,  Type
    CheckStatsNode ( stats, 1,      1,      Node::DIRECTORY_LIST_NODE );
    CheckStatsNode ( stats, 5,      5,      Node::FILE_NODE );  // 3xcpp + mscorlib.dll
    CheckStatsNode ( stats, 1,      0,      Node::COMPILER_NODE );
    CheckStatsNode ( stats, 3,      0,      Node::OBJECT_NODE );
    CheckStatsNode ( stats, 1,      0,      Node::LIBRARY_NODE );
    CheckStatsNode ( stats, 1,      1,      Node::ALIAS_NODE );
    CheckStatsTotal( stats, 12,     7 );
}

// TestCLRToCPPBridge
//------------------------------------------------------------------------------
void TestCLR::TestCLRToCPPBridge() const
{
    FBuildOptions options;
    options.m_ForceCleanBuild = true;
    options.m_ShowSummary = true; // required to generate stats for node count checks

    FBuildStats stats = Build( options, true, "BridgeTest-Exe" );

    Process p;
    p.Spawn( "../../../../tmp/Test/CLR/Bridge/Bridge.exe", nullptr, nullptr, nullptr );
    int ret = p.WaitForExit();
    TEST_ASSERT( ret == 15613223 ); // verify expected ret code
}

//------------------------------------------------------------------------------
