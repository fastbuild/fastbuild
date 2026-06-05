// DTLTOJsonParser - Parse LLVM DTLTO distribution JSON
//------------------------------------------------------------------------------
#pragma once

// Includes
//------------------------------------------------------------------------------
#include "Core/Containers/Array.h"
#include "Core/Strings/AString.h"

// DTLTOJsonParser
//------------------------------------------------------------------------------
class DTLTOJsonParser
{
public:
    // struct Job
    // {
    //     Array<AString> m_Args;
    //     Array<AString> m_Inputs;
    //     Array<AString> m_Outputs;
    // };

    struct DTLTOData
    {
        AString m_LinkerOutput;
        // Array<Job> m_Jobs;
    };

    explicit DTLTOJsonParser( const char * fileName );

    bool Load();
    bool Parse();
    const DTLTOData & GetData() const { return m_Data; }

private:
    typedef bool ( DTLTOJsonParser::*MatcherFunc )();

    struct PropertyMatcher
    {
        const char * m_PropertyName;
        MatcherFunc m_Matcher;
    };

    static const char EndOfInput = '\0'; // Sentinel past end of input

    bool AtEnd() const { return m_Pos >= m_End; }
    char Peek() const { return AtEnd() ? EndOfInput : *m_Pos; }
    char Consume() { return AtEnd() ? EndOfInput : *m_Pos++; }

    void SkipWhitespaces();
    bool MatchChar( char c );
    bool MatchString( AString & outString );
    template <size_t NUM_MATCHERS>
    bool MatchObject( const PropertyMatcher ( &matchers )[ NUM_MATCHERS ] );

    // DTLTO specific matchers
    bool MatchCommonProp();
    bool MatchLinkerOutputProp();

    AString m_FileName;
    AString m_Buffer;   // Raw file contents; m_Pos/m_End index into this
    const char * m_Pos = nullptr;
    const char * m_End = nullptr;
    // Job m_CurrentJob;
    DTLTOData m_Data;
};

//------------------------------------------------------------------------------
