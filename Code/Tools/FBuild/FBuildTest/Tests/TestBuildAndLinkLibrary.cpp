// TestBuildAndLinkLibrary.cpp
//------------------------------------------------------------------------------

// Includes
//------------------------------------------------------------------------------
#include "FBuildTest.h"

#include "Tools/FBuild/FBuildCore/FBuild.h"

#include "Core/FileIO/FileIO.h"
#include "Core/Strings/AStackString.h"

// TestBuildAndLinkLibrary
//------------------------------------------------------------------------------
class TestBuildAndLinkLibrary : public FBuildTest
{
private:
    DECLARE_TESTS

    void TestParseBFF() const;
    void TestBuildLib() const;
    void TestBuildLib_NoRebuild() const;
    void TestBuildLib_NoRebuild_BFFChange() const;
    void TestLibMerge() const;
    void TestLibMerge_NoRebuild() const;
    void TestLibMerge_NoRebuild_BFFChange() const;
    void DeleteFile() const;

    const char * GetBuildLibDBFileName() const { return "../tmp/Test/BuildAndLinkLibrary/buildlib.fdb"; }
    const char * GetMergeLibDBFileName() const { return "../tmp/Test/BuildAndLinkLibrary/mergelib.fdb"; }
};

// Register Tests
//------------------------------------------------------------------------------
REGISTER_TESTS_BEGIN( TestBuildAndLinkLibrary )
    REGISTER_TEST( TestParseBFF )
    REGISTER_TEST( TestBuildLib )
    REGISTER_TEST( TestBuildLib_NoRebuild )
    REGISTER_TEST( TestBuildLib_NoRebuild_BFFChange )
    REGISTER_TEST( TestLibMerge )
    REGISTER_TEST( TestLibMerge_NoRebuild )
    REGISTER_TEST( TestLibMerge_NoRebuild_BFFChange )
    REGISTER_TEST( DeleteFile )
REGISTER_TESTS_END

// TestStackFramesEmpty
//------------------------------------------------------------------------------
void TestBuildAndLinkLibrary::TestParseBFF() const
{
    FBuildTestOptions options;
    options.m_ConfigFile = "Tools/FBuild/FBuildTest/Data/TestBuildAndLinkLibrary/fbuild.bff";

    FBuild fBuild( options );
    TEST_ASSERT( fBuild.Initialize() );
}

// TestBuildLib
//------------------------------------------------------------------------------
void TestBuildAndLinkLibrary::TestBuildLib() const
{
    FBuildTestOptions options;
    options.m_ConfigFile = "Tools/FBuild/FBuildTest/Data/TestBuildAndLinkLibrary/fbuild.bff";

    FBuild fBuild( options );
    TEST_ASSERT( fBuild.Initialize() );

    const AStackString<> lib( "../tmp/Test/BuildAndLinkLibrary/test.lib" );
    #if defined( __WINDOWS__ )
        const AStackString<> obj1( "../tmp/Test/BuildAndLinkLibrary/a.obj" );
        const AStackString<> obj2( "../tmp/Test/BuildAndLinkLibrary/b.obj" );
        const AStackString<> obj3( "../tmp/Test/BuildAndLinkLibrary/c.obj" );
    #else
        const AStackString<> obj1( "../tmp/Test/BuildAndLinkLibrary/a.o" );
        const AStackString<> obj2( "../tmp/Test/BuildAndLinkLibrary/b.o" );
        const AStackString<> obj3( "../tmp/Test/BuildAndLinkLibrary/c.o" );
    #endif

    // clean up anything left over from previous runs
    EnsureFileDoesNotExist( lib );
    EnsureFileDoesNotExist( obj1 );
    EnsureFileDoesNotExist( obj2 );
    EnsureFileDoesNotExist( obj3 );

    // Build
    TEST_ASSERT( fBuild.Build( lib ) );
    TEST_ASSERT( fBuild.SaveDependencyGraph( GetBuildLibDBFileName() ) );

    // make sure all output files are as expected
    EnsureFileExists( lib );
    EnsureFileExists( obj1 );
    EnsureFileExists( obj2 );
    EnsureFileExists( obj3 );

    // Check stats
    //               Seen,  Built,  Type
    CheckStatsNode ( 1,     1,      Node::DIRECTORY_LIST_NODE );
    CheckStatsNode ( 7,     4,      Node::FILE_NODE ); // 3 cpps + librarian
    CheckStatsNode ( 1,     1,      Node::COMPILER_NODE );
    CheckStatsNode ( 3,     3,      Node::OBJECT_NODE );
    CheckStatsNode ( 1,     1,      Node::LIBRARY_NODE );
    CheckStatsTotal( 13,    10 );
}

