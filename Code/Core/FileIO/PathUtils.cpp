// PathUtils.cpp
//------------------------------------------------------------------------------

// Includes
//------------------------------------------------------------------------------
#include "Core/PrecompiledHeader.h"

#include "PathUtils.h"
#include "FileIO.h"
#include "Core/Math/Random.h"
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
    #if defined( __LINUX__ ) || defined( __IOS__ )
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
                    if ( ( nextChar == NATIVE_SLASH ) || ( nextChar == OTHER_SLASH ) )
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

// GetRelativePath
//------------------------------------------------------------------------------
/*static*/ void PathUtils::GetRelativePath(
    const AString & root, const AString & otherFile,
    AString & otherFileRelativePath,
    const bool rebaseFile )
{
    bool useSubDir = false;
    if ( !root.IsEmpty() )
    {
        AStackString<> mutableRoot( root );
        EnsureTrailingSlash( mutableRoot );
        useSubDir = otherFile.BeginsWithI( mutableRoot );
        if ( useSubDir )
        {
            // file is in sub dir, put in otherFileRelativePath but use sub dir relative location
            otherFileRelativePath = otherFile.Get() + mutableRoot.GetLength();
        }
    }
    if ( !useSubDir )
    {
        if ( rebaseFile )
        {
            // file is in some completely other directory, so put in root of otherFileRelativePath
            const char * lastSlash = otherFile.FindLast( NATIVE_SLASH );
            otherFileRelativePath = ( lastSlash ? lastSlash + 1 : otherFile.Get() );
        }
        else
        {
            // not rebasing file, so return original path
            otherFileRelativePath = otherFile;
        }
    }
    if ( !IsFullPath( otherFileRelativePath ) &&     // if relative path and
         !otherFileRelativePath.BeginsWith( "." ) )  // does not start with a dot
    {
        // prepend dot and slash, to make relative
        AStackString<> newRelPath( "." );
        newRelPath += NATIVE_SLASH;
        newRelPath += otherFileRelativePath;
        otherFileRelativePath = newRelPath;
    }
}

// GetObfuscatedSandboxTmp
//------------------------------------------------------------------------------
/*static*/ void PathUtils::GetObfuscatedSandboxTmp(
    const bool sandboxEnabled,
    const AString & workingDir,
    const AString & sandboxTmp,
    AString & obfuscatedSandboxTmp )
{
    if ( sandboxEnabled )
    {
        // get absolute path
        AStackString<> absSandboxTmp;
        const AString * sandboxTmpToUse = &sandboxTmp;
        if ( !PathUtils::IsFullPath( sandboxTmp ) )
        {
            CleanPath( workingDir, sandboxTmp, absSandboxTmp );
            sandboxTmpToUse = &absSandboxTmp;
        }
        AStackString<> uniqueString;
        Random::GetCryptRandomString( uniqueString );
        AStackString<> wildcard;
        // find folder name with same size
        for ( uint32_t i = 0; i < uniqueString.GetLength(); ++i )
        {
            wildcard += "?";
        }
        Array< AString > results( 1, true );
        bool dirFound = false;
        if ( FileIO::GetFiles( *sandboxTmpToUse,
                                wildcard,
                                false,  // recurse
                                true,   // includeDirs
                                &results ) )
        {
            // reuse existing dir
            if ( results.Begin() != results.End() )
            {
                // choose first matching file
                obfuscatedSandboxTmp = *results.Begin();
                dirFound = !obfuscatedSandboxTmp.IsEmpty();
            }
        }
        if ( !dirFound )
        {
            // create new dir
            obfuscatedSandboxTmp.Assign( *sandboxTmpToUse );
            PathUtils::EnsureTrailingSlash( obfuscatedSandboxTmp );
            obfuscatedSandboxTmp.Append( uniqueString );
        }
    }
}

//------------------------------------------------------------------------------
