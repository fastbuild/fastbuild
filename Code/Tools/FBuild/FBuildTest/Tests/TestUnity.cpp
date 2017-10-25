// TestUnity.cpp
//------------------------------------------------------------------------------

// Includes
//------------------------------------------------------------------------------
#include "FBuildTest.h"

#include "Tools/FBuild/FBuildCore/FBuild.h"
#include "Tools/FBuild/FBuildCore/BFF/BFFParser.h"

#include "Core/Containers/AutoPtr.h"
#include "Core/FileIO/FileIO.h"
#include "Core/FileIO/FileStream.h"
#include "Core/Process/Thread.h"
#include "Core/Strings/AStackString.h"

// TestUnity
//------------------------------------------------------------------------------
class TestUnity : public FBuildTest
{
private:
    DECLARE_TESTS

    // Helpers
    FBuildStats BuildGenerate( FBuildOptions options = FBuildOptions(), bool useDB = true ) const;
    const char * GetTestGenerateDBFileName() const { return "../../../../tmp/Test/Unity/generate.fdb"; }
    FBuildStats BuildCompile( FBuildOptions options = FBuildOptions(), bool useDB = true ) const;
    const char * GetTestCompileDBFileName() const { return "../../../../tmp/Test/Unity/compile.fdb"; }

    // Tests
    void TestGenerate() const;
    void TestGenerate_NoRebuild() const;
    void TestCompile() const;
    void TestCompile_NoRebuild() const;
    void TestGenerateFromExplicitList() const;
    void TestExcludedFiles() const;
    void IsolateFromUnity_Regression() const;
};

// Register Tests
//------------------------------------------------------------------------------
REGISTER_TESTS_BEGIN( TestUnity )
    REGISTER_TEST( TestGenerate )           // clean build of unity files
    REGISTER_TEST( TestGenerate_NoRebuild ) // check nothing rebuilds
    REGISTER_TEST( TestCompile )            // compile a library using unity inputs
    REGISTER_TEST( TestCompile_NoRebuild )  // check nothing rebuilds
    REGISTER_TEST( TestGenerateFromExplicitList ) // create a unity with manually provided files
    REGISTER_TEST( TestExcludedFiles )      // Ensure files are correctly excluded
    REGISTER_TEST( IsolateFromUnity_Regression )
REGISTER_TESTS_END

// BuildGenerate
//------------------------------------------------------------------------------
FBuildStats TestUnity::BuildGenerate( FBuildOptions options, bool useDB ) const
{
    options.m_ConfigFile = "Data/TestUnity/unity.bff";
    options.m_ShowSummary = true; // required to generate stats for node count checks

    FBuild fBuild( options );
    TEST_ASSERT( fBuild.Initialize( useDB ? GetTestGenerateDBFileName() : nullptr ) );

    // Implement Unity and activate this test
    TEST_ASSERT( fBuild.Build( AStackString<>( "Unity-Test" ) ) );
    TEST_ASSERT( fBuild.SaveDependencyGraph( GetTestGenerateDBFileName() ) );

    return fBuild.GetStats();
}

// TestGenerate
//------------------------------------------------------------------------------
void TestUnity::TestGenerate() const
{
    FBuildOptions options;
    options.m_ShowSummary = true; // required to generate stats for node count checks
    options.m_ForceCleanBuild = true;

    EnsureFileDoesNotExist( "../../../../tmp/Test/Unity/Unity1.cpp" );
    EnsureFileDoesNotExist( "../../../../tmp/Test/Unity/Unity2.cpp" );

    FBuildStats stats = BuildGenerate( options, false ); // don't use DB

    EnsureFileExists( "../../../../tmp/Test/Unity/Unity1.cpp" );
    EnsureFileExists( "../../../../tmp/Test/Unity/Unity2.cpp" );

    // Check stats
    //                      Seen,   Built,  Type
    CheckStatsNode ( stats, 1,      1,      Node::DIRECTORY_LIST_NODE );
    CheckStatsNode ( stats, 1,      1,      Node::UNITY_NODE );
    CheckStatsTotal( stats, 2,      2 );
}

