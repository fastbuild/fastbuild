// PathUtils.h
//------------------------------------------------------------------------------
#pragma once

// Forward Declarations
//------------------------------------------------------------------------------
class AString;

// Defines
//------------------------------------------------------------------------------
#if defined( __WINDOWS__ )
    #define NATIVE_SLASH        ( '\\' )
    #define NATIVE_SLASH_STR    ( "\\" )
    #define NATIVE_DOUBLE_SLASH ( "\\\\" )
    #define OTHER_SLASH         ( '/' )
#elif defined( __LINUX__ ) || defined( __APPLE__ )
    #define NATIVE_SLASH        ( '/' )
    #define NATIVE_SLASH_STR    ( "/" )
    #define NATIVE_DOUBLE_SLASH ( "//" )
    #define OTHER_SLASH         ( '\\' )
#endif

// For places that explicitly need slashes a certain way
// use these defines to signify the intent
#define BACK_SLASH    ( '\\' )
#define FORWARD_SLASH ( '/' )

// FileIO
//------------------------------------------------------------------------------
class PathUtils
{
public:
    // Query Helpers
    //--------------
    static bool IsFolderPath( const AString & path );
    static bool IsFullPath( const AString & path );
    static bool ArePathsEqual( const AString & cleanPathA, const AString & cleanPathB );
    static bool IsWildcardMatch( const char * pattern, const char * path );
    static bool PathBeginsWith( const AString & cleanPath, const AString & cleanSubPath );
    static bool PathEndsWithFile( const AString & cleanPath, const AString & fileName );

    // Cleanup Helpers
    //----------------
    static void EnsureTrailingSlash( AString & path );
    static void FixupFolderPath( AString & path );
    static void FixupFilePath( AString & path );

    // Misc
    //----------------
    static void StripFileExtension( AString & filePath );
    static void GetRelativePath( const AString & basePath,
                                 const AString & fileName,
                                 AString & outRelativeFileName );
};

//------------------------------------------------------------------------------
