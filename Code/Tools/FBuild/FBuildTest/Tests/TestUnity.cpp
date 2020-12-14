// TestUnity.cpp
//------------------------------------------------------------------------------

// Includes
//------------------------------------------------------------------------------
#include "FBuildTest.h"

// FBuildCore
#include "Tools/FBuild/FBuildCore/BFF/BFFParser.h"
#include "Tools/FBuild/FBuildCore/FBuild.h"
#include "Tools/FBuild/FBuildCore/Graph/UnityNode.h"

// Core
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
    FBuildStats BuildGenerate( FBuildTestOptions options = FBuildTestOptions(), bool useDB = true, bool forceMigration = false ) const;
    const char * GetTestGenerateDBFileName() const { return "../../../../tmp/Test/Unity/generate.fdb"; }
    FBuildStats BuildCompile( FBuildTestOptions options = FBuildTestOptions(), bool useDB = true, bool forceMigration = false ) const;
    const char * GetTestCompileDBFileName() const { return "../../../../tmp/Test/Unity/compile.fdb"; }

    // Tests
    void TestGenerate() const;
    void TestGenerate_NoRebuild() const;
    void TestGenerate_NoRebuild_BFFChange() const;
    void DetectDeletedUnityFiles() const;
    void TestCompile() const;
    void TestCompile_NoRebuild() const;
    void TestCompile_NoRebuild_BFFChange() const;
    void TestGenerateFromExplicitList() const;
    void TestExcludedFiles() const;
    void IsolateFromUnity_Regression() const;
    void UnityInputIsolatedFiles() const;
    void IsolateListFile() const;
    void ClangStaticAnalysis() const;
    void ClangStaticAnalysis_InjectHeader() const;
    void LinkMultiple() const;
    void LinkMultiple_InputFiles() const;
    void SortFiles() const;
    void CacheUsingRelativePaths() const;
};

// Register Tests
//------------------------------------------------------------------------------
REGISTER_TESTS_BEGIN( TestUnity )
    REGISTER_TEST( TestGenerate )           // clean build of unity files
    REGISTER_TEST( TestGenerate_NoRebuild ) // check nothing rebuilds
    REGISTER_TEST( TestGenerate_NoRebuild_BFFChange ) // check nothing rebuilds after a BFF change
    REGISTER_TEST( DetectDeletedUnityFiles )
    REGISTER_TEST( TestCompile )            // compile a library using unity inputs
    REGISTER_TEST( TestCompile_NoRebuild )  // check nothing rebuilds
    REGISTER_TEST( TestCompile_NoRebuild_BFFChange )  // check nothing rebuilds after a BFF change
    REGISTER_TEST( TestGenerateFromExplicitList ) // create a unity with manually provided files
    REGISTER_TEST( TestExcludedFiles )      // Ensure files are correctly excluded
    REGISTER_TEST( IsolateFromUnity_Regression )
    REGISTER_TEST( UnityInputIsolatedFiles )
    REGISTER_TEST( IsolateListFile )
    REGISTER_TEST( ClangStaticAnalysis )
    REGISTER_TEST( ClangStaticAnalysis_InjectHeader )
    REGISTER_TEST( LinkMultiple )
    REGISTER_TEST( LinkMultiple_InputFiles )
    REGISTER_TEST( SortFiles )
    REGISTER_TEST( CacheUsingRelativePaths )
REGISTER_TESTS_END

// BuildGenerate
//------------------------------------------------------------------------------
FBuildStats TestUnity::BuildGenerate( FBuildTestOptions options, bool useDB, bool forceMigration ) const
{
    options.m_ConfigFile = "Tools/FBuild/FBuildTest/Data/TestUnity/unity.bff";
    options.m_ShowSummary = true; // required to generate stats for node count checks
    options.m_ForceDBMigration_Debug = forceMigration;

    FBuild fBuild( options );
    TEST_ASSERT( fBuild.Initialize( useDB ? GetTestGenerateDBFileName() : nullptr ) );

    // Implement Unity and activate this test
    TEST_ASSERT( fBuild.Build( "Unity-Test" ) );
    TEST_ASSERT( fBuild.SaveDependencyGraph( GetTestGenerateDBFileName() ) );

    return fBuild.GetStats();
}

// TestGenerate
//------------------------------------------------------------------------------
void TestUnity::TestGenerate() const
{
    FBuildTestOptions options;
    options.m_ShowSummary = true; // required to generate stats for node count checks
    options.m_ForceCleanBuild = true;

    EnsureFileDoesNotExist( "../tmp/Test/Unity/Unity1.cpp" );
    EnsureFileDoesNotExist( "../tmp/Test/Unity/Unity2.cpp" );

    FBuildStats stats = BuildGenerate( options, false ); // don't use DB

    EnsureFileExists( "../tmp/Test/Unity/Unity1.cpp" );
    EnsureFileExists( "../tmp/Test/Unity/Unity2.cpp" );

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
    AStackString<> unity1( "../tmp/Test/Unity/Unity1.cpp" );
    AStackString<> unity2( "../tmp/Test/Unity/Unity2.cpp" );

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
    CheckStatsNode ( stats, 1,      0,      Node::UNITY_NODE );
    CheckStatsTotal( stats, 2,      1 );
}

