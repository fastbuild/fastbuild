// TestZW.cpp
//------------------------------------------------------------------------------

// Includes
//------------------------------------------------------------------------------
#include "FBuildTest.h"

#include "Core/Strings/AStackString.h"
#include "Tools/FBuild/FBuildCore/BFF/BFFParser.h"
#include "Tools/FBuild/FBuildCore/FBuild.h"

// TestZW
//------------------------------------------------------------------------------
class TestZW : public FBuildTest
{
private:
    DECLARE_TESTS

    // Tests
    #if defined( __WINDOWS__ )
        void Caching() const;
    #endif
};

// Register Tests
//------------------------------------------------------------------------------
REGISTER_TESTS_BEGIN( TestZW )
    #if defined( __WINDOWS__ )
        REGISTER_TEST( Caching )
    #endif
REGISTER_TESTS_END

// Caching
//------------------------------------------------------------------------------
#if defined( __WINDOWS__ )
    void TestZW::Caching() const
    {
        FBuildTestOptions baseOptions;
        baseOptions.m_ConfigFile = "Tools/FBuild/FBuildTest/Data/TestZW/Caching/fbuild.bff";

        const char * objFile = "../tmp/Test/ZW/Caching/file.obj";
        const char * dbFileName = "../tmp/Test/ZW/Caching/fbuild.fdb";

        // Build, writing to cache
        {
            FBuildOptions options( baseOptions );
            options.m_UseCacheWrite = true;
            options.m_ForceCleanBuild = true;

            FBuild fBuild( options );
            TEST_ASSERT( fBuild.Initialize() );

            EnsureFileDoesNotExist( objFile );

            TEST_ASSERT( fBuild.Build( "Caching" ) );
            TEST_ASSERT( fBuild.SaveDependencyGraph( dbFileName ) );

            EnsureFileExists( objFile );

            // Check we wrote to the cache
            const FBuildStats & stats = fBuild.GetStats();
            CheckStatsNode ( stats, 1,      1,      Node::OBJECT_NODE );
            TEST_ASSERT( stats.GetCacheStores() == 1 );
        }

        // No op
        {
            FBuild fBuild( baseOptions );
            TEST_ASSERT( fBuild.Initialize( dbFileName ) );
            TEST_ASSERT( fBuild.Build( "Caching" ) );

            // Ensure nothing was built
            const FBuildStats & stats = fBuild.GetStats();
            CheckStatsNode ( stats, 1,      0,      Node::OBJECT_NODE );
            TEST_ASSERT( stats.GetCacheStores() == 0 );
            TEST_ASSERT( stats.GetCacheHits() == 0 );
        }

        EnsureFileDoesNotExist( objFile );

        // Read from cache
        {
            FBuildOptions options( baseOptions );
            options.m_UseCacheRead = true;

            FBuild fBuild( options );
            TEST_ASSERT( fBuild.Initialize() );
            TEST_ASSERT( fBuild.Build( "Caching" ) );
            TEST_ASSERT( fBuild.SaveDependencyGraph( dbFileName ) );

            EnsureFileExists( objFile );

            // Ensure retrieval from cache
            const FBuildStats & stats = fBuild.GetStats();
            CheckStatsNode ( stats, 1,      0,      Node::OBJECT_NODE );
            TEST_ASSERT( stats.GetCacheStores() == 0 );
            TEST_ASSERT( stats.GetCacheHits() == 1 );
        }

        // No op (after cache retrieval)
        {
            FBuild fBuild( baseOptions );
            TEST_ASSERT( fBuild.Initialize( dbFileName ) );
            TEST_ASSERT( fBuild.Build( "Caching" ) );

            // Ensure nothing was built
            const FBuildStats & stats = fBuild.GetStats();
            CheckStatsNode ( stats, 1,      0,      Node::OBJECT_NODE );
            TEST_ASSERT( stats.GetCacheStores() == 0 );
            TEST_ASSERT( stats.GetCacheHits() == 0 );
        }
    }
#endif

//------------------------------------------------------------------------------
