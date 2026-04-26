// TestPrecompiledHeaders.cpp
//------------------------------------------------------------------------------

// Includes
//------------------------------------------------------------------------------
#include "FBuildTest.h"

#include "Tools/FBuild/FBuildCore/FBuild.h"

#include "Core/FileIO/FileIO.h"
#include "Core/Process/Thread.h"
#include "Core/Strings/AStackString.h"

//------------------------------------------------------------------------------
TEST_GROUP( TestPrecompiledHeaders, FBuildTest )
{
public:
    // Helpers
    FBuildStats Build( FBuildTestOptions options = FBuildTestOptions(),
                       bool useDB = true,
                       const char * target = nullptr ) const;
    const char * GetPCHDBFileName() const
    {
        return "../tmp/Test/PrecompiledHeaders/pch.fdb";
    }
    const char * GetPCHDBClangFileName() const
    {
        return "../tmp/Test/PrecompiledHeaders/pchclang-windows.fdb";
    }
};

// Build
//------------------------------------------------------------------------------
FBuildStats TestPrecompiledHeaders::Build( FBuildTestOptions options, bool useDB, const char * target ) const
{
    options.m_ConfigFile = "Tools/FBuild/FBuildTest/Data/TestPrecompiledHeaders/fbuild.bff";
    options.m_ShowSummary = true; // required to generate stats for node count checks

    FBuildForTest fBuild( options );
    TEST_ASSERT( fBuild.Initialize( useDB ? GetPCHDBFileName() : nullptr ) );

    TEST_ASSERT( fBuild.Build( target ? target : "PCHTest" ) );
    TEST_ASSERT( fBuild.SaveDependencyGraph( GetPCHDBFileName() ) );

    return fBuild.GetStats();
}

//------------------------------------------------------------------------------
TEST_CASE( TestPrecompiledHeaders, TestPCH )
{
    FBuildTestOptions options;
    options.m_ForceCleanBuild = true;
    options.m_UseCacheWrite = true;

#if defined( __WINDOWS__ )
    AStackString obj( "../tmp/Test/PrecompiledHeaders/PCHUser.obj" );
    AStackString pch( "../tmp/Test/PrecompiledHeaders/PrecompiledHeader.pch" );
#elif defined( __LINUX__ )
    AStackString obj( "../tmp/Test/PrecompiledHeaders/PCHUser.o" );
    AStackString pch( "../tmp/Test/PrecompiledHeaders/PrecompiledHeader.h.gch" );
#elif defined( __OSX__ )
    AStackString obj( "../tmp/Test/PrecompiledHeaders/PCHUser.o" );
    AStackString pch( "../tmp/Test/PrecompiledHeaders/PrecompiledHeader.pch" );
#endif
    EnsureFileDoesNotExist( obj );
    EnsureFileDoesNotExist( pch );

    FBuildStats stats = Build( options, false ); // don't use DB

    EnsureFileExists( obj );
    EnsureFileExists( pch );

    // Check stats: Seen, Built, Type
    uint32_t numF = 4; // pch.h / slow.h / pchuser.cpp
#if defined( __WINDOWS__ )
    numF++; // pch.cpp
#endif
    CheckStatsNode( stats, numF, 3, Node::FILE_NODE );
    CheckStatsNode( stats, 1, 1, Node::COMPILER_NODE );
    CheckStatsNode( stats, 2, 2, Node::OBJECT_NODE );// obj + pch obj
    CheckStatsNode( stats, 1, 1, Node::OBJECT_LIST_NODE );
    CheckStatsNode( stats, 1, 1, Node::DIRECTORY_LIST_NODE );
    CheckStatsNode( stats, 1, 1, Node::ALIAS_NODE );
    CheckStatsNode( stats, 1, 1, Node::EXE_NODE );
    CheckStatsTotal( stats, numF + 7, 10 );

    // check we wrote all objects to the cache
    TEST_ASSERT( stats.GetStatsFor( Node::OBJECT_NODE ).m_NumCacheStores == 2 ); // pch and obj using pch
}

