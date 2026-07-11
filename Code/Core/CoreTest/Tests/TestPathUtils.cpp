// TestPathUtils.cpp
//------------------------------------------------------------------------------

// Includes
//------------------------------------------------------------------------------
#include "TestFramework/TestGroup.h"

#include "Core/FileIO/PathUtils.h"
#include "Core/Strings/AStackString.h"

//------------------------------------------------------------------------------
TEST_GROUP( TestPathUtils, TestGroupTest )
{
public:
};

//------------------------------------------------------------------------------
#if defined( __WINDOWS__ )
TEST_CASE( TestPathUtils, TestFixupFolderPath )
{
    #define DOCHECK( before, after ) \
        do { \
            AStackString path( before ); \
            PathUtils::FixupFolderPath( path ); \
            TEST_ASSERT( path == after ); \
        } while ( false )

    // standard windows path
    DOCHECK( "c:\\folder", "c:\\folder\\" );

    // redundant slashes
    DOCHECK( "c:\\folder\\\\thing", "c:\\folder\\thing\\" );

    // UNC path double slash is preserved
    DOCHECK( "\\\\server\\folder", "\\\\server\\folder\\" );

    #undef DOCHECK
}
#endif

//------------------------------------------------------------------------------
TEST_CASE( TestPathUtils, TestPathBeginsWith )
{
#define DOCHECK( path, subPath, expectedResult ) \
    do { \
        const AStackString a( path ); \
        const AStackString b( subPath ); \
        const bool result = PathUtils::PathBeginsWith( a, b ); \
        TEST_ASSERT( result == expectedResult ); \
    } while ( false )

#if defined( __WINDOWS__ )
    DOCHECK( "c:\\folder\\subFolder\\", "c:\\folder\\", true );
    DOCHECK( "c:\\folder\\subFolder", "c:\\folder", true );

    DOCHECK( "c:\\anotherfolder\\subFolder\\", "c:\\folder\\", false );
    DOCHECK( "c:\\anotherfolder\\subFolder", "c:\\folder", false );
#else
    DOCHECK( "/folder/subFolder/", "/folder/", true );
    DOCHECK( "/folder/subFolder", "/folder", true );

    DOCHECK( "/anotherfolder/subFolder/", "/folder/", false );
    DOCHECK( "/anotherfolder/subFolder", "/folder", false );
#endif

#if defined( __LINUX__ )
    // Case sensitivity checks
    DOCHECK( "/FOLDER/subFolder/", "/folder/", false );
    DOCHECK( "/FOLDER/subFolder", "/folder", false );
#endif

#undef DOCHECK
}

//------------------------------------------------------------------------------
TEST_CASE( TestPathUtils, TestPathEndsWithFile )
{
#define DOCHECK( path, subPath, expectedResult ) \
    do { \
        const AStackString a( path ); \
        const AStackString b( subPath ); \
        const bool result = PathUtils::PathEndsWithFile( a, b ); \
        TEST_ASSERT( result == expectedResult ); \
    } while ( false )

#if defined( __WINDOWS__ )
    DOCHECK( "c:\\folder\\file.cpp", "file.cpp", true );
    DOCHECK( "c:\\file.cpp", "file.cpp", true );

    DOCHECK( "c:\\folder\\anotherfile.cpp", "file.cpp", false );
    DOCHECK( "c:\\anotherfile.cpp", "file.cpp", false );
#else
    DOCHECK( "/folder/file.cpp", "file.cpp", true );
    DOCHECK( "/file.cpp", "file.cpp", true );

    DOCHECK( "/folder/anotherfile.cpp", "file.cpp", false );
    DOCHECK( "/anotherfile.cpp", "file.cpp", false );
#endif

#if defined( __LINUX__ )
    // Case sensitivity checks
    DOCHECK( "/folder/FILE.cpp", "file.cpp", false );
    DOCHECK( "/FILE.cpp", "file.cpp", false );
#endif

#undef DOCHECK
}

//------------------------------------------------------------------------------
TEST_CASE( TestPathUtils, GetRelativePath )
{
#define DOCHECK( base, path, expectedResult ) \
    do { \
        AStackString result; \
        PathUtils::GetRelativePath( AStackString( base ), \
                                    AStackString( path ), \
                                    result ); \
        TEST_ASSERTM( result == expectedResult, "Expected: %s\nGot     : %s", expectedResult, result.Get() ); \
    } while ( false )

#if defined( __WINDOWS__ )
    // Simple case
    DOCHECK( "C:\\dir\\",   "C:\\dir\\file.cpp",    "file.cpp" );
    DOCHECK( "C:\\dir\\",   "C:\\dir\\folder\\",    "folder\\" );
    DOCHECK( "dir\\",       "dir\\file.cpp",        "file.cpp" );
    DOCHECK( "dir\\",       "dir\\folder\\",        "folder\\" );

    // Relative dirs
    DOCHECK( "C:\\dir\\",   "C:\\other\\file.cpp",  "..\\other\\file.cpp" );
    DOCHECK( "C:\\dir\\",   "C:\\other\\folder\\",  "..\\other\\folder\\" );
    DOCHECK( "dir\\A\\",    "dir\\B\\file.cpp",      "..\\B\\file.cpp" );
    DOCHECK( "dir\\A\\",    "dir\\B\\folder\\",      "..\\B\\folder\\" );

    // Case-insensitive
    DOCHECK( "C:\\dir\\",   "c:\\dir\\file.cpp",    "file.cpp" );
    DOCHECK( "C:\\dir\\",   "C:\\DIR\\folder\\",    "folder\\" );
    DOCHECK( "dir\\",       "DIR\\file.cpp",        "file.cpp" );
    DOCHECK( "dir\\",       "DIR\\folder\\",        "folder\\" );

    // Not relative
    DOCHECK( "C:\\dir\\",   "X:\\file.cpp",         "X:\\file.cpp" );
    DOCHECK( "C:\\dir\\",   "X:\\dir\\folder\\",    "X:\\dir\\folder\\" );
    DOCHECK( "dir\\",       "file.cpp",             "file.cpp" );
    DOCHECK( "dir\\",       "other\\folder\\",      "other\\folder\\" );

    // UNC Path
    DOCHECK( "\\server\\dir\\",   "\\server\\file.cpp",         "..\\file.cpp" );
    DOCHECK( "\\server\\dir\\",   "\\server\\dir\\folder\\",    "folder\\" );

    // Directory with substring matches
    DOCHECK( "C:\\folder",   "C:\\folderA",         "..\\folderA" );
#else
    // Simple case
    DOCHECK( "/dir/",       "/dir/file.cpp",        "file.cpp" );
    DOCHECK( "/dir/",       "/dir/folder/",         "folder/" );

    // Relative dirs
    DOCHECK( "/dir/",       "/other/file.cpp",      "../other/file.cpp" );
    DOCHECK( "/dir/",       "/other/folder/",       "../other/folder/" );

    #if defined( __OSX__ )
    // Case-insensitive
    DOCHECK( "/dir/",       "/dir/file.cpp",        "file.cpp" );
    DOCHECK( "/dir/",       "/DIR/folder/",         "folder/" );
    #endif

    // Not relative
    DOCHECK( "dir/",        "other/file.cpp",       "other/file.cpp" );
    DOCHECK( "dir/",        "other/folder/",        "other/folder/" );

    // Directory with substring matches
    DOCHECK( "/folder",     "/folderA",             "../folderA" );
#endif

#undef DOCHECK
}

//------------------------------------------------------------------------------
