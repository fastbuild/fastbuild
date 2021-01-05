// TestCache.cpp
//------------------------------------------------------------------------------

// Includes
//------------------------------------------------------------------------------
#include "FBuildTest.h"

// FBuild
#include "Tools/FBuild/FBuildCore/FBuild.h"
#include "Tools/FBuild/FBuildCore/Graph/ObjectNode.h"
#include "Tools/FBuild/FBuildCore/Graph/SettingsNode.h"
#include "Tools/FBuild/FBuildCore/Protocol/Server.h"

// Core
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
    void ConsistentCacheKeysWithDist() const;

    void LightCache_IncludeUsingMacro() const;
    void LightCache_IncludeUsingMacro2() const;
    void LightCache_IncludeUsingMacro3() const;
    void LightCache_IncludeUsingUndefinedMacros1() const;
    void LightCache_IncludeUsingUndefinedMacros2() const;
    void LightCache_IncludeUsingUndefinedMacros3() const;
    void LightCache_IncludeHierarchy() const;
    void LightCache_CyclicInclude() const;
    void LightCache_ImportDirective() const;

    // MSVC Static Analysis tests
    const char* const mAnalyzeMSVCBFFPath = "Tools/FBuild/FBuildTest/Data/TestCache/Analyze_MSVC/fbuild.bff";
    const char* const mAnalyzeMSVCXMLFile1 = "../tmp/Test/Cache/Analyze_MSVC/Analyze+WarningsOnly/file1.nativecodeanalysis.xml";
    const char* const mAnalyzeMSVCXMLFile2 = "../tmp/Test/Cache/Analyze_MSVC/Analyze+WarningsOnly/file2.nativecodeanalysis.xml";
    void Analyze_MSVC_WarningsOnly_Write() const;
    void Analyze_MSVC_WarningsOnly_Read() const;
    void Analyze_MSVC_WarningsOnly_WriteFromDist() const;
    void Analyze_MSVC_WarningsOnly_ReadFromDist() const;

    // Helpers
    void CheckForDependencies( const FBuildForTest & fBuild, const char * files[], size_t numFiles ) const;
    void LightCache_IncludeUsingUndefinedMacros( const char * consfigFile,
                                                 bool expectedBuildResult,
                                                 bool expectedLightCacheUsage,
                                                 const char * lightCacheError ) const;

    TestCache & operator = ( TestCache & other ) = delete; // Avoid warnings about implicit deletion of operators
};

