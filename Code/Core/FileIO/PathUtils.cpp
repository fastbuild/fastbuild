// PathUtils.cpp
//------------------------------------------------------------------------------

// Includes
//------------------------------------------------------------------------------
#include "Core/PrecompiledHeader.h"

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

// IsfullPath
//------------------------------------------------------------------------------
/*static*/ bool PathUtils::IsFullPath( const AString & path )
{
	#if defined( __WINDOWS__ )
		// full paths on Windows are in X: format
		if ( path.GetLength() >= 2 )
		{
			return ( path[ 1 ] == ':' );
		}
		return false;
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

// FixupFilerPath
//------------------------------------------------------------------------------
/*static*/ void PathUtils::FixupFilePath( AString & path )
{
	// Normalize slashes - TODO:C This could be optimized into one pass
	path.Replace( OTHER_SLASH, NATIVE_SLASH );
	while( path.Replace( NATIVE_DOUBLE_SLASH, NATIVE_SLASH_STR ) ) {}

	// Sanity check - calling this function on a folder path is an error
	ASSERT( path.EndsWith( NATIVE_SLASH ) == false );
}

//------------------------------------------------------------------------------
