// TestObjectList.cpp
//------------------------------------------------------------------------------

// Includes
//------------------------------------------------------------------------------
#include "FBuildTest.h"

// FBuildCore
#include "Tools/FBuild/FBuildCore/BFF/BFFParser.h"
#include "Tools/FBuild/FBuildCore/BFF/Functions/FunctionObjectList.h"
#include "Tools/FBuild/FBuildCore/FBuild.h"

// Core
#include "Core/Strings/AStackString.h"

// TestObjectList
//------------------------------------------------------------------------------
class TestObjectList : public FBuildTest
{
private:
    DECLARE_TESTS

    // Tests
    void TestExcludedFiles() const;
    void CompilerInputFilesRoot() const;
    void ConflictingObjects1() const;
    void ConflictingObjects2() const;
    void ExtraOutputFolders_PathExtraction() const;
    void ObjectListChaining() const;
    #if defined( __WINDOWS__ )
        void ExtraOutputFolders_Build() const;
    #endif
};

// Register Tests
//------------------------------------------------------------------------------
REGISTER_TESTS_BEGIN( TestObjectList )
    REGISTER_TEST( TestExcludedFiles )      // Ensure files are correctly excluded
    REGISTER_TEST( CompilerInputFilesRoot )
    REGISTER_TEST( ConflictingObjects1 )
    REGISTER_TEST( ConflictingObjects2 )
    REGISTER_TEST( ExtraOutputFolders_PathExtraction )
    REGISTER_TEST( ObjectListChaining )
    #if defined( __WINDOWS__ )
        REGISTER_TEST( ExtraOutputFolders_Build )
    #endif
REGISTER_TESTS_END

