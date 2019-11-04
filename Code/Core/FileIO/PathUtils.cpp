// PathUtils.cpp
//------------------------------------------------------------------------------

// Includes
//------------------------------------------------------------------------------
#include "PathUtils.h"
#include "Core/Strings/AStackString.h"

// Exists
//------------------------------------------------------------------------------
/*static*/ bool PathUtils::IsFolderPath( const AString & path )
{
    size_t pathLen = path.GetLength();
    if ( pathLen > 0 )
    {
        const char lastChar = path[ pathLen - 1 ];

        // Handle both slash types so we cope with non-cleaned paths
        if ( ( lastChar == NATIVE_SLASH ) || ( lastChar == OTHER_SLASH ) )
        {
            return true;
        }
    }
    return false;
}

// IsFullPath
//------------------------------------------------------------------------------
/*static*/ bool PathUtils::IsFullPath( const AString & path )
{
    #if defined( __WINDOWS__ )
        // full paths on Windows have a drive letter and colon, or are unc
        return ( ( path.GetLength() >= 2 && path[ 1 ] == ':' ) ||
                 path.BeginsWith( NATIVE_DOUBLE_SLASH ) );
    #elif defined( __LINUX__ ) || defined( __APPLE__ )
        // full paths on Linux/OSX/IOS begin with a slash
        return path.BeginsWith( NATIVE_SLASH );
    #endif
}

// ArePathsEqual
//------------------------------------------------------------------------------
/*static*/ bool PathUtils::ArePathsEqual(const AString & cleanPathA, const AString & cleanPathB)
{
    #if defined( __LINUX__ )
        // Case Sensitive
        return ( cleanPathA == cleanPathB );
    #endif

    #if defined( __WINDOWS__ ) || defined( __OSX__ )
        // Case Insensitive
        return ( cleanPathA.CompareI( cleanPathB ) == 0 );
    #endif
}

// IsWildcardMatch
//------------------------------------------------------------------------------
/*static*/ bool PathUtils::IsWildcardMatch( const char * pattern, const char * path )
{
    #if defined( __LINUX__ )
        // Linux : Case sensitive
        return AString::Match( pattern, path );
    #else
        // Windows & OSX : Case insensitive
        return AString::MatchI( pattern, path );
    #endif
}

// PathBeginsWith
//------------------------------------------------------------------------------
/*static*/ bool PathUtils::PathBeginsWith( const AString & cleanPath, const AString & cleanSubPath )
{
    #if defined( __LINUX__ )
        // Linux : Case sensitive
        return cleanPath.BeginsWith( cleanSubPath );
    #else
        // Windows & OSX : Case insensitive
        return cleanPath.BeginsWithI( cleanSubPath );
    #endif
}

// PathEndsWithFile
//------------------------------------------------------------------------------
/*static*/ bool PathUtils::PathEndsWithFile( const AString & cleanPath, const AString & fileName )
{
    // Work out if ends match
    #if defined( __LINUX__ )
        // Linux : Case sensitive
        bool endMatch = cleanPath.EndsWith( fileName );
    #else
        // Windows & OSX : Case insensitive
        bool endMatch = cleanPath.EndsWithI( fileName );
    #endif
    if ( !endMatch )
    {
        return false;
    }

    // if it's an entire match (a full path for example)
    if ( cleanPath.GetLength() == fileName.GetLength() )
    {
        return true;
    }

    // Sanity check - if fileName was longer then path (or equal) we can't get here
    ASSERT( cleanPath.GetLength() > fileName.GetLength() );
    const size_t potentialSlashIndex = ( cleanPath.GetLength() - fileName.GetLength() ) - 1; // -1 for char before match
    const char potentialSlash = cleanPath[ potentialSlashIndex ];
    if ( potentialSlash == NATIVE_SLASH )
    {
        return true; // full filename part matches (e.g. c:\thing\stuff.cpp | stuff.cpp )
    }
    return false; // fileName is only a partial match (e.g. c:\thing\otherstuff.cpp | stuff.cpp )
}

// EnsureTrailingSlash
//------------------------------------------------------------------------------
/*static*/ void PathUtils::EnsureTrailingSlash( AString & path )
{
    // check for exsiting slash
    size_t pathLen = path.GetLength();
    if ( pathLen > 0 )
    {
        const char lastChar = path[ pathLen - 1 ];
        if ( lastChar == NATIVE_SLASH )
        {
            return; // good slash - nothing to do
        }
        if ( lastChar == OTHER_SLASH )
        {
            // bad slash, do fixup
            path[ pathLen - 1 ] = NATIVE_SLASH;
            return;
        }
    }

    // add slash
    path += NATIVE_SLASH;
}