// Register Tests
//------------------------------------------------------------------------------
REGISTER_TESTS_BEGIN( TestCache )
    REGISTER_TEST( Write )
    REGISTER_TEST( Read )
    REGISTER_TEST( ReadWrite )
    REGISTER_TEST( ConsistentCacheKeysWithDist )
    #if defined( __WINDOWS__ )
        REGISTER_TEST( LightCache_IncludeUsingMacro )
        REGISTER_TEST( LightCache_IncludeUsingMacro2 )
        REGISTER_TEST( LightCache_IncludeUsingMacro3 )
        REGISTER_TEST( LightCache_IncludeUsingUndefinedMacros1 )
        REGISTER_TEST( LightCache_IncludeUsingUndefinedMacros2 )
        REGISTER_TEST( LightCache_IncludeUsingUndefinedMacros3 )
        REGISTER_TEST( LightCache_IncludeHierarchy )
        REGISTER_TEST( LightCache_CyclicInclude )
        REGISTER_TEST( LightCache_ImportDirective )
        REGISTER_TEST( Analyze_MSVC_WarningsOnly_Write )
        REGISTER_TEST( Analyze_MSVC_WarningsOnly_Read )

        // Distribution of /analyze is not currently supported due to preprocessor/_PREFAST_ inconsistencies
        //REGISTER_TEST( Analyze_MSVC_WarningsOnly_WriteFromDist )
        //REGISTER_TEST( Analyze_MSVC_WarningsOnly_ReadFromDist )
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
        PROFILE_SECTION( "Normal" );

        options.m_ConfigFile = "Tools/FBuild/FBuildTest/Data/TestCache/cache.bff";

        FBuildForTest fBuild( options );
        TEST_ASSERT( fBuild.Initialize() );

        TEST_ASSERT( fBuild.Build( "ObjectList" ) );

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
            PROFILE_SECTION( "Light" );

            options.m_ConfigFile = "Tools/FBuild/FBuildTest/Data/TestCache/lightcache.bff";

            FBuildForTest fBuild( options );
            TEST_ASSERT( fBuild.Initialize() );

            TEST_ASSERT( fBuild.Build( "ObjectList" ) );

            // Ensure cache was written to
            const FBuildStats::Stats & objStats = fBuild.GetStats().GetStatsFor( Node::OBJECT_NODE );
            TEST_ASSERT( objStats.m_NumCacheStores == objStats.m_NumProcessed );
            TEST_ASSERT( objStats.m_NumBuilt == objStats.m_NumProcessed );

            // Ensure LightCache was used
            TEST_ASSERT( fBuild.GetStats().GetLightCacheCount() == objStats.m_NumCacheStores );

            numDepsB = fBuild.GetRecursiveDependencyCount( "ObjectList" );
            TEST_ASSERT( numDepsB > 0 );
        }

        TEST_ASSERT( numDepsB >= numDepsA );
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
        PROFILE_SECTION( "Normal" );

        options.m_ConfigFile = "Tools/FBuild/FBuildTest/Data/TestCache/cache.bff";

        FBuildForTest fBuild( options );
        TEST_ASSERT( fBuild.Initialize() );

        TEST_ASSERT( fBuild.Build( "ObjectList" ) );

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
            PROFILE_SECTION( "Light" );

            options.m_ConfigFile = "Tools/FBuild/FBuildTest/Data/TestCache/lightcache.bff";

            FBuildForTest fBuild( options );
            TEST_ASSERT( fBuild.Initialize() );

            TEST_ASSERT( fBuild.Build( "ObjectList" ) );

            // Ensure cache was written to
            const FBuildStats::Stats & objStats = fBuild.GetStats().GetStatsFor( Node::OBJECT_NODE );
            TEST_ASSERT( objStats.m_NumCacheHits == objStats.m_NumProcessed );
            TEST_ASSERT( objStats.m_NumBuilt == 0 );

            // Ensure LightCache was used
            TEST_ASSERT( fBuild.GetStats().GetLightCacheCount() == objStats.m_NumCacheHits );

            numDepsB = fBuild.GetRecursiveDependencyCount( "ObjectList" );
            TEST_ASSERT( numDepsB > 0 );
        }

        TEST_ASSERT( numDepsB >= numDepsA );
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
        PROFILE_SECTION( "Normal" );
        options.m_ConfigFile = "Tools/FBuild/FBuildTest/Data/TestCache/cache.bff";

        FBuildForTest fBuild( options );
        TEST_ASSERT( fBuild.Initialize() );

        TEST_ASSERT( fBuild.Build( "ObjectList" ) );

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
            PROFILE_SECTION( "Light" );

            options.m_ConfigFile = "Tools/FBuild/FBuildTest/Data/TestCache/lightcache.bff";

            FBuildForTest fBuild( options );
            TEST_ASSERT( fBuild.Initialize() );

            TEST_ASSERT( fBuild.Build( "ObjectList" ) );

            // Ensure cache was written to
            const FBuildStats::Stats & objStats = fBuild.GetStats().GetStatsFor( Node::OBJECT_NODE );
            TEST_ASSERT( objStats.m_NumCacheHits == objStats.m_NumProcessed );
            TEST_ASSERT( objStats.m_NumBuilt == 0 );

            // Ensure LightCache was used
            TEST_ASSERT( fBuild.GetStats().GetLightCacheCount() == objStats.m_NumCacheHits );

            numDepsB = fBuild.GetRecursiveDependencyCount( "ObjectList" );
            TEST_ASSERT( numDepsB > 0 );
        }

        TEST_ASSERT( numDepsB >= numDepsA );
    #endif
}

// ConsistentCacheKeysWithDist
//------------------------------------------------------------------------------
void TestCache::ConsistentCacheKeysWithDist() const
{
    FBuildTestOptions options;
    options.m_CacheVerbose = true;
    options.m_ConfigFile = "Tools/FBuild/FBuildTest/Data/TestCache/ConsistentCacheKeys/fbuild.bff";

    // Ensure compilation is performed "remotely"
    options.m_AllowDistributed = true;
    options.m_AllowLocalRace = false;
    options.m_NoLocalConsumptionOfRemoteJobs = true;

    // Write Only
    {
        options.m_UseCacheRead = false;
        options.m_UseCacheWrite = true;

        // Compile
        FBuildForTest fBuild( options );
        TEST_ASSERT( fBuild.Initialize() );

        Server s;
        s.Listen( Protocol::PROTOCOL_TEST_PORT );

        TEST_ASSERT( fBuild.Build( "ConsistentCacheKeys" ) );

        // Check for cache hit
        TEST_ASSERT( fBuild.GetStats().GetStatsFor( Node::OBJECT_NODE ).m_NumCacheStores == 1 );
    }

    // Read Only
    {
        options.m_UseCacheRead = true;
        options.m_UseCacheWrite = false;

        // Compile with /analyze (warnings only)
        FBuildForTest fBuild( options );
        TEST_ASSERT( fBuild.Initialize() );

        Server s;
        s.Listen( Protocol::PROTOCOL_TEST_PORT );

        TEST_ASSERT( fBuild.Build( "ConsistentCacheKeys" ) );

        // Check for cache hit
        TEST_ASSERT( fBuild.GetStats().GetStatsFor( Node::OBJECT_NODE ).m_NumCacheHits == 1 );
    }

    // Ensure that we used the same key for reading and writing the cache
    const AString & output = GetRecordedOutput();
    const char * store = output.Find( "Cache Store" );
    const char * hit = output.Find( "Cache Hit" );
    TEST_ASSERT( store && hit );
    const char * storeQuote1 = output.Find( '\'', store );
    const char * hitQuote1 = output.Find( '\'', hit );
    TEST_ASSERT( storeQuote1 && hitQuote1 );
    const char * storeQuote2 = output.Find( '\'', storeQuote1 + 1 );
    const char * hitQuote2 = output.Find( '\'', hitQuote1 + 1 );
    TEST_ASSERT( storeQuote2 && hitQuote2 );
    AStackString<> storeKey( storeQuote1 + 1, storeQuote2 );
    AStackString<> hitKey( hitQuote1 + 1, hitQuote2 );
    TEST_ASSERT( storeKey.IsEmpty() == false );
    TEST_ASSERT( storeKey == hitKey );
}

