// BFFTokenizer.h
//------------------------------------------------------------------------------
#pragma once

// Includes
//------------------------------------------------------------------------------
// FBuildCore
#include "Tools/FBuild/FBuildCore/BFF/BFFMacros.h"
#include "Tools/FBuild/FBuildCore/BFF/Tokenizer/BFFToken.h"

// Core
#include "Core/Containers/Array.h"

// Forward Declarations
//------------------------------------------------------------------------------
class AString;
class BFFTokenRange;

// BFFTokenizer
//------------------------------------------------------------------------------
class BFFTokenizer
{
public:
    // Constructors
    BFFTokenizer();
    ~BFFTokenizer();

    // Process bff file hierarchy from a root file
    bool TokenizeFromFile( const AString & fileName );

    // Preocess from an buffer in memory (for tests)
    bool TokenizeFromString( const AString & fileName, const AString & fileContents );

    // Access results
    const Array<BFFToken> &     GetTokens() const { return m_Tokens; }
    const Array<BFFFile *> &    GetUsedFiles() const { return m_Files; }

protected:
    bool Tokenize( const AString & fileName, const BFFToken * token );
    bool Tokenize( const BFFFile * file );
    bool Tokenize( const BFFFile & file, const char * pos, const char * end );

    bool GetQuotedString( const BFFFile & file, const char * & pos, AString & outString ) const;
    bool GetDirective( const BFFFile & file, const char * & pos, AString & outDirectiveName ) const;

    bool HandleIdentifier( const char * & pos, const char * end, const BFFFile & file );
    bool HandleVariable( const char * & pos, const char * end, const BFFFile & file );
    bool HandleDirective( const char * & pos, const char * end, const BFFFile & file );

    bool HandleDirective_Define( const BFFFile & file, const char * & pos, const char * end, BFFTokenRange & argsIter );
    bool HandleDirective_Else( const BFFFile & file, const char * & pos, const char * end, BFFTokenRange & argsIter );
    bool HandleDirective_If( const BFFFile & file, const char * & pos, const char * end, BFFTokenRange & argsIter );
    bool HandleDirective_IfExists( BFFTokenRange & iter, bool & outResult );
    bool HandleDirective_IfFileExists(  const BFFFile & file, BFFTokenRange & iter, bool & outResult );
    bool HandleDirective_IfDefined( BFFTokenRange & iter, bool & outResult );
    bool ParseToEndIf( const char * & pos, const char * end, const BFFFile & file, bool allowElse, const char * & outBlockEnd, bool * outIsElse );
    bool HandleDirective_Import( const BFFFile & file, const char * & pos, const char * end, BFFTokenRange & argsIter );
    bool HandleDirective_Include( const BFFFile & file, const char * & pos, const char * end, BFFTokenRange & argsIter );
    bool HandleDirective_Once( const BFFFile & file, const char * & pos, const char * end, BFFTokenRange & argsIter );
    bool HandleDirective_Undef( const BFFFile & file, const char * & pos, const char * end, BFFTokenRange & argsIter );

    void ExpandIncludePath( const BFFFile & file, AString & includePath ) const;

    struct IncludedFile
    {
        AString     m_FileName;
        bool        m_Once;
    };

    Array<BFFToken>     m_Tokens;
    Array<BFFFile *>    m_Files;
    BFFMacros           m_Macros;
    uint32_t            m_Depth = 0;
    bool                m_ParsingDirective = false;
};

//------------------------------------------------------------------------------
