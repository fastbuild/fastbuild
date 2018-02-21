// TestObjectList.cpp
//------------------------------------------------------------------------------

// Includes
//------------------------------------------------------------------------------
#include "FBuildTest.h"

#include "Tools/FBuild/FBuildCore/FBuild.h"
#include "Tools/FBuild/FBuildCore/BFF/BFFParser.h"
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
    #if defined( __WINDOWS__ )
        void ExtraOutputFolders() const;
    #endif
};

// Register Tests
//------------------------------------------------------------------------------
REGISTER_TESTS_BEGIN( TestObjectList )
    REGISTER_TEST( TestExcludedFiles )      // Ensure files are correctly excluded
    REGISTER_TEST( CompilerInputFilesRoot )
    #if defined( __WINDOWS__ )
        REGISTER_TEST( ExtraOutputFolders )
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

// ExtraOutputFolders
//------------------------------------------------------------------------------
#if defined( __WINDOWS__ )
    void TestObjectList::ExtraOutputFolders() const
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