// TestGenerate_NoRebuild_BFFChange
//------------------------------------------------------------------------------
void TestUnity::TestGenerate_NoRebuild_BFFChange() const
{
    AStackString<> unity1( "../tmp/Test/Unity/Unity1.cpp" );
    AStackString<> unity2( "../tmp/Test/Unity/Unity2.cpp" );

    EnsureFileExists( unity1 );
    EnsureFileExists( unity2 );

    // Unity must be "built" every time, but it only writes files when they change
    // so record the time before and after
    uint64_t dateTime1 = FileIO::GetFileLastWriteTime( unity1 );
    uint64_t dateTime2 = FileIO::GetFileLastWriteTime( unity2 );

    // NTFS file resolution is 100ns, so sleep long enough to ensure
    // an invalid write would modify the time
    Thread::Sleep( 1 ); // 1ms

    FBuildTestOptions options;
    const bool useDB = true;
    const bool forceMigration = true;
    FBuildStats stats = BuildGenerate( options, useDB, forceMigration );

    // Make sure files have not been changed
    TEST_ASSERT( dateTime1 == FileIO::GetFileLastWriteTime( unity1 ) );
    TEST_ASSERT( dateTime2 == FileIO::GetFileLastWriteTime( unity2 ) );

    // Check stats
    //                      Seen,   Built,  Type
    CheckStatsNode ( stats, 1,      1,      Node::DIRECTORY_LIST_NODE );
    CheckStatsNode ( stats, 1,      0,      Node::UNITY_NODE );
    CheckStatsTotal( stats, 2,      1 );
}

// DetectDeletedUnityFiles
//------------------------------------------------------------------------------
void TestUnity::DetectDeletedUnityFiles() const
{
    // Ensure that a generated Unity file that has been deleted is
    // detected and regenerated
    
    EnsureFileDoesNotExist( "../tmp/Test/Unity/Unity1.cpp" );
    EnsureFileDoesNotExist( "../tmp/Test/Unity/Unity2.cpp" );

    // Build
    FBuildTestOptions options;
    options.m_ShowBuildReason = true;
    BuildGenerate( options, false ); // don't laod DB

    EnsureFileExists( "../tmp/Test/Unity/Unity1.cpp" );
    EnsureFileExists( "../tmp/Test/Unity/Unity2.cpp" );

    // Delete one of the generated files
    EnsureFileDoesNotExist( "../tmp/Test/Unity/Unity2.cpp" );

    // Build again
    FBuildStats stats = BuildGenerate( options, true ); // load DB

    // File should have been recreated
    EnsureFileExists( "../tmp/Test/Unity/Unity2.cpp" );

    // Ensure build has the expected cause
    TEST_ASSERT( GetRecordedOutput().Find( "Need to build" ) &&
                 GetRecordedOutput().Find( "(Output" ) &&
                 GetRecordedOutput().Find( "missing)" ) );

    // Check stats
    //                      Seen,   Built,  Type
    CheckStatsNode ( stats, 1,      1,      Node::DIRECTORY_LIST_NODE );
    CheckStatsNode ( stats, 1,      1,      Node::UNITY_NODE );
    CheckStatsTotal( stats, 2,      2 );
}

// BuildCompile
//------------------------------------------------------------------------------
FBuildStats TestUnity::BuildCompile( FBuildTestOptions options, bool useDB, bool forceMigration ) const
{
    options.m_ConfigFile = "Tools/FBuild/FBuildTest/Data/TestUnity/unity.bff";
    options.m_ShowSummary = true; // required to generate stats for node count checks
    options.m_ForceDBMigration_Debug = forceMigration;

    FBuild fBuild( options );
    TEST_ASSERT( fBuild.Initialize( useDB ? GetTestCompileDBFileName() : nullptr ) );

    // Implement Unity and activate this test
    TEST_ASSERT( fBuild.Build( "Unity-Compiled" ) );
    TEST_ASSERT( fBuild.SaveDependencyGraph( GetTestCompileDBFileName() ) );

    return fBuild.GetStats();
}

