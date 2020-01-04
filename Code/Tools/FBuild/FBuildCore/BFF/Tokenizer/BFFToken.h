// BFFToken.h
//------------------------------------------------------------------------------
#pragma once

// Includes
//------------------------------------------------------------------------------
// FBuildCore
#include "Tools/FBuild/FBuildCore/BFF/BFFFile.h"

// Core
#include "Core/Strings/AString.h"

// Forward Declarations
//------------------------------------------------------------------------------
class BFFFile;

// BFFTokenType
//------------------------------------------------------------------------------
enum class BFFTokenType : uint8_t
{
    Invalid,               // Input that can't be reconciled
    Identifier,            // Unspecified identifier
    Function,              // ForEach, Compiler etc
    Variable,              // .Var or ^Var
    Keyword,               // in etc
    Number,                // 12, 56, -102 etc
    Operator,              // = += > ! == etc
    OpeningRoundBracket,   // (
    ClosingRoundBracket,   // )
    OpeningCurlyBracket,   // {
    ClosingCurlyBracket,   // }
    OpeningSquareBracket,  // [
    ClosingSquareBracket,  // ]
    String,                // "Hello"
    Boolean,               // true or false
    Comma,                 // Comma
};

// BFFToken
//------------------------------------------------------------------------------
class BFFToken
{
public:
    // Constructors
    BFFToken( const BFFFile & file, const char * sourcePos, BFFTokenType type, const char * valueStart, const char * valueEnd );
    BFFToken( const BFFFile & file, const char * sourcePos, BFFTokenType type, const AString & stringValue );
    BFFToken( const BFFFile & file, const char * sourcePos, BFFTokenType type, const bool boolValue );

    // Convenience wrappers
    bool IsInvalid() const                          { return ( m_Type == BFFTokenType::Invalid ); }
    bool IsIdentifier() const                       { return ( m_Type == BFFTokenType::Identifier ); }
    bool IsFunction() const                         { return ( m_Type == BFFTokenType::Function ); }
    bool IsKeyword() const                          { return ( m_Type == BFFTokenType::Keyword ); }
    bool IsKeyword( const char * keyword ) const    { return ( m_Type == BFFTokenType::Keyword ) && ( m_String == keyword ); }
    bool IsNumber() const                           { return ( m_Type == BFFTokenType::Number ); }
    bool IsOperator() const                         { return ( m_Type == BFFTokenType::Operator ); }
    bool IsOperator( const char * op ) const        { return ( m_Type == BFFTokenType::Operator ) && ( m_String == op ); }
    bool IsOperator( const char op ) const          { return ( m_Type == BFFTokenType::Operator ) && ( m_String[ 0 ] == op ); }
    bool IsOpeningRoundBracket() const              { return ( m_Type == BFFTokenType::OpeningRoundBracket ); }
    bool IsClosingRoundBracket() const              { return ( m_Type == BFFTokenType::ClosingRoundBracket ); }
    bool IsOpeningCurlyBracket() const              { return ( m_Type == BFFTokenType::OpeningCurlyBracket ); }
    bool IsClosingCurlyBracket() const              { return ( m_Type == BFFTokenType::ClosingCurlyBracket ); }
    bool IsOpeningSquareBracket() const             { return ( m_Type == BFFTokenType::OpeningSquareBracket ); }
    bool IsClosingSquareBracket() const             { return ( m_Type == BFFTokenType::ClosingSquareBracket ); }
    bool IsString() const                           { return ( m_Type == BFFTokenType::String ); }
    bool IsBooelan() const                          { return ( m_Type == BFFTokenType::Boolean ); }
    bool IsVariable() const                         { return ( m_Type == BFFTokenType::Variable ); }
    bool IsComma() const                            { return ( m_Type == BFFTokenType::Comma ); }

    BFFTokenType    GetType() const                 { return m_Type; }
    const AString & GetValueString() const          { return m_String; }
    int32_t         GetValueInt() const             { return m_Integer; }
    bool            GetBoolean() const              { return m_Boolean; }

    const BFFFile & GetSourceFile() const           { return m_BFFFile; }
    const AString & GetSourceFileName() const       { return m_BFFFile.GetFileName(); }
    const AString & GetSourceFileContents() const   { return m_BFFFile.GetSourceFileContents(); }
    const char *    GetSourcePos() const            { return m_SourcePos; }

    void GetPosInfo( uint32_t & outLine, uint32_t & outColumn, const char * & outLineStart ) const;

private:
    BFFTokenType    m_Type;
    bool            m_Boolean   = false;
    int32_t         m_Integer   = 0;
    AString         m_String;
    const BFFFile & m_BFFFile;
    const char *    m_SourcePos = nullptr;
};

//------------------------------------------------------------------------------