//------------------------------------------------------------------------------
#if defined( __WINDOWS__ )
TEST_CASE( TestPrecompiledHeaders, TestPCH_DifferentObj_MSVC )
{
    FBuildTestOptions options;
    options.m_ConfigFile = "Tools/FBuild/FBuildTest/Data/TestPrecompiledHeaders/DifferentObj_MSVC/fbuild.bff";

    const AStackString obj( "../tmp/Test/PrecompiledHeaders/PCHUser.obj" );
    const AStackString pch( "../tmp/Test/PrecompiledHeaders/PrecompiledHeader.pch" );

    EnsureFileDoesNotExist( obj );
    EnsureFileDoesNotExist( pch );

    FBuildStats stats = Build( options, false ); // don't use DB

    EnsureFileExists( obj );
    EnsureFileExists( pch );

    // Check stats: Seen, Built, Type
    CheckStatsNode( stats, 5, 3, Node::FILE_NODE );
    CheckStatsNode( stats, 1, 1, Node::COMPILER_NODE );
    CheckStatsNode( stats, 2, 2, Node::OBJECT_NODE );// obj + pch obj
    CheckStatsNode( stats, 1, 1, Node::OBJECT_LIST_NODE );
    CheckStatsNode( stats, 1, 1, Node::DIRECTORY_LIST_NODE );
    CheckStatsNode( stats, 1, 1, Node::ALIAS_NODE );
    CheckStatsNode( stats, 1, 1, Node::EXE_NODE );
    CheckStatsTotal( stats, 12, 10 );
}
#endif

//------------------------------------------------------------------------------
TEST_CASE( TestPrecompiledHeaders, TestPCH_NoRebuild )
{
    FBuildStats stats = Build();

    // Check stats: Seen, Built, Type
    uint32_t numF = 4; // pch.h / slow.h / pchuser.cpp / linker exe
#if defined( __WINDOWS__ )
    numF++; // pch.cpp
#endif
    CheckStatsNode( stats, numF, numF, Node::FILE_NODE );  // cpp + pch cpp + pch .h
    CheckStatsNode( stats, 1, 0, Node::COMPILER_NODE );
    CheckStatsNode( stats, 2, 0, Node::OBJECT_NODE );// obj + pch obj
    CheckStatsNode( stats, 1, 0, Node::OBJECT_LIST_NODE );
    CheckStatsNode( stats, 1, 1, Node::DIRECTORY_LIST_NODE );
    CheckStatsNode( stats, 1, 1, Node::ALIAS_NODE );
    CheckStatsNode( stats, 1, 0, Node::EXE_NODE );
    CheckStatsTotal( stats, 7 + numF, 2 + numF );
}

//------------------------------------------------------------------------------
TEST_CASE( TestPrecompiledHeaders, TestPCH_NoRebuild_BFFChange )
{
    FBuildTestOptions options;
    options.m_ForceDBMigration_Debug = true;
    FBuildStats stats = Build( options );

    // Check stats: Seen, Built, Type
    uint32_t numF = 4; // pch.h / slow.h / pchuser.cpp / linker exe
#if defined( __WINDOWS__ )
    numF++; // pch.cpp
#endif
    CheckStatsNode( stats, numF, numF, Node::FILE_NODE );  // cpp + pch cpp + pch .h
    CheckStatsNode( stats, 1, 0, Node::COMPILER_NODE );
    CheckStatsNode( stats, 2, 0, Node::OBJECT_NODE );// obj + pch obj
    CheckStatsNode( stats, 1, 0, Node::OBJECT_LIST_NODE );
    CheckStatsNode( stats, 1, 1, Node::DIRECTORY_LIST_NODE );
    CheckStatsNode( stats, 1, 1, Node::ALIAS_NODE );
    CheckStatsNode( stats, 1, 0, Node::EXE_NODE );
    CheckStatsTotal( stats, 7 + numF, 2 + numF );
}

//------------------------------------------------------------------------------
TEST_CASE( TestPrecompiledHeaders, TestPCHWithCache )
{
    FBuildTestOptions options;
    options.m_ForceCleanBuild = true;
    options.m_UseCacheRead = true;

#if defined( __WINDOWS__ )
    AStackString obj( "../tmp/Test/PrecompiledHeaders/PCHUser.obj" );
    AStackString pch( "../tmp/Test/PrecompiledHeaders/PrecompiledHeader.pch" );
#elif defined( __LINUX__ )
    AStackString obj( "../tmp/Test/PrecompiledHeaders/PCHUser.o" );
    AStackString pch( "../tmp/Test/PrecompiledHeaders/PrecompiledHeader.h.gch" );
#elif defined( __OSX__ )
    AStackString obj( "../tmp/Test/PrecompiledHeaders/PCHUser.o" );
    AStackString pch( "../tmp/Test/PrecompiledHeaders/PrecompiledHeader.pch" );
#endif
    EnsureFileDoesNotExist( obj );
    EnsureFileDoesNotExist( pch );

    FBuildStats stats = Build( options, false ); // don't use DB

    EnsureFileExists( obj );
    EnsureFileExists( pch );

    // Check stats: Seen, Built, Type
    uint32_t numF = 4; // pch.h / slow.h / pchuser.cpp / linker exe
#if defined( __WINDOWS__ )
    numF++; // pch.cpp
#endif
    CheckStatsNode( stats, numF, 3, Node::FILE_NODE );  // cpp + pch
    CheckStatsNode( stats, 1, 1, Node::COMPILER_NODE );
    CheckStatsNode( stats, 2, 0, Node::OBJECT_NODE ); // obj + pch obj
    CheckStatsNode( stats, 1, 1, Node::OBJECT_LIST_NODE );
    CheckStatsNode( stats, 1, 1, Node::DIRECTORY_LIST_NODE );
    CheckStatsNode( stats, 1, 1, Node::ALIAS_NODE );
    CheckStatsNode( stats, 1, 1, Node::EXE_NODE );
    CheckStatsTotal( stats, 7 + numF, 8 );

    // check all objects came from the cache
    TEST_ASSERT( stats.GetStatsFor( Node::OBJECT_NODE ).m_NumCacheHits == 2 ); // pch & obj
}

