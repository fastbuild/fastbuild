// LightCache
//------------------------------------------------------------------------------

// Includes
//------------------------------------------------------------------------------
#include "LightCache.h"

// FBuildCore
#include "Tools/FBuild/FBuildCore/Graph/NodeGraph.h"
#include "Tools/FBuild/FBuildCore/Graph/ObjectNode.h"
#include "Tools/FBuild/FBuildCore/Helpers/ProjectGeneratorBase.h"

// Core
#if defined( __WINDOWS__ )
    #include "Core/Env/WindowsHeader.h"
#endif
#include "Core/FileIO/FileIO.h"
#include "Core/FileIO/FileStream.h"
#include "Core/FileIO/PathUtils.h"
#include "Core/Math/xxHash.h"
#include "Core/Profile/Profile.h"
#include "Core/Strings/AStackString.h"

// External
#define XXH_STATIC_LINKING_ONLY
#include "xxhash.h"
#undef XXH_STATIC_LINKING_ONLY

// CONSTRUCTOR
//------------------------------------------------------------------------------
LightCache::LightCache()
    : m_IncludePaths( 32, true )
    , m_IncludeStack( 32, true )
    , m_UniqueIncludes( 1024, true )
    , m_FileExistsCache( 1024, true )
{
    static_assert( sizeof( m_XXHashState ) == sizeof( XXH64_state_t ), "Mismatched size" );
}

// DESTRUCTOR
//------------------------------------------------------------------------------
LightCache::~LightCache() = default;

// Hash
//------------------------------------------------------------------------------
bool LightCache::Hash( ObjectNode * node,
                       const AString & compilerArgs,
                       uint64_t & outSourceHash,
                       Array< AString > & outIncludes )
{
    PROFILE_FUNCTION

    ProjectGeneratorBase::ExtractIntellisenseOptions( compilerArgs,
                                                      "-I",
                                                      "/I",
                                                      m_IncludePaths,
                                                      false,    //escapeQuotes,
                                                      false );  //keepFullOption

	// Ensure all includes are slash terminated
    for ( AString & includePath : m_IncludePaths )
    {
	    if ( includePath.EndsWith( NATIVE_SLASH ) || includePath.EndsWith( OTHER_SLASH ) )
        {
            continue;
        }
    	includePath += NATIVE_SLASH;
    }

    const AString & rootFileName = node->GetSourceFile()->GetName();
    FileStream rootFile;
    if ( rootFile.Open( rootFileName.Get(), FileStream::READ_ONLY ) == false )
    {
        return false;
    }

    // Prepare the hash stream
    XXH64_reset( (XXH64_state_t *)m_XXHashState, 0 ); // seed = 0

    bool result = Hash( rootFileName, rootFile );

    // Finalize the hash
    outSourceHash = XXH64_digest( (const XXH64_state_t *)m_XXHashState );

    // Give includes to caller
    outIncludes.Swap( m_UniqueIncludes );

    return result;
}

// Hash
//------------------------------------------------------------------------------
bool LightCache::Hash( const AString& fileName, FileStream & f )
{
	// Get the canonical path to the file and early out if we already saw it
    // TODO:C We could avoid this work if we canonicalized the paths earlier
	AStackString<> fullPath;
    NodeGraph::CleanPath( fileName, fullPath );

	// Have we seen this path before?
    // - We can end up in the same final file via multiple different include mechanisms
    //   a) -I path + "file.h"
    //   b) "../file.h"
	if ( m_UniqueIncludes.Find( fullPath ) )
	{
        return true;
    }
	m_UniqueIncludes.Append( fullPath );

	// Read all contents
	const size_t fileSize = f.GetFileSize();
    AString fileContents;
    fileContents.SetLength( (uint32_t)fileSize );
    if ( f.Read( fileContents.Get(), fileSize ) != fileSize )
    {
        return false;
    }
	f.Close();

	// process it
	m_IncludeStack.Append( fullPath );
	bool result = ProcessFile( fileContents );
	m_IncludeStack.Pop();

	return result;
}