// FixupFolderPath
//------------------------------------------------------------------------------
/*static*/ void PathUtils::FixupFolderPath( AString & path )
{
    // Normalize slashes - TODO:C This could be optimized into one pass
    path.Replace( OTHER_SLASH, NATIVE_SLASH );
    #if defined( __WINDOWS__ )
        bool isUNCPath = path.BeginsWith( NATIVE_DOUBLE_SLASH );
    #endif
    while( path.Replace( NATIVE_DOUBLE_SLASH, NATIVE_SLASH_STR ) ) {}

    #if defined( __WINDOWS__ )
        if ( isUNCPath )
        {
            AStackString<> copy( path );
            path.Clear();
            path += NATIVE_SLASH; // Restore double slash by adding one back
            path += copy;
        }
    #endif

    // ensure slash termination
    if ( path.EndsWith( NATIVE_SLASH ) == false )
    {
        path += NATIVE_SLASH;
    }
}

// FixupFilePath
//------------------------------------------------------------------------------
/*static*/ void PathUtils::FixupFilePath( AString & path )
{
    // Normalize slashes - TODO:C This could be optimized into one pass
    path.Replace( OTHER_SLASH, NATIVE_SLASH );
    while( path.Replace( NATIVE_DOUBLE_SLASH, NATIVE_SLASH_STR ) ) {}

    // Sanity check - calling this function on a folder path is an error
    ASSERT( path.EndsWith( NATIVE_SLASH ) == false );
}

// StripFileExtension
//------------------------------------------------------------------------------
/*static*/ void PathUtils::StripFileExtension( AString & filePath )
{
    const char * lastDot = filePath.FindLast( '.' );
    if (lastDot)
    {
        filePath.SetLength( (uint32_t)( lastDot - filePath.Get() ) );
    }
}

// GetRelativePath
//------------------------------------------------------------------------------
/*static*/ void PathUtils::GetRelativePath( const AString & basePath,
                                            const AString & fileName,
                                            AString & outRelativeFileName,
                                            const bool rebaseFile )
{
    // Makes no sense to call with empty basePath
    ASSERT( basePath.IsEmpty() == false );

    // Can only determine relative paths if both are of the same scope
    ASSERT( IsFullPath( basePath ) == IsFullPath( fileName ) );

    // Handle base paths which are not slash terminated
    if ( basePath.EndsWith( NATIVE_SLASH ) == false )
    {
        AStackString<> basePathCopy( basePath );
        basePathCopy += NATIVE_SLASH;
        GetRelativePath( basePathCopy, fileName, outRelativeFileName, rebaseFile );
        return;
    }

    // Find common sub-path
    const char * pathA = basePath.Get();
    const char * pathB = fileName.Get();
    const char * itA = pathA;
    const char * itB = pathB;
    char compA = *itA;
    char compB = *itB;

    #if defined( __WINDOWS__ ) || defined( __OSX__ )
        // Windows & OSX: Case insensitive
        if ( ( compA >= 'A' ) && ( compA <= 'Z' ) )
        {
            compA = 'a' + ( compA - 'A' );
        }
        if ( ( compB >= 'A' ) && ( compB <= 'Z' ) )
        {
            compB = 'a' + ( compB - 'A' );
        }
    #endif

    while ( ( compA == compB ) && ( compA != '\0' ) )
    {
        const bool dirToken = ( ( *itA == '/' ) || ( *itA == '\\' ) );
        itA++;
        compA = *itA;
        itB++;
        compB = *itB;
        if ( dirToken )
        {
            pathA = itA;
            pathB = itB;
        }

        #if defined( __WINDOWS__ ) || defined( __OSX__ )
            // Windows & OSX: Case insensitive
            if ( ( compA >= 'A' ) && ( compA <= 'Z' ) )
            {
                compA = 'a' + ( compA - 'A' );
            }
            if ( ( compB >= 'A' ) && ( compB <= 'Z' ) )
            {
                compB = 'a' + ( compB - 'A' );
            }
        #endif
    }

    const bool hasCommonSubPath = ( pathA != basePath.Get() );
    if ( hasCommonSubPath )
    {
        // Build relative path

        // For every remaining dir in the project path, go up one directory
        outRelativeFileName.Clear();
        for ( ;; )
        {
            const char c = *pathA;
            if ( c == 0 )
            {
                break;
            }
            if ( ( c == '/' ) || ( c == '\\' ) )
            {
                outRelativeFileName += "..";
                outRelativeFileName += NATIVE_SLASH;
            }
            ++pathA;
        }

        // Add remainder of source path relative to the common sub path
        outRelativeFileName += pathB;
    }
    else
    {
        // No common sub-path
        if ( rebaseFile )
        {
            // file is in some completely other directory, so put in root of basePath
            const char * lastSlash = fileName.FindLast( NATIVE_SLASH );
            outRelativeFileName = ( lastSlash ? lastSlash + 1 : fileName.Get() );
        }
        else
        {
            // not rebasing file, so use fileName as-is
            outRelativeFileName = fileName;
        }
    }
}

