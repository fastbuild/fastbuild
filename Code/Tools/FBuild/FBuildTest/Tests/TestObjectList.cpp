// TestObjectList.cpp
//------------------------------------------------------------------------------

// Includes
//------------------------------------------------------------------------------
#include "FBuildTest.h"

#include "Tools/FBuild/FBuildCore/FBuild.h"
#include "Tools/FBuild/FBuildCore/BFF/BFFParser.h"
#include "Tools/FBuild/FBuildCore/BFF/Functions/FunctionObjectList.h"
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
    void ExtraOutputFolders_PathExtraction() const;
    #if defined( __WINDOWS__ )
        void ExtraOutputFolders_Build() const;
    #endif
};

// Register Tests
//------------------------------------------------------------------------------
REGISTER_TESTS_BEGIN( TestObjectList )
    REGISTER_TEST( TestExcludedFiles )      // Ensure files are correctly excluded
    REGISTER_TEST( CompilerInputFilesRoot )
    REGISTER_TEST( ExtraOutputFolders_PathExtraction )
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

// CompilerInputFilesRoot
//------------------------------------------------------------------------------
void TestObjectList::CompilerInputFilesRoot() const
{
    FBuildTestOptions options;
    options.m_ConfigFile = "Tools/FBuild/FBuildTest/Data/TestObjectList/CompilerInputFilesRoot/fbuild.bff";

    FBuild fBuild( options );
    TEST_ASSERT( fBuild.Initialize() );
    TEST_ASSERT( fBuild.Build( AStackString<>( "ObjectList" ) ) );
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
    TEST_ASSERT( pdbPath.EndsWith( "pdb" ) && !pdbPath.EndsWith( ".pdb" ));
    TEST_ASSERT( asmPath.BeginsWith( "Tools" ) );
    TEST_ASSERT( asmPath.EndsWith( "asm" ) && !pdbPath.EndsWith( ".asm" ));
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
            TEST_ASSERT( fBuild.Build( AStackString<>( "ObjectList" ) ) );

            EnsureFileExists( objectListASMFile );
            EnsureFileExists( objectListPDBFile );
        }

        // Library
        {
            FBuild fBuild( options );
            TEST_ASSERT( fBuild.Initialize() );
            TEST_ASSERT( fBuild.Build( AStackString<>( "Library" ) ) );

            EnsureFileExists( libraryASMFile );
            EnsureFileExists( libraryPDBFile );
        }
    }
#endif

//------------------------------------------------------------------------------
