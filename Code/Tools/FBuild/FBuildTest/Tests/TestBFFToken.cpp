// TestBFFToken.cpp
//------------------------------------------------------------------------------

// Includes
//------------------------------------------------------------------------------
#include "Tools/FBuild/FBuildTest/Tests/FBuildTest.h"

// FBuildCore
#include "Tools/FBuild/FBuildCore/BFF/BFFFile.h"
#include "Tools/FBuild/FBuildCore/BFF/Tokenizer/BFFToken.h"

// Core
#include "Core/Strings/AStackString.h"

//------------------------------------------------------------------------------
class TestBFFToken : public FBuildTest
{
private:
    DECLARE_TESTS

    void Invalid() const;
    void String() const;
    void Int() const;
    void Bool() const;
    void Identifier() const;
    void Function() const;
    void Variable() const;
    void Keyword() const;
    void Operator() const;
    void Brace() const;
    void Comma() const;
    void EndOfFile() const;

    TestBFFToken & operator=( const TestBFFToken & other ) = delete;

    const BFFFile m_File{ "filename.bff", AString::GetEmpty() };
};

// Register Tests
//------------------------------------------------------------------------------
REGISTER_TESTS_BEGIN( TestBFFToken )
    REGISTER_TEST( Invalid )
    REGISTER_TEST( String )
    REGISTER_TEST( Int )
    REGISTER_TEST( Bool )
    REGISTER_TEST( Identifier )
    REGISTER_TEST( Function )
    REGISTER_TEST( Variable )
    REGISTER_TEST( Keyword )
    REGISTER_TEST( Operator )
    REGISTER_TEST( Brace )
    REGISTER_TEST( Comma )
    REGISTER_TEST( EndOfFile )
REGISTER_TESTS_END

//------------------------------------------------------------------------------
void TestBFFToken::Invalid() const
{
    BFFToken token( BFFToken::InvalidType::eInvalid, m_File, "" );
    TEST_ASSERT( token.GetType() == BFFTokenType::Invalid );
}

//------------------------------------------------------------------------------
void TestBFFToken::String() const
{
    const AStackString stringValue( "string" );
    const BFFToken token( stringValue, m_File, "" );
    TEST_ASSERT( token.IsString() );
    TEST_ASSERT( token.GetType() == BFFTokenType::String );
    TEST_ASSERT( token.GetValueString() == stringValue );
}

//------------------------------------------------------------------------------
void TestBFFToken::Int() const
{
    const int32_t intValue = 99;
    const BFFToken token( intValue, m_File, "" );
    TEST_ASSERT( token.IsNumber() );
    TEST_ASSERT( token.GetType() == BFFTokenType::Number );
    TEST_ASSERT( token.GetValueInt() == intValue );
    TEST_ASSERT( token.GetValueString() == "99" );
}

//------------------------------------------------------------------------------
void TestBFFToken::Bool() const
{
    const bool boolValue = true;
    const BFFToken token( boolValue, m_File, "" );
    TEST_ASSERT( token.IsBoolean() );
    TEST_ASSERT( token.GetType() == BFFTokenType::Boolean );
    TEST_ASSERT( token.GetBoolean() == boolValue );
    TEST_ASSERT( token.GetValueString() == "true" );
}

//------------------------------------------------------------------------------
void TestBFFToken::Identifier() const
{
    const AStackString identifierName( "identifier" );
    const BFFToken token( BFFToken::IdentifierType::eIdentifier, m_File, "", identifierName );
    TEST_ASSERT( token.IsIdentifier() );
    TEST_ASSERT( token.GetType() == BFFTokenType::Identifier );
    TEST_ASSERT( token.GetValueString() == identifierName );
}

//------------------------------------------------------------------------------
void TestBFFToken::Function() const
{
    const AStackString functionName( "Function" );
    const BFFToken token( BFFToken::FunctionType::eFunction, m_File, "", functionName );
    TEST_ASSERT( token.IsFunction() );
    TEST_ASSERT( token.GetType() == BFFTokenType::Function );
    TEST_ASSERT( token.GetValueString() == functionName );
}

//------------------------------------------------------------------------------
void TestBFFToken::Variable() const
{
    const AStackString variableName( ".Variable" );
    const BFFToken token( BFFToken::VariableType::eVariable, m_File, "", variableName );
    TEST_ASSERT( token.IsVariable() );
    TEST_ASSERT( token.GetType() == BFFTokenType::Variable );
    TEST_ASSERT( token.GetValueString() == variableName );
}