// LightCache_IncludeUsingMacro
//------------------------------------------------------------------------------
void TestCache::LightCache_IncludeUsingMacro() const
{
    // Files can be included via macros and those macros can result in different
    // includes for a given header:
    //  - file.1.cpp defines PATH_AS_MACRO as file.1.h
    //  - file.2.cpp defines PATH_AS_MACRO as file.2.h
    //  - bothe files include file.h which includes a file using PATH_AS_MACRO
    //
    //    file.1.cpp       file.2.cpp
    //         |                |
    //          |              |
    //           |-- file.h --|
    //          |              |
    //         |                |
    //     file.1.h         file.2.h

    FBuildTestOptions options;
    options.m_CacheVerbose = true;
    options.m_ConfigFile = "Tools/FBuild/FBuildTest/Data/TestCache/LightCache_IncludeUsingMacro/fbuild.bff";

    const char * expectedFiles[] = { "file.1.cpp", "file.1.h", "file.2.cpp", "file.2.h", "file.h" };

    // Single thread
    options.m_NumWorkerThreads = 1; // Single threaded, to ensure dependency re-use

    // Write (single thread)
    {
        options.m_UseCacheRead = false;
        options.m_UseCacheWrite = true;

        FBuildForTest fBuild( options );
        TEST_ASSERT( fBuild.Initialize() );

        TEST_ASSERT( fBuild.Build( "ObjectList" ) );

        // Ensure we that we used the LightCache
        const FBuildStats::Stats & objStats = fBuild.GetStats().GetStatsFor( Node::OBJECT_NODE );
        TEST_ASSERT( objStats.m_NumCacheStores == 2 );
        TEST_ASSERT( fBuild.GetStats().GetLightCacheCount() == objStats.m_NumCacheStores );

        CheckForDependencies( fBuild, expectedFiles, sizeof( expectedFiles ) / sizeof( const char * ) );
    }

    // Read (single thread)
    {
        options.m_UseCacheRead = true;
        options.m_UseCacheWrite = false;

        FBuildForTest fBuild( options );
        TEST_ASSERT( fBuild.Initialize() );

        TEST_ASSERT( fBuild.Build( "ObjectList" ) );

        // Ensure we that we used the LightCache
        const FBuildStats::Stats & objStats = fBuild.GetStats().GetStatsFor( Node::OBJECT_NODE );
        TEST_ASSERT( objStats.m_NumCacheHits == 2 );
        TEST_ASSERT( fBuild.GetStats().GetLightCacheCount() == objStats.m_NumCacheHits );

        CheckForDependencies( fBuild, expectedFiles, sizeof( expectedFiles ) / sizeof( const char * ) );
    }

    // Multiple threads
    options.m_NumWorkerThreads = 2;

    // Write (multiple threads)
    {
        options.m_UseCacheRead = false;
        options.m_UseCacheWrite = true;

        FBuildForTest fBuild( options );
        TEST_ASSERT( fBuild.Initialize() );

        TEST_ASSERT( fBuild.Build( "ObjectList" ) );

        // Ensure we that we used the LightCache
        const FBuildStats::Stats & objStats = fBuild.GetStats().GetStatsFor( Node::OBJECT_NODE );
        TEST_ASSERT( objStats.m_NumCacheStores == 2 );
        TEST_ASSERT( fBuild.GetStats().GetLightCacheCount() == objStats.m_NumCacheStores );

        CheckForDependencies( fBuild, expectedFiles, sizeof( expectedFiles ) / sizeof( const char * ) );
    }

    // Read (multiple threads)
    {
        options.m_UseCacheRead = true;
        options.m_UseCacheWrite = false;

        FBuildForTest fBuild( options );
        TEST_ASSERT( fBuild.Initialize() );

        TEST_ASSERT( fBuild.Build( "ObjectList" ) );

        // Ensure we that we used the LightCache
        const FBuildStats::Stats & objStats = fBuild.GetStats().GetStatsFor( Node::OBJECT_NODE );
        TEST_ASSERT( objStats.m_NumCacheHits == 2 );
        TEST_ASSERT( fBuild.GetStats().GetLightCacheCount() == objStats.m_NumCacheHits );

        CheckForDependencies( fBuild, expectedFiles, sizeof( expectedFiles ) / sizeof( const char * ) );
    }
}