// ProcessFile
//------------------------------------------------------------------------------
bool LightCache::ProcessFile( const AString & fileContents )
{
    // Update the hash stream for the file contents
    XXH64_update( (XXH64_state_t *)m_XXHashState, fileContents.Get(), fileContents.GetLength() );

	const char * pos = fileContents.Get();
	for (;;)
	{
		// skip leading whitespace
		SkipWhitespace( pos );
		if ( *pos == 0 )
		{
			break;
		}

		// did we hit the end of a line?
		if ( IsAtEndOfLine( pos ) )
		{
			// line is entirely whitepace
			SkipLineEnd( pos );
			continue;
		}

		// line contains something useful
		// is it an include directive?
		const char c = *pos;
		if ( c == '#' )
		{
			pos++;
			SkipWhitespace( pos );
			if ( AString::StrNCmp( pos, "include", 7 ) == 0 )
			{
				pos += 7;
				SkipWhitespace( pos );
				if ( ( *pos != '"' ) && ( *pos != '<' ) )
				{
					// TODO:A We encountered an include we can't handle (using a macro for the path for example)
					return false;
				}
				const bool angleBracketForm = ( *pos == '<' );
				++pos;
				const char * includeStart = pos;
				SkipToEndOfQuotedString( pos );
				const char * includeEnd = pos;
				AStackString<> include( includeStart, includeEnd );

				// Recurse
                ProcessInclude( include, angleBracketForm );

				SkipToEndOfLine( pos );
				SkipLineEnd( pos );
				continue;
			}
		}

		// block comments
		if ( ( c == '/' ) && ( pos[ 1 ] == '*' ) )
		{
			pos += 2; // skip /*
			for (;;)
			{
				const char thisChar = *pos;
				// end of data?
				if ( thisChar == 0 )
				{
					break;
				}
				// enf of comment block?
				if ( ( thisChar == '*' ) && ( pos[ 1 ] == '/' ) )
				{
					pos +=2;
					break;
				}

				// line ending - windows
				if ( c == '\r' )
				{
					++pos;
					if ( *pos == '\n' )
					{
						++pos;
					}
					continue;
				}

				// line ending - unix
				if ( c == '\n' )
				{
					++pos;
					continue;
				}

				// part of comment
				++pos;
			}
		}

		// a non-include line
		SkipToEndOfLine( pos );
		SkipLineEnd( pos );
	}

    return true;
}

// ProcessInclude
//------------------------------------------------------------------------------
void LightCache::ProcessInclude( const AString & include, bool angleBracketForm )
{
	// From MSDN: http://msdn.microsoft.com/en-us/library/36k2cdd4.aspx

	// #include "file.h"
	if ( !angleBracketForm )
	{
		// 1. In the same directory as the file that contains the #include statement.
		// 2. In the directories of the currently opened include files, in the reverse order in which they were opened. The search begins in the directory of the parent include file and continues upward through the directories of any grandparent include files.
		if ( ProcessIncludeFromIncludeStack( include ) )
		{
			return;
		}

		// 3. Along the path that's specified by each /I compiler option.
		if ( ProcessIncludeFromIncludePath( include ) )
		{
			return;
		}

		// 4. Along the paths that are specified by the INCLUDE environment variable.
		// TODO: Implement
		//ASSERT( false );
	}

	// #include <file.h>
	if ( angleBracketForm )
	{
		// 1. Along the path that's specified by each /I compiler option.
		if ( ProcessIncludeFromIncludePath( include ) )
		{
			return;
		}

		// 2. When compiling occurs on the command line, along the paths that are specified by the INCLUDE environment variable.
		//ASSERT( false ); // TODO: Implement
	}
	
	// Include not found. This is ok because:
    // a) The file might not be needed. If the include is within an inactive part of the file
    //    such as a comment or ifdef'd for example. If compilation succeeds, the file should
    //    not be part of our dependencies anyway, so this is ok.
    // b) The files is genuinely missing, in which case compilation will fail. If compilation
    //    fails then we won't bake the dependencies with the missing file, so this is ok.
}

