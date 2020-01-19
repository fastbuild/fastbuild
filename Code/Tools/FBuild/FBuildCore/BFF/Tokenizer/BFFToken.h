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
    Invalid,        // Input that can't be reconciled
    Identifier,     // Unspecified identifier
    Function,       // ForEach, Compiler etc
    Variable,       // .Var or ^Var
    Keyword,        // in etc
    Number,         // 12, 56, -102 etc
    Operator,       // = += > ! == etc
    RoundBracket,   // ( or )
    CurlyBracket,   // { or }
    SquareBracket,  // [ or ]
    String,         // "Hello"
    Boolean,        // true or false
    Comma,          // Comma
    EndOfFile
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

    // Allow move construction
    BFFToken( BFFToken && other );

    // Prevent copies (to be sure we're moving)
    BFFToken( BFFToken & ) = delete;
    BFFToken & operator = ( BFFToken & ) = delete;

    // Convenience wrappers
    bool IsInvalid() const                          { return ( m_Type == BFFTokenType::Invalid ); }
    bool IsIdentifier() const                       { return ( m_Type == BFFTokenType::Identifier ); }
    bool IsFunction() const                         { return ( m_Type == BFFTokenType::Function ); }
    bool IsKeyword() const                          { return ( m_Type == BFFTokenType::Keyword ); }
    bool IsKeyword( const char * keyword ) const    { return ( m_Type == BFFTokenType::Keyword ) && ( m_String == keyword ); }
    bool IsNumber() const                           { return ( m_Type == BFFTokenType::Number ); }
    bool IsOperator() const                         { return ( m_Type == BFFTokenType::Operator ); }
    bool IsOperator( const char * op ) const        { return ( m_Type == BFFTokenType::Operator ) && ( m_String == op ); }
    bool IsOperator( const char op ) const          { return ( m_Type == BFFTokenType::Operator ) && ( m_String[ 0 ] == op ) && ( m_String.GetLength() == 1 ); }
    bool IsRoundBracket( const char c ) const       { return ( m_Type == BFFTokenType::RoundBracket ) && ( m_String[ 0 ] == c ); }
    bool IsCurlyBracket( const char c ) const       { return ( m_Type == BFFTokenType::CurlyBracket ) && ( m_String[ 0 ] == c ); }
    bool IsSquareBracket( const char c ) const      { return ( m_Type == BFFTokenType::SquareBracket ) && ( m_String[ 0 ] == c ); }
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
