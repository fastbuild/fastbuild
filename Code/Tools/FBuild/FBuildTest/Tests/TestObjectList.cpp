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

//------------------------------------------------------------------------------
TEST_GROUP( TestObjectList, FBuildTest )
{
public:
};

//------------------------------------------------------------------------------
TEST_CASE( TestObjectList, Exclusions )
{
    FBuildTestOptions options;
    options.m_ConfigFile = "Tools/FBuild/FBuildTest/Data/TestObjectList/Exclusions/fbuild.bff";
    FBuildForTest fBuild( options );
    TEST_ASSERT( fBuild.Initialize() );

    // build (via alias)
    TEST_ASSERT( fBuild.Build( "Test" ) );

    // Check all the exclusion methods worked as expected
    // clang-format off
    const char * const nodesToCheck[] =
    {
        "ExcludedFiles-FileName",
        "ExcludedFiles-FileNameWithPath-ForwardSlash",
        "ExcludedFiles-FileNameWithPath-Backslash",
        "ExcludedFiles-FileNameWithRelativePath-ForwardSlash",
        "ExcludedFiles-FileNameWithRelativePath-Backslash",
        "ExcludePattern",
        "ExcludePattern-ForwardSlash",
        "ExcludePattern-Backslash",
    };
    // clang-format on
    for ( const char * const nodeName : nodesToCheck )
    {
        // Get the ObjectListNode
        const Node * objectListNode = fBuild.GetNode( nodeName );
        TEST_ASSERT( objectListNode );
        TEST_ASSERT( objectListNode->GetType() == Node::OBJECT_LIST_NODE );

        // Check that it has one dynamic dependency, and that it's the 'B' file
        TEST_ASSERT( objectListNode->GetDynamicDependencies().GetSize() == 1 );
#if defined( __WINDOWS__ )
        TEST_ASSERT( objectListNode->GetDynamicDependencies()[ 0 ].GetNode()->GetName().EndsWithI( "ok.obj" ) );
#else
        TEST_ASSERT( objectListNode->GetDynamicDependencies()[ 0 ].GetNode()->GetName().EndsWithI( "ok.o" ) );
#endif
    }
}

//------------------------------------------------------------------------------
TEST_CASE( TestObjectList, CompilerInputFilesRoot )
{
    FBuildTestOptions options;
    options.m_ConfigFile = "Tools/FBuild/FBuildTest/Data/TestObjectList/CompilerInputFilesRoot/fbuild.bff";

    FBuildForTest fBuild( options );
    TEST_ASSERT( fBuild.Initialize() );
    TEST_ASSERT( fBuild.Build( "ObjectList" ) );
}

//------------------------------------------------------------------------------
TEST_CASE( TestObjectList, ConflictingObjects1 )
{
    //
    // An ObjectList that builds two different files to the same location
    //
    FBuildTestOptions options;
    options.m_ConfigFile = "Tools/FBuild/FBuildTest/Data/TestObjectList/ConflictingObjects/fbuild1.bff";

    FBuildForTest fBuild( options );
    TEST_ASSERT( fBuild.Initialize() );
    TEST_ASSERT( fBuild.Build( "ObjectList" ) == false );
    TEST_ASSERT( GetRecordedOutput().Find( "Conflicting objects found" ) );
}

//------------------------------------------------------------------------------
TEST_CASE( TestObjectList, ConflictingObjects2 )
{
    //
    // Two ObjectLists that build the same file with different settings to the same location
    //
    FBuildTestOptions options;
    options.m_ConfigFile = "Tools/FBuild/FBuildTest/Data/TestObjectList/ConflictingObjects/fbuild2.bff";

    FBuildForTest fBuild( options );
    TEST_ASSERT( fBuild.Initialize() );
    TEST_ASSERT( fBuild.Build( "ObjectLists" ) == false );
    TEST_ASSERT( GetRecordedOutput().Find( "Conflicting objects found" ) );
}

