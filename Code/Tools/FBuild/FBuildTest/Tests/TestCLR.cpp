// TestCLR.cpp
//------------------------------------------------------------------------------
#if defined( __WINDOWS__ )

// Includes
//------------------------------------------------------------------------------
    #include "FBuildTest.h"

// FBuildCore
    #include "Tools/FBuild/FBuildCore/BFF/BFFParser.h"
    #include "Tools/FBuild/FBuildCore/FBuild.h"
    #include "Tools/FBuild/FBuildCore/Graph/ObjectNode.h"

    #include "Core/FileIO/FileIO.h"
    #include "Core/Process/Process.h"
    #include "Core/Strings/AStackString.h"

//------------------------------------------------------------------------------
TEST_GROUP( TestCLR, FBuildTest )
{
public:
    // Helpers
    FBuildStats Build( FBuildTestOptions options, bool useDB, const char * target ) const;
    const char * GetTestDBFileName() const
    {
        return "../tmp/Test/CLR/test.fdb";
    }
};

//------------------------------------------------------------------------------
FBuildStats TestCLR::Build( FBuildTestOptions options, bool useDB, const char * target ) const
{
    options.m_ConfigFile = "Tools/FBuild/FBuildTest/Data/TestCLR/clr.bff";

    FBuildForTest fBuild( options );
    TEST_ASSERT( fBuild.Initialize( useDB ? GetTestDBFileName() : nullptr ) );

    // Build it
    TEST_ASSERT( fBuild.Build( target ) );
    TEST_ASSERT( fBuild.SaveDependencyGraph( GetTestDBFileName() ) );

    return fBuild.GetStats();
}

//------------------------------------------------------------------------------
TEST_CASE( TestCLR, CLRDetection )
{
    // CLR code cannot be distributed or cached. Check the compiler args to determine
    // if CLR is used.

    // Init
    FBuildTestOptions options;
    options.m_ConfigFile = "Tools/FBuild/FBuildTest/Data/TestCLR/CLRDetection/fbuild.bff";
    FBuildForTest fBuild( options );
    TEST_ASSERT( fBuild.Initialize() );

    // Build (so ObjectNodes are created)
    // (we don't care if the build actually passed or not)
    fBuild.Build( "all" );

    // Get ObjectNodes
    StackArray<const Node *> nodes;
    fBuild.GetNodesOfType( Node::OBJECT_NODE, nodes );
    TEST_ASSERT( nodes.GetSize() == 2 );

    // Check flags
    for ( const Node * node : nodes )
    {
        // Ensure CLR was detected
        TEST_ASSERT( node->CastTo<ObjectNode>()->IsUsingCLR() );

        // Ensure distribution is disabled
        TEST_ASSERT( node->CastTo<ObjectNode>()->IsDistributable() == false );

        // Ensure caching is disabled
        TEST_ASSERT( node->CastTo<ObjectNode>()->IsCacheable() == false );
    }
}

//------------------------------------------------------------------------------
TEST_CASE( TestCLR, Test )
{
    FBuildTestOptions options;
    options.m_ForceCleanBuild = true;
    options.m_UseCacheWrite = true;

    EnsureFileDoesNotExist( "../tmp/Test/CLR/clr.lib" );

    FBuildStats stats = Build( options, false, "CLR-Target" ); // don't use DB

    EnsureFileExists( "../tmp/Test/CLR/clr.lib" );

    // Check stats: Seen, Built, Type
    CheckStatsNode( stats, 4, 2, Node::FILE_NODE );  // cpp + librarian
    CheckStatsNode( stats, 1, 1, Node::COMPILER_NODE );
    CheckStatsNode( stats, 1, 1, Node::OBJECT_NODE );
    CheckStatsNode( stats, 1, 1, Node::LIBRARY_NODE );
    CheckStatsNode( stats, 1, 1, Node::ALIAS_NODE );
    CheckStatsTotal( stats, 8, 6 );

    TEST_ASSERT( stats.GetCacheStores() == 0 ); // cache not supported due to compiler bug
}

