// TestPrecompiledHeaders.cpp
//------------------------------------------------------------------------------

// Includes
//------------------------------------------------------------------------------
#include "FBuildTest.h"

#include "Tools/FBuild/FBuildCore/FBuild.h"

#include "Core/FileIO/FileIO.h"
#include "Core/Strings/AStackString.h"

// TestPrecompiledHeaders
//------------------------------------------------------------------------------
class TestPrecompiledHeaders : public FBuildTest
{
private:
	DECLARE_TESTS

	// Helpers
	FBuildStats Build( FBuildOptions options = FBuildOptions(),
					   bool useDB = true,
					   const char * target = nullptr ) const;
	const char * GetPCHDBFileName() const { return "../../../../tmp/Test/PrecompiledHeaders/pch.fdb"; }
	const char * GetPCHDBClangFileName() const { return "../../../../tmp/Test/PrecompiledHeaders/pchclang-windows.fdb"; }

	// Tests
	void TestPCH() const;
	void TestPCH_NoRebuild() const;
	void TestPCHWithCache() const;
	void TestPCHWithCache_NoRebuild() const;
	void PreventUselessCacheTraffic_MSVC() const;
    
    // Clang on Windows
    #if defined( __WINDOWS__ )
        void TestPCHClangWindows() const;
        void TestPCHClangWindows_NoRebuild() const;
        void TestPCHClangWindowsWithCache() const;
        void TestPCHClangWindowsWithCache_NoRebuild() const;
    #endif
};

// Register Tests
//------------------------------------------------------------------------------
REGISTER_TESTS_BEGIN( TestPrecompiledHeaders )
	REGISTER_TEST( TestPCH )
	REGISTER_TEST( TestPCH_NoRebuild )	
	REGISTER_TEST( TestPCHWithCache )
	REGISTER_TEST( TestPCHWithCache_NoRebuild )
    #if defined( __WINDOWS__ )
		REGISTER_TEST( PreventUselessCacheTraffic_MSVC )
        REGISTER_TEST( TestPCHClangWindows )
        REGISTER_TEST( TestPCHClangWindows_NoRebuild )
        REGISTER_TEST( TestPCHClangWindowsWithCache )
        REGISTER_TEST( TestPCHClangWindowsWithCache_NoRebuild )
    #endif
REGISTER_TESTS_END

// Build
//------------------------------------------------------------------------------
FBuildStats TestPrecompiledHeaders::Build( FBuildOptions options, bool useDB, const char * target ) const
{
	options.m_ConfigFile = "Data/TestPrecompiledHeaders/fbuild.bff";
	options.m_ShowSummary = true; // required to generate stats for node count checks

	FBuild fBuild( options );
	TEST_ASSERT( fBuild.Initialize( useDB ? GetPCHDBFileName() : nullptr ) );

	TEST_ASSERT( fBuild.Build( AStackString<>( target ? target : "PCHTest" ) ) );
	TEST_ASSERT( fBuild.SaveDependencyGraph( GetPCHDBFileName() ) );	

	return fBuild.GetStats();
}

// TestPCH
//------------------------------------------------------------------------------
void TestPrecompiledHeaders::TestPCH() const
{
	FBuildOptions options;
	options.m_ForceCleanBuild = true;
	options.m_UseCacheWrite = true;
	options.m_ShowSummary = true; // required to generate stats for node count checks

	#if defined( __WINDOWS__ )
		AStackString<> obj( "../../../../tmp/Test/PrecompiledHeaders/PCHUser.obj" );
        AStackString<> pch( "../../../../tmp/Test/PrecompiledHeaders/PrecompiledHeader.pch" );
	#elif defined( __LINUX__ )
		AStackString<> obj( "../../../../tmp/Test/PrecompiledHeaders/PCHUser.o" );
        AStackString<> pch( "../../../../tmp/Test/PrecompiledHeaders/PrecompiledHeader.h.gch" );
    #elif defined( __OSX__ )
        AStackString<> obj( "../../../../tmp/Test/PrecompiledHeaders/PCHUser.o" );
        AStackString<> pch( "../../../../tmp/Test/PrecompiledHeaders/PrecompiledHeader.pch" );
	#endif
	EnsureFileDoesNotExist( obj );
	EnsureFileDoesNotExist( pch );

	FBuildStats stats = Build( options, false ); // don't use DB

	EnsureFileExists( obj );
	EnsureFileExists( pch );

	// Check stats
	//						Seen,	Built,	Type
	uint32_t numF = 3; // pch.h / slow.h / pchuser.cpp
	#if defined( __WINDOWS__ )
		numF++; // pch.cpp
	#endif
	CheckStatsNode ( stats,	numF,	2,		Node::FILE_NODE );
	CheckStatsNode ( stats,	1,		1,		Node::COMPILER_NODE );
	CheckStatsNode ( stats,	2,		2,		Node::OBJECT_NODE );// obj + pch obj
	CheckStatsNode ( stats,	1,		1,		Node::OBJECT_LIST_NODE );
	CheckStatsNode ( stats,	1,		1,		Node::DIRECTORY_LIST_NODE );
	CheckStatsNode ( stats,	1,		1,		Node::ALIAS_NODE );
	CheckStatsNode ( stats,	1,		1,		Node::EXE_NODE );
	CheckStatsTotal( stats,	numF+7,	9 );

	// check we wrote all objects to the cache
	TEST_ASSERT( stats.GetStatsFor( Node::OBJECT_NODE ).m_NumCacheStores == 2 ); // pch and obj using pch
}