// LightCache_IncludeUsingMacro2
//------------------------------------------------------------------------------
void TestCache::LightCache_IncludeUsingMacro2() const
{
    // Defines found while parsing must be stored for re-use along with discovered
    // includes.
    //
    //    file.1.cpp       file.2.cpp
    //         |                |
    //         |                |
    //    header1.h        header1.h       <<- macro defined here
    //         |                |
    //         |                |
    //    header2.h        header2.h       <<- included via macro

    FBuildTestOptions options;
    options.m_CacheVerbose = true;
    options.m_ConfigFile = "Tools/FBuild/FBuildTest/Data/TestCache/LightCache_IncludeUsingMacro2/fbuild.bff";

    const char * expectedFiles[] = { "file.1.cpp", "file.2.cpp", "header1.h", "header2.h" };

    // Single thread
    options.m_NumWorkerThreads = 1; // Single threaded, to ensure dependency re-use

    // Write (single thread)
    {
        options.m_UseCacheRead = false;
        options.m_UseCacheWrite = true;

        FBuildForTest fBuild( options );
        TEST_ASSERT( fBuild.Initialize() );

        TEST_ASSERT( fBuild.Build( "ObjectList" ) );

        // Ensure we that we used the LightCache
        const FBuildStats::Stats & objStats = fBuild.GetStats().GetStatsFor( Node::OBJECT_NODE );
        TEST_ASSERT( objStats.m_NumCacheStores == 2 );
        TEST_ASSERT( fBuild.GetStats().GetLightCacheCount() == objStats.m_NumCacheStores );

        CheckForDependencies( fBuild, expectedFiles, sizeof( expectedFiles ) / sizeof( const char * ) );
    }

    // Read (single thread)
    {
        options.m_UseCacheRead = true;
        options.m_UseCacheWrite = false;

        FBuildForTest fBuild( options );
        TEST_ASSERT( fBuild.Initialize() );

        TEST_ASSERT( fBuild.Build( "ObjectList" ) );

        // Ensure we that we used the LightCache
        const FBuildStats::Stats & objStats = fBuild.GetStats().GetStatsFor( Node::OBJECT_NODE );
        TEST_ASSERT( objStats.m_NumCacheHits == 2 );
        TEST_ASSERT( fBuild.GetStats().GetLightCacheCount() == objStats.m_NumCacheHits );

        CheckForDependencies( fBuild, expectedFiles, sizeof( expectedFiles ) / sizeof( const char * ) );
    }

    // Multiple threads
    options.m_NumWorkerThreads = 2;

    // Write (multiple threads)
    {
        options.m_UseCacheRead = false;
        options.m_UseCacheWrite = true;

        FBuildForTest fBuild( options );
        TEST_ASSERT( fBuild.Initialize() );

        TEST_ASSERT( fBuild.Build( "ObjectList" ) );

        // Ensure we that we used the LightCache
        const FBuildStats::Stats & objStats = fBuild.GetStats().GetStatsFor( Node::OBJECT_NODE );
        TEST_ASSERT( objStats.m_NumCacheStores == 2 );
        TEST_ASSERT( fBuild.GetStats().GetLightCacheCount() == objStats.m_NumCacheStores );

        CheckForDependencies( fBuild, expectedFiles, sizeof( expectedFiles ) / sizeof( const char * ) );
    }

    // Read (multiple threads)
    {
        options.m_UseCacheRead = true;
        options.m_UseCacheWrite = false;

        FBuildForTest fBuild( options );
        TEST_ASSERT( fBuild.Initialize() );

        TEST_ASSERT( fBuild.Build( "ObjectList" ) );

        // Ensure we that we used the LightCache
        const FBuildStats::Stats & objStats = fBuild.GetStats().GetStatsFor( Node::OBJECT_NODE );
        TEST_ASSERT( objStats.m_NumCacheHits == 2 );
        TEST_ASSERT( fBuild.GetStats().GetLightCacheCount() == objStats.m_NumCacheHits );

        CheckForDependencies( fBuild, expectedFiles, sizeof( expectedFiles ) / sizeof( const char * ) );
    }
}

// LightCache_IncludeUsingMacro3
//------------------------------------------------------------------------------
void TestCache::LightCache_IncludeUsingMacro3() const
{
    // Defines are accumulated during traversal, resulting in the main defines
    // vector vector being resized while being iterated. This needs to be handled
    // correctly.

    FBuildTestOptions options;
    options.m_CacheVerbose = true;
    options.m_ConfigFile = "Tools/FBuild/FBuildTest/Data/TestCache/LightCache_IncludeUsingMacro3/fbuild.bff";

    const char * expectedFiles[] = { "file.cpp", "header1.h", "header2.h" };

    // Single thread
    options.m_NumWorkerThreads = 1;

    // Write (single thread)
    {
        options.m_UseCacheRead = false;
        options.m_UseCacheWrite = true;

        FBuildForTest fBuild( options );
        TEST_ASSERT( fBuild.Initialize() );

        TEST_ASSERT( fBuild.Build( "ObjectList" ) );

        // Ensure we that we used the LightCache
        const FBuildStats::Stats & objStats = fBuild.GetStats().GetStatsFor( Node::OBJECT_NODE );
        TEST_ASSERT( objStats.m_NumCacheStores == 1 );
        TEST_ASSERT( fBuild.GetStats().GetLightCacheCount() == objStats.m_NumCacheStores );

        CheckForDependencies( fBuild, expectedFiles, sizeof( expectedFiles ) / sizeof( const char * ) );
    }
}