//------------------------------------------------------------------------------
TEST_CASE( TestCLR, Test_NoBuild )
{
    FBuildTestOptions options;
    FBuildStats stats = Build( options, true, "CLR-Target" );

    // Check stats: Seen, Built, Type
    CheckStatsNode( stats, 4, 4, Node::FILE_NODE );  // cpp + h + mscorlib + librarian
    CheckStatsNode( stats, 1, 0, Node::COMPILER_NODE );
    CheckStatsNode( stats, 1, 0, Node::OBJECT_NODE );
    CheckStatsNode( stats, 1, 0, Node::LIBRARY_NODE );
    CheckStatsNode( stats, 1, 1, Node::ALIAS_NODE );
    CheckStatsTotal( stats, 8, 5 );
}

//------------------------------------------------------------------------------
TEST_CASE( TestCLR, TestCache )
{
    FBuildTestOptions options;
    options.m_ForceCleanBuild = true;
    options.m_UseCacheRead = true;

    EnsureFileDoesNotExist( "../tmp/Test/CLR/clr.lib" );

    FBuildStats stats = Build( options, false, "CLR-Target" ); // don't use DB

    EnsureFileExists( "../tmp/Test/CLR/clr.lib" );

    // Check stats: Seen, Built, Type
    CheckStatsNode( stats, 4, 2, Node::FILE_NODE );  // cpp + librarian
    CheckStatsNode( stats, 1, 1, Node::COMPILER_NODE );
    CheckStatsNode( stats, 1, 1, Node::OBJECT_NODE );
    CheckStatsNode( stats, 1, 1, Node::LIBRARY_NODE );
    CheckStatsNode( stats, 1, 1, Node::ALIAS_NODE );
    CheckStatsTotal( stats, 8, 6 );

    TEST_ASSERT( stats.GetCacheHits() == 0 ); // cache not supported dur to compiler bug
}

//------------------------------------------------------------------------------
TEST_CASE( TestCLR, TestParallelBuild )
{
    FBuildTestOptions options;
    options.m_ForceCleanBuild = true;

    EnsureFileDoesNotExist( "../tmp/Test/CLR/clrmulti.lib" );

    FBuildStats stats = Build( options, false, "CLR-Parallel-Target" ); // don't use DB

    EnsureFileExists( "../tmp/Test/CLR/clrmulti.lib" );

    // Check stats: Seen, Built, Type
    CheckStatsNode( stats, 1, 1, Node::DIRECTORY_LIST_NODE );
    CheckStatsNode( stats, 6, 4, Node::FILE_NODE );  // 3xcpp + librarian
    CheckStatsNode( stats, 1, 1, Node::COMPILER_NODE );
    CheckStatsNode( stats, 3, 3, Node::OBJECT_NODE );
    CheckStatsNode( stats, 1, 1, Node::LIBRARY_NODE );
    CheckStatsNode( stats, 1, 1, Node::ALIAS_NODE );
    CheckStatsTotal( stats, 13, 11 );
}

//------------------------------------------------------------------------------
TEST_CASE( TestCLR, TestParallelBuild_NoBuild )
{
    FBuildTestOptions options;

    FBuildStats stats = Build( options, true, "CLR-Parallel-Target" );

    // Check stats: Seen, Built, Type
    CheckStatsNode( stats, 1, 1, Node::DIRECTORY_LIST_NODE );
    CheckStatsNode( stats, 6, 6, Node::FILE_NODE );  // 3xcpp + mscorlib.dll + librarian
    CheckStatsNode( stats, 1, 0, Node::COMPILER_NODE );
    CheckStatsNode( stats, 3, 0, Node::OBJECT_NODE );
    CheckStatsNode( stats, 1, 0, Node::LIBRARY_NODE );
    CheckStatsNode( stats, 1, 1, Node::ALIAS_NODE );
    CheckStatsTotal( stats, 13, 8 );
}

//------------------------------------------------------------------------------
    #if 0 // TODO:B FIX this test
TEST_CASE( TestCLR, TestCLRToCPPBridge )
{
    FBuildTestOptions options;
    options.m_ForceCleanBuild = true;

    Build( options, true, "BridgeTest-Exe" );

    Process p;
    p.Spawn( "../tmp/Test/CLR/Bridge/Bridge.exe", nullptr, nullptr, nullptr );
    int ret = p.WaitForExit();
    TEST_ASSERT( ret == 15613223 ); // verify expected ret code
}
    #endif

//------------------------------------------------------------------------------
#endif