// TestBuildLib_NoRebuild
//------------------------------------------------------------------------------
void TestBuildAndLinkLibrary::TestBuildLib_NoRebuild() const
{
    FBuildTestOptions options;
    options.m_ConfigFile = "Tools/FBuild/FBuildTest/Data/TestBuildAndLinkLibrary/fbuild.bff";

    FBuild fBuild( options );
    TEST_ASSERT( fBuild.Initialize( GetBuildLibDBFileName() ) );

    const AStackString<> lib( "../tmp/Test/BuildAndLinkLibrary/test.lib" );

    // Build
    TEST_ASSERT( fBuild.Build( lib ) );

    // Check stats
    //               Seen,  Built,  Type
    CheckStatsNode ( 1,     1,      Node::DIRECTORY_LIST_NODE );
    CheckStatsNode ( 7,     7,      Node::FILE_NODE ); // 3 cpps + 3 headers + librarian
    CheckStatsNode ( 1,     0,      Node::COMPILER_NODE );
    CheckStatsNode ( 3,     0,      Node::OBJECT_NODE );
    CheckStatsNode ( 1,     0,      Node::LIBRARY_NODE );
    CheckStatsTotal( 13,    8 );
}

// TestBuildLib_NoRebuild_BFFChange
//------------------------------------------------------------------------------
void TestBuildAndLinkLibrary::TestBuildLib_NoRebuild_BFFChange() const
{
    FBuildOptions options;
    options.m_ConfigFile = "Tools/FBuild/FBuildTest/Data/TestBuildAndLinkLibrary/fbuild.bff";
    options.m_ShowSummary = true; // required to generate stats for node count checks
    options.m_ForceDBMigration_Debug = true;

    FBuild fBuild( options );
    TEST_ASSERT( fBuild.Initialize( GetBuildLibDBFileName() ) );

    const AStackString<> lib( "../tmp/Test/BuildAndLinkLibrary/test.lib" );

    // Build
    TEST_ASSERT( fBuild.Build( lib ) );

    // Check stats
    //               Seen,  Built,  Type
    CheckStatsNode ( 1,     1,      Node::DIRECTORY_LIST_NODE );
    CheckStatsNode ( 7,     7,      Node::FILE_NODE ); // 3 cpps + 3 headers + librarian
    CheckStatsNode ( 1,     0,      Node::COMPILER_NODE );
    CheckStatsNode ( 3,     0,      Node::OBJECT_NODE );
    CheckStatsNode ( 1,     0,      Node::LIBRARY_NODE );
    CheckStatsTotal( 13,    8 );
}

// TestLibMerge
//------------------------------------------------------------------------------
void TestBuildAndLinkLibrary::TestLibMerge() const
{
    FBuildTestOptions options;
    options.m_ForceCleanBuild = true;
    options.m_ConfigFile = "Tools/FBuild/FBuildTest/Data/TestBuildAndLinkLibrary/fbuild.bff";

    FBuild fBuild( options );
    TEST_ASSERT( fBuild.Initialize() );

    const AStackString<> lib( "../tmp/Test/BuildAndLinkLibrary/merged.lib" );

    // clean up anything left over from previous runs
    EnsureFileDoesNotExist( lib );

    // Build
    TEST_ASSERT( fBuild.Build( lib ) );
    TEST_ASSERT( fBuild.SaveDependencyGraph( GetMergeLibDBFileName() ) );

    // make sure all output files are as expected
    EnsureFileExists( lib );

    // Check stats
    //               Seen,  Built,  Type
    CheckStatsNode ( 7,     4,      Node::FILE_NODE ); // 3x .cpp + 3x .h + librarian
    CheckStatsNode ( 1,     1,      Node::COMPILER_NODE );
    CheckStatsNode ( 1,     1,      Node::OBJECT_LIST_NODE );
    CheckStatsNode ( 3,     3,      Node::OBJECT_NODE );
    CheckStatsNode ( 3,     3,      Node::LIBRARY_NODE ); // 2 libs + merge lib
    CheckStatsTotal( 15,    12 );
}

// TestLibMerge_NoRebuild
//------------------------------------------------------------------------------
void TestBuildAndLinkLibrary::TestLibMerge_NoRebuild() const
{
    FBuildTestOptions options;
    options.m_ConfigFile = "Tools/FBuild/FBuildTest/Data/TestBuildAndLinkLibrary/fbuild.bff";

    FBuild fBuild( options );
    TEST_ASSERT( fBuild.Initialize( GetMergeLibDBFileName() ) );

    const AStackString<> lib( "../tmp/Test/BuildAndLinkLibrary/merged.lib" );

    // Build
    TEST_ASSERT( fBuild.Build( lib ) );

    // Check stats
    //               Seen,  Built,  Type
    CheckStatsNode ( 7,     7,      Node::FILE_NODE ); // 3 cpps + 3 headers + librarian
    CheckStatsNode ( 1,     0,      Node::COMPILER_NODE );
    CheckStatsNode ( 1,     0,      Node::OBJECT_LIST_NODE );
    CheckStatsNode ( 3,     0,      Node::OBJECT_NODE );
    CheckStatsNode ( 3,     0,      Node::LIBRARY_NODE ); // 2 libs + merge lib
    CheckStatsTotal( 15,    7 );
}

