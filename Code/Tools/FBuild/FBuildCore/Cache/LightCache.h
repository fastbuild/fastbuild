// LightCache
//------------------------------------------------------------------------------
#pragma once

// Includes
//------------------------------------------------------------------------------
// Core
#include <Core/Containers/Array.h>
#include <Core/Process/Mutex.h>
#include <Core/Strings/AString.h>

// Forward Declarations
//------------------------------------------------------------------------------
class AString;
class FileStream;
class IncludedFile;
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

    static void ClearCachedFiles();

protected:
    void                    Hash( IncludedFile * file, FileStream & f );
    const IncludedFile *    ProcessInclude( const AString & include, bool angleBracketForm );
    const IncludedFile *    ProcessIncludeFromFullPath( const AString & include, bool & outCyclic );
    const IncludedFile *    ProcessIncludeFromIncludeStack( const AString & include, bool & outCyclic );
    const IncludedFile *    ProcessIncludeFromIncludePath( const AString & include, bool & outCyclic );
    const IncludedFile *    FileExists( const AString & fileName );

    void SkipWhitespace( const char * & pos ) const;
    bool IsAtEndOfLine( const char * pos ) const;
    void SkipLineEnd( const char * & pos ) const;
    void SkipToEndOfLine( const char * & pos ) const;
    void SkipToEndOfQuotedString( const char * & pos ) const;

    Array< AString >                m_IncludePaths;             // Paths to search for includes (from -I etc)
    Array< const IncludedFile * >   m_AllIncludedFiles;         // List of files seen during parsing
    Array< const IncludedFile * >   m_IncludeStack;             // Stack of includes, for file relative checks
    bool                            m_ProblemParsing;           // Did we encounter some code we couldn't parse?
};

//------------------------------------------------------------------------------
