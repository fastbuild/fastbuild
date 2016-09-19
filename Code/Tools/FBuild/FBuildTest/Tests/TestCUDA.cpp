// TestCUDA.cpp
//------------------------------------------------------------------------------

// Includes
//------------------------------------------------------------------------------
#include "FBuildTest.h"
#include "Tools/FBuild/FBuildCore/FBuild.h"
//#include "Tools/FBuild/FBuildCore/Graph/ExeNode.h"
#include "Tools/FBuild/FBuildCore/Graph/NodeGraph.h"

//#include "Core/FileIO/FileIO.h"
#include "Core/Strings/AStackString.h"

// TestCUDA
//------------------------------------------------------------------------------
class TestCUDA : public FBuildTest
{
private:
    DECLARE_TESTS

    void Build() const;
    void Build_NoRebuild() const;
    void Build_CacheHit() const;
};

// Register Tests
//------------------------------------------------------------------------------
REGISTER_TESTS_BEGIN( TestCUDA )
    REGISTER_TEST( Build )
    REGISTER_TEST( Build_NoRebuild )
    REGISTER_TEST( Build_CacheHit )
REGISTER_TESTS_END

// Build
//------------------------------------------------------------------------------
void TestCUDA::Build() const
{
    FBuildOptions options;
    options.m_ConfigFile = "Data/TestCUDA/cuda.bff";
    options.m_ForceCleanBuild = true;
    options.m_UseCacheWrite = true;
    options.m_ShowSummary = true; // required to generate stats for node count checks
    FBuild fBuild( options );
    TEST_ASSERT( fBuild.Initialize() );

    const AStackString<> obj( "../../../../tmp/Test/CUDA/test.obj" );

    // clean up anything left over from previous runs
    EnsureFileDoesNotExist( obj );

    // build (via alias)
    TEST_ASSERT( fBuild.Build( AStackString<>( "CUDA-Obj" ) ) );
    TEST_ASSERT( fBuild.SaveDependencyGraph( "../../../../tmp/Test/CUDA/cuda.fdb" ) );

    // make sure all output is where it is expected
    EnsureFileExists( obj );

    // Check stats
    //               Seen,  Built,  Type
    CheckStatsNode ( 65,    1,      Node::FILE_NODE ); // many included files
    CheckStatsNode ( 1,     1,      Node::COMPILER_NODE );
    CheckStatsNode ( 1,     1,      Node::OBJECT_NODE );
    CheckStatsNode ( 1,     1,      Node::OBJECT_LIST_NODE );
    CheckStatsTotal( 68,    4 );

    // Test cache was stored to
    const FBuildStats::Stats & objStats = fBuild.GetStats().GetStatsFor( Node::OBJECT_NODE );
    TEST_ASSERT( objStats.m_NumCacheStores == 1 );
}

// Build_NoRebuild
//------------------------------------------------------------------------------
void TestCUDA::Build_NoRebuild() const
{
    FBuildOptions options;
    options.m_ConfigFile = "Data/TestCUDA/cuda.bff";
    options.m_ShowSummary = true; // required to generate stats for node count checks
    FBuild fBuild( options );
    TEST_ASSERT( fBuild.Initialize( "../../../../tmp/Test/CUDA/cuda.fdb" ) );

    const AStackString<> obj( "../../../../tmp/Test/CUDA/test.obj" );

    // build (via alias)
    TEST_ASSERT( fBuild.Build( AStackString<>( "CUDA-Obj" ) ) );

    // make sure all output is where it is expected
    EnsureFileExists( obj );

    // Check stats
    //               Seen,  Built,  Type
    CheckStatsNode ( 65,    65,     Node::FILE_NODE ); // many included files
    CheckStatsNode ( 1,     0,      Node::COMPILER_NODE );
    CheckStatsNode ( 1,     0,      Node::OBJECT_NODE );
    CheckStatsNode ( 1,     0,      Node::OBJECT_LIST_NODE );
    CheckStatsTotal( 68,    65 );
}

// Build_CacheHit
//------------------------------------------------------------------------------
void TestCUDA::Build_CacheHit() const
{
    FBuildOptions options;
    options.m_ConfigFile = "Data/TestCUDA/cuda.bff";
    options.m_UseCacheRead = true;
    options.m_ShowSummary = true; // required to generate stats for node count checks
    FBuild fBuild( options );
    TEST_ASSERT( fBuild.Initialize( "../../../../tmp/Test/CUDA/cuda.fdb" ) );

    const AStackString<> obj( "../../../../tmp/Test/CUDA/test.obj" );

    // clean up anything left over from previous runs
    EnsureFileDoesNotExist( obj );

    // build (via alias)
    TEST_ASSERT( fBuild.Build( AStackString<>( "CUDA-Obj" ) ) );

    // make sure all output is where it is expected
    EnsureFileExists( obj );

    // Check stats
    //               Seen,  Built,  Type
    CheckStatsNode ( 65,    65,     Node::FILE_NODE ); // many included files
    CheckStatsNode ( 1,     0,      Node::COMPILER_NODE );
    CheckStatsNode ( 1,     0,      Node::OBJECT_NODE );
    CheckStatsNode ( 1,     1,      Node::OBJECT_LIST_NODE );
    CheckStatsTotal( 68,    66 );

    // Test we got a cache hit
    const FBuildStats::Stats & objStats = fBuild.GetStats().GetStatsFor( Node::OBJECT_NODE );
    TEST_ASSERT( objStats.m_NumCacheHits == 1 );
}

//------------------------------------------------------------------------------
