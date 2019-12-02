// TestPathUtils.cpp
//------------------------------------------------------------------------------

// Includes
//------------------------------------------------------------------------------
#include "TestFramework/UnitTest.h"

#include "Core/FileIO/PathUtils.h"
#include "Core/Strings/AStackString.h"

// TestPathUtils
//------------------------------------------------------------------------------
class TestPathUtils : public UnitTest
{
private:
    DECLARE_TESTS

    void TestFixupFolderPath() const;
    void TestPathBeginsWith() const;
    void TestPathEndsWithFile() const;
    void GetRelativePath() const;
};

// Register Tests
//------------------------------------------------------------------------------
REGISTER_TESTS_BEGIN( TestPathUtils )
    // Increment
    REGISTER_TEST( TestFixupFolderPath )
    REGISTER_TEST( TestPathBeginsWith )
    REGISTER_TEST( TestPathEndsWithFile )
    REGISTER_TEST( GetRelativePath )
REGISTER_TESTS_END

// TestFixupFolderPath
//------------------------------------------------------------------------------
void TestPathUtils::TestFixupFolderPath() const
{
    #define DOCHECK( before, after ) \
    { \
        AStackString<> path( before ); \
        PathUtils::FixupFolderPath( path ); \
        TEST_ASSERT( path == after ); \
    }

    #if defined( __WINDOWS__ )
        // standard windows path
        DOCHECK( "c:\\folder", "c:\\folder\\" )

        // redundant slashes
        DOCHECK( "c:\\folder\\\\thing", "c:\\folder\\thing\\" )

        // UNC path double slash is preserved
        DOCHECK( "\\\\server\\folder", "\\\\server\\folder\\" )
    #endif

    #undef DOCHECK
}

// TestPathBeginsWith
//------------------------------------------------------------------------------
void TestPathUtils::TestPathBeginsWith() const
{
    #define DOCHECK( path, subPath, expectedResult ) \
    { \
        AStackString<> a( path ); \
        AStackString<> b( subPath ); \
        bool result = PathUtils::PathBeginsWith( a, b ); \
        TEST_ASSERT( result == expectedResult ); \
    }

    #if defined( __WINDOWS__)
        DOCHECK( "c:\\folder\\subFolder\\", "c:\\folder\\", true )
        DOCHECK( "c:\\folder\\subFolder", "c:\\folder", true )

        DOCHECK( "c:\\anotherfolder\\subFolder\\", "c:\\folder\\", false )
        DOCHECK( "c:\\anotherfolder\\subFolder", "c:\\folder", false )
    #else
        DOCHECK( "/folder/subFolder/", "/folder/", true )
        DOCHECK( "/folder/subFolder", "/folder", true )

        DOCHECK( "/anotherfolder/subFolder/", "/folder/", false )
        DOCHECK( "/anotherfolder/subFolder", "/folder", false )
    #endif

    #if defined( __LINUX__ )
        // Case sensitivity checks
        DOCHECK( "/FOLDER/subFolder/", "/folder/", false )
        DOCHECK( "/FOLDER/subFolder", "/folder", false )
    #endif

    #undef DOCHECK
}

// TestPathEndsWithFile
//------------------------------------------------------------------------------
void TestPathUtils::TestPathEndsWithFile() const
{
    #define DOCHECK( path, subPath, expectedResult ) \
    { \
        AStackString<> a( path ); \
        AStackString<> b( subPath ); \
        bool result = PathUtils::PathEndsWithFile( a, b ); \
        TEST_ASSERT( result == expectedResult ); \
    }

    #if defined( __WINDOWS__)
        DOCHECK( "c:\\folder\\file.cpp", "file.cpp", true )
        DOCHECK( "c:\\file.cpp", "file.cpp", true )

        DOCHECK( "c:\\folder\\anotherfile.cpp", "file.cpp", false )
        DOCHECK( "c:\\anotherfile.cpp", "file.cpp", false )
    #else
        DOCHECK( "/folder/file.cpp", "file.cpp", true )
        DOCHECK( "/file.cpp", "file.cpp", true )

        DOCHECK( "/folder/anotherfile.cpp", "file.cpp", false )
        DOCHECK( "/anotherfile.cpp", "file.cpp", false )
    #endif

    #if defined( __LINUX__ )
        // Case sensitivity checks
        DOCHECK( "/folder/FILE.cpp", "file.cpp", false )
        DOCHECK( "/FILE.cpp", "file.cpp", false )
    #endif

    #undef DOCHECK
}