//------------------------------------------------------------------------------
TEST_CASE( TestPrecompiledHeaders, TestPCHWithCache_NoRebuild )
{
    FBuildStats stats = Build();

    // Check stats: Seen, Built, Type
    uint32_t numF = 4; // pch.h / slow.h / pchuser.cpp / linker exe
#if defined( __WINDOWS__ )
    numF++; // pch.cpp
#endif
    CheckStatsNode( stats, numF, numF, Node::FILE_NODE );  // cpp + pch cpp + pch .h
    CheckStatsNode( stats, 1, 0, Node::COMPILER_NODE );
    CheckStatsNode( stats, 2, 0, Node::OBJECT_NODE );// obj + pch obj
    CheckStatsNode( stats, 1, 0, Node::OBJECT_LIST_NODE );
    CheckStatsNode( stats, 1, 1, Node::DIRECTORY_LIST_NODE );
    CheckStatsNode( stats, 1, 1, Node::ALIAS_NODE );
    CheckStatsNode( stats, 1, 0, Node::EXE_NODE );
    CheckStatsTotal( stats, 7 + numF, 2 + numF );
}

//------------------------------------------------------------------------------
TEST_CASE( TestPrecompiledHeaders, TestPCHWithCache_BFFChange )
{
#if defined( __OSX__ )
    // HFS+ has surprisingly poor time resolution (1 second) which makes the
    // ObjectListNode appear to not need rebuilding, because the tests complete
    // so quickly. This work-around keeps our state consistent with other platforms
    // (the actual thing we are primarily tests (the pchuser rebuild) is not impacted
    // by this)
    Thread::Sleep( 1100 );
#endif

    // Delete the object that uses the PCH, but not the PCH obj itself
    // to ensure the object can be pulled from the cache after db migration
    // With the MSVC compiler, this ensures the PCHCacheKey is not lost
#if defined( __WINDOWS__ )
    const char * target = "../tmp/Test/PrecompiledHeaders/PCHUser.obj";
#else
    const char * target = "../tmp/Test/PrecompiledHeaders/PCHUser.o";
#endif
    EnsureFileExists( target );
    EnsureFileDoesNotExist( target );

    // Force migration
    FBuildTestOptions options;
    options.m_ForceDBMigration_Debug = true;
    options.m_UseCacheRead = true;
    FBuildStats stats = Build( options );

    // Check stats: Seen, Built, Type
    uint32_t numF = 4; // pch.h / slow.h / pchuser.cpp / linker exe
#if defined( __WINDOWS__ )
    numF++; // pch.cpp
#endif
    CheckStatsNode( stats, numF, numF, Node::FILE_NODE );  // cpp + pch cpp + pch .h
    CheckStatsNode( stats, 1, 0, Node::COMPILER_NODE );
    CheckStatsNode( stats, 2, 0, Node::OBJECT_NODE );// obj + pch obj
    CheckStatsNode( stats, 1, 1, Node::OBJECT_LIST_NODE );
    CheckStatsNode( stats, 1, 1, Node::DIRECTORY_LIST_NODE );
    CheckStatsNode( stats, 1, 1, Node::ALIAS_NODE );
    CheckStatsNode( stats, 1, 1, Node::EXE_NODE );
    CheckStatsTotal( stats, 7 + numF, 4 + numF );

    // Ensure the object was pulled from the cache
    TEST_ASSERT( stats.GetStatsFor( Node::OBJECT_NODE ).m_NumCacheHits == 1 );
}