// LightCache_IncludeUsingUndefinedMacros1
//------------------------------------------------------------------------------
void TestCache::LightCache_IncludeUsingUndefinedMacros1() const
{
    // An include using a macro which is not defined, but guarded so unused
    //
    // #ifdef MACRO_WHICH_IS_NOT_DEFINED
    //     #include MACRO_WHICH_IS_NOT_DEFINED
    // #endif
    //
    LightCache_IncludeUsingUndefinedMacros( "1/fbuild.bff",
                                            true,       // Build passes, since macro is not used
                                            true,       // LightCache can be used since include is ignored
                                            nullptr );  // No error
}

// LightCache_IncludeUsingUndefinedMacros2
//------------------------------------------------------------------------------
void TestCache::LightCache_IncludeUsingUndefinedMacros2() const
{
    // An include using a macro which is not defined, but is used
    //
    // #include MACRO_WHICH_IS_NOT_DEFINED
    //
    LightCache_IncludeUsingUndefinedMacros( "2/fbuild.bff",
                                            false,  // Overall build fails due to invalid defines
                                            true,   // LightCache could be used
                                            nullptr );  // No LightCache error
}

// LightCache_IncludeUsingUndefinedMacros3
//------------------------------------------------------------------------------
void TestCache::LightCache_IncludeUsingUndefinedMacros3() const
{
    // An include using a macro which is defined, but is not a direct include
    // We don't support that, but we must detect and disable the LightCache to
    // ensure builds are correct.
    //
    // #define COMPLEX_MACRO( arg ) arg
    // #include COMPLEX_MACRO( "file.h" )
    //
    // This
    LightCache_IncludeUsingUndefinedMacros( "3/fbuild.bff",
                                            true,   // Overall build works as code is valid
                                            false,  // LightCache cannot be used (falls back to normal cache)
                                            "Could not resolve macro 'COMPLEX_MACRO'" ); // Expected error
}

// LightCache_IncludeUsingUndefinedMacros
//------------------------------------------------------------------------------
void TestCache::LightCache_IncludeUsingUndefinedMacros( const char * configFile,
                                                        bool expectedBuildResult,
                                                        bool lightCacheCompatible,
                                                        const char * lightCacheError ) const
{
    // Options
    FBuildTestOptions options;
    options.m_CacheVerbose = true;
    options.m_ConfigFile.Format( "Tools/FBuild/FBuildTest/Data/TestCache/LightCache_IncludeUsingUndefinedMacros/%s", configFile );
    options.m_NumWorkerThreads = 1; // Single thread
    options.m_UseCacheRead = false;
    options.m_UseCacheWrite = true;

    // Initialize and Build
    FBuildForTest fBuild( options );
    TEST_ASSERT( fBuild.Initialize() );
    TEST_ASSERT( fBuild.Build( "ObjectList" ) == expectedBuildResult );

    // Check LightCache compatibility
    // (can be compatible even if compilation fails)
    if ( lightCacheCompatible )
    {
        TEST_ASSERT( fBuild.GetStats().GetLightCacheCount() == 1 );
    }
    else
    {
        TEST_ASSERT( fBuild.GetStats().GetLightCacheCount() == 0 );
        TEST_ASSERT( GetRecordedOutput().Find( "LightCache cannot be used" ) );
    }

    // Check for expected LightCache error
    if ( lightCacheError )
    {
        TEST_ASSERT( GetRecordedOutput().Find( lightCacheError ) );
    }
}

