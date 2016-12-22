// TestCachePlugin.cpp
//------------------------------------------------------------------------------

// Includes
//------------------------------------------------------------------------------
#include "FBuildTest.h"
#include "Tools/FBuild/FBuildCore/FBuild.h"
#include "Core/Strings/AStackString.h"

// TestCachePlugin
//------------------------------------------------------------------------------
class TestCachePlugin : public FBuildTest
{
private:
    DECLARE_TESTS

    void BuildPlugin() const;
    void UsePlugin() const;
    void PluginOptionsSavedToDB() const;
};

// Register Tests
//------------------------------------------------------------------------------
REGISTER_TESTS_BEGIN( TestCachePlugin )
    REGISTER_TEST( BuildPlugin )
    REGISTER_TEST( UsePlugin )
    REGISTER_TEST( PluginOptionsSavedToDB )
REGISTER_TESTS_END

// BuildPlugin
//------------------------------------------------------------------------------
void TestCachePlugin::BuildPlugin() const
{
    FBuildOptions options;
    options.m_ForceCleanBuild = true;
    options.m_ConfigFile = "Data/TestCachePlugin/buildplugin.bff";

    FBuild fBuild( options );
    TEST_ASSERT( fBuild.Initialize() );

    const bool is64bits = ( sizeof(void *) == 8 );
    TEST_ASSERT( fBuild.Build( AStackString<>( is64bits ? "Plugin-DLL-X64" : "Plugin-DLL-X86" ) ) );
}

// UsePlugin
//------------------------------------------------------------------------------
void TestCachePlugin::UsePlugin() const
{
    FBuildOptions options;
    options.m_ForceCleanBuild = true;
    options.m_ShowSummary = true; // required to generate stats for node count checks
    options.m_UseCacheRead = true;
    options.m_UseCacheWrite = true;
    const bool is64bits = ( sizeof(void *) == 8 );
    options.m_ConfigFile = is64bits ? "Data/TestCachePlugin/useplugin.bff" : "Data/TestCachePlugin/useplugin-x86.bff";

    {
        FBuild fBuild( options );
        TEST_ASSERT( fBuild.Initialize() );

        TEST_ASSERT( fBuild.Build( AStackString<>( "TestFiles-Lib" ) ) );
        TEST_ASSERT( fBuild.GetStats().GetCacheStores() == 1 );
        TEST_ASSERT( fBuild.GetStats().GetCacheHits() == 0 );
    }

    {
        options.m_UseCacheWrite = false;

        FBuild fBuild( options );
        TEST_ASSERT( fBuild.Initialize() );

        TEST_ASSERT( fBuild.Build( AStackString<>( "TestFiles-Lib" ) ) );
        TEST_ASSERT( fBuild.GetStats().GetCacheStores() == 0 );
        TEST_ASSERT( fBuild.GetStats().GetCacheHits() == 0 );
    }
}

// PluginOptionsSavedToDB
//------------------------------------------------------------------------------
void TestCachePlugin::PluginOptionsSavedToDB() const
{
    FBuildOptions options;
    options.m_ConfigFile = "Data/TestCachePlugin/useplugin.bff";

    AStackString<> cachePath;
    AStackString<> cachePluginDLL;

    {
        // Init the DB from the BFF
        FBuild f( options );
        TEST_ASSERT( f.Initialize() );

        // sotre a copy of the cache params
        cachePath = f.GetCachePath();
        cachePluginDLL = f.GetCachePluginDLL();
        TEST_ASSERT( !cachePath.IsEmpty() );
        TEST_ASSERT( !cachePluginDLL.IsEmpty() );

        // save the db to disk
        TEST_ASSERT( f.SaveDependencyGraph( "../../../../tmp/Test/CachePlugin/CachePlugin.fdb" ) );
    }

    {
        // reload from the db
        FBuild f( options );
        TEST_ASSERT( f.Initialize( "../../../../tmp/Test/CachePlugin/CachePlugin.fdb" ) );

        // check that the cache params were persisted
        TEST_ASSERT( cachePath == f.GetCachePath() );
        TEST_ASSERT( cachePluginDLL == f.GetCachePluginDLL() );
    }
}

//------------------------------------------------------------------------------