// TestCompile
//------------------------------------------------------------------------------
void TestUnity::TestCompile() const
{
    FBuildTestOptions options;
    options.m_ForceCleanBuild = true;
    options.m_ShowSummary = true; // required to generate stats for node count checks

    EnsureFileDoesNotExist( "../tmp/Test/Unity/Unity.lib" );

    FBuildStats stats = BuildCompile( options, false ); // don't use DB

    EnsureFileExists( "../tmp/Test/Unity/Unity.lib" );

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
    CheckStatsNode ( stats, 1,      0,      Node::UNITY_NODE );
    CheckStatsNode ( stats, numF,   numF,   Node::FILE_NODE );
    CheckStatsNode ( stats, 1,      0,      Node::COMPILER_NODE );
    CheckStatsNode ( stats, 3,      0,      Node::OBJECT_NODE );
    CheckStatsNode ( stats, 1,      0,      Node::LIBRARY_NODE );
    CheckStatsNode ( stats, 1,      1,      Node::ALIAS_NODE );
    CheckStatsTotal( stats, 8+numF, 2+numF );
}

// TestCompile_NoRebuild_BFFChange
//------------------------------------------------------------------------------
void TestUnity::TestCompile_NoRebuild_BFFChange() const
{
    FBuildTestOptions options;
    const bool useDB = true;
    const bool forceMigration = true;
    FBuildStats stats = BuildCompile( options, useDB, forceMigration );

    // Check stats
    //                      Seen,   Built,  Type
    uint32_t numF = 10; // pch + 2x generated unity files + 6 source cpp files
    #if defined( __WINDOWS__ )
        numF++; // pch.cpp
    #endif
    CheckStatsNode ( stats, 1,      1,      Node::DIRECTORY_LIST_NODE );
    CheckStatsNode ( stats, 1,      0,      Node::UNITY_NODE );
    CheckStatsNode ( stats, numF,   numF,   Node::FILE_NODE );
    CheckStatsNode ( stats, 1,      0,      Node::COMPILER_NODE );
    CheckStatsNode ( stats, 3,      0,      Node::OBJECT_NODE );
    CheckStatsNode ( stats, 1,      0,      Node::LIBRARY_NODE );
    CheckStatsNode ( stats, 1,      1,      Node::ALIAS_NODE );
    CheckStatsTotal( stats, 8+numF, 2+numF );
}

// TestGenerateFromExplicitList
//------------------------------------------------------------------------------
void TestUnity::TestGenerateFromExplicitList() const
{
    FBuildTestOptions options;
    options.m_ConfigFile = "Tools/FBuild/FBuildTest/Data/TestUnity/unity.bff";

    FBuild fBuild( options );
    TEST_ASSERT( fBuild.Initialize() );

    TEST_ASSERT( fBuild.Build( "Unity-Explicit-Files" ) );

    // Check stats
    //               Seen,  Built,  Type
    CheckStatsNode ( 1,     1,      Node::UNITY_NODE );
    CheckStatsTotal( 4,     4 );
}