// TestGenerate_NoRebuild
//------------------------------------------------------------------------------
void TestUnity::TestGenerate_NoRebuild() const
{
    AStackString<> unity1( "../../../../tmp/Test/Unity/Unity1.cpp" );
    AStackString<> unity2( "../../../../tmp/Test/Unity/Unity2.cpp" );

    EnsureFileExists( unity1 );
    EnsureFileExists( unity2 );

    // Unity must be "built" every time, but it only writes files when they change
    // so record the time before and after
    uint64_t dateTime1 = FileIO::GetFileLastWriteTime( unity1 );
    uint64_t dateTime2 = FileIO::GetFileLastWriteTime( unity2 );

    // NTFS file resolution is 100ns, so sleep long enough to ensure
    // an invalid write would modify the time
    #if defined( __WINDOWS__ )
        Thread::Sleep( 1 ); // 1ms
    #elif defined( __OSX__ )
        Thread::Sleep( 1000 ); // Work around low time resolution of HFS+
    #elif defined( __LINUX__ )
        Thread::Sleep( 1000 ); // Work around low time resolution of ext2/ext3/reiserfs and time caching used by used by others
    #endif

    FBuildStats stats = BuildGenerate();

    // Make sure files have not been changed
    TEST_ASSERT( dateTime1 == FileIO::GetFileLastWriteTime( unity1 ) );
    TEST_ASSERT( dateTime2 == FileIO::GetFileLastWriteTime( unity2 ) );

    // Check stats
    //                      Seen,   Built,  Type
    CheckStatsNode ( stats, 1,      1,      Node::DIRECTORY_LIST_NODE );
    CheckStatsNode ( stats, 1,      1,      Node::UNITY_NODE );
    CheckStatsTotal( stats, 2,      2 );
}

// BuildCompile
//------------------------------------------------------------------------------
FBuildStats TestUnity::BuildCompile( FBuildOptions options, bool useDB ) const
{
    options.m_ConfigFile = "Data/TestUnity/unity.bff";
    options.m_ShowSummary = true; // required to generate stats for node count checks

    FBuild fBuild( options );
    TEST_ASSERT( fBuild.Initialize( useDB ? GetTestCompileDBFileName() : nullptr ) );

    // Implement Unity and activate this test
    TEST_ASSERT( fBuild.Build( AStackString<>( "Unity-Compiled" ) ) );
    TEST_ASSERT( fBuild.SaveDependencyGraph( GetTestCompileDBFileName() ) );

    return fBuild.GetStats();
}

// TestCompile
//------------------------------------------------------------------------------
void TestUnity::TestCompile() const
{
    FBuildOptions options;
    options.m_ForceCleanBuild = true;
    options.m_ShowSummary = true; // required to generate stats for node count checks

    EnsureFileDoesNotExist( "../../../../tmp/Test/Unity/Unity.lib" );

    FBuildStats stats = BuildCompile( options, false ); // don't use DB

    EnsureFileExists( "../../../../tmp/Test/Unity/Unity.lib" );

    // Check stats
    //                      Seen,   Built,  Type
    uint32_t numF = 10; // pch + 2x generated unity files + 6 source cpp files + librarian
    #if defined( __WINDOWS__ )
        numF++; // pch.cpp
    #endif
    CheckStatsNode ( stats, 1,      1,      Node::DIRECTORY_LIST_NODE );
    CheckStatsNode ( stats, 1,      1,      Node::UNITY_NODE );
    CheckStatsNode ( stats, numF,   4,      Node::FILE_NODE ); // pch + 2x generated unity files built
    CheckStatsNode ( stats, 1,      1,      Node::COMPILER_NODE );
    CheckStatsNode ( stats, 3,      3,      Node::OBJECT_NODE );
    CheckStatsNode ( stats, 1,      1,      Node::LIBRARY_NODE );
    CheckStatsNode ( stats, 1,      1,      Node::ALIAS_NODE );
    CheckStatsTotal( stats, 8+numF, 12 );
}