// LightCache_IncludeHierarchy
//------------------------------------------------------------------------------
void TestCache::LightCache_IncludeHierarchy() const
{
    // Two files can include "common.h" in such a way that common.h includes a
    // different file because of the rules about which directories are searched
    // for includes
    //
    //     Folder1/file.cpp  Folder2/file.cpp
    //            |                |
    //             |              |
    //              |- Common.h -|
    //             |              |
    //            |                |
    //     Folder1/file.h    Folder2/file.h
    //

    FBuildTestOptions options;
    options.m_CacheVerbose = true;
    options.m_ConfigFile = "Tools/FBuild/FBuildTest/Data/TestCache/LightCache_IncludeHierarchy/fbuild.bff";

    const char * expectedFiles[] = { "Folder1/file.cpp", "Folder1/file.h", "Folder2/file.cpp", "Folder2/file.h", "common.h" };

    // Write (single thread)
    {
        options.m_UseCacheRead = false;
        options.m_UseCacheWrite = true;
        options.m_NumWorkerThreads = 1; // Single threaded, to ensure dependency re-use

        FBuildForTest fBuild( options );
        TEST_ASSERT( fBuild.Initialize() );

        TEST_ASSERT( fBuild.Build( "ObjectList" ) );

        // Ensure we that we used the LightCache
        const FBuildStats::Stats & objStats = fBuild.GetStats().GetStatsFor( Node::OBJECT_NODE );
        TEST_ASSERT( objStats.m_NumCacheStores == 2 );
        TEST_ASSERT( fBuild.GetStats().GetLightCacheCount() == objStats.m_NumCacheStores );

        CheckForDependencies( fBuild, expectedFiles, sizeof( expectedFiles ) / sizeof( const char * ) );
    }

    // Read (single thread)
    {
        options.m_UseCacheRead = true;
        options.m_UseCacheWrite = false;
        options.m_NumWorkerThreads = 1;

        FBuildForTest fBuild( options );
        TEST_ASSERT( fBuild.Initialize() );

        TEST_ASSERT( fBuild.Build( "ObjectList" ) );

        // Ensure we that we used the LightCache
        const FBuildStats::Stats & objStats = fBuild.GetStats().GetStatsFor( Node::OBJECT_NODE );
        TEST_ASSERT( objStats.m_NumCacheHits == 2 );
        TEST_ASSERT( fBuild.GetStats().GetLightCacheCount() == objStats.m_NumCacheHits );

        CheckForDependencies( fBuild, expectedFiles, sizeof( expectedFiles ) / sizeof( const char * ) );
    }

    // Write (multiple threads)
    {
        options.m_UseCacheRead = false;
        options.m_UseCacheWrite = true;
        options.m_NumWorkerThreads = 2;

        FBuildForTest fBuild( options );
        TEST_ASSERT( fBuild.Initialize() );

        TEST_ASSERT( fBuild.Build( "ObjectList" ) );

        // Ensure we that we used the LightCache
        const FBuildStats::Stats & objStats = fBuild.GetStats().GetStatsFor( Node::OBJECT_NODE );
        TEST_ASSERT( objStats.m_NumCacheStores == 2 );
        TEST_ASSERT( fBuild.GetStats().GetLightCacheCount() == objStats.m_NumCacheStores );

        CheckForDependencies( fBuild, expectedFiles, sizeof( expectedFiles ) / sizeof( const char * ) );
    }

    // Read (multiple threads)
    {
        options.m_UseCacheRead = true;
        options.m_UseCacheWrite = false;
        options.m_NumWorkerThreads = 2;

        FBuildForTest fBuild( options );
        TEST_ASSERT( fBuild.Initialize() );

        TEST_ASSERT( fBuild.Build( "ObjectList" ) );

        // Ensure we that we used the LightCache
        const FBuildStats::Stats & objStats = fBuild.GetStats().GetStatsFor( Node::OBJECT_NODE );
        TEST_ASSERT( objStats.m_NumCacheHits == 2 );
        TEST_ASSERT( fBuild.GetStats().GetLightCacheCount() == objStats.m_NumCacheHits );

        CheckForDependencies( fBuild, expectedFiles, sizeof( expectedFiles ) / sizeof( const char * ) );
    }
}

// LightCache_CyclicInclude
//------------------------------------------------------------------------------
void TestCache::LightCache_CyclicInclude() const
{
    FBuildTestOptions options;
    options.m_CacheVerbose = true;
    options.m_ConfigFile = "Tools/FBuild/FBuildTest/Data/TestCache/LightCache_CyclicInclude/fbuild.bff";

    // Write
    {
        options.m_UseCacheRead = false;
        options.m_UseCacheWrite = true;

        FBuildForTest fBuild( options );
        TEST_ASSERT( fBuild.Initialize() );

        TEST_ASSERT( fBuild.Build( "ObjectList" ) );

        // Ensure everything was stored to the cache using the LightCache
        const FBuildStats::Stats & objStats = fBuild.GetStats().GetStatsFor( Node::OBJECT_NODE );
        TEST_ASSERT( objStats.m_NumCacheStores == objStats.m_NumProcessed );
        TEST_ASSERT( objStats.m_NumBuilt == objStats.m_NumProcessed );
        TEST_ASSERT( fBuild.GetStats().GetLightCacheCount() == objStats.m_NumCacheStores );
    }

    // Read
    {
        options.m_UseCacheWrite = false;
        options.m_UseCacheRead = true;

        FBuildForTest fBuild( options );
        TEST_ASSERT( fBuild.Initialize() );

        TEST_ASSERT( fBuild.Build( "ObjectList" ) );

        // Ensure everything came from the cache using the LightCache
        const FBuildStats::Stats & objStats = fBuild.GetStats().GetStatsFor( Node::OBJECT_NODE );
        TEST_ASSERT( objStats.m_NumCacheHits == objStats.m_NumProcessed );
        TEST_ASSERT( objStats.m_NumBuilt == 0 );
        TEST_ASSERT( fBuild.GetStats().GetLightCacheCount() == objStats.m_NumCacheHits );
    }
}