// GetRelativePath
//------------------------------------------------------------------------------
void TestPathUtils::GetRelativePath() const
{
    #define DOCHECK( base, path, expectedResult ) \
    { \
        AStackString<> result; \
        PathUtils::GetRelativePath( AStackString<>( base ), \
                                    AStackString<>( path ), \
                                    result ); \
        TEST_ASSERTM( result == expectedResult, "Expected: %s\nGot     : %s", expectedResult, result.Get() ); \
    }

    #if defined( __WINDOWS__ )
        // Simple case
        DOCHECK( "C:\\dir\\",   "C:\\dir\\file.cpp",    "file.cpp" )
        DOCHECK( "C:\\dir\\",   "C:\\dir\\folder\\",    "folder\\" )
        DOCHECK( "dir\\",       "dir\\file.cpp",        "file.cpp" )
        DOCHECK( "dir\\",       "dir\\folder\\",        "folder\\" )

        // Relative dirs
        DOCHECK( "C:\\dir\\",   "C:\\other\\file.cpp",  "..\\other\\file.cpp" )
        DOCHECK( "C:\\dir\\",   "C:\\other\\folder\\",  "..\\other\\folder\\" )
        DOCHECK( "dir\\A\\",    "dir\\B\\file.cpp",      "..\\B\\file.cpp" )
        DOCHECK( "dir\\A\\",    "dir\\B\\folder\\",      "..\\B\\folder\\" )

        // Case-insensitive
        DOCHECK( "C:\\dir\\",   "c:\\dir\\file.cpp",    "file.cpp" )
        DOCHECK( "C:\\dir\\",   "C:\\DIR\\folder\\",    "folder\\" )
        DOCHECK( "dir\\",       "DIR\\file.cpp",        "file.cpp" )
        DOCHECK( "dir\\",       "DIR\\folder\\",        "folder\\" )

        // Not relative
        DOCHECK( "C:\\dir\\",   "X:\\file.cpp",         "X:\\file.cpp" )
        DOCHECK( "C:\\dir\\",   "X:\\dir\\folder\\",    "X:\\dir\\folder\\" )
        DOCHECK( "dir\\",       "file.cpp",             "file.cpp" )
        DOCHECK( "dir\\",       "other\\folder\\",      "other\\folder\\" )

        // UNC Path
        DOCHECK( "\\server\\dir\\",   "\\server\\file.cpp",         "..\\file.cpp" )
        DOCHECK( "\\server\\dir\\",   "\\server\\dir\\folder\\",    "folder\\" )

        // Directory with substring matches
        DOCHECK( "C:\\folder",   "C:\\folderA",         "..\\folderA" )
    #else
        // Simple case
        DOCHECK( "/dir/",       "/dir/file.cpp",        "file.cpp" )
        DOCHECK( "/dir/",       "/dir/folder/",         "folder/" )

        // Relative dirs
        DOCHECK( "/dir/",       "/other/file.cpp",      "../other/file.cpp" )
        DOCHECK( "/dir/",       "/other/folder/",       "../other/folder/" )

        #if defined( __OSX__ )
            // Case-insensitive
            DOCHECK( "/dir/",       "/dir/file.cpp",        "file.cpp" )
            DOCHECK( "/dir/",       "/DIR/folder/",         "folder/" )
        #endif

        // Not relative
        DOCHECK( "dir/",        "other/file.cpp",       "other/file.cpp" )
        DOCHECK( "dir/",        "other/folder/",        "other/folder/" )

        // Directory with substring matches
        DOCHECK( "/folder",     "/folderA",             "../folderA" )
    #endif

    #undef DOCHECK
}

//------------------------------------------------------------------------------