//------------------------------------------------------------------------------
void TestBFFToken::Keyword() const
{
    // Check mapping is correct and bi-directional
#define CHECK( keywordString, type ) \
    { \
        TEST_ASSERT( AStackString( keywordString ) == BFFKeyword::GetString( BFFKeyword::Type::type ) ); \
        TEST_ASSERT( BFFKeyword::GetType( AStackString( keywordString ) ) == BFFKeyword::Type::type ); \
    } \
    ++count

    uint32_t count = 0;
    CHECK( BFF_KEYWORD_DEFINE, eDefine );
    CHECK( BFF_KEYWORD_ELIF, eElif );
    CHECK( BFF_KEYWORD_ELSE, eElse );
    CHECK( BFF_KEYWORD_ENDIF, eEndIf );
    CHECK( BFF_KEYWORD_EXISTS, eExists );
    CHECK( BFF_KEYWORD_FALSE, eFalse );
    CHECK( BFF_KEYWORD_FILE_EXISTS, eFileExists );
    CHECK( BFF_KEYWORD_FUNCTION, eFunction );
    CHECK( BFF_KEYWORD_IF, eIf );
    CHECK( BFF_KEYWORD_IMPORT, eImport );
    CHECK( BFF_KEYWORD_IN, eIn );
    CHECK( BFF_KEYWORD_INCLUDE, eInclude );
    CHECK( BFF_KEYWORD_NOT, eNot );
    CHECK( BFF_KEYWORD_ONCE, eOnce );
    CHECK( BFF_KEYWORD_TRUE, eTrue );
    CHECK( BFF_KEYWORD_UNDEF, eUndef );

    // Ensure every keyword was checked
    TEST_ASSERT( count == static_cast<size_t>( BFFKeyword::Type::Count ) );

#undef CHECK

    // Test BFFToken
    const BFFToken token( BFFKeyword::Type::eInclude, m_File, "" );
    TEST_ASSERT( token.GetType() == BFFTokenType::Keyword );
    TEST_ASSERT( token.IsKeyword() );
    TEST_ASSERT( token.GetKeywordType() == BFFKeyword::Type::eInclude );
    TEST_ASSERT( AStackString( BFF_KEYWORD_INCLUDE ) == BFFKeyword::GetString( token.GetKeywordType() ) );
    TEST_ASSERT( token.GetValueString() == "include" );
}

//------------------------------------------------------------------------------
void TestBFFToken::Operator() const
{
    // Check mapping is correct and bi-directional
#define CHECK( operatorString, type ) \
    { \
        TEST_ASSERT( AStackString( operatorString ) == BFFOperator::GetString( BFFOperator::Type::type ) ); \
        TEST_ASSERT( BFFOperator::GetType( AStackString( operatorString ) ) == BFFOperator::Type::type ); \
    } \
    ++count

    uint32_t count = 0;
    CHECK( BFF_OPERATOR_ASSIGN, eAssign );
    CHECK( BFF_OPERATOR_GREATER_THAN, eGreaterThan );
    CHECK( BFF_OPERATOR_LESS_THAN, eLessThan );
    CHECK( BFF_OPERATOR_NOT, eNot );
    CHECK( BFF_OPERATOR_PLUS, ePlus );
    CHECK( BFF_OPERATOR_MINUS, eMinus );
    CHECK( BFF_OPERATOR_EQUAL, eEqual );
    CHECK( BFF_OPERATOR_NOT_EQUAL, eNotEqual );
    CHECK( BFF_OPERATOR_GREATER_THAN_OR_EQUAL, eGreaterThanOrEqual );
    CHECK( BFF_OPERATOR_LESS_THAN_OR_EQUAL, eLessThanOrEqual );
    CHECK( BFF_OPERATOR_AND, eAnd );
    CHECK( BFF_OPERATOR_OR, eOr );

    // Ensure every keyword was checked
    TEST_ASSERT( count == static_cast<size_t>( BFFOperator::Type::Count ) );

#undef CHECK

    // Test BFFToken
    const BFFToken token( BFFOperator::Type::eEqual, m_File, "" );
    TEST_ASSERT( token.GetType() == BFFTokenType::Operator );
    TEST_ASSERT( token.IsOperator() );
    TEST_ASSERT( token.GetOperatorType() == BFFOperator::Type::eEqual );
    TEST_ASSERT( AStackString( "==" ) == BFFOperator::GetString( token.GetOperatorType() ) );
    TEST_ASSERT( token.GetValueString() == "==" );
}

//------------------------------------------------------------------------------
void TestBFFToken::Brace() const
{
#define CHECK( braceType, braceTypeString ) \
    do { \
        const BFFToken token( BFFToken::BraceType::braceType, m_File, "" ); \
        TEST_ASSERT( token.IsBrace() ); \
        TEST_ASSERT( token.GetBraceType() == BFFToken::BraceType::braceType ); \
        TEST_ASSERT( token.GetValueString() == braceTypeString ); \
        /* Some code relies on the enum value being equal to the char value */ \
        TEST_ASSERT( token.GetValueString()[ 0 ] == static_cast<char>( BFFToken::BraceType::braceType ) ); \
    } while ( false )

    CHECK( eRoundLeft, "(" );
    CHECK( eRoundRight, ")" );
    CHECK( eCurlyLeft, "{" );
    CHECK( eCurlyRight, "}" );
    CHECK( eSquareLeft, "[" );
    CHECK( eSquareRight, "]" );

#undef CHECK
}

//------------------------------------------------------------------------------
void TestBFFToken::Comma() const
{
    const BFFToken token( BFFToken::CommaType::eComma, m_File, "" );
    TEST_ASSERT( token.GetType() == BFFTokenType::Comma );
    TEST_ASSERT( token.GetValueString() == "," );
}

//------------------------------------------------------------------------------
void TestBFFToken::EndOfFile() const
{
    const BFFToken token( BFFToken::EndOfFileType::eEndOfFile, m_File, "" );
    TEST_ASSERT( token.GetType() == BFFTokenType::EndOfFile );
}

//------------------------------------------------------------------------------