//------------------------------------------------------------------------------
#if defined( __WINDOWS__ )
TEST_CASE( TestPrecompiledHeaders, PreventUselessCacheTraffic_MSVC )
{
    // Build the PCH locally, without going via the cache (no store, no hit)
    {
        FBuildTestOptions options;
        options.m_ForceCleanBuild = true;

        const bool useDB = true;
        const char * target = "../tmp/Test/PrecompiledHeaders/PrecompiledHeader.pch";
        FBuildStats stats = Build( options, useDB, target );

        // Ensure PCH was built, and not cached
        CheckStatsNode( stats, 1, 1, Node::OBJECT_NODE ); // pch built
        TEST_ASSERT( stats.GetStatsFor( Node::OBJECT_NODE ).m_NumCacheHits == 0 );
        TEST_ASSERT( stats.GetStatsFor( Node::OBJECT_NODE ).m_NumCacheStores == 0 );
    }

    // Build the objects dependent on the PCH and make sure they are:
    // a) not retrieved (since they are incompatible with our locally made PCH)
    // b) not stored (no-one can ever use them, since we didn't store the PCH)
    {
        FBuildTestOptions options;
        options.m_UseCacheRead = true;
        options.m_UseCacheWrite = true;

        FBuildStats stats = Build( options );

        // Ensure PCH was built, and not cached
        CheckStatsNode( stats, 2, 1, Node::OBJECT_NODE ); // obj built
        TEST_ASSERT( stats.GetStatsFor( Node::OBJECT_NODE ).m_NumCacheHits == 0 );
        TEST_ASSERT( stats.GetStatsFor( Node::OBJECT_NODE ).m_NumCacheStores == 0 );
    }
}
#endif

//------------------------------------------------------------------------------
TEST_CASE( TestPrecompiledHeaders, CacheUniqueness )
{
    // Two headers, differing only in unused defines
    const char * pchA = "Tools/FBuild/FBuildTest/Data/TestPrecompiledHeaders/CacheUniqueness/PrecompiledHeaderA.h";
    const char * pchB = "Tools/FBuild/FBuildTest/Data/TestPrecompiledHeaders/CacheUniqueness/PrecompiledHeaderB.h";
    const char * dstPCH = "../tmp/Test/PrecompiledHeaders/CacheUniqueness/PrecompiledHeader.h";
#if defined( __WINDOWS__ )
    // On windows we need a CPP to create the PCH
    const char * pchCPP = "Tools/FBuild/FBuildTest/Data/TestPrecompiledHeaders/CacheUniqueness/PrecompiledHeader.cpp";
    const char * dstPCHCPP = "../tmp/Test/PrecompiledHeaders/CacheUniqueness/PrecompiledHeader.cpp";
#endif
    const char * pchUser = "Tools/FBuild/FBuildTest/Data/TestPrecompiledHeaders/CacheUniqueness/PCHUser.cpp";
    const char * dstPCHUser = "../tmp/Test/PrecompiledHeaders/CacheUniqueness/PCHUser.cpp";

    // Copy the files to the tmp Dir
    TEST_ASSERT( FileIO::EnsurePathExists( AStackString( "../tmp/Test/PrecompiledHeaders/CacheUniqueness/" ) ) );
    TEST_ASSERT( FileIO::FileCopy( pchA, dstPCH ) );
#if defined( __WINDOWS__ )
    TEST_ASSERT( FileIO::FileCopy( pchCPP, dstPCHCPP ) );
#endif
    TEST_ASSERT( FileIO::FileCopy( pchUser, dstPCHUser ) );

    FBuildTestOptions baseOptions;
    baseOptions.m_ConfigFile = "Tools/FBuild/FBuildTest/Data/TestPrecompiledHeaders/CacheUniqueness/fbuild.bff";
    baseOptions.m_ForceCleanBuild = true;

    // Compile A
    {
        FBuildTestOptions options( baseOptions );
        options.m_UseCacheWrite = true;

        FBuildForTest fBuild( options );
        TEST_ASSERT( fBuild.Initialize( nullptr ) );

        AStackString target( "PCHTest-Uniqueness" );

        TEST_ASSERT( fBuild.Build( target ) );

        // Ensure we stored to the cache
        TEST_ASSERT( fBuild.GetStats().GetCacheStores() == 2 );
    }

    // Replace header with one that differs only by the define it declares
    TEST_ASSERT( FileIO::FileCopy( pchB, dstPCH ) );

    // Compile B
    {
        FBuildTestOptions options( baseOptions );
        options.m_UseCacheRead = true;

        FBuildForTest fBuild( options );
        TEST_ASSERT( fBuild.Initialize( nullptr ) );

        AStackString target( "PCHTest-Uniqueness" );

        TEST_ASSERT( fBuild.Build( target ) );

        // Should not have retrieved from the cache
        TEST_ASSERT( fBuild.GetStats().GetCacheHits() == 0 );
    }
}