// TestPCH_NoRebuild
//------------------------------------------------------------------------------
void TestPrecompiledHeaders::TestPCH_NoRebuild() const
{
	FBuildStats stats = Build();

	// Check stats
	//						Seen,	Built,	Type
	uint32_t numF = 3; // pch.h / slow.h / pchuser.cpp
	#if defined( __WINDOWS__ )
		numF++; // pch.cpp
	#endif
	CheckStatsNode ( stats,	numF,	numF,	Node::FILE_NODE );	// cpp + pch cpp + pch .h
	CheckStatsNode ( stats,	1,		0,		Node::COMPILER_NODE );
	CheckStatsNode ( stats,	2,		0,		Node::OBJECT_NODE );// obj + pch obj
	CheckStatsNode ( stats,	1,		0,		Node::OBJECT_LIST_NODE );
	CheckStatsNode ( stats,	1,		1,		Node::DIRECTORY_LIST_NODE );
	CheckStatsNode ( stats,	1,		1,		Node::ALIAS_NODE );
	CheckStatsNode ( stats,	1,		0,		Node::EXE_NODE );
	CheckStatsTotal( stats,	7+numF,	2+numF );
}

// TestPCHWithCache
//------------------------------------------------------------------------------
void TestPrecompiledHeaders::TestPCHWithCache() const
{
	FBuildOptions options;
	options.m_ForceCleanBuild = true;
	options.m_UseCacheRead = true;
	options.m_ShowSummary = true; // required to generate stats for node count checks

	#if defined( __WINDOWS__ )
		AStackString<> obj( "../../../../tmp/Test/PrecompiledHeaders/PCHUser.obj" );
        AStackString<> pch( "../../../../tmp/Test/PrecompiledHeaders/PrecompiledHeader.pch" );
	#elif defined( __LINUX__ )
		AStackString<> obj( "../../../../tmp/Test/PrecompiledHeaders/PCHUser.o" );
        AStackString<> pch( "../../../../tmp/Test/PrecompiledHeaders/PrecompiledHeader.h.gch" );
    #elif defined( __OSX__ )
        AStackString<> obj( "../../../../tmp/Test/PrecompiledHeaders/PCHUser.o" );
        AStackString<> pch( "../../../../tmp/Test/PrecompiledHeaders/PrecompiledHeader.pch" );
    #endif    
	EnsureFileDoesNotExist( obj );
	EnsureFileDoesNotExist( pch );

	FBuildStats stats = Build( options, false ); // don't use DB

	EnsureFileExists( obj );
	EnsureFileExists( pch );

	// Check stats
	//						Seen,	Built,	Type
	uint32_t numF = 3; // pch.h / slow.h / pchuser.cpp
	#if defined( __WINDOWS__ )
		numF++; // pch.cpp
	#endif
	CheckStatsNode ( stats,	numF,	2,		Node::FILE_NODE );	// cpp + pch
	CheckStatsNode ( stats,	1,		1,		Node::COMPILER_NODE );
	CheckStatsNode ( stats, 2,		0,		Node::OBJECT_NODE ); // obj + pch obj
	CheckStatsNode ( stats,	1,		1,		Node::OBJECT_LIST_NODE );
	CheckStatsNode ( stats,	1,		1,		Node::DIRECTORY_LIST_NODE );
	CheckStatsNode ( stats,	1,		1,		Node::ALIAS_NODE );
	CheckStatsNode ( stats,	1,		1,		Node::EXE_NODE );
	CheckStatsTotal( stats, 7+numF,	7 );

	// check all objects came from the cache
	TEST_ASSERT( stats.GetStatsFor( Node::OBJECT_NODE ).m_NumCacheHits == 2 ); // pch & obj
}

