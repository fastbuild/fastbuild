// LightCache
//------------------------------------------------------------------------------
#pragma once

// Includes
//------------------------------------------------------------------------------
// Core
#include <Core/Containers/Array.h>
#include <Core/Env/MSVCStaticAnalysis.h>
#include <Core/Env/Types.h>
#include <Core/Process/Mutex.h>
#include <Core/Strings/AString.h>

// Forward Declarations
//------------------------------------------------------------------------------
class AString;
class FileStream;
class IncludedFile;
class IncludeDefine;
class ObjectNode;
enum class IncludeType : uint8_t;

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

    // Get text description of problem(s) if Hash() fails
    const AString & GetErrors() const { return m_Errors; }

    static void ClearCachedFiles();

protected:
    void                    Parse( IncludedFile * file, FileStream & f );
    bool                    ParseDirective( IncludedFile & file, const char * & pos );
    bool                    ParseDirective_Include( IncludedFile & file, const char * & pos );
    bool                    ParseDirective_Define( IncludedFile & file, const char * & pos );
    bool                    ParseDirective_Import( IncludedFile & file, const char * & pos );
    void                    SkipCommentBlock( const char * & pos );
    bool                    ParseIncludeString( const char * & pos, AString & outIncludePath, IncludeType & outIncludeType );
    bool                    ParseMacroName( const char * & pos, AString & outMacroName );
    void                    ProcessInclude( const AString & include, IncludeType type );
    const IncludedFile *    ProcessIncludeFromFullPath( const AString & include, bool & outCyclic );
    const IncludedFile *    ProcessIncludeFromIncludeStack( const AString & include, bool & outCyclic );
    const IncludedFile *    ProcessIncludeFromIncludePath( const AString & include, bool & outCyclic );
    const IncludedFile *    FileExists( const AString & fileName );

    void                    AddError( IncludedFile * file,
                                      const char * pos,
                                      MSVC_SAL_PRINTF const char * formatString,
                                      ... ) FORMAT_STRING( 4, 5 );

    static void SkipWhitespace( const char * & pos );
    static bool IsAtEndOfLine( const char * pos );
    static void SkipLineEnd( const char * & pos );
    static void SkipToEndOfLine( const char * & pos );
    static bool SkipToEndOfQuotedString( const char * & pos );

    static void ExtractLine( const char * pos, AString & outLine );

    Array< AString >                m_IncludePaths;             // Paths to search for includes (from -I etc)
    Array< const IncludedFile * >   m_AllIncludedFiles;         // List of files seen during parsing
    Array< const IncludedFile * >   m_IncludeStack;             // Stack of includes, for file relative checks
    Array< const IncludeDefine * >  m_IncludeDefines;           // Macros describing files to include
    AString                         m_Errors;                   // Did we encounter some code we couldn't parse?
};

//------------------------------------------------------------------------------
