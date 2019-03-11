// LightCache
//------------------------------------------------------------------------------
#pragma once

// Includes
//------------------------------------------------------------------------------
// Core
#include <Core/Containers/Array.h>
#include <Core/Strings/AString.h>

// Forward Declarations
//------------------------------------------------------------------------------
class AString;
class FileStream;
class ObjectNode;

// LightCache
//------------------------------------------------------------------------------
class LightCache
{
public:
    LightCache();
    ~LightCache();

    bool Hash( ObjectNode * node,                // Object to be compiled
               const AString & compilerArgs,     // Args to extract include paths from
               uint64_t & outSourceHash,         // Resulting hash of source code
               Array< AString > & outIncludes ); // Discovered dependencies

protected:
    bool Hash( const AString & fileName, FileStream & f );
    bool ProcessFile( const AString & fileContents );
    void ProcessInclude( const AString & include, bool angleBracketForm );
    bool ProcessIncludeFromIncludeStack( const AString & include );
    bool ProcessIncludeFromIncludePath( const AString & include );
    bool FileExists( const AString & fileName, FileStream & outFileStream );

    void SkipWhitespace( const char * & pos ) const;
    bool IsAtEndOfLine( const char * pos ) const;
    void SkipLineEnd( const char * & pos ) const;
    void SkipToEndOfLine( const char * & pos ) const;
    void SkipToEndOfQuotedString( const char * & pos ) const;

    Array< AString > m_IncludePaths;            // Paths to search for includes (from -I etc)
    Array< AString > m_IncludeStack;            // Stack of includes, for file relative checks
    Array< AString > m_UniqueIncludes;          // Final list of uniquely seen includes

    // Cache header search to speed up include searches
    struct FileExistsCacheEntry
    {
        inline bool operator == ( uint32_t pathHash ) const { return m_PathHash == pathHash; }

        uint32_t    m_PathHash;                 // Hash of path to file
        bool        m_Exists;                   // Whether the files exists
    };
    Array< FileExistsCacheEntry > m_FileExistsCache;

    // Hash of all source files to feed into cache key
    uint64_t m_XXHashState[ 11 ]; // Avoid including xxHash header
};

//------------------------------------------------------------------------------
