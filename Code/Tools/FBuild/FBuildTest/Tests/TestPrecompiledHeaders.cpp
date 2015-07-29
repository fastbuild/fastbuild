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
	FBuildStats Build( FBuildOptions options = FBuildOptions(), bool useDB = true ) const;
	const char * GetPCHDBFileName() const { return "../../../../ftmp/Test/PrecompiledHeaders/pch.fdb"; }
	const char * GetPCHDBClangFileName() const { return "../../../../ftmp/Test/PrecompiledHeaders/pchclang.fdb"; }

	// Tests
	void TestPCH() const;
	void TestPCH_NoRebuild() const;
	void TestPCHWithCache() const;
	void TestPCHWithCache_NoRebuild() const;
	void TestPCHClang() const;
	void TestPCHClang_NoRebuild() const;
	void TestPCHClangWithCache() const;
	void TestPCHClangWithCache_NoRebuild() const;
};

// Register Tests
//------------------------------------------------------------------------------
REGISTER_TESTS_BEGIN( TestPrecompiledHeaders )
	REGISTER_TEST( TestPCH )
	REGISTER_TEST( TestPCH_NoRebuild )	
	REGISTER_TEST( TestPCHWithCache )
	REGISTER_TEST( TestPCHWithCache_NoRebuild )
	REGISTER_TEST( TestPCHClang )
	REGISTER_TEST( TestPCHClang_NoRebuild )
	REGISTER_TEST( TestPCHClangWithCache )
	REGISTER_TEST( TestPCHClangWithCache_NoRebuild )
REGISTER_TESTS_END

// Build
//------------------------------------------------------------------------------
FBuildStats TestPrecompiledHeaders::Build( FBuildOptions options, bool useDB ) const
{
	options.m_ConfigFile = "Data/TestPrecompiledHeaders/fbuild.bff";
	options.m_ShowSummary = true; // required to generate stats for node count checks

	FBuild fBuild( options );
	TEST_ASSERT( fBuild.Initialize( useDB ? GetPCHDBFileName() : nullptr ) );

	AStackString<> target( "PCHTest" );

	TEST_ASSERT( fBuild.Build( target ) );
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
		AStackString<> obj( "../../../../ftmp/Test/PrecompiledHeaders/PCHUser.obj" );
	#else
		AStackString<> obj( "../../../../ftmp/Test/PrecompiledHeaders/PCHUser.o" );
	#endif
	AStackString<> pch( "../../../../ftmp/Test/PrecompiledHeaders/PrecompiledHeader.pch" );
	AStackString<> lib( "../../../../ftmp/Test/PrecompiledHeaders/TestPCH.lib" );
	EnsureFileDoesNotExist( obj );
	EnsureFileDoesNotExist( pch );
	EnsureFileDoesNotExist( lib );

	FBuildStats stats = Build( options, false ); // don't use DB

	EnsureFileExists( obj );
	EnsureFileExists( pch );
	EnsureFileExists( lib );

	// Check stats
	//						Seen,	Built,	Type
	CheckStatsNode ( stats,	3,		2,		Node::FILE_NODE );	// cpp + pch 
	CheckStatsNode ( stats,	1,		1,		Node::COMPILER_NODE );
	CheckStatsNode ( stats,	2,		2,		Node::OBJECT_NODE );// obj + pch obj
	CheckStatsNode ( stats,	1,		1,		Node::LIBRARY_NODE );
	CheckStatsNode ( stats,	1,		1,		Node::DIRECTORY_LIST_NODE );
	CheckStatsNode ( stats,	1,		1,		Node::ALIAS_NODE );
	CheckStatsNode ( stats,	1,		1,		Node::DLL_NODE );
	CheckStatsTotal( stats,	10,		9 );

	// check we wrote all objects to the cache
	TEST_ASSERT( stats.GetStatsFor( Node::OBJECT_NODE ).m_NumCacheStores == 1 ); // only the main obj can be cached
}