// TestExcludedFiles
//------------------------------------------------------------------------------
void TestObjectList::TestExcludedFiles() const
{
    FBuildTestOptions options;
    options.m_ConfigFile = "Tools/FBuild/FBuildTest/Data/TestObjectList/Exclusions/fbuild.bff";

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

// CompilerInputFilesRoot
//------------------------------------------------------------------------------
void TestObjectList::CompilerInputFilesRoot() const
{
    FBuildTestOptions options;
    options.m_ConfigFile = "Tools/FBuild/FBuildTest/Data/TestObjectList/CompilerInputFilesRoot/fbuild.bff";

    FBuild fBuild( options );
    TEST_ASSERT( fBuild.Initialize() );
    TEST_ASSERT( fBuild.Build( "ObjectList" ) );
}

// ConflictingObjects1
//------------------------------------------------------------------------------
void TestObjectList::ConflictingObjects1() const
{
    //
    // An ObjectList that builds two different files to the same location
    //
    FBuildTestOptions options;
    options.m_ConfigFile = "Tools/FBuild/FBuildTest/Data/TestObjectList/ConflictingObjects/fbuild1.bff";

    FBuild fBuild( options );
    TEST_ASSERT( fBuild.Initialize() );
    TEST_ASSERT( fBuild.Build( "ObjectList" ) == false );
    TEST_ASSERT( GetRecordedOutput().Find( "Conflicting objects found" ) );
}

// ConflictingObjects2
//------------------------------------------------------------------------------
void TestObjectList::ConflictingObjects2() const
{
    //
    // Two ObjectLists that build the same file with different settings to the same location
    //
    FBuildTestOptions options;
    options.m_ConfigFile = "Tools/FBuild/FBuildTest/Data/TestObjectList/ConflictingObjects/fbuild2.bff";

    FBuild fBuild( options );
    TEST_ASSERT( fBuild.Initialize() );
    TEST_ASSERT( fBuild.Build( "ObjectLists" ) == false );
    TEST_ASSERT( GetRecordedOutput().Find( "Conflicting objects found" ) );
}

// ExtraOutputFolders_PathExtraction
//------------------------------------------------------------------------------
void TestObjectList::ExtraOutputFolders_PathExtraction() const
{
    // Check that these cases are handled:
    // - mixed slashes
    // - ../
    // - double //
    AStackString<> args( " /FdTools\\FBuild\\FBuildTest\\Data/../../../../../tmp/Test/ObjectList/ExtraOutputPaths/ObjectList//pdb/file.pdb"
                         " /FaTools\\FBuild\\FBuildTest\\Data/../../../../../tmp/Test/ObjectList/ExtraOutputPaths/ObjectList//asm/file.asm" );

    // Getthe paths
    AStackString<> pdbPath, asmPath;
    FunctionObjectList::GetExtraOutputPaths( args, pdbPath, asmPath );

    // Check that the entire span is correctly captured
    TEST_ASSERT( pdbPath.BeginsWith( "Tools" ) );
    TEST_ASSERT( pdbPath.EndsWith( "pdb" ) && !pdbPath.EndsWith( ".pdb" ) );
    TEST_ASSERT( asmPath.BeginsWith( "Tools" ) );
    TEST_ASSERT( asmPath.EndsWith( "asm" ) && !pdbPath.EndsWith( ".asm" ) );
}

// ObjectListChaining
//  - Ensure that an ObjectList consuming the output of another ObjectList
//    is managed correctly
//------------------------------------------------------------------------------
void TestObjectList::ObjectListChaining() const
{
    FBuildTestOptions options;
    options.m_ConfigFile = "Tools/FBuild/FBuildTest/Data/TestObjectList/ObjectListChaining/fbuild.bff";
    const char * dbFile = "../tmp/Test/TestObjectList/ObjectListChaining/fbuild.fdb";

    AString depGraphText1( 8 * 1024 );
    AString depGraphText2( 8 * 1024 );

    // Build
    {
        FBuildForTest fBuild( options );
        TEST_ASSERT( fBuild.Initialize() );
        TEST_ASSERT( fBuild.Build( "ObjectList2" ) );

        // Save DB for reloading below
        TEST_ASSERT( fBuild.SaveDependencyGraph( dbFile ) );

        // Check stats
        //               Seen,  Built,  Type
        CheckStatsNode(     2,      2,  Node::OBJECT_LIST_NODE );
        CheckStatsNode(     2,      2,  Node::OBJECT_NODE );
        CheckStatsNode(     2,      2,  Node::DIRECTORY_LIST_NODE );

        fBuild.SerializeDepGraphToText( "ObjectList2", depGraphText1 );
    }

    // Check no-rebuild
    {
        FBuildForTest fBuild( options );
        TEST_ASSERT( fBuild.Initialize( dbFile ) );
        TEST_ASSERT( fBuild.Build( "ObjectList2" ) );

        // Check stats
        //               Seen,  Built,  Type
        CheckStatsNode(     2,      0,  Node::OBJECT_LIST_NODE );
        CheckStatsNode(     2,      0,  Node::OBJECT_NODE );
        CheckStatsNode(     2,      2,  Node::DIRECTORY_LIST_NODE );
    }

    // Check no-rebuild DB migration
    {
        options.m_ForceDBMigration_Debug = true;

        FBuildForTest fBuild( options );
        TEST_ASSERT( fBuild.Initialize( dbFile ) );
        TEST_ASSERT( fBuild.Build( "ObjectList2" ) );

        // Check stats
        //               Seen,  Built,  Type
        CheckStatsNode( 2, 0, Node::OBJECT_LIST_NODE );
        CheckStatsNode( 2, 0, Node::OBJECT_NODE );
        CheckStatsNode( 2, 2, Node::DIRECTORY_LIST_NODE );

        fBuild.SerializeDepGraphToText( "ObjectList2", depGraphText2 );
    }

    // Check node graph is the same after migration
    TEST_ASSERT( depGraphText1 == depGraphText2 );
}

// ExtraOutputFolders_Build
//------------------------------------------------------------------------------
#if defined( __WINDOWS__ )
    void TestObjectList::ExtraOutputFolders_Build() const
    {
        FBuildTestOptions options;
        options.m_ConfigFile = "Tools/FBuild/FBuildTest/Data/TestObjectList/ExtraOutputPaths/fbuild.bff";

        const char * objectListASMFile  = "../tmp/Test/ObjectList/ExtraOutputPaths/ObjectList/asm/file.asm";
        const char * objectListASMDir   = "../tmp/Test/ObjectList/ExtraOutputPaths/ObjectList/asm/";
        const char * objectListPDBFile  = "../tmp/Test/ObjectList/ExtraOutputPaths/ObjectList/pdb/file.pdb";
        const char * objectListPDBDir   = "../tmp/Test/ObjectList/ExtraOutputPaths/ObjectList/pdb/";
        const char * libraryASMFile     = "../tmp/Test/ObjectList/ExtraOutputPaths/Library/asm/file.asm";
        const char * libraryASMDir      = "../tmp/Test/ObjectList/ExtraOutputPaths/Library/asm/";
        const char * libraryPDBFile     = "../tmp/Test/ObjectList/ExtraOutputPaths/Library/pdb/file.pdb";
        const char * libraryPDBDir      = "../tmp/Test/ObjectList/ExtraOutputPaths/Library/pdb/";

        // Cleanup from previous runs to ensure we're really testing folder creation
        EnsureFileDoesNotExist( objectListASMFile );
        EnsureDirDoesNotExist( objectListASMDir );
        EnsureFileDoesNotExist( objectListPDBFile );
        EnsureDirDoesNotExist( objectListPDBDir );
        EnsureFileDoesNotExist( libraryASMFile );
        EnsureDirDoesNotExist( libraryASMDir );
        EnsureFileDoesNotExist( libraryPDBFile );
        EnsureDirDoesNotExist( libraryPDBDir );

        // ObjectList
        {
            FBuild fBuild( options );
            TEST_ASSERT( fBuild.Initialize() );
            TEST_ASSERT( fBuild.Build( "ObjectList" ) );

            EnsureFileExists( objectListASMFile );
            EnsureFileExists( objectListPDBFile );
        }

        // Library
        {
            FBuild fBuild( options );
            TEST_ASSERT( fBuild.Initialize() );
            TEST_ASSERT( fBuild.Build( "Library" ) );

            EnsureFileExists( libraryASMFile );
            EnsureFileExists( libraryPDBFile );
        }
    }
#endif

//------------------------------------------------------------------------------
