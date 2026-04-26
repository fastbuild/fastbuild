// BFFToken.h
//------------------------------------------------------------------------------
#pragma once

// Includes
//------------------------------------------------------------------------------
// FBuildCore
#include "Tools/FBuild/FBuildCore/BFF/BFFFile.h"
#include "Tools/FBuild/FBuildCore/BFF/BFFKeywords.h"
#include "Tools/FBuild/FBuildCore/BFF/BFFOperators.h"

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
    Operator,       // = > ! == etc
    Brace,          // (, ), {, }, [, ]
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
    enum class CommaType : uint8_t
    {
        eComma
    };

    enum class BraceType : uint8_t
    {
        eRoundLeft = '(',
        eRoundRight = ')',
        eCurlyLeft = '{',
        eCurlyRight = '}',
        eSquareLeft = '[',
        eSquareRight = ']',
    };

    enum class VariableType : uint8_t
    {
        eVariable
    };

    enum class EndOfFileType : uint8_t
    {
        eEndOfFile
    };

    enum class FunctionType : uint8_t
    {
        eFunction
    };

    enum class IdentifierType : uint8_t
    {
        eIdentifier
    };

    enum class InvalidType : uint8_t
    {
        eInvalid
    };

    // Constructors
    FORCE_INLINE BFFToken( InvalidType invalidType, const BFFFile & file, const char * sourcePos );
    FORCE_INLINE BFFToken( BFFOperator::Type opType, const BFFFile & file, const char * sourcePos );
    FORCE_INLINE BFFToken( CommaType commaType, const BFFFile & file, const char * sourcePos );
    FORCE_INLINE BFFToken( BraceType braceType, const BFFFile & file, const char * sourcePos );
    FORCE_INLINE BFFToken( VariableType variableType, const BFFFile & file, const char * sourcePos, const char * endPos );
    FORCE_INLINE BFFToken( VariableType variableType, const BFFFile & file, const char * sourcePos, const AString & value );
    FORCE_INLINE BFFToken( EndOfFileType oefType, const BFFFile & file, const char * sourcePos );
    FORCE_INLINE BFFToken( BFFKeyword::Type keywordType, const BFFFile & file, const char * sourcePos );
    FORCE_INLINE BFFToken( FunctionType functionType, const BFFFile & file, const char * sourcePos, const AString & name );
    FORCE_INLINE BFFToken( IdentifierType identifierType, const BFFFile & file, const char * sourcePos, const AString & name );
    FORCE_INLINE BFFToken( const AString & string, const BFFFile & file, const char * sourcePos );
    FORCE_INLINE BFFToken( bool b, const BFFFile & file, const char * sourcePos );
    FORCE_INLINE BFFToken( int32_t i, const BFFFile & file, const char * sourcePos );

    // Allow move construction
    BFFToken( BFFToken && other ) = default;

    // Prevent copies (to be sure we're moving)
    BFFToken( BFFToken & ) = delete;
    BFFToken & operator=( BFFToken & ) = delete;

    // Convenience wrappers
    bool IsInvalid() const { return ( m_Type == BFFTokenType::Invalid ); }

    // Braces
    [[nodiscard]] bool IsBrace() const { return ( m_Type == BFFTokenType::Brace ); }
    [[nodiscard]] bool IsRoundLeftBracket() const { return ( m_BraceType == BFFToken::BraceType::eRoundLeft ); }
    [[nodiscard]] bool IsRoundRightBracket() const { return ( m_BraceType == BFFToken::BraceType::eRoundRight ); }
    [[nodiscard]] bool IsCurlyLeftBracket() const { return ( m_BraceType == BFFToken::BraceType::eCurlyLeft ); }
    [[nodiscard]] bool IsCurlyRightBracket() const { return ( m_BraceType == BFFToken::BraceType::eCurlyRight ); }
    [[nodiscard]] bool IsSquareLeftBracket() const { return ( m_BraceType == BFFToken::BraceType::eSquareLeft ); }
    [[nodiscard]] bool IsSquareRightBracket() const { return ( m_BraceType == BFFToken::BraceType::eSquareRight ); }
    [[nodiscard]] BraceType GetMatchingClosingBracketType() const;
    [[nodiscard]] BraceType GetBraceType() const;

    // Comma
    [[nodiscard]] bool IsComma() const { return ( m_Type == BFFTokenType::Comma ); }

    // String
    [[nodiscard]] bool IsString() const { return ( m_Type == BFFTokenType::String ); }

    // Boolean
    [[nodiscard]] bool IsBoolean() const { return ( m_Type == BFFTokenType::Boolean ); }
    [[nodiscard]] bool GetBoolean() const;

    // Integer
    [[nodiscard]] bool IsNumber() const { return ( m_Type == BFFTokenType::Number ); }
    [[nodiscard]] int32_t GetValueInt() const;

    // Keyword
    [[nodiscard]] bool IsKeyword() const { return ( m_Type == BFFTokenType::Keyword ); }
    [[nodiscard]] bool IsKeyword( BFFKeyword::Type keyword ) const { return IsKeyword() && ( m_KeywordType == keyword ); }
    [[nodiscard]] BFFKeyword::Type GetKeywordType() const;

    // Operator
    [[nodiscard]] bool IsOperator() const { return ( m_Type == BFFTokenType::Operator ); }
    [[nodiscard]] bool IsOperator( BFFOperator::Type op ) const { return IsOperator() && ( m_OperatorType == op ); }
    [[nodiscard]] BFFOperator::Type GetOperatorType() const;

    // Identifier
    [[nodiscard]] bool IsIdentifier() const { return ( m_Type == BFFTokenType::Identifier ); }

    // Function
    [[nodiscard]] bool IsFunction() const { return ( m_Type == BFFTokenType::Function ); }

    // Variable
    [[nodiscard]] bool IsVariable() const { return ( m_Type == BFFTokenType::Variable ); }

    // Generic Access
    BFFTokenType GetType() const { return m_Type; }
    [[nodiscard]] const AString & GetValueString() const;

    const BFFFile & GetSourceFile() const { return m_BFFFile; }
    const AString & GetSourceFileName() const { return m_BFFFile.GetFileName(); }
    const AString & GetSourceFileContents() const { return m_BFFFile.GetSourceFileContents(); }
    const char * GetSourcePos() const { return m_SourcePos; }

    void GetPosInfo( uint32_t & outLine, uint32_t & outColumn, const char *& outLineStart ) const;

    // Some variables come from built-in declarations so we need a proxy BFFToken for those
    static const BFFToken & GetBuiltInToken() { return s_BuiltInToken; }