// LightCache_ImportDirective
//------------------------------------------------------------------------------
void TestCache::LightCache_ImportDirective() const
{
    FBuildTestOptions options;
    options.m_ForceCleanBuild = true;
    options.m_UseCacheWrite = true;
    options.m_CacheVerbose = true;
    options.m_ConfigFile = "Tools/FBuild/FBuildTest/Data/TestCache/LightCache_ImportDirective/fbuild.bff";

    FBuildForTest fBuild( options );
    TEST_ASSERT( fBuild.Initialize() );

    TEST_ASSERT( fBuild.Build( "ObjectList" ) );

    // Ensure cache we fell back to normal caching
    const FBuildStats::Stats & objStats = fBuild.GetStats().GetStatsFor( Node::OBJECT_NODE );
    TEST_ASSERT( objStats.m_NumCacheStores == 1 );

    // Ensure we detected that we could not use the LightCache
    TEST_ASSERT( objStats.m_NumLightCache == 0 );

    // Check for expected error in output (from -cacheverbose)
    TEST_ASSERT( GetRecordedOutput().Find( "#import is unsupported." ) );
}

// Analyze_MSVC_WarningsOnly_Write
//------------------------------------------------------------------------------
void TestCache::Analyze_MSVC_WarningsOnly_Write() const
{
    FBuildTestOptions options;
    options.m_ForceCleanBuild = true;
    options.m_UseCacheWrite = true;
    options.m_CacheVerbose = true;
    options.m_ConfigFile = mAnalyzeMSVCBFFPath;

    EnsureFileDoesNotExist( mAnalyzeMSVCXMLFile1 );
    EnsureFileDoesNotExist( mAnalyzeMSVCXMLFile2 );

    // Compile with /analyze (warnings only) (cache write)
    FBuildForTest fBuild( options );
    TEST_ASSERT( fBuild.Initialize() );
    TEST_ASSERT( fBuild.Build( "Analyze+WarningsOnly" ) );

    // Check for cache store
    TEST_ASSERT( fBuild.GetStats().GetStatsFor( Node::OBJECT_NODE ).m_NumCacheStores == 2 );

    // Check for expected warnings
    const AString& output = GetRecordedOutput();
    // file1.cpp
    TEST_ASSERT( output.Find( "warning C6201" ) && output.Find( "Index '32' is out of valid index range" ) );
    TEST_ASSERT( output.Find( "warning C6386" ) && output.Find( "Buffer overrun while writing to 'buffer'" ) );
    // file2.cpp
    #if _MSC_VER >= 1910 // From VS2017 or later
        TEST_ASSERT( output.Find( "warning C6387" ) && output.Find( "could be '0':  this does not adhere to the specification for the function" ) );
    #endif

    // Check analysis file is present with expected errors
    AString xml;
    LoadFileContentsAsString( mAnalyzeMSVCXMLFile1, xml );
    TEST_ASSERT( xml.Find( "<DEFECTCODE>6201</DEFECTCODE>" ) );
    TEST_ASSERT( xml.Find( "<DEFECTCODE>6386</DEFECTCODE>" ) );
    LoadFileContentsAsString( mAnalyzeMSVCXMLFile2, xml );
    #if _MSC_VER >= 1910 // From VS2017 or later
        TEST_ASSERT( xml.Find( "<DEFECTCODE>6387</DEFECTCODE>" ) );
    #endif
}

// Analyze_MSVC_WarningsOnly_Read
//------------------------------------------------------------------------------
void TestCache::Analyze_MSVC_WarningsOnly_Read() const
{
    FBuildTestOptions options;
    options.m_ForceCleanBuild = true;
    options.m_UseCacheRead = true;
    options.m_CacheVerbose = true;
    options.m_ConfigFile = mAnalyzeMSVCBFFPath;

    EnsureFileDoesNotExist( mAnalyzeMSVCXMLFile1 );
    EnsureFileDoesNotExist( mAnalyzeMSVCXMLFile2 );

    // Compile with /analyze (warnings only) (cache read)
    FBuildForTest fBuild( options );
    TEST_ASSERT( fBuild.Initialize() );
    TEST_ASSERT( fBuild.Build( "Analyze+WarningsOnly" ) );

    // Check for cache hit
    TEST_ASSERT( fBuild.GetStats().GetStatsFor( Node::OBJECT_NODE ).m_NumCacheHits == 2 );

    // NOTE: Process output will not contain warnings (as compilation was skipped)

    // Check analysis file is present with expected errors
    AString xml;
    LoadFileContentsAsString( mAnalyzeMSVCXMLFile1, xml );
    TEST_ASSERT( xml.Find( "<DEFECTCODE>6201</DEFECTCODE>" ) );
    TEST_ASSERT( xml.Find( "<DEFECTCODE>6386</DEFECTCODE>" ) );
    LoadFileContentsAsString( mAnalyzeMSVCXMLFile2, xml );
    #if _MSC_VER >= 1910 // From VS2017 or later
        TEST_ASSERT( xml.Find( "<DEFECTCODE>6387</DEFECTCODE>" ) );
    #endif
}