// TestPCH_NoRebuild
//------------------------------------------------------------------------------
void TestPrecompiledHeaders::TestPCH_NoRebuild() const
{
	FBuildStats stats = Build();

	// Check stats
	//						Seen,	Built,	Type
	CheckStatsNode ( stats,	3,		3,		Node::FILE_NODE );	// cpp + pch cpp + pch .h
	CheckStatsNode ( stats,	1,		0,		Node::COMPILER_NODE );
	CheckStatsNode ( stats,	2,		0,		Node::OBJECT_NODE );// obj + pch obj
	CheckStatsNode ( stats,	1,		0,		Node::LIBRARY_NODE );
	CheckStatsNode ( stats,	1,		1,		Node::DIRECTORY_LIST_NODE );
	CheckStatsNode ( stats,	1,		1,		Node::ALIAS_NODE );
	CheckStatsNode ( stats,	1,		0,		Node::DLL_NODE );
	CheckStatsTotal( stats,	10,		5 );
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
		AStackString<> obj( "../../../../ftmp/Test/PrecompiledHeaders/PCHUser.obj" );
	#else
		AStackString<> obj( "../../../../ftmp/Test/PrecompiledHeaders/PCHUser.o" );
	#endif
	AStackString<> pch( "../../../../ftmp/Test/PrecompiledHeaders/PrecompiledHeader.pch" );
	AStackString<> lib( "../../../../ftmp/Test/PrecompiledHeaders/TestPCH.lib" );
	EnsureFileDoesNotExist( obj );
	EnsureFileDoesNotExist( pch );
	EnsureFileDoesNotExist( lib );

	FBuildStats stats = Build( options, false ); // don't use DB

	EnsureFileExists( obj );
	EnsureFileExists( pch );
	EnsureFileExists( lib );

	// Check stats
	//						Seen,	Built,	Type
	CheckStatsNode ( stats,	3,		2,		Node::FILE_NODE );	// cpp + pch
	CheckStatsNode ( stats,	1,		1,		Node::COMPILER_NODE );
	CheckStatsNode ( stats,	2,		1,		Node::OBJECT_NODE );// obj + pch obj (build pch only)
	CheckStatsNode ( stats,	1,		1,		Node::LIBRARY_NODE );
	CheckStatsNode ( stats,	1,		1,		Node::DIRECTORY_LIST_NODE );
	CheckStatsNode ( stats,	1,		1,		Node::ALIAS_NODE );
	CheckStatsNode ( stats,	1,		1,		Node::DLL_NODE );
	CheckStatsTotal( stats,	10,		8 );

	// check all objects came from the cache
	TEST_ASSERT( stats.GetStatsFor( Node::OBJECT_NODE ).m_NumCacheHits == 1 );
}

// TestPCHWithCache_NoRebuild
//------------------------------------------------------------------------------
void TestPrecompiledHeaders::TestPCHWithCache_NoRebuild() const
{
	FBuildStats stats = Build();

	// Check stats
	//						Seen,	Built,	Type
	CheckStatsNode ( stats,	3,		3,		Node::FILE_NODE );	// cpp + pch cpp + pch .h
	CheckStatsNode ( stats,	1,		0,		Node::COMPILER_NODE );
	CheckStatsNode ( stats,	2,		0,		Node::OBJECT_NODE );// obj + pch obj
	CheckStatsNode ( stats,	1,		0,		Node::LIBRARY_NODE );
	CheckStatsNode ( stats,	1,		1,		Node::DIRECTORY_LIST_NODE );
	CheckStatsNode ( stats,	1,		1,		Node::ALIAS_NODE );
	CheckStatsNode ( stats,	1,		0,		Node::DLL_NODE );
	CheckStatsTotal( stats,	10,		5 );
}

// TestPCHClang
//------------------------------------------------------------------------------
void TestPrecompiledHeaders::TestPCHClang() const
{
	FBuildOptions options;
	options.m_ConfigFile = "Data/TestPrecompiledHeaders/fbuild.bff";
	options.m_ForceCleanBuild = true;
	options.m_UseCacheWrite = true;
	options.m_ShowSummary = true; // required to generate stats for node count checks

	#if defined( __WINDOWS__ )
		AStackString<> obj( "../../../../ftmp/Test/PrecompiledHeaders/Clang/PCHUser.obj" );
	#else
		AStackString<> obj( "../../../../ftmp/Test/PrecompiledHeaders/Clang/PCHUser.o" );
	#endif
	AStackString<> pch( "../../../../ftmp/Test/PrecompiledHeaders/Clang/PrecompiledHeader.pch" );
	EnsureFileDoesNotExist( obj );
	EnsureFileDoesNotExist( pch );

	FBuild fBuild( options );
	TEST_ASSERT( fBuild.Initialize( nullptr ) );

	AStackString<> target( "PCHTestClang" );

	TEST_ASSERT( fBuild.Build( target ) );
	TEST_ASSERT( fBuild.SaveDependencyGraph( GetPCHDBClangFileName() ) );	

	EnsureFileExists( obj );
	EnsureFileExists( pch );

	// Check stats
	//				Seen,	Built,	Type
	CheckStatsNode ( 2,		2,		Node::FILE_NODE );	// cpp + pch
	CheckStatsNode ( 1,		1,		Node::COMPILER_NODE );
	CheckStatsNode ( 2,		2,		Node::OBJECT_NODE );// obj + pch obj
	CheckStatsNode ( 1,		1,		Node::OBJECT_LIST_NODE );
	CheckStatsTotal( 6,		6 );

	// check we wrote all objects to the cache
	TEST_ASSERT( fBuild.GetStats().GetStatsFor( Node::OBJECT_NODE ).m_NumCacheStores == 2 ); // can store the pch & the obj using it
}

