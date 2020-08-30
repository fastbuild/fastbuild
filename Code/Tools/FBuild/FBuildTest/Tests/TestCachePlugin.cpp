// TestCachePlugin.cpp
//------------------------------------------------------------------------------

// Includes
//------------------------------------------------------------------------------
#include "FBuildTest.h"

// FBuildCore
#include "Tools/FBuild/FBuildCore/FBuild.h"
#include "Tools/FBuild/FBuildCore/Graph/SettingsNode.h"

// Core
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

    // Ensure old plugins with only mangled names on Windows continue to work)
    void BuildPlugin_Old() const;
    void UsePlugin_Old() const;
};

// Register Tests
//------------------------------------------------------------------------------
REGISTER_TESTS_BEGIN( TestCachePlugin )
    REGISTER_TEST( BuildPlugin )
    REGISTER_TEST( UsePlugin )
    REGISTER_TEST( PluginOptionsSavedToDB )

    // Ensure old plugins with only mangled names on Windows continue to work)
    #if defined( __WINDOWS__ )
        REGISTER_TEST( BuildPlugin_Old )
        REGISTER_TEST( UsePlugin_Old )
    #endif
REGISTER_TESTS_END

// BuildPlugin
//------------------------------------------------------------------------------
void TestCachePlugin::BuildPlugin() const
{
    FBuildTestOptions options;
    options.m_ForceCleanBuild = true;
    options.m_ConfigFile = "Tools/FBuild/FBuildTest/Data/TestCachePlugin/Interface/buildplugin.bff";

    FBuild fBuild( options );
    TEST_ASSERT( fBuild.Initialize() );

    TEST_ASSERT( fBuild.Build( "Plugin-DLL-X64" ) );
}

// UsePlugin
//------------------------------------------------------------------------------
void TestCachePlugin::UsePlugin() const
{
    FBuildTestOptions options;
    options.m_ForceCleanBuild = true;
    options.m_UseCacheRead = true;
    options.m_UseCacheWrite = true;
    options.m_ConfigFile = "Tools/FBuild/FBuildTest/Data/TestCachePlugin/Interface/useplugin.bff";

    // Write
    {
        FBuild fBuild( options );
        TEST_ASSERT( fBuild.Initialize() );

        TEST_ASSERT( GetRecordedOutput().Find( "Missing CachePluginDLL function" ) == nullptr );

        TEST_ASSERT( GetRecordedOutput().Find( "CacheInitEx Called" ) );

        TEST_ASSERT( fBuild.Build( "TestFiles-Lib" ) );
        TEST_ASSERT( fBuild.GetStats().GetCacheStores() == 1 );
        TEST_ASSERT( fBuild.GetStats().GetCacheHits() == 0 );

        TEST_ASSERT( GetRecordedOutput().Find( "CachePublish Called" ) );
    }

    TEST_ASSERT( GetRecordedOutput().Find( "CacheShutdown Called" ) );

    // Read
    {
        options.m_UseCacheWrite = false;

        FBuild fBuild( options );
        TEST_ASSERT( fBuild.Initialize() );

        TEST_ASSERT( fBuild.Build( "TestFiles-Lib" ) );
        TEST_ASSERT( fBuild.GetStats().GetCacheStores() == 0 );
        TEST_ASSERT( fBuild.GetStats().GetCacheHits() == 0 );

        TEST_ASSERT( GetRecordedOutput().Find( "CacheRetrieve Called" ) );
    }

    // OutputInfo
    {
        FBuild fBuild( options );
        TEST_ASSERT( fBuild.Initialize() );
        TEST_ASSERT( fBuild.CacheOutputInfo() );

        TEST_ASSERT( GetRecordedOutput().Find( "CacheOutputInfo Called" ) );
    }

    // Trim
    {
        FBuild fBuild( options );
        TEST_ASSERT( fBuild.Initialize() );
        TEST_ASSERT( fBuild.CacheTrim() );

        TEST_ASSERT( GetRecordedOutput().Find( "CacheTrim Called" ) );
    }
}

// PluginOptionsSavedToDB
//------------------------------------------------------------------------------
void TestCachePlugin::PluginOptionsSavedToDB() const
{
    FBuildTestOptions options;
    options.m_ConfigFile = "Tools/FBuild/FBuildTest/Data/TestCachePlugin/OldInterface/useplugin.bff";

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

// BuildPlugin_Old
//------------------------------------------------------------------------------
void TestCachePlugin::BuildPlugin_Old() const
{
    FBuildTestOptions options;
    options.m_ForceCleanBuild = true;
    options.m_ConfigFile = "Tools/FBuild/FBuildTest/Data/TestCachePlugin/OldInterface/buildplugin.bff";

    FBuild fBuild( options );
    TEST_ASSERT( fBuild.Initialize() );

    TEST_ASSERT( fBuild.Build( "Plugin-DLL-X64" ) );
}

// UsePlugin_Old
//------------------------------------------------------------------------------
void TestCachePlugin::UsePlugin_Old() const
{
    FBuildTestOptions options;
    options.m_ForceCleanBuild = true;
    options.m_UseCacheRead = true;
    options.m_UseCacheWrite = true;
    options.m_ConfigFile = "Tools/FBuild/FBuildTest/Data/TestCachePlugin/OldInterface/useplugin.bff";

    // Read
    {
        FBuild fBuild( options );
        TEST_ASSERT( fBuild.Initialize() );
        TEST_ASSERT( GetRecordedOutput().Find( "Missing CachePluginDLL function" ) == nullptr );

        TEST_ASSERT( fBuild.Build( "TestFiles-Lib" ) );
        TEST_ASSERT( fBuild.GetStats().GetCacheStores() == 1 );
        TEST_ASSERT( fBuild.GetStats().GetCacheHits() == 0 );
    }

    // Write
    {
        options.m_UseCacheWrite = false;

        FBuild fBuild( options );
        TEST_ASSERT( fBuild.Initialize() );

        TEST_ASSERT( fBuild.Build( "TestFiles-Lib" ) );
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

//------------------------------------------------------------------------------
