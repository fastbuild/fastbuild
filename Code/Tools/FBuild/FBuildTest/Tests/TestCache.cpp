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
};

// Register Tests
//------------------------------------------------------------------------------
REGISTER_TESTS_BEGIN( TestCache )
    REGISTER_TEST( Write )
    REGISTER_TEST( Read )
    REGISTER_TEST( ReadWrite )
    REGISTER_TEST( LightCache_IncludeUsingMacro )
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
        TEST_ASSERT( objStats.m_NumCacheStores == 2 );
        TEST_ASSERT( objStats.m_NumBuilt == 2 );

        numDepsA = fBuild.GetRecursiveDependencyCount( "ObjectList" );
        TEST_ASSERT( numDepsA > 0 );
    }

    // Light cache
    size_t numDepsB = 0;
    {
        PROFILE_SECTION( "Light" )

        options.m_ConfigFile = "Tools/FBuild/FBuildTest/Data/TestCache/lightcache.bff";

        FBuildForTest fBuild( options );
        TEST_ASSERT( fBuild.Initialize() );

        TEST_ASSERT( fBuild.Build( AStackString<>( "ObjectList" ) ) );

        // Ensure cache was written to
        const FBuildStats::Stats & objStats = fBuild.GetStats().GetStatsFor( Node::OBJECT_NODE );
        TEST_ASSERT( objStats.m_NumCacheStores == 2 );
        TEST_ASSERT( objStats.m_NumBuilt == 2 );

        numDepsB = fBuild.GetRecursiveDependencyCount( "ObjectList" );
        TEST_ASSERT( numDepsB > 0 );
    }

    TEST_ASSERT( numDepsB >= numDepsA );

    // Ensure LightCache did not fail
    TEST_ASSERT( GetRecordedOutput().Find( "Light cache cannot be used for" ) == nullptr );
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
        TEST_ASSERT( objStats.m_NumCacheHits == 2 );
        TEST_ASSERT( objStats.m_NumBuilt == 0 );

        numDepsA = fBuild.GetRecursiveDependencyCount( "ObjectList" );
        TEST_ASSERT( numDepsA > 0 );
    }

    // Light cache
    size_t numDepsB = 0;
    {
        PROFILE_SECTION( "Light" )

        options.m_ConfigFile = "Tools/FBuild/FBuildTest/Data/TestCache/lightcache.bff";

        FBuildForTest fBuild( options );
        TEST_ASSERT( fBuild.Initialize() );

        TEST_ASSERT( fBuild.Build( AStackString<>( "ObjectList" ) ) );

        // Ensure cache was written to
        const FBuildStats::Stats & objStats = fBuild.GetStats().GetStatsFor( Node::OBJECT_NODE );
        TEST_ASSERT( objStats.m_NumCacheHits == 2 );
        TEST_ASSERT( objStats.m_NumBuilt == 0 );

        numDepsB = fBuild.GetRecursiveDependencyCount( "ObjectList" );
        TEST_ASSERT( numDepsB > 0 );
    }

    TEST_ASSERT( numDepsB >= numDepsA );

    // Ensure LightCache did not fail
    TEST_ASSERT( GetRecordedOutput().Find( "Light cache cannot be used for" ) == nullptr );
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
        TEST_ASSERT( objStats.m_NumCacheHits == 2 );
        TEST_ASSERT( objStats.m_NumBuilt == 0 );

        numDepsA = fBuild.GetRecursiveDependencyCount( "ObjectList" );
        TEST_ASSERT( numDepsA > 0 );
    }

    // Light cache
    size_t numDepsB = 0;
    {
        PROFILE_SECTION( "Light" )

        options.m_ConfigFile = "Tools/FBuild/FBuildTest/Data/TestCache/lightcache.bff";

        FBuildForTest fBuild( options );
        TEST_ASSERT( fBuild.Initialize() );

        TEST_ASSERT( fBuild.Build( AStackString<>( "ObjectList" ) ) );

        // Ensure cache was written to
        const FBuildStats::Stats & objStats = fBuild.GetStats().GetStatsFor( Node::OBJECT_NODE );
        TEST_ASSERT( objStats.m_NumCacheHits == 2 );
        TEST_ASSERT( objStats.m_NumBuilt == 0 );

        numDepsB = fBuild.GetRecursiveDependencyCount( "ObjectList" );
        TEST_ASSERT( numDepsB > 0 );
    }

    TEST_ASSERT( numDepsB >= numDepsA );

    // Ensure LightCache did not fail
    TEST_ASSERT( GetRecordedOutput().Find( "Light cache cannot be used for" ) == nullptr );
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

//------------------------------------------------------------------------------
