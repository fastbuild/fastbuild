// TestCache.cpp
//------------------------------------------------------------------------------

// Includes
//------------------------------------------------------------------------------
#include "FBuildTest.h"
#include "Tools/FBuild/FBuildCore/FBuild.h"
#include "Tools/FBuild/FBuildCore/Graph/SettingsNode.h"
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
    void CacheCustomCompilerOutput() const;
};

// Register Tests
//------------------------------------------------------------------------------
REGISTER_TESTS_BEGIN( TestCache )
	REGISTER_TEST(Write)
	REGISTER_TEST(Read)
	REGISTER_TEST(ReadWrite)
	REGISTER_TEST(CacheCustomCompilerOutput)
REGISTER_TESTS_END

// Write
//------------------------------------------------------------------------------
void TestCache::Write() const
{
    FBuildTestOptions options;
    options.m_ForceCleanBuild = true;
    options.m_UseCacheWrite = true;
    options.m_CacheVerbose = true;
    options.m_ConfigFile = "Tools/FBuild/FBuildTest/Data/TestCache/cache.bff";

    FBuild fBuild( options );
    TEST_ASSERT( fBuild.Initialize() );

    TEST_ASSERT( fBuild.Build( AStackString<>( "ObjectList" ) ) );

    // Ensure cache was written to
    const FBuildStats::Stats & objStats = fBuild.GetStats().GetStatsFor( Node::OBJECT_NODE );
    TEST_ASSERT( objStats.m_NumCacheStores == 2 );
    TEST_ASSERT( objStats.m_NumBuilt == 2 );
}

// Read
//------------------------------------------------------------------------------
void TestCache::Read() const
{
    FBuildTestOptions options;
    options.m_ForceCleanBuild = true;
    options.m_UseCacheRead = true;
    options.m_CacheVerbose = true;
    options.m_ConfigFile = "Tools/FBuild/FBuildTest/Data/TestCache/cache.bff";

    FBuild fBuild( options );
    TEST_ASSERT( fBuild.Initialize() );

    TEST_ASSERT( fBuild.Build( AStackString<>( "ObjectList" ) ) );

    // Ensure cache was written to
    const FBuildStats::Stats & objStats = fBuild.GetStats().GetStatsFor( Node::OBJECT_NODE );
    TEST_ASSERT( objStats.m_NumCacheHits == 2 );
    TEST_ASSERT( objStats.m_NumBuilt == 0 );
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
    options.m_ConfigFile = "Tools/FBuild/FBuildTest/Data/TestCache/cache.bff";

    FBuild fBuild( options );
    TEST_ASSERT( fBuild.Initialize() );

    TEST_ASSERT( fBuild.Build( AStackString<>( "ObjectList" ) ) );

    // Ensure cache was written to
    const FBuildStats::Stats & objStats = fBuild.GetStats().GetStatsFor( Node::OBJECT_NODE );
    TEST_ASSERT( objStats.m_NumCacheHits == 2 );
    TEST_ASSERT( objStats.m_NumBuilt == 0 );
}



// CacheCustomCompilerOutput
//------------------------------------------------------------------------------
void TestCache::CacheCustomCompilerOutput() const
{
	FBuildTestOptions options;
	options.m_ForceCleanBuild = true;
	options.m_UseCacheRead = true;
	options.m_UseCacheWrite = true;
	options.m_ShowInfo = true;
	options.m_ConfigFile = "Tools/FBuild/FBuildTest/Data/TestCache/compiler.bff";

	// Create SimpleCompiler
	{
		FBuild fBuild(options);
		TEST_ASSERT(fBuild.Initialize());

		TEST_ASSERT(fBuild.Build(AStackString<>("all")));
	}


	options.m_ConfigFile = "Tools/FBuild/FBuildTest/Data/TestCache/cache.bff";

	// Test cache not being filled
	{
		FBuild fBuild(options);
		TEST_ASSERT(fBuild.Initialize());

		TEST_ASSERT(fBuild.Build(AStackString<>("CompileResources-NonCaching")));
		TEST_ASSERT(fBuild.GetStats().GetCacheStores() == 0);
		TEST_ASSERT(fBuild.GetStats().GetCacheHits() == 0);
	}

	// Test cache getting filled
	{
		FBuild fBuild(options);
		TEST_ASSERT(fBuild.Initialize());

		TEST_ASSERT(fBuild.Build(AStackString<>("CompileResources-Caching")));
		TEST_ASSERT(fBuild.GetStats().GetCacheStores() == 1);
		TEST_ASSERT(fBuild.GetStats().GetCacheHits() == 0);
	}

	// Test cache getting filled
	{
		options.m_UseCacheWrite = false;
		FBuild fBuild(options);
		TEST_ASSERT(fBuild.Initialize());

		TEST_ASSERT(fBuild.Build(AStackString<>("CompileResources-Caching")));
		TEST_ASSERT(fBuild.GetStats().GetCacheStores() == 0);
		TEST_ASSERT(fBuild.GetStats().GetCacheHits() == 1);
	}
}
//------------------------------------------------------------------------------
