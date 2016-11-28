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
    FBuildStats BuildInternal( FBuildOptions options = FBuildOptions(), bool useDB = true ) const;
    const char * GetDBFile() const { return "../tmp/Test/BuildFBuild/TestFBuild.db"; }

    // Tests
    void BuildClean() const;
    void Build_NoRebuild() const;
    void BuildCleanWithCache() const;

    void DBSavePerformance() const;
};

// Register Tests
//------------------------------------------------------------------------------
REGISTER_TESTS_BEGIN( TestBuildFBuild )
    REGISTER_TEST( BuildClean )             // clean build (populating cache)
    REGISTER_TEST( Build_NoRebuild )        // check no rebuild
    REGISTER_TEST( BuildCleanWithCache )    // clean, reading from cache
    REGISTER_TEST( Build_NoRebuild )        // check no rebuild again

    REGISTER_TEST( DBSavePerformance )      // Time to save a non-trivial DB
REGISTER_TESTS_END

// BuildInternal
//------------------------------------------------------------------------------
FBuildStats TestBuildFBuild::BuildInternal( FBuildOptions options, bool useDB ) const
{
    AStackString<> codeDir;
    GetCodeDir( codeDir );

    options.m_ConfigFile = "Tools/FBuild/FBuildTest/Data/TestBuildFBuild/fbuild.bff";
    options.m_ShowSummary = true;
    options.SetWorkingDir( codeDir );

    FBuild fBuild( options );
    TEST_ASSERT( fBuild.Initialize( useDB ? GetDBFile() : nullptr ) );

    #if defined( __WINDOWS__ )
        const AStackString<> lib( "../tmp/Test/BuildFBuild/Win32/Debug/Core/core.lib" );
        const AStackString<> lib2( "../tmp/Test/BuildFBuild/Win32/Debug/FBuildCore/fbuildcore.lib" );
        const AStackString<> lib3( "../tmp/Test/BuildFBuild/Win32/Release/FBuildApp/fbuildapp.lib" );
        const AStackString<> exe( "../tmp/Test/BuildFBuild/Win32/Release/Tools/FBuild/FBuildApp/fbuild.exe" );
    #endif
    #if defined( __OSX__ )
        const AStackString<> lib( "../tmp/Test/BuildFBuild/OSX/Debug/Core/core.lib" );
        const AStackString<> lib2( "../tmp/Test/BuildFBuild/OSX/Debug/FBuildCore/fbuildcore.lib" );
        const AStackString<> lib3( "../tmp/Test/BuildFBuild/OSX/Release/FBuildApp/fbuildapp.lib" );
        const AStackString<> exe( "../tmp/Test/BuildFBuild/OSX/Release/Tools/FBuild/FBuildApp/fbuild.exe" );
    #endif
    #if defined( __LINUX__ )
        const AStackString<> lib( "../tmp/Test/BuildFBuild/Linux/Debug/Core/Core.lib" );
        const AStackString<> lib2( "../tmp/Test/BuildFBuild/Linux/Debug/FBuildCore/FBuildCore.lib" );
        const AStackString<> lib3( "../tmp/Test/BuildFBuild/Linux/Release/FBuildApp/FBuildApp.lib" );
        const AStackString<> exe( "../tmp/Test/BuildFBuild/Linux/Release/Tools/FBuild/FBuildApp/FBuild.exe" );
    #endif
    const AStackString<> target( "fbuild" );

    TEST_ASSERT( fBuild.Build( target ) );

    // make sure all output files are as expected
    EnsureFileExists( lib );
    EnsureFileExists( lib2 );
    EnsureFileExists( lib3 );
    EnsureFileExists( exe );

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
    FBuildOptions options;
    options.m_ForceCleanBuild = true;
    options.m_UseCacheWrite = true;
    FBuildStats stats = BuildInternal( options, false ); // don't use the DB so it's a clean build

    // check everything was built and stored to the cache
    const FBuildStats::Stats & objStats = stats.GetStatsFor( Node::OBJECT_NODE );
    TEST_ASSERT( objStats.m_NumProcessed > 10 ); // not exact so we don't have to update it
    TEST_ASSERT( objStats.m_NumBuilt == objStats.m_NumProcessed ); // everything rebuilt
    TEST_ASSERT( objStats.m_NumCacheStores == objStats.m_NumBuilt ); // everything stored to the cache
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

// BuildCleanWithCache
//------------------------------------------------------------------------------
void TestBuildFBuild::BuildCleanWithCache() const
{
    FBuildOptions options;
    options.m_ForceCleanBuild = true;
    options.m_UseCacheRead = true;
    FBuildStats stats = BuildInternal( options );

    // test everything was retrieved from the cache
    const FBuildStats::Stats & objStats = stats.GetStatsFor( Node::OBJECT_NODE );
    TEST_ASSERT( objStats.m_NumProcessed > 10 ); // not exact so we don't have to update it
    TEST_ASSERT( objStats.m_NumBuilt == 0 ); // nothing built
    TEST_ASSERT( objStats.m_NumCacheHits == objStats.m_NumProcessed ); // everything read from cache
}

// DBSavePerformance
//------------------------------------------------------------------------------
void TestBuildFBuild::DBSavePerformance() const
{
    AStackString<> codeDir;
    GetCodeDir( codeDir );

    FBuildOptions options;
    options.m_ConfigFile = "Tools/FBuild/FBuildTest/Data/TestBuildFBuild/fbuild.bff";
    options.SetWorkingDir( codeDir );

    FBuild fBuild( options );
    TEST_ASSERT( fBuild.Initialize( GetDBFile() ) );

    MemoryStream ms( 64 * 1024 * 1024 );

    Timer t;
    for ( size_t i=0; i<100; ++i )
    {
        fBuild.SaveDependencyGraph( ms, "unused.fdb" );
    }

    const float time = ( t.GetElapsed() * 1000.0f ) / 100.0f;
    OUTPUT( "DB Save Time: %2.3f ms\n", time);
}

//------------------------------------------------------------------------------