private:
    const BFFTokenType m_Type;
    union
    {
        int32_t m_Integer = 0; // Initializes entire union
        BFFOperator::Type m_OperatorType;
        BraceType m_BraceType;
        BFFKeyword::Type m_KeywordType;
        bool m_Boolean;
    };
    mutable AString m_String;
    const BFFFile & m_BFFFile;
    const char * m_SourcePos = nullptr;

    // Static Data
    static const BFFFile s_BuiltInFile;
    static const BFFToken s_BuiltInToken;
};

//------------------------------------------------------------------------------
inline BFFToken::BFFToken( InvalidType /*invalidType*/, const BFFFile & file, const char * sourcePos )
    : m_Type( BFFTokenType::Invalid )
    , m_BFFFile( file )
    , m_SourcePos( sourcePos )
{
}

//------------------------------------------------------------------------------
inline BFFToken::BFFToken( BFFOperator::Type opType, const BFFFile & file, const char * sourcePos )
    : m_Type( BFFTokenType::Operator )
    , m_OperatorType( opType )
    , m_BFFFile( file )
    , m_SourcePos( sourcePos )
{
}

//------------------------------------------------------------------------------
inline BFFToken::BFFToken( CommaType /*commaType*/, const BFFFile & file, const char * sourcePos )
    : m_Type( BFFTokenType::Comma )
    , m_BFFFile( file )
    , m_SourcePos( sourcePos )
{
}

//------------------------------------------------------------------------------
inline BFFToken::BFFToken( BraceType braceType, const BFFFile & file, const char * sourcePos )
    : m_Type( BFFTokenType::Brace )
    , m_BraceType( braceType )
    , m_BFFFile( file )
    , m_SourcePos( sourcePos )
{
}

