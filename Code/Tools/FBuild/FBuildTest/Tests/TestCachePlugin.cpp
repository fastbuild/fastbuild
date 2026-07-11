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

//------------------------------------------------------------------------------
TEST_GROUP( TestCachePlugin, FBuildTest )
{
public:
};

//------------------------------------------------------------------------------
TEST_CASE( TestCachePlugin, BuildPlugin )
{
    FBuildTestOptions options;
    options.m_ForceCleanBuild = true;
    options.m_ConfigFile = "Tools/FBuild/FBuildTest/Data/TestCachePlugin/Interface/buildplugin.bff";

    FBuildForTest fBuild( options );
    TEST_ASSERT( fBuild.Initialize() );

    TEST_ASSERT( fBuild.Build( "Plugin-DLL-X64" ) );
}

//------------------------------------------------------------------------------
TEST_CASE( TestCachePlugin, UsePlugin )
{
    FBuildTestOptions options;
    options.m_ForceCleanBuild = true;
    options.m_UseCacheRead = true;
    options.m_UseCacheWrite = true;
    options.m_ConfigFile = "Tools/FBuild/FBuildTest/Data/TestCachePlugin/Interface/useplugin.bff";

    // Write
    {
        FBuildForTest fBuild( options );
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

        FBuildForTest fBuild( options );
        TEST_ASSERT( fBuild.Initialize() );

        TEST_ASSERT( fBuild.Build( "TestFiles-Lib" ) );
        TEST_ASSERT( fBuild.GetStats().GetCacheStores() == 0 );
        TEST_ASSERT( fBuild.GetStats().GetCacheHits() == 0 );

        TEST_ASSERT( GetRecordedOutput().Find( "CacheRetrieve Called" ) );
    }

    // OutputInfo
    {
        FBuildForTest fBuild( options );
        TEST_ASSERT( fBuild.Initialize() );
        TEST_ASSERT( fBuild.CacheOutputInfo() );

        TEST_ASSERT( GetRecordedOutput().Find( "CacheOutputInfo Called" ) );
    }

    // Trim
    {
        FBuildForTest fBuild( options );
        TEST_ASSERT( fBuild.Initialize() );
        TEST_ASSERT( fBuild.CacheTrim() );

        TEST_ASSERT( GetRecordedOutput().Find( "CacheTrim Called" ) );
    }
}

//------------------------------------------------------------------------------
TEST_CASE( TestCachePlugin, PluginOptionsSavedToDB )
{
    FBuildTestOptions options;
    options.m_ConfigFile = "Tools/FBuild/FBuildTest/Data/TestCachePlugin/OldInterface/useplugin.bff";

    AStackString cachePath;
    AStackString cachePluginDLL;

    {
        // Init the DB from the BFF
        FBuildForTest f( options );
        TEST_ASSERT( f.Initialize() );

        // store a copy of the cache params
        cachePath = f.GetSettings()->GetCachePath();
        cachePluginDLL = f.GetSettings()->GetCachePluginDLL();
        TEST_ASSERT( !cachePath.IsEmpty() );
        TEST_ASSERT( !cachePluginDLL.IsEmpty() );

        // save the db to disk
        TEST_ASSERT( f.SaveDependencyGraph( "../tmp/Test/CachePlugin/CachePlugin.fdb" ) );
    }

    {
        // reload from the db
        FBuildForTest f( options );
        TEST_ASSERT( f.Initialize( "../tmp/Test/CachePlugin/CachePlugin.fdb" ) );

        // check that the cache params were persisted
        TEST_ASSERT( cachePath == f.GetSettings()->GetCachePath() );
        TEST_ASSERT( cachePluginDLL == f.GetSettings()->GetCachePluginDLL() );
    }
}

//------------------------------------------------------------------------------
#if defined( __WINDOWS__ )
TEST_CASE( TestCachePlugin, BuildPlugin_Old )
{
    FBuildTestOptions options;
    options.m_ForceCleanBuild = true;
    options.m_ConfigFile = "Tools/FBuild/FBuildTest/Data/TestCachePlugin/OldInterface/buildplugin.bff";

    FBuildForTest fBuild( options );
    TEST_ASSERT( fBuild.Initialize() );

    TEST_ASSERT( fBuild.Build( "Plugin-DLL-X64" ) );
}
#endif

//------------------------------------------------------------------------------
#if defined( __WINDOWS__ )
TEST_CASE( TestCachePlugin, UsePlugin_Old )
{
    FBuildTestOptions options;
    options.m_ForceCleanBuild = true;
    options.m_UseCacheRead = true;
    options.m_UseCacheWrite = true;
    options.m_ConfigFile = "Tools/FBuild/FBuildTest/Data/TestCachePlugin/OldInterface/useplugin.bff";

    // Read
    {
        FBuildForTest fBuild( options );
        TEST_ASSERT( fBuild.Initialize() );
        TEST_ASSERT( GetRecordedOutput().Find( "Missing CachePluginDLL function" ) == nullptr );

        TEST_ASSERT( fBuild.Build( "TestFiles-Lib" ) );
        TEST_ASSERT( fBuild.GetStats().GetCacheStores() == 1 );
        TEST_ASSERT( fBuild.GetStats().GetCacheHits() == 0 );
    }

    // Write
    {
        options.m_UseCacheWrite = false;

        FBuildForTest fBuild( options );
        TEST_ASSERT( fBuild.Initialize() );

        TEST_ASSERT( fBuild.Build( "TestFiles-Lib" ) );
        TEST_ASSERT( fBuild.GetStats().GetCacheStores() == 0 );
        TEST_ASSERT( fBuild.GetStats().GetCacheHits() == 0 );
    }

    // OutputInfo
    {
        FBuildForTest fBuild( options );
        TEST_ASSERT( fBuild.Initialize() );
        TEST_ASSERT( fBuild.CacheOutputInfo() );
    }

    // Trim
    {
        FBuildForTest fBuild( options );
        TEST_ASSERT( fBuild.Initialize() );
        TEST_ASSERT( fBuild.CacheTrim() );
    }
}
#endif

//------------------------------------------------------------------------------