//------------------------------------------------------------------------------
TEST_CASE( TestPrecompiledHeaders, CacheUniqueness2 )
{
    // Ensure the same source file built into two locations
    // is cached correctly in both

    // Initialize
    FBuildTestOptions options;
    options.m_ConfigFile = "Tools/FBuild/FBuildTest/Data/TestPrecompiledHeaders/CacheUniqueness2/fbuild.bff";
    options.m_ForceCleanBuild = true;

    AStackString target( "PCHTest-CacheUniqueness2" );

    // Write to cache
    {
        FBuildTestOptions optionsCopy( options );
        optionsCopy.m_UseCacheWrite = true;
        FBuildForTest fBuild( optionsCopy );
        TEST_ASSERT( fBuild.Initialize( nullptr ) );

        TEST_ASSERT( fBuild.Build( target ) );

        TEST_ASSERT( FBuild::Get().GetStats().GetCacheStores() == 4 );
        // Check stats: Seen, Built, Type
        CheckStatsNode( 4, 4, Node::OBJECT_NODE );
        CheckStatsNode( 2, 2, Node::OBJECT_LIST_NODE );
    }

    // Read from cache
    {
        FBuildTestOptions optionsCopy( options );
        optionsCopy.m_UseCacheRead = true;
        FBuildForTest fBuild( optionsCopy );
        TEST_ASSERT( fBuild.Initialize( nullptr ) );

        TEST_ASSERT( fBuild.Build( target ) );

        TEST_ASSERT( FBuild::Get().GetStats().GetCacheHits() == 4 );
        // Check stats: Seen, Built, Type
        CheckStatsNode( 4, 0, Node::OBJECT_NODE );
        CheckStatsNode( 2, 2, Node::OBJECT_LIST_NODE );
    }
}

//------------------------------------------------------------------------------
TEST_CASE( TestPrecompiledHeaders, Deoptimization )
{
    // Initialize
    FBuildTestOptions options;
    options.m_ConfigFile = "Tools/FBuild/FBuildTest/Data/TestPrecompiledHeaders/Deoptimization/fbuild.bff";
    options.m_ForceCleanBuild = true;

    FBuildForTest fBuild( options );
    TEST_ASSERT( fBuild.Initialize( nullptr ) );

    // Copy files to temp dir
    TEST_ASSERT( FileIO::EnsurePathExists( AStackString( "../tmp/Test/PrecompiledHeaders/Deoptimization/" ) ) );
    TEST_ASSERT( FileIO::FileCopy( "Tools/FBuild/FBuildTest/Data/TestPrecompiledHeaders/Deoptimization/PrecompiledHeader.cpp", "../tmp/Test/PrecompiledHeaders/Deoptimization/PrecompiledHeader.cpp" ) );
    TEST_ASSERT( FileIO::FileCopy( "Tools/FBuild/FBuildTest/Data/TestPrecompiledHeaders/Deoptimization/PrecompiledHeader.h", "../tmp/Test/PrecompiledHeaders/Deoptimization/PrecompiledHeader.h" ) );

    // Mark copied files as writable, which normally activates deoptimization (since we have it enabled)
    // It should be ignored for the precompiled header (which is what we are testing)
    TEST_ASSERT( FileIO::SetReadOnly( "../tmp/Test/PrecompiledHeaders/Deoptimization/PrecompiledHeader.cpp", false ) );
    TEST_ASSERT( FileIO::SetReadOnly( "../tmp/Test/PrecompiledHeaders/Deoptimization/PrecompiledHeader.h", false ) );

    AStackString target( "PCHTest-Deoptimization" );

    TEST_ASSERT( fBuild.Build( target ) );

    // Check stats: Seen, Built, Type
    CheckStatsNode( 1, 1, Node::OBJECT_NODE );
    CheckStatsNode( 1, 1, Node::OBJECT_LIST_NODE );

    // Make sure nothing was deoptimized
    TEST_ASSERT( GetRecordedOutput().FindI( "**Deoptimized**" ) == nullptr );
}

//------------------------------------------------------------------------------
TEST_CASE( TestPrecompiledHeaders, PCHOnly )
{
    // Initialize
    FBuildTestOptions options;
    options.m_ConfigFile = "Tools/FBuild/FBuildTest/Data/TestPrecompiledHeaders/PCHOnly/fbuild.bff";

    FBuildForTest fBuild( options );
    TEST_ASSERT( fBuild.Initialize( nullptr ) );

    TEST_ASSERT( fBuild.Build( "PCHOnly" ) );

    // Check stats: Seen, Built, Type
    CheckStatsNode( 1, 1, Node::OBJECT_NODE );
    CheckStatsNode( 1, 1, Node::OBJECT_LIST_NODE );
}

//------------------------------------------------------------------------------
TEST_CASE( TestPrecompiledHeaders, PCHReuse )
{
    // Initialize
    FBuildTestOptions options;
    options.m_ConfigFile = "Tools/FBuild/FBuildTest/Data/TestPrecompiledHeaders/PCHReuse/fbuild.bff";

    FBuildForTest fBuild( options );
    TEST_ASSERT( fBuild.Initialize( nullptr ) );

    TEST_ASSERT( fBuild.Build( "PCHReuse" ) );

    // Check stats: Seen, Built, Type
    CheckStatsNode( 2, 2, Node::OBJECT_NODE );
    CheckStatsNode( 1, 1, Node::OBJECT_LIST_NODE );
}