//------------------------------------------------------------------------------
TEST_CASE( TestObjectList, ExtraOutputFolders_PathExtraction )
{
    // Check that these cases are handled:
    // - mixed slashes
    // - ../
    // - double //
    AStackString args( " /FdTools\\FBuild\\FBuildTest\\Data/../../../../../tmp/Test/ObjectList/ExtraOutputPaths/ObjectList//pdb/file.pdb"
                       " /FaTools\\FBuild\\FBuildTest\\Data/../../../../../tmp/Test/ObjectList/ExtraOutputPaths/ObjectList//asm/file.asm"
                       " /sourceDependencies Tools\\FBuild\\FBuildTest\\Data/../../../../../tmp/Test/ObjectList/ExtraOutputPaths/ObjectList//srcDeps/file.json" );

    // Get the paths
    AStackString pdbPath;
    AStackString asmPath;
    AStackString sourceDependenciesPath;
    FunctionObjectList::GetExtraOutputPaths( args, pdbPath, asmPath, sourceDependenciesPath );

    // Check that the entire span is correctly captured
    TEST_ASSERT( pdbPath.BeginsWith( "Tools" ) );
    TEST_ASSERT( pdbPath.EndsWith( "pdb" ) && !pdbPath.EndsWith( ".pdb" ) );
    TEST_ASSERT( asmPath.BeginsWith( "Tools" ) );
    TEST_ASSERT( asmPath.EndsWith( "asm" ) && !asmPath.EndsWith( ".asm" ) );
    TEST_ASSERT( sourceDependenciesPath.BeginsWith( "Tools" ) );
    TEST_ASSERT( sourceDependenciesPath.EndsWith( "srcDeps" ) && !sourceDependenciesPath.EndsWith( ".json" ) );
}

//------------------------------------------------------------------------------
TEST_CASE( TestObjectList, ObjectListChaining )
{
    //  - Ensure that an ObjectList consuming the output of another ObjectList
    //    is managed correctly
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

        // Check stats: Seen, Built, Type
        CheckStatsNode( 2, 2, Node::OBJECT_LIST_NODE );
        CheckStatsNode( 4, 4, Node::OBJECT_NODE );
        CheckStatsNode( 1, 1, Node::DIRECTORY_LIST_NODE );

        fBuild.SerializeDepGraphToText( "ObjectList2", depGraphText1 );
    }

    // Check no-rebuild
    {
        FBuildForTest fBuild( options );
        TEST_ASSERT( fBuild.Initialize( dbFile ) );
        TEST_ASSERT( fBuild.Build( "ObjectList2" ) );

        // Check stats: Seen, Built, Type
        CheckStatsNode( 2, 0, Node::OBJECT_LIST_NODE );
        CheckStatsNode( 4, 0, Node::OBJECT_NODE );
        CheckStatsNode( 1, 1, Node::DIRECTORY_LIST_NODE );
    }

    // Check no-rebuild DB migration
    {
        options.m_ForceDBMigration_Debug = true;

        FBuildForTest fBuild( options );
        TEST_ASSERT( fBuild.Initialize( dbFile ) );
        TEST_ASSERT( fBuild.Build( "ObjectList2" ) );

        // Check stats: Seen, Built, Type
        CheckStatsNode( 2, 0, Node::OBJECT_LIST_NODE );
        CheckStatsNode( 4, 0, Node::OBJECT_NODE );
        CheckStatsNode( 1, 1, Node::DIRECTORY_LIST_NODE );

        fBuild.SerializeDepGraphToText( "ObjectList2", depGraphText2 );
    }

    // Check node graph is the same after migration
    TEST_ASSERT( depGraphText1 == depGraphText2 );
}

