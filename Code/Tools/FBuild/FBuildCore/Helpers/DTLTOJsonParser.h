// DTLTOJsonParser - Parse LLVM DTLTO distribution JSON
//------------------------------------------------------------------------------
#pragma once

// Includes
//------------------------------------------------------------------------------
#include "Tools/FBuild/FBuildCore/Helpers/DTLTOData.h"

#include "Core/Strings/AString.h"

// DTLTOJsonParser
//------------------------------------------------------------------------------
class DTLTOJsonParser
{
public:
    explicit DTLTOJsonParser( const AString & buffer );

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
    template <typename ELEMENT_PARSER>
    bool MatchArray( ELEMENT_PARSER parseElement );
    bool MatchStringArray( Array<AString> & out );
    bool MatchStringArrayProp( const char * propertyName, Array<AString> & out );

    // DTLTO specific matchers
    bool MatchCommonProp();
    bool MatchLinkerOutputProp();
    bool MatchArgsProp();
    bool MatchInputsProp();

    const char * m_Pos = nullptr; // Cursor into the caller-owned buffer
    const char * m_End = nullptr;
    // DTLTOData::Job * m_CurrentJob = nullptr; // Job being parsed; points into m_Data.m_Jobs
    DTLTOData m_Data;
};

//------------------------------------------------------------------------------