//------------------------------------------------------------------------------
TEST_CASE( TestPrecompiledHeaders, PCHMultipleUses )
{
    // Initialize
    FBuildTestOptions options;
    options.m_ConfigFile = "Tools/FBuild/FBuildTest/Data/TestPrecompiledHeaders/PCHMultipleUses/fbuild.bff";

    FBuildForTest fBuild( options );
    TEST_ASSERT( fBuild.Initialize( nullptr ) );

    TEST_ASSERT( fBuild.Build( "PCHMultipleUses" ) );

    // Check stats: Seen, Built, Type
    CheckStatsNode( 3, 3, Node::OBJECT_NODE );
    CheckStatsNode( 2, 2, Node::OBJECT_LIST_NODE );
    CheckStatsNode( 1, 1, Node::EXE_NODE );
}

//------------------------------------------------------------------------------
TEST_CASE( TestPrecompiledHeaders, PCHRedefinitionError )
{
    // Initialize
    FBuildTestOptions options;
    options.m_ConfigFile = "Tools/FBuild/FBuildTest/Data/TestPrecompiledHeaders/PCHRedefinitionError/fbuild.bff";

    FBuildForTest fBuild( options );

    // Expect failure
    TEST_ASSERT( fBuild.Initialize( nullptr ) == false );
    TEST_ASSERT( GetRecordedOutput().Find( "Error #1301 - ObjectList() - Precompiled Header target" ) &&
                 GetRecordedOutput().Find( "has already been defined" ) );
}

//------------------------------------------------------------------------------
TEST_CASE( TestPrecompiledHeaders, PCHNotDefinedError )
{
    // Initialize
    FBuildTestOptions options;
    options.m_ConfigFile = "Tools/FBuild/FBuildTest/Data/TestPrecompiledHeaders/PCHNotDefinedError/fbuild.bff";

    FBuildForTest fBuild( options );

    // Expect failure
    TEST_ASSERT( fBuild.Initialize( nullptr ) == false );
    TEST_ASSERT( GetRecordedOutput().Find( "Error #1104 - ObjectList() - 'PCHOutputFile'" ) &&
                 GetRecordedOutput().Find( " is not defined" ) );
}

