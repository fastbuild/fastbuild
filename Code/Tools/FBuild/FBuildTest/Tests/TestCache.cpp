// TestExe.cpp
//------------------------------------------------------------------------------

// Includes
//------------------------------------------------------------------------------
#include "FBuildTest.h"
#include "Tools/FBuild/FBuildCore/FBuild.h"
//#include "Tools/FBuild/FBuildCore/Graph/ExeNode.h"
//#include "Tools/FBuild/FBuildCore/Graph/NodeGraph.h"

//#include "Core/FileIO/FileIO.h"
//#include "Core/Process/Process.h"
#include "Core/Strings/AStackString.h"

// TestCache
//------------------------------------------------------------------------------
class TestCache : public FBuildTest
{
private:
    DECLARE_TESTS

    void CacheCustomCompilerOutput() const;
};

// Register Tests
//------------------------------------------------------------------------------
REGISTER_TESTS_BEGIN( TestCache )
    REGISTER_TEST( CacheCustomCompilerOutput )
REGISTER_TESTS_END

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