// Analyze_MSVC_WarningsOnly_WriteFromDist
//------------------------------------------------------------------------------
void TestCache::Analyze_MSVC_WarningsOnly_WriteFromDist() const
{
    FBuildTestOptions options;
    options.m_ForceCleanBuild = true;
    options.m_UseCacheWrite = true;
    options.m_CacheVerbose = true;
    options.m_ConfigFile = mAnalyzeMSVCBFFPath;

    // Ensure compilation is performed "remotely"
    options.m_AllowDistributed = true;
    options.m_AllowLocalRace = false;
    options.m_NoLocalConsumptionOfRemoteJobs = true;

    EnsureFileDoesNotExist( mAnalyzeMSVCXMLFile1 );
    EnsureFileDoesNotExist( mAnalyzeMSVCXMLFile2 );

    // Compile with /analyze (warnings only)
    FBuildForTest fBuild( options );
    TEST_ASSERT( fBuild.Initialize() );

    Server s;
    s.Listen( Protocol::PROTOCOL_TEST_PORT );

    TEST_ASSERT( fBuild.Build( "Analyze+WarningsOnly" ) );

    // Check for cache hit
    TEST_ASSERT( fBuild.GetStats().GetStatsFor( Node::OBJECT_NODE ).m_NumCacheStores == 2 );

    // Check for expected warnings
    const AString & output = GetRecordedOutput();
    // file1.cpp
    TEST_ASSERT( output.Find( "warning C6201" ) && output.Find( "Index '32' is out of valid index range" ) );
    TEST_ASSERT( output.Find( "warning C6386" ) && output.Find( "Buffer overrun while writing to 'buffer'" ) );
    // file2.cpp
    #if _MSC_VER >= 1910 // From VS2017 or later
        TEST_ASSERT( output.Find( "warning C6387" ) && output.Find( "could be '0':  this does not adhere to the specification for the function" ) );
    #endif

    // Check analysis file is present with expected errors
    AString xml;
    LoadFileContentsAsString( mAnalyzeMSVCXMLFile1, xml );
    TEST_ASSERT( xml.Find( "<DEFECTCODE>6201</DEFECTCODE>" ) );
    TEST_ASSERT( xml.Find( "<DEFECTCODE>6386</DEFECTCODE>" ) );
    LoadFileContentsAsString( mAnalyzeMSVCXMLFile2, xml );
    #if _MSC_VER >= 1910 // From VS2017 or later
        TEST_ASSERT( xml.Find( "<DEFECTCODE>6387</DEFECTCODE>" ) );
    #endif
}

// Analyze_MSVC_WarningsOnly_ReadFromDist
//------------------------------------------------------------------------------
void TestCache::Analyze_MSVC_WarningsOnly_ReadFromDist() const
{
    FBuildTestOptions options;
    options.m_ForceCleanBuild = true;
    options.m_UseCacheRead = true;
    options.m_CacheVerbose = true;
    options.m_ConfigFile = mAnalyzeMSVCBFFPath;

    // Ensure compilation is performed "remotely"
    options.m_AllowDistributed = true;
    options.m_AllowLocalRace = false;
    options.m_NoLocalConsumptionOfRemoteJobs = true;

    EnsureFileDoesNotExist( mAnalyzeMSVCXMLFile1 );
    EnsureFileDoesNotExist( mAnalyzeMSVCXMLFile2 );

    // Compile with /analyze (warnings only)
    FBuildForTest fBuild( options );
    TEST_ASSERT( fBuild.Initialize() );

    Server s;
    s.Listen( Protocol::PROTOCOL_TEST_PORT );

    TEST_ASSERT( fBuild.Build( "Analyze+WarningsOnly" ) );

    // Check for cache hit
    TEST_ASSERT( fBuild.GetStats().GetStatsFor( Node::OBJECT_NODE ).m_NumCacheHits == 2 );

    // NOTE: Process output will not contain warnings (as compilation was skipped)

    // Check analysis file is present with expected errors
    AString xml;
    LoadFileContentsAsString( mAnalyzeMSVCXMLFile1, xml );
    TEST_ASSERT( xml.Find( "<DEFECTCODE>6201</DEFECTCODE>" ) );
    TEST_ASSERT( xml.Find( "<DEFECTCODE>6386</DEFECTCODE>" ) );
    LoadFileContentsAsString( mAnalyzeMSVCXMLFile2, xml );
    #if _MSC_VER >= 1910 // From VS2017 or later
        TEST_ASSERT( xml.Find( "<DEFECTCODE>6387</DEFECTCODE>" ) );
    #endif
}

// CheckForDependencies
//------------------------------------------------------------------------------
void TestCache::CheckForDependencies( const FBuildForTest & fBuild, const char * files[], size_t numFiles ) const
{
    Array< const Node * > nodes;
    fBuild.GetNodesOfType( Node::FILE_NODE, nodes );
    for ( size_t i = 0; i < numFiles; ++i )
    {
        AStackString<> file( files[ i ] );
        #if defined( __WINDOWS__ )
            file.Replace( '/', '\\' ); // Allow calling code to not have to care about the platform
        #endif
        bool found = false;
        for ( const Node * node : nodes )
        {
            if ( node->GetName().EndsWith( file ) )
            {
                found = true;
                break;
            }
        }
        TEST_ASSERTM( found, "Missing dependency: %s", files[ i ] );
    }
}

//------------------------------------------------------------------------------