//------------------------------------------------------------------------------
#if defined( __WINDOWS__ )
TEST_CASE( TestPrecompiledHeaders, PrecompiledHeaderCacheAnalyze_MSVC )
{
    // Initialize
    FBuildTestOptions options;
    options.m_ConfigFile = "Tools/FBuild/FBuildTest/Data/TestPrecompiledHeaders/CacheAnalyze_MSVC/fbuild.bff";
    options.m_ForceCleanBuild = true;

    AStackString target( "PCHTest-CacheAnalyze_MSVC" );
    const char * files[] = {
        "../tmp/Test/PrecompiledHeaders/CacheAnalyze_MSVC/1/PrecompiledHeader.pch",
        "../tmp/Test/PrecompiledHeaders/CacheAnalyze_MSVC/1/PrecompiledHeader.pch.obj",
        "../tmp/Test/PrecompiledHeaders/CacheAnalyze_MSVC/1/PrecompiledHeader.pchast",
        "../tmp/Test/PrecompiledHeaders/CacheAnalyze_MSVC/1/PrecompiledHeader.pch.nativecodeanalysis.xml",

        "../tmp/Test/PrecompiledHeaders/CacheAnalyze_MSVC/2/PrecompiledHeader.pch",
        "../tmp/Test/PrecompiledHeaders/CacheAnalyze_MSVC/2/PrecompiledHeader.pch.obj",
        "../tmp/Test/PrecompiledHeaders/CacheAnalyze_MSVC/2/PrecompiledHeader.pchast",
        "../tmp/Test/PrecompiledHeaders/CacheAnalyze_MSVC/2/PrecompiledHeader.pch.nativecodeanalysis.xml",

        "../tmp/Test/PrecompiledHeaders/CacheAnalyze_MSVC/3/With.Period/PrecompiledHeader.pch",
        "../tmp/Test/PrecompiledHeaders/CacheAnalyze_MSVC/3/With.Period/PrecompiledHeader.pch.obj",
        "../tmp/Test/PrecompiledHeaders/CacheAnalyze_MSVC/3/With.Period/PrecompiledHeader.pchast",
        "../tmp/Test/PrecompiledHeaders/CacheAnalyze_MSVC/3/With.Period/PrecompiledHeader.pch.nativecodeanalysis.xml",

        "../tmp/Test/PrecompiledHeaders/CacheAnalyze_MSVC/4/PrecompiledHeaderWith.Period.pch",
        "../tmp/Test/PrecompiledHeaders/CacheAnalyze_MSVC/4/PrecompiledHeaderWith.Period.pch.obj",
        "../tmp/Test/PrecompiledHeaders/CacheAnalyze_MSVC/4/PrecompiledHeaderWith.Period.pchast",
        "../tmp/Test/PrecompiledHeaders/CacheAnalyze_MSVC/4/PrecompiledHeaderWith.Period.pch.nativecodeanalysis.xml",

        "../tmp/Test/PrecompiledHeaders/CacheAnalyze_MSVC/5/PrecompiledHeader.otherext",
        "../tmp/Test/PrecompiledHeaders/CacheAnalyze_MSVC/5/PrecompiledHeader.otherext.obj",
        "../tmp/Test/PrecompiledHeaders/CacheAnalyze_MSVC/5/PrecompiledHeader.otherextast",
        "../tmp/Test/PrecompiledHeaders/CacheAnalyze_MSVC/5/PrecompiledHeader.otherext.nativecodeanalysis.xml",
    };

    // Write to cache
    {
        FBuildTestOptions optionsCopy( options );
        optionsCopy.m_UseCacheWrite = true;
        FBuildForTest fBuild( optionsCopy );
        TEST_ASSERT( fBuild.Initialize( nullptr ) );

        // Delete pch and related files (if left from an old build)
        for ( const char * file : files )
        {
            FileIO::FileDelete( file );
        }

        TEST_ASSERT( fBuild.Build( target ) );

        // Check stats: Seen, Built, Type
        CheckStatsNode( 15, 15, Node::OBJECT_NODE );
        CheckStatsNode( 5, 5, Node::OBJECT_LIST_NODE );

        TEST_ASSERT( FBuild::Get().GetStats().GetCacheStores() == 15 );

        // Make sure all expected files exist now
        for ( const char * file : files )
        {
            TEST_ASSERT( FileIO::FileExists( file ) );
        }

        // Delete pch and related files
        for ( const char * file : files )
        {
            TEST_ASSERT( FileIO::FileDelete( file ) );
        }
    }

    // Read from cache
    {
        FBuildTestOptions optionsCopy( options );
        optionsCopy.m_UseCacheRead = true;
        FBuildForTest fBuild( optionsCopy );
        TEST_ASSERT( fBuild.Initialize( nullptr ) );

        TEST_ASSERT( fBuild.Build( target ) );

        // Check stats: Seen, Built, Type
        CheckStatsNode( 15, 0, Node::OBJECT_NODE );
        CheckStatsNode( 5, 5, Node::OBJECT_LIST_NODE );

        TEST_ASSERT( FBuild::Get().GetStats().GetCacheHits() == 15 );

        // Ensure all files were retrieved from the cache
        for ( const char * file : files )
        {
            TEST_ASSERT( FileIO::FileExists( file ) );
        }
    }
}
#endif

//------------------------------------------------------------------------------
#if defined( __WINDOWS__ )
TEST_CASE( TestPrecompiledHeaders, TestPCHClangWindows )
{
    FBuildTestOptions options;
    options.m_ConfigFile = "Tools/FBuild/FBuildTest/Data/TestPrecompiledHeaders/fbuild.bff";
    options.m_ForceCleanBuild = true;
    options.m_UseCacheWrite = true;

    AStackString obj( "../tmp/Test/PrecompiledHeaders/Clang-Windows/PCHUser.obj" );
    AStackString pch( "../tmp/Test/PrecompiledHeaders/Clang-Windows/PrecompiledHeader.pch" );
    AStackString pchObj( "../tmp/Test/PrecompiledHeaders/Clang-Windows/PrecompiledHeader.pch.obj" );
    EnsureFileDoesNotExist( obj );
    EnsureFileDoesNotExist( pch );
    EnsureFileDoesNotExist( pchObj );

    FBuildForTest fBuild( options );
    TEST_ASSERT( fBuild.Initialize( nullptr ) );

    AStackString target( "PCHTestClang-Windows" );

    TEST_ASSERT( fBuild.Build( target ) );
    TEST_ASSERT( fBuild.SaveDependencyGraph( GetPCHDBClangFileName() ) );

    // Check stats: Seen, Built, Type
    CheckStatsNode( 5, 3, Node::FILE_NODE );  // pch.cpp + pch.h + slow.h + pchuser.cpp + linker
    CheckStatsNode( 1, 1, Node::COMPILER_NODE );
    CheckStatsNode( 2, 2, Node::OBJECT_NODE );// obj + pch obj
    CheckStatsNode( 1, 1, Node::OBJECT_LIST_NODE );

    // check we wrote all objects to the cache
    TEST_ASSERT( fBuild.GetStats().GetStatsFor( Node::OBJECT_NODE ).m_NumCacheStores == 2 ); // can store the pch & the obj using it
}
#endif

