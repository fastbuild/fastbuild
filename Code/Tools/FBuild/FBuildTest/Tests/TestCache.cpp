// TestCache.cpp
//------------------------------------------------------------------------------

// Includes
//------------------------------------------------------------------------------
#include "FBuildTest.h"
#include "Tools/FBuild/FBuildCore/FBuild.h"
#include "Tools/FBuild/FBuildCore/Graph/SettingsNode.h"
#include "Core/Profile/Profile.h"
#include "Core/Strings/AStackString.h"

// TestCache
//------------------------------------------------------------------------------
class TestCache : public FBuildTest
{
private:
    DECLARE_TESTS

    void Write() const;
    void Read() const;
    void ReadWrite() const;

    void LightCache_IncludeUsingMacro() const;
    void LightCache_CyclicInclude() const;
};

// Register Tests
//------------------------------------------------------------------------------
REGISTER_TESTS_BEGIN( TestCache )
    REGISTER_TEST( Write )
    REGISTER_TEST( Read )
    REGISTER_TEST( ReadWrite )
    #if defined( __WINDOWS__ )
        REGISTER_TEST( LightCache_IncludeUsingMacro )
        REGISTER_TEST( LightCache_CyclicInclude )
    #endif
REGISTER_TESTS_END

// Write
//------------------------------------------------------------------------------
void TestCache::Write() const
{
    FBuildTestOptions options;
    options.m_ForceCleanBuild = true;
    options.m_UseCacheWrite = true;
    options.m_CacheVerbose = true;

    // Normal caching using compiler's preprocessor
    size_t numDepsA = 0;
    {
        PROFILE_SECTION( "Normal" )

        options.m_ConfigFile = "Tools/FBuild/FBuildTest/Data/TestCache/cache.bff";

        FBuildForTest fBuild( options );
        TEST_ASSERT( fBuild.Initialize() );

        TEST_ASSERT( fBuild.Build( AStackString<>( "ObjectList" ) ) );

        // Ensure cache was written to
        const FBuildStats::Stats & objStats = fBuild.GetStats().GetStatsFor( Node::OBJECT_NODE );
        TEST_ASSERT( objStats.m_NumCacheStores == objStats.m_NumProcessed );
        TEST_ASSERT( objStats.m_NumBuilt == objStats.m_NumProcessed );

        numDepsA = fBuild.GetRecursiveDependencyCount( "ObjectList" );
        TEST_ASSERT( numDepsA > 0 );
    }

    // Light cache
    #if defined( __WINDOWS__ )
        size_t numDepsB = 0;
        {
            PROFILE_SECTION( "Light" )

            options.m_ConfigFile = "Tools/FBuild/FBuildTest/Data/TestCache/lightcache.bff";

            FBuildForTest fBuild( options );
            TEST_ASSERT( fBuild.Initialize() );

            TEST_ASSERT( fBuild.Build( AStackString<>( "ObjectList" ) ) );

            // Ensure cache was written to
            const FBuildStats::Stats & objStats = fBuild.GetStats().GetStatsFor( Node::OBJECT_NODE );
            TEST_ASSERT( objStats.m_NumCacheStores == objStats.m_NumProcessed );
            TEST_ASSERT( objStats.m_NumBuilt == objStats.m_NumProcessed );

            numDepsB = fBuild.GetRecursiveDependencyCount( "ObjectList" );
            TEST_ASSERT( numDepsB > 0 );
        }

        TEST_ASSERT( numDepsB >= numDepsA );

        // Ensure LightCache did not fail
        TEST_ASSERT( GetRecordedOutput().Find( "Light cache cannot be used for" ) == nullptr );
    #endif
}

// Read
//------------------------------------------------------------------------------
void TestCache::Read() const
{
    FBuildTestOptions options;
    options.m_ForceCleanBuild = true;
    options.m_UseCacheRead = true;
    options.m_CacheVerbose = true;

    // Normal caching using compiler's preprocessor
    size_t numDepsA = 0;
    {
        PROFILE_SECTION( "Normal" )

        options.m_ConfigFile = "Tools/FBuild/FBuildTest/Data/TestCache/cache.bff";

        FBuildForTest fBuild( options );
        TEST_ASSERT( fBuild.Initialize() );

        TEST_ASSERT( fBuild.Build( AStackString<>( "ObjectList" ) ) );

        // Ensure cache was written to
        const FBuildStats::Stats & objStats = fBuild.GetStats().GetStatsFor( Node::OBJECT_NODE );
        TEST_ASSERT( objStats.m_NumCacheHits == objStats.m_NumProcessed );
        TEST_ASSERT( objStats.m_NumBuilt == 0 );

        numDepsA = fBuild.GetRecursiveDependencyCount( "ObjectList" );
        TEST_ASSERT( numDepsA > 0 );
    }

    // Light cache
    #if defined( __WINDOWS__ )
        size_t numDepsB = 0;
        {
            PROFILE_SECTION( "Light" )

            options.m_ConfigFile = "Tools/FBuild/FBuildTest/Data/TestCache/lightcache.bff";

            FBuildForTest fBuild( options );
            TEST_ASSERT( fBuild.Initialize() );

            TEST_ASSERT( fBuild.Build( AStackString<>( "ObjectList" ) ) );

            // Ensure cache was written to
            const FBuildStats::Stats & objStats = fBuild.GetStats().GetStatsFor( Node::OBJECT_NODE );
            TEST_ASSERT( objStats.m_NumCacheHits == objStats.m_NumProcessed );
            TEST_ASSERT( objStats.m_NumBuilt == 0 );

            numDepsB = fBuild.GetRecursiveDependencyCount( "ObjectList" );
            TEST_ASSERT( numDepsB > 0 );
        }

        TEST_ASSERT( numDepsB >= numDepsA );

        // Ensure LightCache did not fail
        TEST_ASSERT( GetRecordedOutput().Find( "Light cache cannot be used for" ) == nullptr );
    #endif
}