//------------------------------------------------------------------------------
inline BFFToken::BFFToken( VariableType /*variableType*/, const BFFFile & file, const char * sourcePos, const char * endPos )
    : m_Type( BFFTokenType::Variable )
    , m_String( sourcePos, endPos )
    , m_BFFFile( file )
    , m_SourcePos( sourcePos )
{
}

//------------------------------------------------------------------------------
inline BFFToken::BFFToken( VariableType /*variableType*/, const BFFFile & file, const char * sourcePos, const AString & value )
    : m_Type( BFFTokenType::Variable )
    , m_String( value )
    , m_BFFFile( file )
    , m_SourcePos( sourcePos )
{
}

//------------------------------------------------------------------------------
inline BFFToken::BFFToken( EndOfFileType /*eofType*/, const BFFFile & file, const char * sourcePos )
    : m_Type( BFFTokenType::EndOfFile )
    , m_BFFFile( file )
    , m_SourcePos( sourcePos )
{
}

//------------------------------------------------------------------------------
inline BFFToken::BFFToken( BFFKeyword::Type keywordType, const BFFFile & file, const char * sourcePos )
    : m_Type( BFFTokenType::Keyword )
    , m_KeywordType( keywordType )
    , m_BFFFile( file )
    , m_SourcePos( sourcePos )
{
}

//------------------------------------------------------------------------------
inline BFFToken::BFFToken( FunctionType /*functionType*/, const BFFFile & file, const char * sourcePos, const AString & name )
    : m_Type( BFFTokenType::Function )
    , m_String( name )
    , m_BFFFile( file )
    , m_SourcePos( sourcePos )
{
}

//------------------------------------------------------------------------------
inline BFFToken::BFFToken( IdentifierType /*identifierType*/, const BFFFile & file, const char * sourcePos, const AString & name )
    : m_Type( BFFTokenType::Identifier )
    , m_String( name )
    , m_BFFFile( file )
    , m_SourcePos( sourcePos )
{
}

//------------------------------------------------------------------------------
inline BFFToken::BFFToken( const AString & string, const BFFFile & file, const char * sourcePos )
    : m_Type( BFFTokenType::String )
    , m_String( string )
    , m_BFFFile( file )
    , m_SourcePos( sourcePos )
{
}

//------------------------------------------------------------------------------
inline BFFToken::BFFToken( bool b, const BFFFile & file, const char * sourcePos )
    : m_Type( BFFTokenType::Boolean )
    , m_Boolean( b )
    , m_BFFFile( file )
    , m_SourcePos( sourcePos )
{
}

//------------------------------------------------------------------------------
inline BFFToken::BFFToken( int32_t i, const BFFFile & file, const char * sourcePos )
    : m_Type( BFFTokenType::Number )
    , m_Integer( i )
    , m_BFFFile( file )
    , m_SourcePos( sourcePos )
{
}

//------------------------------------------------------------------------------
inline BFFToken::BraceType BFFToken::GetMatchingClosingBracketType() const
{
    ASSERT( IsBrace() );
    if ( m_BraceType == BraceType::eRoundLeft )
    {
        return BraceType::eRoundRight;
    }
    else if ( m_BraceType == BraceType::eCurlyLeft )
    {
        return BraceType::eCurlyRight;
    }
    else
    {
        ASSERT( m_BraceType == BraceType::eSquareLeft );
        return BraceType::eSquareRight;
    }
}

//------------------------------------------------------------------------------
inline BFFToken::BraceType BFFToken::GetBraceType() const
{
    ASSERT( IsBrace() );
    return m_BraceType;
}

//------------------------------------------------------------------------------
inline bool BFFToken::GetBoolean() const
{
    ASSERT( IsBoolean() );
    return m_Boolean;
}

//------------------------------------------------------------------------------
inline int32_t BFFToken::GetValueInt() const
{
    ASSERT( IsNumber() );
    return m_Integer;
}

//------------------------------------------------------------------------------
inline BFFKeyword::Type BFFToken::GetKeywordType() const
{
    ASSERT( IsKeyword() );
    return m_KeywordType;
}

//------------------------------------------------------------------------------
inline BFFOperator::Type BFFToken::GetOperatorType() const
{
    ASSERT( IsOperator() );
    return m_OperatorType;
}

//------------------------------------------------------------------------------