// TestPCHClang_NoRebuild
//------------------------------------------------------------------------------
void TestPrecompiledHeaders::TestPCHClang_NoRebuild() const
{
	FBuildOptions options;
	options.m_ConfigFile = "Data/TestPrecompiledHeaders/fbuild.bff";
	options.m_ShowSummary = true; // required to generate stats for node count checks

	AStackString<> obj( "../../../../ftmp/Test/PrecompiledHeaders/Clang/PCHUser.obj" );
	AStackString<> pch( "../../../../ftmp/Test/PrecompiledHeaders/Clang/PrecompiledHeader.pch" );

	FBuild fBuild( options );
	TEST_ASSERT( fBuild.Initialize( GetPCHDBClangFileName() ) );

	AStackString<> target( "PCHTestClang" );

	TEST_ASSERT( fBuild.Build( target ) );
	TEST_ASSERT( fBuild.SaveDependencyGraph( GetPCHDBClangFileName() ) );	

	// Check stats
	//				Seen,	Built,	Type
	CheckStatsNode ( 2,		2,		Node::FILE_NODE );	// cpp + pch
	CheckStatsNode ( 1,		0,		Node::COMPILER_NODE );
	CheckStatsNode ( 2,		0,		Node::OBJECT_NODE );// obj + pch obj
	CheckStatsNode ( 1,		0,		Node::OBJECT_LIST_NODE );
	CheckStatsTotal( 6,		2 );
}

// TestPCHClangWithCache
//------------------------------------------------------------------------------
void TestPrecompiledHeaders::TestPCHClangWithCache() const
{
	FBuildOptions options;
	options.m_ConfigFile = "Data/TestPrecompiledHeaders/fbuild.bff";
	options.m_ForceCleanBuild = true;
	options.m_UseCacheRead = true;
	options.m_ShowSummary = true; // required to generate stats for node count checks

	AStackString<> obj( "../../../../ftmp/Test/PrecompiledHeaders/Clang/PCHUser.obj" );
	AStackString<> pch( "../../../../ftmp/Test/PrecompiledHeaders/Clang/PrecompiledHeader.pch" );
	EnsureFileDoesNotExist( obj );
	EnsureFileDoesNotExist( pch );

	FBuild fBuild( options );
	TEST_ASSERT( fBuild.Initialize( nullptr ) );

	AStackString<> target( "PCHTestClang" );

	TEST_ASSERT( fBuild.Build( target ) );
	TEST_ASSERT( fBuild.SaveDependencyGraph( GetPCHDBClangFileName() ) );	

	EnsureFileExists( obj );
	EnsureFileExists( pch );

	// Check stats
	//				Seen,	Built,	Type
	CheckStatsNode ( 2,		2,		Node::FILE_NODE );	// cpp + pch
	CheckStatsNode ( 1,		1,		Node::COMPILER_NODE );
	CheckStatsNode ( 2,		0,		Node::OBJECT_NODE );// obj + pch obj
	CheckStatsNode ( 1,		1,		Node::OBJECT_LIST_NODE );
	CheckStatsTotal( 6,		4 );

	// check we got both objects from the cache
	TEST_ASSERT( fBuild.GetStats().GetStatsFor( Node::OBJECT_NODE ).m_NumCacheHits == 2 ); // pch & the obj using it
}

// TestPCHClangWithCache_NoRebuild
//------------------------------------------------------------------------------
void TestPrecompiledHeaders::TestPCHClangWithCache_NoRebuild() const
{
	FBuildOptions options;
	options.m_ConfigFile = "Data/TestPrecompiledHeaders/fbuild.bff";
	options.m_ShowSummary = true; // required to generate stats for node count checks

	FBuild fBuild( options );
	TEST_ASSERT( fBuild.Initialize( GetPCHDBClangFileName() ) );

	AStackString<> target( "PCHTestClang" );

	TEST_ASSERT( fBuild.Build( target ) );
	TEST_ASSERT( fBuild.SaveDependencyGraph( GetPCHDBClangFileName() ) );	

	// Check stats
	//				Seen,	Built,	Type
	CheckStatsNode ( 2,		2,		Node::FILE_NODE );	// cpp + pch
	CheckStatsNode ( 1,		0,		Node::COMPILER_NODE );
	CheckStatsNode ( 2,		0,		Node::OBJECT_NODE );// obj + pch obj
	CheckStatsNode ( 1,		0,		Node::OBJECT_LIST_NODE );
	CheckStatsTotal( 6,		2 );
}

//------------------------------------------------------------------------------