// TestExcludedFiles
//------------------------------------------------------------------------------
void TestUnity::TestExcludedFiles() const
{
    FBuildTestOptions options;
    options.m_ConfigFile = "Tools/FBuild/FBuildTest/Data/TestUnity/Exclusions/fbuild.bff";

    {
        FBuild fBuild( options );
        TEST_ASSERT( fBuild.Initialize() );

        TEST_ASSERT( fBuild.Build( "ExcludeFileName" ) );
    }

    {
        FBuild fBuild( options );
        TEST_ASSERT( fBuild.Initialize() );

        TEST_ASSERT( fBuild.Build( "ExcludeFilePath" ) );
    }

    {
        FBuild fBuild( options );
        TEST_ASSERT( fBuild.Initialize() );

        TEST_ASSERT( fBuild.Build( "ExcludeFilePathRelative" ) );
    }

    {
        FBuild fBuild( options );
        TEST_ASSERT( fBuild.Initialize() );

        TEST_ASSERT( fBuild.Build( "ExcludeFilePattern" ) );
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

    FBuildTestOptions options;
    options.m_ConfigFile = "Tools/FBuild/FBuildTest/Data/TestUnity/IsolateFromUnity/fbuild.bff";
    FBuild fBuild( options );
    TEST_ASSERT( fBuild.Initialize() );
    TEST_ASSERT( fBuild.Build( "Compile" ) );
}

// UnityInputIsolatedFiles
//------------------------------------------------------------------------------
void TestUnity::UnityInputIsolatedFiles() const
{
    FBuildTestOptions options;
    options.m_ConfigFile = "Tools/FBuild/FBuildTest/Data/TestUnity/UnityInputIsolatedFiles/fbuild.bff";
    FBuild fBuild( options );
    TEST_ASSERT( fBuild.Initialize() );
    TEST_ASSERT( fBuild.Build( "Compile" ) );

    // Check stats
    //               Seen,  Built,  Type
    CheckStatsNode ( 1,     1,      Node::UNITY_NODE );
    CheckStatsNode ( 2,     2,      Node::OBJECT_NODE );
    CheckStatsNode ( 1,     1,      Node::OBJECT_LIST_NODE );
}

// IsolateListFile
//------------------------------------------------------------------------------
void TestUnity::IsolateListFile() const
{
    FBuildTestOptions options;
    options.m_ConfigFile = "Tools/FBuild/FBuildTest/Data/TestUnity/IsolateListFile/fbuild.bff";
    FBuild fBuild( options );
    TEST_ASSERT( fBuild.Initialize() );
    TEST_ASSERT( fBuild.Build( "Compile" ) );

    // Check stats
    //               Seen,  Built,  Type
    CheckStatsNode ( 1,     1,      Node::UNITY_NODE );
    CheckStatsNode ( 2,     2,      Node::OBJECT_NODE );
    CheckStatsNode ( 1,     1,      Node::OBJECT_LIST_NODE );
}

// ClangStaticAnalysis
//------------------------------------------------------------------------------
void TestUnity::ClangStaticAnalysis() const
{
    //
    // Ensure that use of Unity doesn't suppress static analysis warnings with Clang
    //
    FBuildTestOptions options;
    options.m_ConfigFile = "Tools/FBuild/FBuildTest/Data/TestUnity/ClangStaticAnalysis/fbuild.bff";
    //options.m_NoUnity = true;
    FBuild fBuild( options );
    TEST_ASSERT( fBuild.Initialize() );
    TEST_ASSERT( fBuild.Build( "Compile" ) ); // Success, regardless of warnings

    // Check that the various problems we expect to find are found
    TEST_ASSERT( GetRecordedOutput().Find( "Division by zero" ) );
    TEST_ASSERT( GetRecordedOutput().Find( "Address of stack memory" ) );
}

// ClangStaticAnalysis_InjectHeader
//------------------------------------------------------------------------------
void TestUnity::ClangStaticAnalysis_InjectHeader() const
{
    //
    // Ensure headers injected with -include don't prevent fixup from working
    //
    FBuildTestOptions options;
    options.m_ConfigFile = "Tools/FBuild/FBuildTest/Data/TestUnity/ClangStaticAnalysis/fbuild.bff";
    //options.m_NoUnity = true;
    FBuild fBuild( options );
    TEST_ASSERT( fBuild.Initialize() );
    TEST_ASSERT( fBuild.Build( "Compile-InjectHeader" ) ); // Success, regardless of warnings

    // Check that the various problems we expect to find are found
    TEST_ASSERT( GetRecordedOutput().Find( "Division by zero" ) );
    TEST_ASSERT( GetRecordedOutput().Find( "Address of stack memory" ) );
}

// LinkMultiple
//------------------------------------------------------------------------------
void TestUnity::LinkMultiple() const
{
    // Test multiple Unity items with an additional loose file.
    // Ensure that builds occur as expected when items are isolated, and
    // that linker is passed correct objects can links successfully.

    // Code files generated/used by this test
    const char* fileA = "../tmp/Test/Unity/LinkMultiple/Generated/A/a.cpp";
    const char* fileB = "../tmp/Test/Unity/LinkMultiple/Generated/B/b.cpp";
    const char* main = "../tmp/Test/Unity/LinkMultiple/Generated/main.cpp";
    const char* fileAContents = "void FunctionA() {}\n";
    const char* fileBContents = "void FunctionB() {}\n";
    const char* mainContents = "extern void FunctionA();\n"
                               "extern void FunctionB();\n"
                               "int main(int, char *[]) { FunctionA(); FunctionB(); return 0; }\n";

    // Cleanup from previous runs (if files exist)
    FileIO::SetReadOnly( fileA, false );
    FileIO::SetReadOnly( fileB, false );
    EnsureFileDoesNotExist( fileA );
    EnsureFileDoesNotExist( fileB );

    // Create files
    EnsureDirExists( "../tmp/Test/Unity/LinkMultiple/Generated/A/" );
    EnsureDirExists( "../tmp/Test/Unity/LinkMultiple/Generated/B/" );
    MakeFile( fileA, fileAContents );
    MakeFile( fileB, fileBContents );
    MakeFile( main, mainContents );

    // Make files in unity read-only so they are not isolated
    FileIO::SetReadOnly( fileA, true );
    FileIO::SetReadOnly( fileB, true );

    // Common options
    FBuildTestOptions options;
    options.m_ConfigFile = "Tools/FBuild/FBuildTest/Data/TestUnity/LinkMultiple/fbuild.bff";
    const char * dbFile = "../tmp/Test/Unity/LinkMultiple/fbuild.fdb";

    // Compile
    {
        FBuild fBuild( options );
        TEST_ASSERT( fBuild.Initialize() );
        TEST_ASSERT( fBuild.Build( "Exe" ) );
        TEST_ASSERT( fBuild.SaveDependencyGraph( dbFile ) );

        // Check stats
        //               Seen,  Built,  Type
        CheckStatsNode ( 2,     2,      Node::UNITY_NODE );
        CheckStatsNode ( 3,     3,      Node::OBJECT_NODE );
        CheckStatsNode ( 1,     1,      Node::OBJECT_LIST_NODE );
        CheckStatsNode ( 1,     1,      Node::EXE_NODE );
    }

    #if defined( __OSX__ )
        Thread::Sleep( 1000 ); // Work around low time resolution of HFS+
    #endif

    // Isolate one of the files
    {
        // Make file writeable so it is isolated
        FileIO::SetReadOnly( fileA, false );

        FBuild fBuild( options );
        TEST_ASSERT( fBuild.Initialize( dbFile ) );
        TEST_ASSERT( fBuild.Build( "Exe" ) );
        TEST_ASSERT( fBuild.SaveDependencyGraph( dbFile ) );

        // Check stats
        //               Seen,  Built,  Type
        CheckStatsNode ( 2,     1,      Node::UNITY_NODE );
        CheckStatsNode ( 3,     1,      Node::OBJECT_NODE );
        CheckStatsNode ( 1,     1,      Node::OBJECT_LIST_NODE );
        CheckStatsNode ( 1,     1,      Node::EXE_NODE );
    }

    #if defined( __OSX__ )
        Thread::Sleep( 1000 ); // Work around low time resolution of HFS+
    #endif

    // De-Isolate one of the files
    {
        // Make file read-only so it is put back in Unity
        FileIO::SetReadOnly( fileA, true );

        FBuild fBuild( options );
        TEST_ASSERT( fBuild.Initialize( dbFile ) );
        TEST_ASSERT( fBuild.Build( "Exe" ) );
        TEST_ASSERT( fBuild.SaveDependencyGraph( dbFile ) );

        // Check stats
        //               Seen,  Built,  Type
        CheckStatsNode ( 2,     1,      Node::UNITY_NODE );
        CheckStatsNode ( 3,     1,      Node::OBJECT_NODE );
        CheckStatsNode ( 1,     1,      Node::OBJECT_LIST_NODE );
        CheckStatsNode ( 1,     1,      Node::EXE_NODE );
    }

    #if defined( __OSX__ )
        Thread::Sleep( 1000 ); // Work around low time resolution of HFS+
    #endif

    // Force a DB migration and modify state to ensure internal property
    // migration works correctly
    {
        // Make file writeable so it is isolated
        FileIO::SetReadOnly( fileA, false );

        // Force DB migration to emulate bff edit
        FBuildOptions optionsCopy( options );
        optionsCopy.m_ForceDBMigration_Debug = true;

        FBuild fBuild( optionsCopy );
        TEST_ASSERT( fBuild.Initialize( dbFile ) );
        TEST_ASSERT( fBuild.Build( "Exe" ) );

        // Check stats
        //               Seen,  Built,  Type
        CheckStatsNode ( 2,     1,      Node::UNITY_NODE );
        CheckStatsNode ( 3,     1,      Node::OBJECT_NODE );
        CheckStatsNode ( 1,     1,      Node::OBJECT_LIST_NODE );
        CheckStatsNode ( 1,     1,      Node::EXE_NODE );
    }
}

// LinkMultiple_InputFiles
//------------------------------------------------------------------------------
void TestUnity::LinkMultiple_InputFiles() const
{
    // Same as LinkMultiple but with files explicitily specified instead o
    // discovering them via directory listings

    // Code files generated/used by this test
    const char* fileA = "../tmp/Test/Unity/LinkMultiple_InputFiles/Generated/A/a.cpp";
    const char* fileB = "../tmp/Test/Unity/LinkMultiple_InputFiles/Generated/B/b.cpp";
    const char* main = "../tmp/Test/Unity/LinkMultiple_InputFiles/Generated/main.cpp";
    const char* fileAContents = "void FunctionA() {}\n";
    const char* fileBContents = "void FunctionB() {}\n";
    const char* mainContents = "extern void FunctionA();\n"
                               "extern void FunctionB();\n"
                               "int main(int, char *[]) { FunctionA(); FunctionB(); return 0; }\n";

    // Cleanup from previous runs (if files exist)
    FileIO::SetReadOnly( fileA, false );
    FileIO::SetReadOnly( fileB, false );
    EnsureFileDoesNotExist( fileA );
    EnsureFileDoesNotExist( fileB );

    // Create files
    EnsureDirExists( "../tmp/Test/Unity/LinkMultiple_InputFiles/Generated/A/" );
    EnsureDirExists( "../tmp/Test/Unity/LinkMultiple_InputFiles/Generated/B/" );
    MakeFile( fileA, fileAContents );
    MakeFile( fileB, fileBContents );
    MakeFile( main, mainContents );

    // Make files in unity read-only so they are not isolated
    FileIO::SetReadOnly( fileA, true );
    FileIO::SetReadOnly( fileB, true );

    // Common options
    FBuildTestOptions options;
    options.m_ConfigFile = "Tools/FBuild/FBuildTest/Data/TestUnity/LinkMultiple_InputFiles/fbuild.bff";
    const char * dbFile = "../tmp/Test/Unity/LinkMultiple_InputFiles/fbuild.fdb";

    options.m_NumWorkerThreads = 1; // DO NOT SUBMIT

    // Compile
    {
        FBuild fBuild( options );
        TEST_ASSERT( fBuild.Initialize() );
        TEST_ASSERT( fBuild.Build( "Exe" ) );
        TEST_ASSERT( fBuild.SaveDependencyGraph( dbFile ) );

        // Check stats
        //               Seen,  Built,  Type
        CheckStatsNode ( 2,     2,      Node::UNITY_NODE );
        CheckStatsNode ( 3,     3,      Node::OBJECT_NODE );
        CheckStatsNode ( 1,     1,      Node::OBJECT_LIST_NODE );
        CheckStatsNode ( 1,     1,      Node::EXE_NODE );
    }

    #if defined( __OSX__ )
        Thread::Sleep( 1000 ); // Work around low time resolution of HFS+
    #endif

    // Isolate one of the files
    {
        // Make file writeable so it is isolated
        FileIO::SetReadOnly( fileA, false );

        FBuild fBuild( options );
        TEST_ASSERT( fBuild.Initialize( dbFile ) );
        TEST_ASSERT( fBuild.Build( "Exe" ) );
        TEST_ASSERT( fBuild.SaveDependencyGraph( dbFile ) );

        // Check stats
        //               Seen,  Built,  Type
        CheckStatsNode ( 2,     2,      Node::UNITY_NODE );
        CheckStatsNode ( 3,     1,      Node::OBJECT_NODE );
        CheckStatsNode ( 1,     1,      Node::OBJECT_LIST_NODE );
        CheckStatsNode ( 1,     1,      Node::EXE_NODE );
    }

    #if defined( __OSX__ )
        Thread::Sleep( 1000 ); // Work around low time resolution of HFS+
    #endif

    // De-Isolate one of the files
    {
        // Make file read-only so it is put back in Unity
        FileIO::SetReadOnly( fileA, true );

        FBuild fBuild( options );
        TEST_ASSERT( fBuild.Initialize( dbFile ) );
        TEST_ASSERT( fBuild.Build( "Exe" ) );
        TEST_ASSERT( fBuild.SaveDependencyGraph( dbFile ) );

        // Check stats
        //               Seen,  Built,  Type
        CheckStatsNode ( 2,     2,      Node::UNITY_NODE );
        CheckStatsNode ( 3,     1,      Node::OBJECT_NODE );
        CheckStatsNode ( 1,     1,      Node::OBJECT_LIST_NODE );
        CheckStatsNode ( 1,     1,      Node::EXE_NODE );
    }

    #if defined( __OSX__ )
        Thread::Sleep( 1000 ); // Work around low time resolution of HFS+
    #endif

    // Force a DB migration and modify state to ensure internal property
    // migration works correctly
    {
        // Make file writeable so it is isolated
        FileIO::SetReadOnly( fileA, false );

        // Force DB migration to emulate bff edit
        FBuildOptions optionsCopy( options );
        optionsCopy.m_ForceDBMigration_Debug = true;

        FBuild fBuild( optionsCopy );
        TEST_ASSERT( fBuild.Initialize( dbFile ) );
        TEST_ASSERT( fBuild.Build( "Exe" ) );

        // Check stats
        //               Seen,  Built,  Type
        CheckStatsNode ( 2,     2,      Node::UNITY_NODE );
        CheckStatsNode ( 3,     1,      Node::OBJECT_NODE );
        CheckStatsNode ( 1,     1,      Node::OBJECT_LIST_NODE );
        CheckStatsNode ( 1,     1,      Node::EXE_NODE );
    }
}

// SortFiles
//------------------------------------------------------------------------------
void TestUnity::SortFiles() const
{
    // Ensure sorting logic works as expected:
    //  - case insensitive (consistent regardless of file system or OS)
    //  - files before directories

    // Helper which allows access to UnityNode sorting functionality
    class Helper : public UnityNode
    {
    public:
        virtual ~Helper() override
        {
            for ( FileIO::FileInfo * info : m_HelperFileInfos )
            {
                FDELETE info;
            }
        }

        void AddFile( const char * fileName )
        {
            // Create dummy FileIO::FileInfo structure
            FileIO::FileInfo * info = FNEW( FileIO::FileInfo );
            info->m_Name = fileName; // Only the name is important
            #if defined( __WINDOWS__ )
                info->m_Name.Replace( '/', '\\' ); // Allow test to specify unix style slashes
            #endif
            m_HelperFileInfos.Append( info );

            // Add entry
            m_HelperFiles.EmplaceBack( info, nullptr );
        }

        void Sort()
        {
            m_HelperFiles.Sort();
            #if defined( __WINDOWS__ )
                for ( FileIO::FileInfo * info : m_HelperFileInfos )
                {
                    info->m_Name.Replace( '\\', '/' ); // Allow test to specify unix style slashes
                }
            #endif
        }

        const AString & operator[] ( size_t index ) const { return m_HelperFiles[ index ].GetName(); }

        Array< UnityNode::UnityFileAndOrigin >  m_HelperFiles;
        Array< FileIO::FileInfo * >             m_HelperFileInfos;
    };

    // Helper marcos to reduce boilerplate code
    #define SORT( ... )                                                         \
    do {                                                                           \
        const char * inputs[] = { __VA_ARGS__ };                                \
        Helper h;                                                               \
        for ( const char * input : inputs )                                     \
        {                                                                       \
            h.AddFile( input );                                                 \
        }                                                                       \
        h.Sort()

    #define TEST( ... )                                                         \
        const char * outputs[] = { __VA_ARGS__ };                               \
        for ( size_t i = 0; i < (sizeof(outputs) / sizeof(const char *)); ++i ) \
        {                                                                       \
            TEST_ASSERTM( h[ i ] == outputs[ i ], "Mismatch @ index %u: %s != %s", (uint32_t)i, h[ i ].Get(), outputs[ i ] ); \
        }                                                                       \
    } while( false )

    // Basic sanity check
    SORT( "a.cpp", "b.cpp" );
    TEST( "a.cpp", "b.cpp" );

    SORT( "b.cpp", "a.cpp" );
    TEST( "a.cpp", "b.cpp" );

    // Case is ignored at the file level
    SORT( "a.cpp", "B.cpp" );
    TEST( "a.cpp", "B.cpp" );

    SORT( "b.cpp", "A.cpp" );
    TEST( "A.cpp", "b.cpp" );

    // Files in same dir
    SORT( "a/B.cpp", "a/a.cpp" );
    TEST( "a/a.cpp", "a/B.cpp" );

    SORT( "a/b.cpp", "a/A.cpp" );
    TEST( "a/A.cpp", "a/b.cpp" );

    // Files in different dirs of same length
    SORT( "b/a.cpp", "a/a.cpp" );
    TEST( "a/a.cpp", "b/a.cpp" );

    SORT( "B/a.cpp", "a/a.cpp" );
    TEST( "a/a.cpp", "B/a.cpp" );

    // Subdirs come after dirs
    SORT( "a/a.cpp",    "z.cpp" );
    TEST( "z.cpp",      "a/a.cpp" );

    SORT( "A/A.cpp",    "z.cpp" );
    TEST( "z.cpp",      "A/A.cpp" );

    SORT( "Z/A.cpp",    "a.cpp" );
    TEST( "a.cpp",      "Z/A.cpp" );

    SORT( "a.cpp", "bbb/a.cpp", "c.cpp" );
    TEST( "a.cpp", "c.cpp",     "bbb/a.cpp" );

    // subdirs that match filename come after all files
    SORT( "a.cpp/a.cpp",    "a.cpp",    "b.cpp" );
    TEST( "a.cpp",          "b.cpp",    "a.cpp/a.cpp" );

    SORT( "aaa", "aba/a",   "aba" );
    TEST( "aaa", "aba",     "aba/a" );

    // subdirs that are partial matches
    SORT( "aa/a",   "a/a" );
    TEST( "a/a",    "aa/a" );

    // differing depths
    SORT( "Folder/SubDir/a.cpp",    "Folder/z.cpp" );
    TEST( "Folder/z.cpp",           "Folder/SubDir/a.cpp" );

    // same depth but different dirs
    SORT( "Folder/BBB/a.cpp", "Folder/AAA/z.cpp" );
    TEST( "Folder/AAA/z.cpp", "Folder/BBB/a.cpp" );

    // A real example from FASTBuild source
    SORT( "C:/p4/depot/Code/Tools/FBuild/FBuildCore/Cache/ICache.cpp",
          "C:/p4/depot/Code/Tools/FBuild/FBuildCore/FBuild.cpp",
          "C:/p4/depot/Code/Tools/FBuild/FBuildCore/Graph/CopyDirNode.cpp",
          "C:/p4/depot/Code/Tools/FBuild/FBuildCore/FBuildOptions.cpp" );
    TEST( "C:/p4/depot/Code/Tools/FBuild/FBuildCore/FBuild.cpp",
          "C:/p4/depot/Code/Tools/FBuild/FBuildCore/FBuildOptions.cpp",
          "C:/p4/depot/Code/Tools/FBuild/FBuildCore/Cache/ICache.cpp",
          "C:/p4/depot/Code/Tools/FBuild/FBuildCore/Graph/CopyDirNode.cpp" );

    #undef SORT
    #undef CHECK
}

// CacheUsingRelativePaths
//------------------------------------------------------------------------------
void TestUnity::CacheUsingRelativePaths() const
{
    // Source files
    const char * srcPath = "Tools/FBuild/FBuildTest/Data/TestUnity/CacheUsingRelativePaths/";
    const char * fileA = "File.cpp";
    const char * fileB = "Subdir/Header.h";
    const char * fileC = "fbuild.bff";
    const char * files[] = { fileA, fileB, fileC };

    // Dest paths
    const char * dstPathA = "../tmp/Test/Unity/CacheUsingRelativePaths/A/Code";
    const char * dstPathB = "../tmp/Test/Unity/CacheUsingRelativePaths/B/Code";
    const char * dstPaths[] = { dstPathA, dstPathB };

    #if defined( __WINDOWS__ )
        const char * objFileA = "../tmp/Test/Unity/CacheUsingRelativePaths/A/out/Unity1.obj";
    #else
        const char * objFileA = "../tmp/Test/Unity/CacheUsingRelativePaths/A/out/Unity1.o";
    #endif

    // Copy file structure to both destinations
    for ( const char * dstPath : dstPaths )
    {
        for ( const char * file : files )
        {
            AStackString<> src, dst;
            src.Format( "%s/%s", srcPath, file );
            dst.Format( "%s/%s", dstPath, file );
            TEST_ASSERT( FileIO::EnsurePathExistsForFile( dst ) );
            TEST_ASSERT( FileIO::FileCopy( src.Get(), dst.Get() ) );
        }
    }

    // Build in path A, writing to the cache
    {
        // Init
        FBuildTestOptions options;
        options.m_ConfigFile = "fbuild.bff";
        options.m_UseCacheWrite = true;
        AStackString<> codeDir;
        GetCodeDir( codeDir );
        codeDir.Trim( 0, 5 ); // Remove Code/
        codeDir += "tmp/Test/Unity/CacheUsingRelativePaths/A/Code/";
        options.SetWorkingDir( codeDir );
        FBuild fBuild( options );
        TEST_ASSERT( fBuild.Initialize() );

        // Compile
        TEST_ASSERT( fBuild.Build( "ObjectList" ) );

        TEST_ASSERT( fBuild.GetStats().GetCacheStores() == 1 );
    }

    // Check some problematic cases in the object file
    {
        // Read obj file into memory
        AString buffer;
        {
            FileStream f;
            TEST_ASSERT( f.Open( objFileA ) );
            buffer.SetLength( (uint32_t)f.GetFileSize() );
            TEST_ASSERT( f.ReadBuffer( buffer.Get(), f.GetFileSize() ) == f.GetFileSize() );
            buffer.Replace( (char)0, ' ' ); // Make string seaches simpler
        }

        // Check __FILE__ paths are relative
        TEST_ASSERT( buffer.Find( "FILE_MACRO_START_1(./Subdir/Header.h)FILE_MACRO_END_1" ) );
        #if defined( __WINDOWS__ )
            TEST_ASSERT( buffer.Find( "FILE_MACRO_START_2(.\\File.cpp)FILE_MACRO_END_2" ) );
        #else
            TEST_ASSERT( buffer.Find( "FILE_MACRO_START_2(./File.cpp)FILE_MACRO_END_2" ) );
        #endif
    }

    // Build in path B, reading from the cache
    {
        // Init
        FBuildTestOptions options;
        options.m_ConfigFile = "fbuild.bff";
        options.m_UseCacheRead = true;
        AStackString<> codeDir;
        GetCodeDir( codeDir );
        codeDir.Trim( 0, 5 ); // Remove Code/
        codeDir += "tmp/Test/Unity/CacheUsingRelativePaths/B/Code/";
        options.SetWorkingDir( codeDir );
        FBuild fBuild( options );
        TEST_ASSERT( fBuild.Initialize() );

        // Compile
        TEST_ASSERT( fBuild.Build( "ObjectList" ) );

        TEST_ASSERT( fBuild.GetStats().GetCacheHits() == 1 );
    }

}

//------------------------------------------------------------------------------