// TestCompile_NoRebuild
//------------------------------------------------------------------------------
void TestUnity::TestCompile_NoRebuild() const
{
    FBuildStats stats = BuildCompile();

    // Check stats
    //                      Seen,   Built,  Type
    uint32_t numF = 10; // pch + 2x generated unity files + 6 source cpp files + librarian
    #if defined( __WINDOWS__ )
        numF++; // pch.cpp
    #endif
    CheckStatsNode ( stats, 1,      1,      Node::DIRECTORY_LIST_NODE );
    CheckStatsNode ( stats, 1,      1,      Node::UNITY_NODE );
    CheckStatsNode ( stats, numF,   numF,   Node::FILE_NODE );
    CheckStatsNode ( stats, 1,      0,      Node::COMPILER_NODE );
    CheckStatsNode ( stats, 3,      0,      Node::OBJECT_NODE );
    CheckStatsNode ( stats, 1,      0,      Node::LIBRARY_NODE );
    CheckStatsNode ( stats, 1,      1,      Node::ALIAS_NODE );
    CheckStatsTotal( stats, 8+numF, 3+numF );
}

// TestGenerateFromExplicitList
//------------------------------------------------------------------------------
void TestUnity::TestGenerateFromExplicitList() const
{
    FBuildOptions options;
    options.m_ConfigFile = "Data/TestUnity/unity.bff";
    options.m_ShowSummary = true; // required to generate stats for node count checks

    FBuild fBuild( options );
    TEST_ASSERT( fBuild.Initialize() );

    TEST_ASSERT( fBuild.Build( AStackString<>( "Unity-Explicit-Files" ) ) );

    // Check stats
    //               Seen,  Built,  Type
    CheckStatsNode ( 1,     1,      Node::UNITY_NODE );
    CheckStatsTotal( 4,     4 );
}

// TestExcludedFiles
//------------------------------------------------------------------------------
void TestUnity::TestExcludedFiles() const
{
    FBuildOptions options;
    options.m_ConfigFile = "Data/TestUnity/Exclusions/fbuild.bff";

    {
        FBuild fBuild( options );
        TEST_ASSERT( fBuild.Initialize() );

        TEST_ASSERT( fBuild.Build( AStackString<>( "ExcludeFileName" ) ) );
    }

    {
        FBuild fBuild( options );
        TEST_ASSERT( fBuild.Initialize() );

        TEST_ASSERT( fBuild.Build( AStackString<>( "ExcludeFilePath" ) ) );
    }

    {
        FBuild fBuild( options );
        TEST_ASSERT( fBuild.Initialize() );

        TEST_ASSERT( fBuild.Build( AStackString<>( "ExcludeFilePathRelative" ) ) );
    }

    {
        FBuild fBuild( options );
        TEST_ASSERT( fBuild.Initialize() );

        TEST_ASSERT( fBuild.Build( AStackString<>( "ExcludeFilePattern" ) ) );
    }
}

// IsolateFromUnity_Regression
//------------------------------------------------------------------------------
void TestUnity::IsolateFromUnity_Regression() const
{
    // There was a crash when a Unity was:
    // - Using an explicit list of files
    // - UnityInputIsolateWritableFiles was enabled
    // - the files were writable

    FBuildOptions options;
    options.m_ConfigFile = "Data/TestUnity/IsolateFromUnity/fbuild.bff";
    FBuild fBuild( options );
    TEST_ASSERT( fBuild.Initialize() );
    TEST_ASSERT( fBuild.Build( AStackString<>( "Compile" ) ) );
}

//------------------------------------------------------------------------------