// ProcessIncludeFromIncludeStack
//------------------------------------------------------------------------------
bool LightCache::ProcessIncludeFromIncludeStack( const AString & include )
{
    FileStream f;

	const int stackSize = (int)m_IncludeStack.GetSize();
	for ( int i=( stackSize - 1 ); i >=0; --i )
	{
		AStackString<> possibleIncludePath( m_IncludeStack[ i ] );
		const char * lastFwdSlash = possibleIncludePath.FindLast( '/' );
		const char * lastBackSlash = possibleIncludePath.FindLast( '\\' );
		const char * lastSlash = ( lastFwdSlash > lastBackSlash ) ? lastFwdSlash : lastBackSlash;
		ASSERT( lastSlash ); // it's a full path, so it must have a slash

		// truncate to slash (keep slash)
		possibleIncludePath.SetLength( (uint32_t)( lastSlash - possibleIncludePath.Get() ) + 1 );
		possibleIncludePath += include;

		if ( FileExists( possibleIncludePath, f ) )
		{
            if ( f.IsOpen() )
            {
    			return Hash( possibleIncludePath, f );
            }
            return true;
		}	
	}

	return false; // not found
}

// ProcessIncludeFromIncludePath
//------------------------------------------------------------------------------
bool LightCache::ProcessIncludeFromIncludePath( const AString & include )
{
    FileStream f;

    AStackString<> possibleIncludePath;
	for ( const AString & includePath : m_IncludePaths )
	{
		possibleIncludePath = includePath;
		possibleIncludePath += include;

        // Check if the files exists/open it if needed
		if ( FileExists( possibleIncludePath, f ) )
		{
            if ( f.IsOpen() )
            {
                // File is being seen for the first time, so we must parse it
    			return Hash( possibleIncludePath, f );
            }

            // File exists, but we've seen it before so there is nothing further to do
            return true;
		}	
	}

    // File does not exist
	return false;
}

// FileExists 
//------------------------------------------------------------------------------
bool LightCache::FileExists( const AString & fileName, FileStream & outFileStream )
{
    const uint32_t hash = xxHash::Calc32( fileName );
    const FileExistsCacheEntry * entry = m_FileExistsCache.Find( hash );
    if ( entry )
    {
        return entry->m_Exists;
    }

    const bool exists = ( outFileStream.Open( fileName.Get(), FileStream::READ_ONLY ) );

    m_FileExistsCache.Append( FileExistsCacheEntry{ hash, exists } );
    return exists;
}

// SkipWhitepspace
//------------------------------------------------------------------------------
void LightCache::SkipWhitespace( const char * & pos ) const
{
	for (;;)
	{
		const char c = *pos;
		if ( ( c == ' ' ) || ( c == '\t' ) )
		{
			pos++;
			continue;
		}
		break;
	}
}

// IsAtEndOfLine
//------------------------------------------------------------------------------
bool LightCache::IsAtEndOfLine( const char * pos ) const
{
	const char c = *pos;
	return ( ( c == '\r' ) || ( c== '\n' ) );
}

// SkipLineEnd
//------------------------------------------------------------------------------
void LightCache::SkipLineEnd( const char * & pos ) const
{
	while ( IsAtEndOfLine( pos ) )
	{
		++pos;
		continue;
	}
}

// SkipToEndOfLine
//------------------------------------------------------------------------------
void LightCache::SkipToEndOfLine( const char * & pos ) const
{
	for ( ;; )
	{
		const char c = *pos;
		if ( ( c != '\r' ) && ( c != '\n' ) && ( c != '\000' ) )
		{
			++pos;
			continue;
		}
		break;
	}
}

// SkipToEndOfQuotedString
//------------------------------------------------------------------------------
void LightCache::SkipToEndOfQuotedString( const char * & pos ) const
{
	for ( ;; )
	{
		const char c = *pos;
		if ( ( c != '"' ) && ( c != '>' ) )
		{
			++pos;
			continue;
		}
		break;
	}
}

//------------------------------------------------------------------------------
