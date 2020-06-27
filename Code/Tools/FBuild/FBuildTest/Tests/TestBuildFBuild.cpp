// TestBuildFBuild.cpp
//------------------------------------------------------------------------------

// Includes
//------------------------------------------------------------------------------
#include "FBuildTest.h"

#include "Tools/FBuild/FBuildCore/FBuild.h"
#include "Tools/FBuild/FBuildCore/Graph/NodeGraph.h"

#include "Core/FileIO/FileIO.h"
#include "Core/FileIO/MemoryStream.h"
#include "Core/Strings/AStackString.h"
#include "Core/Tracing/Tracing.h"

// TestBuildFBuild
//------------------------------------------------------------------------------
class TestBuildFBuild : public FBuildTest
{
private:
    DECLARE_TESTS

    // Helpers
    FBuildStats BuildInternal( FBuildTestOptions options = FBuildTestOptions(), bool useDB = true, bool forceMigration = false ) const;
    const char * GetDBFile() const { return "../tmp/Test/BuildFBuild/TestFBuild.db"; }

    // Tests
    void BuildClean() const;
    void Build_NoRebuild() const;
    void Build_NoRebuild_BFFChange() const;
    void BuildCleanWithCache() const;

    void DBSavePerformance() const;
};

// Register Tests
//------------------------------------------------------------------------------
REGISTER_TESTS_BEGIN( TestBuildFBuild )
    REGISTER_TEST( BuildClean )             // clean build (populating cache)
    REGISTER_TEST( Build_NoRebuild )        // check no rebuild
    REGISTER_TEST( Build_NoRebuild_BFFChange ) // check no rebuild (bff change)
    REGISTER_TEST( BuildCleanWithCache )    // clean, reading from cache
    REGISTER_TEST( Build_NoRebuild )        // check no rebuild again
    REGISTER_TEST( Build_NoRebuild_BFFChange ) // check no rebuild again (bff change)

    REGISTER_TEST( DBSavePerformance )      // Time to save a non-trivial DB
REGISTER_TESTS_END

// BuildInternal
//------------------------------------------------------------------------------
FBuildStats TestBuildFBuild::BuildInternal( FBuildTestOptions options, bool useDB, bool forceMigration ) const
{
    options.m_ConfigFile = "Tools/FBuild/FBuildTest/Data/TestBuildFBuild/fbuild.bff";
    options.m_ForceDBMigration_Debug = forceMigration;

    FBuild fBuild( options );
    TEST_ASSERT( fBuild.Initialize( useDB ? GetDBFile() : nullptr ) );

    // Build a subset of targets as a sort of smoke test
    Array< AString > targets;
    #if defined( __WINDOWS__ )
        targets.EmplaceBack( "All-x64-Debug" );
        targets.EmplaceBack( "All-x64Clang-Release" );
    #elif defined( __LINUX__ )
        targets.EmplaceBack( "All-x64Linux-Debug" );
        targets.EmplaceBack( "All-x64ClangLinux-Release" );
    #elif defined( __OSX__ )
        targets.EmplaceBack( "All-x64OSX-Debug" );
        targets.EmplaceBack( "All-x64OSX-Release" );
    #else
        #error Unknown platform
    #endif

    TEST_ASSERT( fBuild.Build( targets ) );

    // save the db file - make sure it exists
    TEST_ASSERT( fBuild.SaveDependencyGraph( GetDBFile() ) );
    EnsureFileExists( GetDBFile() );

    return fBuild.GetStats();
}