// CleanPath
//------------------------------------------------------------------------------
/*static*/ void PathUtils::CleanPath( const AString & workingDir, const AString & name, AString & cleanPath, const bool makeFullPath )
{
    ASSERT( &name != &cleanPath );

    char * dst;

    //  - path can be fully qualified
    bool isFullPath = PathUtils::IsFullPath( name );
    if ( !isFullPath && makeFullPath )
    {
        // make a full path by prepending working dir
        // we're making the assumption that we don't need to clean the workingDir
        ASSERT( workingDir.Find( OTHER_SLASH ) == nullptr ); // bad slashes removed
        ASSERT( workingDir.Find( NATIVE_DOUBLE_SLASH ) == nullptr ); // redundant slashes removed

        // build the start of the path
        cleanPath = workingDir;
        cleanPath += NATIVE_SLASH;

        // concatenate
        uint32_t len = cleanPath.GetLength();

        // make sure the dest will be big enough for the extra stuff
        cleanPath.SetLength( cleanPath.GetLength() + name.GetLength() );

        // set the output (which maybe a newly allocated ptr)
        dst = cleanPath.Get() + len;

        isFullPath = true;
    }
    else
    {
        // make sure the dest will be big enough
        cleanPath.SetLength( name.GetLength() );

        // copy from the start
        dst = cleanPath.Get();
    }

    // the untrusted part of the path we need to copy/fix
    const char * src = name.Get();
    const char * const srcEnd = name.GetEnd();

    // clean slashes
    char lastChar = NATIVE_SLASH; // consider first item to follow a path (so "..\file.dat" works)
    #if defined( __WINDOWS__ )
        while ( *src == NATIVE_SLASH || *src == OTHER_SLASH ) { ++src; } // strip leading slashes
    #endif

    const char * lowestRemovableChar = cleanPath.Get();
    if ( isFullPath )
    {
        #if defined( __WINDOWS__ )
            lowestRemovableChar += 3; // e.g. "c:\"
        #else
            lowestRemovableChar += 1; // e.g. "/"
        #endif
    }

    while ( src < srcEnd )
    {
        const char thisChar = *src;

        // hit a slash?
        if ( ( thisChar == NATIVE_SLASH ) || ( thisChar == OTHER_SLASH ) )
        {
            // write it the correct way
            *dst = NATIVE_SLASH;
            dst++;

            // skip until non-slashes
            while ( ( *src == NATIVE_SLASH ) || ( *src == OTHER_SLASH ) )
            {
                src++;
            }
            lastChar = NATIVE_SLASH;
            continue;
        }
        else if ( thisChar == '.' )
        {
            if ( lastChar == NATIVE_SLASH ) // fixed up slash, so we only need to check backslash
            {
                // check for \.\ (or \./)
                char nextChar = *( src + 1 );
                if ( ( nextChar == NATIVE_SLASH ) || ( nextChar == OTHER_SLASH ) )
                {
                    src++; // skip . and slashes
                    while ( ( *src == NATIVE_SLASH ) || ( *src == OTHER_SLASH ) )
                    {
                        ++src;
                    }
                    continue; // leave lastChar as-is, since we added nothing
                }

                // check for \..\ (or \../)
                if ( nextChar == '.' )
                {
                    nextChar = *( src + 2 );
                    if ( ( nextChar == NATIVE_SLASH ) || ( nextChar == OTHER_SLASH ) || ( nextChar == '\0' ) )
                    {
                        src+=2; // skip .. and slashes
                        while ( ( *src == NATIVE_SLASH ) || ( *src == OTHER_SLASH ) )
                        {
                            ++src;
                        }

                        if ( dst > lowestRemovableChar )
                        {
                            --dst; // remove slash

                            while ( dst > lowestRemovableChar ) // e.g. "c:\"
                            {
                                --dst;
                                if ( *dst == NATIVE_SLASH ) // only need to check for cleaned slashes
                                {
                                    ++dst; // keep this slash
                                    break;
                                }
                            }
                        }
                        else if( !isFullPath )
                        {
                            *dst++ = '.';
                            *dst++ = '.';
                            *dst++ = NATIVE_SLASH;
                            lowestRemovableChar = dst;
                        }

                        continue;
                    }
                }
            }
        }

        // write non-slash character
        *dst++ = *src++;
        lastChar = thisChar;
    }

    // correct length of destination
    cleanPath.SetLength( (uint16_t)( dst - cleanPath.Get() ) );
    ASSERT( AString::StrLen( cleanPath.Get() ) == cleanPath.GetLength() );

    // sanity checks
    ASSERT( cleanPath.Find( OTHER_SLASH ) == nullptr ); // bad slashes removed
    ASSERT( cleanPath.Find( NATIVE_DOUBLE_SLASH ) == nullptr ); // redundant slashes removed
}

// GetPathGivenWorkingDir
//------------------------------------------------------------------------------
/*static*/ void PathUtils::GetPathGivenWorkingDir(
    const AString & workingDir, const AString & path,
    AString & resultPath )
{
    if ( !workingDir.IsEmpty() )
    {
        GetRelativePath( workingDir, path, resultPath, false );  // don't rebase file
    }
    else
    {
        // use original path
        resultPath = path;
    }
}

//------------------------------------------------------------------------------