// TestLibMerge_NoRebuild_BFFChange
//------------------------------------------------------------------------------
void TestBuildAndLinkLibrary::TestLibMerge_NoRebuild_BFFChange() const
{
    FBuildOptions options;
    options.m_ConfigFile = "Tools/FBuild/FBuildTest/Data/TestBuildAndLinkLibrary/fbuild.bff";
    options.m_ShowSummary = true; // required to generate stats for node count checks
    options.m_ForceDBMigration_Debug = true;

    FBuild fBuild( options );
    TEST_ASSERT( fBuild.Initialize( GetMergeLibDBFileName() ) );

    const AStackString<> lib( "../tmp/Test/BuildAndLinkLibrary/merged.lib" );

    // Build
    TEST_ASSERT( fBuild.Build( lib ) );

    // Check stats
    //               Seen,  Built,  Type
    CheckStatsNode ( 7,     7,      Node::FILE_NODE ); // 3 cpps + 3 headers + librarian
    CheckStatsNode ( 1,     0,      Node::COMPILER_NODE );
    CheckStatsNode ( 1,     0,      Node::OBJECT_LIST_NODE );
    CheckStatsNode ( 3,     0,      Node::OBJECT_NODE );
    CheckStatsNode ( 3,     0,      Node::LIBRARY_NODE ); // 2 libs + merge lib
    CheckStatsTotal( 15,    7 );
}

// DeleteFile
//------------------------------------------------------------------------------
//  - Ensure that libraries are rebuilt when files are deleted
void TestBuildAndLinkLibrary::DeleteFile() const
{
    const char* fileA = "../tmp/Test/BuildAndLinkLibrary/DeleteFile/GeneratedInput/FileA.cpp";
    const char* fileB = "../tmp/Test/BuildAndLinkLibrary/DeleteFile/GeneratedInput/FileB.cpp";
    const char* database = "../tmp/Test/BuildAndLinkLibrary/DeleteFile/fbuild.fdb";

    // Create two empty files
    {
        // Generate some header files
        EnsureDirExists( "../tmp/Test/BuildAndLinkLibrary/DeleteFile/GeneratedInput/" );
        FileStream f;
        TEST_ASSERT( f.Open( fileA, FileStream::WRITE_ONLY ) );
        f.Close();
        TEST_ASSERT( f.Open( fileB, FileStream::WRITE_ONLY ) );
        f.Close();
    }

    // Compile library for the two files
    {
        // Init
        FBuildTestOptions options;
        options.m_ConfigFile = "Tools/FBuild/FBuildTest/Data/TestBuildAndLinkLibrary/DeleteFile/fbuild.bff";
        options.m_ForceCleanBuild = true;
        FBuild fBuild( options );
        TEST_ASSERT( fBuild.Initialize() );

        // Compile
        TEST_ASSERT( fBuild.Build( "TestLib" ) );

        // Save DB
        TEST_ASSERT( fBuild.SaveDependencyGraph( database ) );

        // Check stats
        //              Seen,   Built,  Type
        CheckStatsNode( 1,      1,      Node::DIRECTORY_LIST_NODE );
        CheckStatsNode( 1,      1,      Node::COMPILER_NODE );
        CheckStatsNode( 2,      2,      Node::OBJECT_NODE );
        CheckStatsNode( 1,      1,      Node::LIBRARY_NODE );
    }

    // Delete one of the input files
    EnsureFileDoesNotExist( fileB );

    // Compile library again
    {
        // Init
        FBuildTestOptions options;
        options.m_ConfigFile = "Tools/FBuild/FBuildTest/Data/BuildAndLinkLibrary/DeleteFile/fbuild.bff";
        FBuild fBuild( options );
        TEST_ASSERT( fBuild.Initialize( database ) );

        // Compile
        TEST_ASSERT( fBuild.Build( "TestLib" ) );

        // Check stats
        //              Seen,   Built,  Type
        CheckStatsNode( 1,      1,      Node::DIRECTORY_LIST_NODE );
        CheckStatsNode( 1,      0,      Node::COMPILER_NODE );
        CheckStatsNode( 1,      0,      Node::OBJECT_NODE );    // NOTE: Only one object seen
        CheckStatsNode( 1,      1,      Node::LIBRARY_NODE );   // NOTE: Library rebuilt
    }
}

//------------------------------------------------------------------------------