// ReadWrite
//------------------------------------------------------------------------------
void TestCache::ReadWrite() const
{
    FBuildTestOptions options;
    options.m_ForceCleanBuild = true;
    options.m_UseCacheRead = true;
    options.m_UseCacheWrite = true;
    options.m_CacheVerbose = true;

    // Normal caching using compiler's preprocessor
    size_t numDepsA = 0;
    {
        PROFILE_SECTION( "Normal" )
        options.m_ConfigFile = "Tools/FBuild/FBuildTest/Data/TestCache/cache.bff";

        FBuildForTest fBuild( options );
        TEST_ASSERT( fBuild.Initialize() );

        TEST_ASSERT( fBuild.Build( AStackString<>( "ObjectList" ) ) );

        // Ensure cache was written to
        const FBuildStats::Stats & objStats = fBuild.GetStats().GetStatsFor( Node::OBJECT_NODE );
        TEST_ASSERT( objStats.m_NumCacheHits == objStats.m_NumProcessed );
        TEST_ASSERT( objStats.m_NumBuilt == 0 );

        numDepsA = fBuild.GetRecursiveDependencyCount( "ObjectList" );
        TEST_ASSERT( numDepsA > 0 );
    }

    // Light cache
    #if defined( __WINDOWS__ )
        size_t numDepsB = 0;
        {
            PROFILE_SECTION( "Light" )

            options.m_ConfigFile = "Tools/FBuild/FBuildTest/Data/TestCache/lightcache.bff";

            FBuildForTest fBuild( options );
            TEST_ASSERT( fBuild.Initialize() );

            TEST_ASSERT( fBuild.Build( AStackString<>( "ObjectList" ) ) );

            // Ensure cache was written to
            const FBuildStats::Stats & objStats = fBuild.GetStats().GetStatsFor( Node::OBJECT_NODE );
            TEST_ASSERT( objStats.m_NumCacheHits == objStats.m_NumProcessed );
            TEST_ASSERT( objStats.m_NumBuilt == 0 );

            numDepsB = fBuild.GetRecursiveDependencyCount( "ObjectList" );
            TEST_ASSERT( numDepsB > 0 );
        }

        TEST_ASSERT( numDepsB >= numDepsA );

        // Ensure LightCache did not fail
        TEST_ASSERT( GetRecordedOutput().Find( "Light cache cannot be used for" ) == nullptr );
    #endif
}

// LightCache_IncludeUsingMacro
//------------------------------------------------------------------------------
void TestCache::LightCache_IncludeUsingMacro() const
{
    FBuildTestOptions options;
    options.m_ForceCleanBuild = true;
    options.m_UseCacheWrite = true;
    options.m_CacheVerbose = true;
    options.m_ConfigFile = "Tools/FBuild/FBuildTest/Data/TestCache/LightCache_IncludeUsingMacro/fbuild.bff";

    FBuildForTest fBuild( options );
    TEST_ASSERT( fBuild.Initialize() );

    TEST_ASSERT( fBuild.Build( AStackString<>( "ObjectList" ) ) );

    // Ensure we detected that we could not use the LightCache
    TEST_ASSERT( GetRecordedOutput().Find( "Light cache cannot be used for" ) );

    // Ensure cache we fell back to normal caching
    const FBuildStats::Stats & objStats = fBuild.GetStats().GetStatsFor( Node::OBJECT_NODE );
    TEST_ASSERT( objStats.m_NumCacheStores == 1 );
}

// LightCache_CyclicInclude
//------------------------------------------------------------------------------
void TestCache::LightCache_CyclicInclude() const
{
    FBuildTestOptions options;
    options.m_ForceCleanBuild = true;
    options.m_UseCacheWrite = true;
    options.m_CacheVerbose = true;
    options.m_ConfigFile = "Tools/FBuild/FBuildTest/Data/TestCache/LightCache_CyclicInclude/fbuild.bff";

    {
        FBuildForTest fBuild( options );
        TEST_ASSERT( fBuild.Initialize() );

        TEST_ASSERT( fBuild.Build( AStackString<>( "ObjectList" ) ) );

        // Ensure cache we fell back to normal caching
        const FBuildStats::Stats & objStats = fBuild.GetStats().GetStatsFor( Node::OBJECT_NODE );
        TEST_ASSERT( objStats.m_NumCacheStores == objStats.m_NumProcessed );
        TEST_ASSERT( objStats.m_NumBuilt == objStats.m_NumProcessed );
    }

    {
        options.m_UseCacheWrite = false;
        options.m_UseCacheRead = true;

        FBuildForTest fBuild( options );
        TEST_ASSERT( fBuild.Initialize() );

        TEST_ASSERT( fBuild.Build( AStackString<>( "ObjectList" ) ) );

        // Ensure cache we fell back to normal caching
        const FBuildStats::Stats & objStats = fBuild.GetStats().GetStatsFor( Node::OBJECT_NODE );
        TEST_ASSERT( objStats.m_NumCacheHits == objStats.m_NumProcessed );
        TEST_ASSERT( objStats.m_NumBuilt == 0 );
    }
}

//------------------------------------------------------------------------------
