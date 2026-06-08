// TestBuildFBuild.cpp
//------------------------------------------------------------------------------

// Includes
//------------------------------------------------------------------------------
#include "FBuildTest.h"

#include "Tools/FBuild/FBuildCore/FBuild.h"
#include "Tools/FBuild/FBuildCore/Graph/NodeGraph.h"

#include "Core/FileIO/ChainedMemoryStream.h"
#include "Core/FileIO/FileIO.h"
#include "Core/Strings/AStackString.h"
#include "Core/Tracing/Tracing.h"

//------------------------------------------------------------------------------
TEST_GROUP( TestBuildFBuild, FBuildTest )
{
public:
    // Helpers
    FBuildStats BuildInternal( FBuildTestOptions options = FBuildTestOptions(), bool useDB = true, bool forceMigration = false ) const;
    const char * GetDBFile() const
    {
        return "../tmp/Test/BuildFBuild/TestFBuild.db";
    }
};

//------------------------------------------------------------------------------
FBuildStats TestBuildFBuild::BuildInternal( FBuildTestOptions options, bool useDB, bool forceMigration ) const
{
    options.m_ConfigFile = "Tools/FBuild/FBuildTest/Data/TestBuildFBuild/fbuild.bff";
    options.m_ForceDBMigration_Debug = forceMigration;

    FBuildForTest fBuild( options );
    TEST_ASSERT( fBuild.Initialize( useDB ? GetDBFile() : nullptr ) );

    // Build a subset of targets as a sort of smoke test
    Array<AString> targets;
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

//------------------------------------------------------------------------------
TEST_CASE( TestBuildFBuild, BuildClean )
{
    // delete files from previous runs
    Array<AString> files;
    files.SetCapacity( 1024 );
    FileIO::GetFiles( AStackString( "../tmp/Test/BuildFBuild" ), AStackString( "*" ), true, &files );
    for ( const AString & file : files )
    {
        FileIO::FileDelete( file.Get() );
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

//------------------------------------------------------------------------------
TEST_CASE( TestBuildFBuild, Build_NoRebuild )
{
    // ensure nothing is rebuilt
    FBuildStats stats = BuildInternal();

    // check nothing rebuilt
    TEST_ASSERT( stats.GetStatsFor( Node::OBJECT_NODE ).m_NumBuilt == 0 );
    TEST_ASSERT( stats.GetStatsFor( Node::LIBRARY_NODE ).m_NumBuilt == 0 );
    TEST_ASSERT( stats.GetStatsFor( Node::EXE_NODE ).m_NumBuilt == 0 );
}

//------------------------------------------------------------------------------
TEST_CASE( TestBuildFBuild, Build_NoRebuild_BFFChange )
{
    // ensure nothing is rebuilt
    const bool forceMigration = true;
    FBuildStats stats = BuildInternal( FBuildTestOptions(), true, forceMigration );

    // check nothing rebuilt
    TEST_ASSERT( stats.GetStatsFor( Node::OBJECT_NODE ).m_NumBuilt == 0 );
    TEST_ASSERT( stats.GetStatsFor( Node::LIBRARY_NODE ).m_NumBuilt == 0 );
    TEST_ASSERT( stats.GetStatsFor( Node::EXE_NODE ).m_NumBuilt == 0 );
}

//------------------------------------------------------------------------------
TEST_CASE( TestBuildFBuild, BuildCleanWithCache )
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

//------------------------------------------------------------------------------
TEST_CASE( TestBuildFBuild, DBSavePerformance )
{
    FBuildTestOptions options;
    options.m_ConfigFile = "Tools/FBuild/FBuildTest/Data/TestBuildFBuild/fbuild.bff";

    FBuildForTest fBuild( options );
    TEST_ASSERT( fBuild.Initialize( GetDBFile() ) );

    const Timer t;
    for ( size_t i = 0; i < 100; ++i )
    {
        ChainedMemoryStream ms( 8 * 1024 * 1024 );
        fBuild.SaveDependencyGraph( ms, "unused.fdb" );
    }

    const float time = ( t.GetElapsed() * 1000.0f ) / 100.0f;
    OUTPUT( "DB Save Time: %2.3f ms\n", (double)time );
}

//------------------------------------------------------------------------------
