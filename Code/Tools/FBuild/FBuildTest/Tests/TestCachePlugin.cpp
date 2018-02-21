// TestCachePlugin.cpp
//------------------------------------------------------------------------------

// Includes
//------------------------------------------------------------------------------
#include "FBuildTest.h"
#include "Tools/FBuild/FBuildCore/FBuild.h"
#include "Tools/FBuild/FBuildCore/Graph/SettingsNode.h"
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
    FBuildTestOptions options;
    options.m_ForceCleanBuild = true;
    options.m_ConfigFile = "Tools/FBuild/FBuildTest/Data/TestCachePlugin/buildplugin.bff";

    FBuild fBuild( options );
    TEST_ASSERT( fBuild.Initialize() );

    const bool is64bits = ( sizeof(void *) == 8 );
    TEST_ASSERT( fBuild.Build( AStackString<>( is64bits ? "Plugin-DLL-X64" : "Plugin-DLL-X86" ) ) );
}

// UsePlugin
//------------------------------------------------------------------------------
void TestCachePlugin::UsePlugin() const
{
    FBuildTestOptions options;
    options.m_ForceCleanBuild = true;
    options.m_UseCacheRead = true;
    options.m_UseCacheWrite = true;
    const bool is64bits = ( sizeof(void *) == 8 );
    options.m_ConfigFile = is64bits ? "Tools/FBuild/FBuildTest/Data/TestCachePlugin/useplugin.bff"
                                    : "Tools/FBuild/FBuildTest/Data/TestCachePlugin/useplugin-x86.bff";

    // Read
    {
        FBuild fBuild( options );
        TEST_ASSERT( fBuild.Initialize() );

        TEST_ASSERT( fBuild.Build( AStackString<>( "TestFiles-Lib" ) ) );
        TEST_ASSERT( fBuild.GetStats().GetCacheStores() == 1 );
        TEST_ASSERT( fBuild.GetStats().GetCacheHits() == 0 );
    }

    // Write
    {
        options.m_UseCacheWrite = false;

        FBuild fBuild( options );
        TEST_ASSERT( fBuild.Initialize() );

        TEST_ASSERT( fBuild.Build( AStackString<>( "TestFiles-Lib" ) ) );
        TEST_ASSERT( fBuild.GetStats().GetCacheStores() == 0 );
        TEST_ASSERT( fBuild.GetStats().GetCacheHits() == 0 );
    }

    // OutputInfo
    {
        FBuild fBuild( options );
        TEST_ASSERT( fBuild.Initialize() );
        TEST_ASSERT( fBuild.CacheOutputInfo() );
    }

    // Trim
    {
        FBuild fBuild( options );
        TEST_ASSERT( fBuild.Initialize() );
        TEST_ASSERT( fBuild.CacheTrim() );
    }
}

// PluginOptionsSavedToDB
//------------------------------------------------------------------------------
void TestCachePlugin::PluginOptionsSavedToDB() const
{
    FBuildTestOptions options;
    options.m_ConfigFile = "Tools/FBuild/FBuildTest/Data/TestCachePlugin/useplugin.bff";

    AStackString<> cachePath;
    AStackString<> cachePluginDLL;

    {
        // Init the DB from the BFF
        FBuild f( options );
        TEST_ASSERT( f.Initialize() );

        // sotre a copy of the cache params
        cachePath = f.GetSettings()->GetCachePath();
        cachePluginDLL = f.GetSettings()->GetCachePluginDLL();
        TEST_ASSERT( !cachePath.IsEmpty() );
        TEST_ASSERT( !cachePluginDLL.IsEmpty() );

        // save the db to disk
        TEST_ASSERT( f.SaveDependencyGraph( "../tmp/Test/CachePlugin/CachePlugin.fdb" ) );
    }

    {
        // reload from the db
        FBuild f( options );
        TEST_ASSERT( f.Initialize( "../tmp/Test/CachePlugin/CachePlugin.fdb" ) );

        // check that the cache params were persisted
        TEST_ASSERT( cachePath == f.GetSettings()->GetCachePath() );
        TEST_ASSERT( cachePluginDLL == f.GetSettings()->GetCachePluginDLL() );
    }
}

//------------------------------------------------------------------------------