// TestPCHWithCache_NoRebuild
//------------------------------------------------------------------------------
void TestPrecompiledHeaders::TestPCHWithCache_NoRebuild() const
{
	FBuildStats stats = Build();

	// Check stats
	//						Seen,	Built,	Type
	uint32_t numF = 3; // pch.h / slow.h / pchuser.cpp
	#if defined( __WINDOWS__ )
		numF++; // pch.cpp
	#endif
	CheckStatsNode ( stats,	numF,	numF,	Node::FILE_NODE );	// cpp + pch cpp + pch .h
	CheckStatsNode ( stats,	1,		0,		Node::COMPILER_NODE );
	CheckStatsNode ( stats,	2,		0,		Node::OBJECT_NODE );// obj + pch obj
	CheckStatsNode ( stats,	1,		0,		Node::OBJECT_LIST_NODE );
	CheckStatsNode ( stats,	1,		1,		Node::DIRECTORY_LIST_NODE );
	CheckStatsNode ( stats,	1,		1,		Node::ALIAS_NODE );
	CheckStatsNode ( stats,	1,		0,		Node::EXE_NODE );
	CheckStatsTotal( stats,	7+numF,	2+numF );
}

// PreventUselessCacheTraffic_MSVC
//------------------------------------------------------------------------------
void TestPrecompiledHeaders::PreventUselessCacheTraffic_MSVC() const
{
	// Build the PCH locally, without going via the cache (no store, no hit)
	{
		FBuildOptions options;
		options.m_ForceCleanBuild = true;

		const bool useDB = true;
		const char * target = "../../../../tmp/Test/PrecompiledHeaders/PrecompiledHeader.pch";
		FBuildStats stats = Build( options, useDB, target );

		// Ensure PCH was built, and not cached
		CheckStatsNode ( stats, 1,		1,		Node::OBJECT_NODE ); // pch built
		TEST_ASSERT( stats.GetStatsFor( Node::OBJECT_NODE ).m_NumCacheHits == 0 );
		TEST_ASSERT( stats.GetStatsFor( Node::OBJECT_NODE ).m_NumCacheStores == 0 );
	}

	// Build the objects dependent on the PCH and make sure they are:
	// a) not retrieved (since they are incompatible with our locally made PCH)
	// b) not stored (no-one can ever use them, since we didn't store the PCH)
	{
		FBuildOptions options;
		options.m_UseCacheRead = true;
		options.m_UseCacheWrite = true;

		FBuildStats stats = Build( options );

		// Ensure PCH was built, and not cached
		CheckStatsNode ( stats, 2,		1,		Node::OBJECT_NODE ); // obj built
		TEST_ASSERT( stats.GetStatsFor( Node::OBJECT_NODE ).m_NumCacheHits == 0 );
		TEST_ASSERT( stats.GetStatsFor( Node::OBJECT_NODE ).m_NumCacheStores == 0 );
	}

}

// TestPCHClang
//------------------------------------------------------------------------------
#if defined( __WINDOWS__ )
    void TestPrecompiledHeaders::TestPCHClangWindows() const
    {
        FBuildOptions options;
        options.m_ConfigFile = "Data/TestPrecompiledHeaders/fbuild.bff";
        options.m_ForceCleanBuild = true;
        options.m_UseCacheWrite = true;
        options.m_ShowSummary = true; // required to generate stats for node count checks

        AStackString<> obj( "../../../../tmp/Test/PrecompiledHeaders/Clang-Windows/PCHUser.obj" );
        AStackString<> pch( "../../../../tmp/Test/PrecompiledHeaders/Clang-Windows/PrecompiledHeader.pch" );
        EnsureFileDoesNotExist( obj );
        EnsureFileDoesNotExist( pch );

        FBuild fBuild( options );
        TEST_ASSERT( fBuild.Initialize( nullptr ) );

        AStackString<> target( "PCHTestClang-Windows" );

        TEST_ASSERT( fBuild.Build( target ) );
        TEST_ASSERT( fBuild.SaveDependencyGraph( GetPCHDBClangFileName() ) );	

        EnsureFileExists( obj );
        EnsureFileExists( pch );

        // Check stats
        //				Seen,	Built,	Type
        CheckStatsNode ( 3,		2,		Node::FILE_NODE );	// pch.h + slow.h + pchuser.cpp
        CheckStatsNode ( 1,		1,		Node::COMPILER_NODE );
        CheckStatsNode ( 2,		2,		Node::OBJECT_NODE );// obj + pch obj
        CheckStatsNode ( 1,		1,		Node::OBJECT_LIST_NODE );
        CheckStatsTotal( 7,		6 );

        // check we wrote all objects to the cache
        TEST_ASSERT( fBuild.GetStats().GetStatsFor( Node::OBJECT_NODE ).m_NumCacheStores == 2 ); // can store the pch & the obj using it
    }
#endif