//------------------------------------------------------------------------------
TEST_CASE( TestObjectList, ObjectListChaining_Bad )
{
    // Ensure that an ObjectList consuming the output of another ObjectList
    // is managed correctly when using PreBuildDependencies and dynamic discovery
    // of output files.
    //
    // NOTE: This setup has some fundamental problems and should generally not be used.
    //       For example, deletion of a file in the source directory won't remove the
    //       intermediate file and copies of the stale file can still occur, and/or
    //       cause build failures.
    //
    // The preferred mechanism is to chain ObjectLists together via .CompileInputObjectList
    // (see ObjectListChaining test above)
    //
    // This test is kept to ensure that setup doesn't regress for users who might be
    // using it
    //
    FBuildTestOptions options;
    options.m_ConfigFile = "Tools/FBuild/FBuildTest/Data/TestObjectList/ObjectListChaining_Bad/fbuild.bff";
    const char * dbFile = "../tmp/Test/TestObjectList/ObjectListChaining_Bad/fbuild.fdb";

    AString depGraphText1( 8 * 1024 );
    AString depGraphText2( 8 * 1024 );

    // Build
    {
        FBuildForTest fBuild( options );
        TEST_ASSERT( fBuild.Initialize() );
        TEST_ASSERT( fBuild.Build( "ObjectList2" ) );

        // Save DB for reloading below
        TEST_ASSERT( fBuild.SaveDependencyGraph( dbFile ) );

        // Check stats: Seen, Built, Type
        CheckStatsNode( 2, 2, Node::OBJECT_LIST_NODE );
        CheckStatsNode( 2, 2, Node::OBJECT_NODE );
        CheckStatsNode( 2, 2, Node::DIRECTORY_LIST_NODE );

        fBuild.SerializeDepGraphToText( "ObjectList2", depGraphText1 );
    }

    // Check no-rebuild
    {
        FBuildForTest fBuild( options );
        TEST_ASSERT( fBuild.Initialize( dbFile ) );
        TEST_ASSERT( fBuild.Build( "ObjectList2" ) );

        // Check stats: Seen, Built, Type
        CheckStatsNode( 2, 0, Node::OBJECT_LIST_NODE );
        CheckStatsNode( 2, 0, Node::OBJECT_NODE );
        CheckStatsNode( 2, 2, Node::DIRECTORY_LIST_NODE );
    }

    // Check no-rebuild DB migration
    {
        options.m_ForceDBMigration_Debug = true;

        FBuildForTest fBuild( options );
        TEST_ASSERT( fBuild.Initialize( dbFile ) );
        TEST_ASSERT( fBuild.Build( "ObjectList2" ) );

        // Check stats: Seen, Built, Type
        CheckStatsNode( 2, 0, Node::OBJECT_LIST_NODE );
        CheckStatsNode( 2, 0, Node::OBJECT_NODE );
        CheckStatsNode( 2, 2, Node::DIRECTORY_LIST_NODE );

        fBuild.SerializeDepGraphToText( "ObjectList2", depGraphText2 );
    }

    // Check node graph is the same after migration
    TEST_ASSERT( depGraphText1 == depGraphText2 );
}

//------------------------------------------------------------------------------
#if defined( __WINDOWS__ )
TEST_CASE( TestObjectList, ExtraOutputFolders_Build )
{
    FBuildTestOptions options;
    options.m_ConfigFile = "Tools/FBuild/FBuildTest/Data/TestObjectList/ExtraOutputPaths/fbuild.bff";

    const char * objectListASMFile = "../tmp/Test/ObjectList/ExtraOutputPaths/ObjectList/asm/file.asm";
    const char * objectListASMDir = "../tmp/Test/ObjectList/ExtraOutputPaths/ObjectList/asm/";
    const char * objectListPDBFile = "../tmp/Test/ObjectList/ExtraOutputPaths/ObjectList/pdb/file.pdb";
    const char * objectListPDBDir = "../tmp/Test/ObjectList/ExtraOutputPaths/ObjectList/pdb/";
    const char * libraryASMFile = "../tmp/Test/ObjectList/ExtraOutputPaths/Library/asm/file.asm";
    const char * libraryASMDir = "../tmp/Test/ObjectList/ExtraOutputPaths/Library/asm/";
    const char * libraryPDBFile = "../tmp/Test/ObjectList/ExtraOutputPaths/Library/pdb/file.pdb";
    const char * libraryPDBDir = "../tmp/Test/ObjectList/ExtraOutputPaths/Library/pdb/";

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
        FBuildForTest fBuild( options );
        TEST_ASSERT( fBuild.Initialize() );
        TEST_ASSERT( fBuild.Build( "ObjectList" ) );

        EnsureFileExists( objectListASMFile );
        EnsureFileExists( objectListPDBFile );
    }

    // Library
    {
        FBuildForTest fBuild( options );
        TEST_ASSERT( fBuild.Initialize() );
        TEST_ASSERT( fBuild.Build( "Library" ) );

        EnsureFileExists( libraryASMFile );
        EnsureFileExists( libraryPDBFile );
    }
}
#endif

//------------------------------------------------------------------------------