// BuildClean
//------------------------------------------------------------------------------
void TestBuildFBuild::BuildClean() const
{
    // delete files from previous runs
    Array< AString > files( 1024, true );
    FileIO::GetFiles( AStackString<>( "../tmp/Test/BuildFBuild" ), AStackString<>( "*" ), true, &files );
    for ( Array< AString >::Iter it = files.Begin();
          it != files.End();
          it++ )
    {
        FileIO::FileDelete( (*it).Get() );
    }

    // Do a clean build and populate the cache
    FBuildTestOptions options;
    options.m_ForceCleanBuild = true;
    options.m_UseCacheWrite = true;
    FBuildStats stats = BuildInternal( options, false ); // don't use the DB so it's a clean build

    // check everything was built and stored to the cache
    const FBuildStats::Stats & objStats = stats.GetStatsFor( Node::OBJECT_NODE );
    TEST_ASSERT( objStats.m_NumProcessed > 10 ); // not exact so we don't have to update it
    TEST_ASSERT( objStats.m_NumBuilt == objStats.m_NumProcessed ); // everything rebuilt

    #if defined( __WINDOWS__ )
        // One windows, 2 .res files are built which can't be stored, and everything else can
        TEST_ASSERT( objStats.m_NumCacheStores == ( objStats.m_NumBuilt - 2 ) );
    #else
        TEST_ASSERT( objStats.m_NumCacheStores == objStats.m_NumBuilt ); // everything stored to the cache
    #endif
}

// Build_NoRebuild
//------------------------------------------------------------------------------
void TestBuildFBuild::Build_NoRebuild() const
{
    // ensure nothing is rebuilt
    FBuildStats stats = BuildInternal();

    // check nothing rebuilt
    TEST_ASSERT( stats.GetStatsFor( Node::OBJECT_NODE ).m_NumBuilt == 0 );
    TEST_ASSERT( stats.GetStatsFor( Node::LIBRARY_NODE ).m_NumBuilt == 0 );
    TEST_ASSERT( stats.GetStatsFor( Node::EXE_NODE ).m_NumBuilt == 0 );
}

// Build_NoRebuild_BFFChange
//------------------------------------------------------------------------------
void TestBuildFBuild::Build_NoRebuild_BFFChange() const
{
    // ensure nothing is rebuilt
    const bool forceMigration = true;
    FBuildStats stats = BuildInternal( FBuildTestOptions(), true, forceMigration );

    // check nothing rebuilt
    TEST_ASSERT( stats.GetStatsFor( Node::OBJECT_NODE ).m_NumBuilt == 0 );
    TEST_ASSERT( stats.GetStatsFor( Node::LIBRARY_NODE ).m_NumBuilt == 0 );
    TEST_ASSERT( stats.GetStatsFor( Node::EXE_NODE ).m_NumBuilt == 0 );
}

// BuildCleanWithCache
//------------------------------------------------------------------------------
void TestBuildFBuild::BuildCleanWithCache() const
{
    FBuildTestOptions options;
    options.m_ForceCleanBuild = true;
    options.m_UseCacheRead = true;
    FBuildStats stats = BuildInternal( options );

    // test everything was retrieved from the cache
    const FBuildStats::Stats & objStats = stats.GetStatsFor( Node::OBJECT_NODE );
    TEST_ASSERT( objStats.m_NumProcessed > 10 ); // not exact so we don't have to update it
    #if defined( __WINDOWS__ )
        // One windows, 2 .res files are built, and everything else comes from the cache
        TEST_ASSERT( objStats.m_NumBuilt == 2 );
        TEST_ASSERT( objStats.m_NumCacheHits == ( objStats.m_NumProcessed - 2 ) );
    #else
        TEST_ASSERT( objStats.m_NumBuilt == 0 ); // nothing built
        TEST_ASSERT( objStats.m_NumCacheHits == objStats.m_NumProcessed ); // everything read from cache
    #endif
}

// DBSavePerformance
//------------------------------------------------------------------------------
void TestBuildFBuild::DBSavePerformance() const
{
    FBuildTestOptions options;
    options.m_ConfigFile = "Tools/FBuild/FBuildTest/Data/TestBuildFBuild/fbuild.bff";

    FBuild fBuild( options );
    TEST_ASSERT( fBuild.Initialize( GetDBFile() ) );

    MemoryStream ms( 64 * 1024 * 1024, 1024 * 1024 );

    Timer t;
    for ( size_t i = 0; i < 100; ++i )
    {
        ms.Reset();
        fBuild.SaveDependencyGraph( ms, "unused.fdb" );
    }

    const float time = ( t.GetElapsed() * 1000.0f ) / 100.0f;
    OUTPUT( "DB Save Time: %2.3f ms\n", (double)time );
}

//------------------------------------------------------------------------------