//------------------------------------------------------------------------------
#if defined( __WINDOWS__ )
TEST_CASE( TestPrecompiledHeaders, TestPCHClangWindows_NoRebuild )
{
    FBuildTestOptions options;
    options.m_ConfigFile = "Tools/FBuild/FBuildTest/Data/TestPrecompiledHeaders/fbuild.bff";

    FBuildForTest fBuild( options );
    TEST_ASSERT( fBuild.Initialize( GetPCHDBClangFileName() ) );

    AStackString target( "PCHTestClang-Windows" );

    TEST_ASSERT( fBuild.Build( target ) );
    TEST_ASSERT( fBuild.SaveDependencyGraph( GetPCHDBClangFileName() ) );

    // Check stats: Seen, Built, Type
    CheckStatsNode( 5, 5, Node::FILE_NODE );  // pch.cpp + pch.h + slow.h + pchuser.cpp + linker
    CheckStatsNode( 1, 0, Node::COMPILER_NODE );
    CheckStatsNode( 2, 0, Node::OBJECT_NODE );// obj + pch obj
    CheckStatsNode( 1, 0, Node::OBJECT_LIST_NODE );
}
#endif

//------------------------------------------------------------------------------
#if defined( __WINDOWS__ )
TEST_CASE( TestPrecompiledHeaders, TestPCHClangWindowsWithCache )
{
    FBuildTestOptions options;
    options.m_ConfigFile = "Tools/FBuild/FBuildTest/Data/TestPrecompiledHeaders/fbuild.bff";
    options.m_ForceCleanBuild = true;
    options.m_UseCacheRead = true;

    AStackString obj( "../tmp/Test/PrecompiledHeaders/Clang-Windows/PCHUser.obj" );
    AStackString pch( "../tmp/Test/PrecompiledHeaders/Clang-Windows/PrecompiledHeader.pch" );
    AStackString pchObj( "../tmp/Test/PrecompiledHeaders/Clang-Windows/PrecompiledHeader.pch.obj" );
    EnsureFileDoesNotExist( obj );
    EnsureFileDoesNotExist( pch );
    EnsureFileDoesNotExist( pchObj );

    FBuildForTest fBuild( options );
    TEST_ASSERT( fBuild.Initialize( nullptr ) );

    AStackString target( "PCHTestClang-Windows" );

    TEST_ASSERT( fBuild.Build( target ) );
    TEST_ASSERT( fBuild.SaveDependencyGraph( GetPCHDBClangFileName() ) );

    EnsureFileExists( obj );
    EnsureFileExists( pch );

    // Check stats: Seen, Built, Type
    CheckStatsNode( 5, 3, Node::FILE_NODE );  // pch.cpp + pch.h + slow.h + pchuser.cpp + linker
    CheckStatsNode( 1, 1, Node::COMPILER_NODE );
    CheckStatsNode( 2, 0, Node::OBJECT_NODE );// obj + pch obj
    CheckStatsNode( 1, 1, Node::OBJECT_LIST_NODE );

    // check we got both objects from the cache
    TEST_ASSERT( fBuild.GetStats().GetStatsFor( Node::OBJECT_NODE ).m_NumCacheHits == 2 ); // pch & the obj using it
}
#endif

//------------------------------------------------------------------------------
#if defined( __WINDOWS__ )
TEST_CASE( TestPrecompiledHeaders, TestPCHClangWindowsWithCache_NoRebuild )
{
    FBuildTestOptions options;
    options.m_ConfigFile = "Tools/FBuild/FBuildTest/Data/TestPrecompiledHeaders/fbuild.bff";

    FBuildForTest fBuild( options );
    TEST_ASSERT( fBuild.Initialize( GetPCHDBClangFileName() ) );

    AStackString target( "PCHTestClang-Windows" );

    TEST_ASSERT( fBuild.Build( target ) );
    TEST_ASSERT( fBuild.SaveDependencyGraph( GetPCHDBClangFileName() ) );

    // Check stats: Seen, Built, Type
    CheckStatsNode( 5, 5, Node::FILE_NODE );  // pch.cpp + pch.h + slow.h + pchuser.cpp + linker
    CheckStatsNode( 1, 0, Node::COMPILER_NODE );
    CheckStatsNode( 2, 0, Node::OBJECT_NODE );// obj + pch obj
    CheckStatsNode( 1, 0, Node::OBJECT_LIST_NODE );
}
#endif

//------------------------------------------------------------------------------