// TestPCHClang_NoRebuild
//------------------------------------------------------------------------------
#if defined( __WINDOWS__ )
    void TestPrecompiledHeaders::TestPCHClangWindows_NoRebuild() const
    {
        FBuildOptions options;
        options.m_ConfigFile = "Data/TestPrecompiledHeaders/fbuild.bff";
        options.m_ShowSummary = true; // required to generate stats for node count checks

        AStackString<> obj( "../../../../tmp/Test/PrecompiledHeaders/Clang-Windows/PCHUser.obj" );
        AStackString<> pch( "../../../../tmp/Test/PrecompiledHeaders/Clang-Windows/PrecompiledHeader.pch" );

        FBuild fBuild( options );
        TEST_ASSERT( fBuild.Initialize( GetPCHDBClangFileName() ) );

        AStackString<> target( "PCHTestClang-Windows" );

        TEST_ASSERT( fBuild.Build( target ) );
        TEST_ASSERT( fBuild.SaveDependencyGraph( GetPCHDBClangFileName() ) );	

        // Check stats
        //				Seen,	Built,	Type
        CheckStatsNode ( 3,		3,		Node::FILE_NODE );	// pch.h + slow.h + pchuser.cpp
        CheckStatsNode ( 1,		0,		Node::COMPILER_NODE );
        CheckStatsNode ( 2,		0,		Node::OBJECT_NODE );// obj + pch obj
        CheckStatsNode ( 1,		0,		Node::OBJECT_LIST_NODE );
        CheckStatsTotal( 7,		3 );
    }
#endif

// TestPCHClangWithCache
//------------------------------------------------------------------------------
#if defined( __WINDOWS__ )
    void TestPrecompiledHeaders::TestPCHClangWindowsWithCache() const
    {
        FBuildOptions options;
        options.m_ConfigFile = "Data/TestPrecompiledHeaders/fbuild.bff";
        options.m_ForceCleanBuild = true;
        options.m_UseCacheRead = true;
        options.m_ShowSummary = true; // required to generate stats for node count checks

        AStackString<> obj( "../../../../tmp/Test/PrecompiledHeaders/Clang-Windows/PCHUser.obj" );
        AStackString<> pch( "../../../../tmp/Test/PrecompiledHeaders/Clang-Windows/PrecompiledHeader.pch" );
        EnsureFileDoesNotExist( obj );
        EnsureFileDoesNotExist( pch );

        FBuild fBuild( options );
        TEST_ASSERT( fBuild.Initialize( nullptr ) );

        AStackString<> target( "PCHTestClang-Windows" );

        TEST_ASSERT( fBuild.Build( target ) );
        TEST_ASSERT( fBuild.SaveDependencyGraph( GetPCHDBClangFileName() ) );	

        EnsureFileExists( obj );
        EnsureFileExists( pch );

        // Check stats
        //				Seen,	Built,	Type
        CheckStatsNode ( 3,		2,		Node::FILE_NODE );	// pch.h + slow.h + pchuser.cpp
        CheckStatsNode ( 1,		1,		Node::COMPILER_NODE );
        CheckStatsNode ( 2,		0,		Node::OBJECT_NODE );// obj + pch obj
        CheckStatsNode ( 1,		1,		Node::OBJECT_LIST_NODE );
        CheckStatsTotal( 7,		4 );

        // check we got both objects from the cache
        TEST_ASSERT( fBuild.GetStats().GetStatsFor( Node::OBJECT_NODE ).m_NumCacheHits == 2 ); // pch & the obj using it
    }
#endif

// TestPCHClangWithCache_NoRebuild
//------------------------------------------------------------------------------
#if defined( __WINDOWS__ )
    void TestPrecompiledHeaders::TestPCHClangWindowsWithCache_NoRebuild() const
    {
        FBuildOptions options;
        options.m_ConfigFile = "Data/TestPrecompiledHeaders/fbuild.bff";
        options.m_ShowSummary = true; // required to generate stats for node count checks

        FBuild fBuild( options );
        TEST_ASSERT( fBuild.Initialize( GetPCHDBClangFileName() ) );

        AStackString<> target( "PCHTestClang-Windows" );

        TEST_ASSERT( fBuild.Build( target ) );
        TEST_ASSERT( fBuild.SaveDependencyGraph( GetPCHDBClangFileName() ) );	

        // Check stats
        //				Seen,	Built,	Type
        CheckStatsNode ( 3,		3,		Node::FILE_NODE );	// pch.h + slow.h + pchuser.cpp
        CheckStatsNode ( 1,		0,		Node::COMPILER_NODE );
        CheckStatsNode ( 2,		0,		Node::OBJECT_NODE );// obj + pch obj
        CheckStatsNode ( 1,		0,		Node::OBJECT_LIST_NODE );
        CheckStatsTotal( 7,		3 );
    }
#endif

//------------------------------------------------------------------------------
