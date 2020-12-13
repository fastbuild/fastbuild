// BFFBooleanExpParser
//------------------------------------------------------------------------------

// Includes
//------------------------------------------------------------------------------
#include "BFFBooleanExpParser.h"

#include "Tools/FBuild/FBuildCore/Error.h"
#include <Tools/FBuild/FBuildCore/BFF/BFFKeywords.h>
#include <Tools/FBuild/FBuildCore/BFF/BFFParser.h>
#include <Tools/FBuild/FBuildCore/BFF/BFFStackFrame.h>
#include <Tools/FBuild/FBuildCore/BFF/BFFVariable.h>
#include "Tools/FBuild/FBuildCore/BFF/Tokenizer/BFFTokenRange.h"

#include "Core/Strings/AStackString.h"

enum class Operator
{
    OP_UNKNOWN,

    OP_IN,
    OP_NOT_IN,

    OP_EQUAL,
    OP_NOT_EQUAL,
    OP_LESS_THAN,
    OP_LESS_THAN_OR_EQUAL,
    OP_GREATER_THAN,
    OP_GREATER_THAN_OR_EQUAL,

    OP_AND,
    OP_OR,
    // OP_XOR, not supported yet.
};


// GetVar
//------------------------------------------------------------------------------
static const BFFVariable * GetVar( const Function * function, const BFFToken * token )
{
    if ( token->IsVariable() == false )
    {
        Error::Error_1200_ExpectedVar( token, function );
        return nullptr;
    }

    AString varName;
    bool varParentScope = false;
    if ( BFFParser::ParseVariableName( token, varName, varParentScope ) == false )
    {
        return nullptr; // ParseVariableName will have emitted error
    }

    const BFFVariable * var = nullptr;
    const BFFStackFrame * varFrame = (varParentScope) ? BFFStackFrame::GetParentDeclaration( varName, BFFStackFrame::GetCurrent()->GetParent(), var )
        : nullptr;
    if ( false == varParentScope )
    {
        var = BFFStackFrame::GetVar( varName, nullptr );
    }

    if ( (varParentScope && (nullptr == varFrame)) || (var == nullptr) )
    {
        Error::Error_1009_UnknownVariable( token, function, varName );
        return nullptr;
    }

    return var;
}

// GetBool
//------------------------------------------------------------------------------
static bool GetBool( const Function * function, const BFFToken * token, bool & value )
{
    if ( token->IsBooelan() )
    {
        value = token->GetBoolean();
        return true;
    }

    if ( token->IsVariable() )
    {
        const BFFVariable * var = GetVar( function, token );
        if ( var == nullptr )
        {
            return false;
        }
        if ( !var->IsBool() )
        {
            Error::Error_1050_PropertyMustBeOfType( token, function, var->GetName().Get(), var->GetType(), BFFVariable::VAR_BOOL );
            return false;
        }
        value = var->GetBool();
        return true;
    }

    Error::Error_1256_UnexpectedToken( token, function );
    return false;
}

// GetInt
//------------------------------------------------------------------------------
static bool GetInt( const Function * function, const BFFToken * token, int32_t & value )
{
    if ( token->IsNumber() )
    {
        value = token->GetValueInt();
        return true;
    }

    if ( token->IsVariable() )
    {
        const BFFVariable * var = GetVar( function, token );
        if ( var == nullptr )
        {
            return false;
        }
        if ( !var->IsInt() )
        {
            Error::Error_1050_PropertyMustBeOfType( token, function, var->GetName().Get(), var->GetType(), BFFVariable::VAR_INT );
            return false;
        }
        value = var->GetInt();
        return true;
    }

    Error::Error_1256_UnexpectedToken( token, function );
    return false;
}

// GetString
//------------------------------------------------------------------------------
static bool GetString( const Function * function, const BFFToken * token, AString & value )
{
    if ( token->IsString() )
    {
        value = token->GetValueString();
        return true;
    }

    if ( token->IsVariable() )
    {
        const BFFVariable * var = GetVar( function, token );
        if ( var == nullptr )
        {
            return false;
        }
        if ( !var->IsString() )
        {
            Error::Error_1050_PropertyMustBeOfType( token, function, var->GetName().Get(), var->GetType(), BFFVariable::VAR_STRING );
            return false;
        }
        value = var->GetString();
        return true;
    }

    Error::Error_1256_UnexpectedToken( token, function );
    return false;
}

// GetArrayOfString
// Get the array of strings, `value` is guaranteed to be not null if returns true.
//------------------------------------------------------------------------------
static bool GetArrayOfString( const Function * function, const BFFToken * token, const Array<AString> * & value )
{
    if ( token->IsVariable() )
    {
        const BFFVariable * var = GetVar( function, token );
        if ( var == nullptr )
        {
            return false;
        }
        if ( !var->IsArrayOfStrings() )
        {
            Error::Error_1050_PropertyMustBeOfType( token, function, var->GetName().Get(), var->GetType(), BFFVariable::VAR_ARRAY_OF_STRINGS );
            return false;
        }
        value = &var->GetArrayOfStrings();
        return true;
    }

    Error::Error_1256_UnexpectedToken( token, function );
    return false;
}

static const Array<Operator> BoolOperators
{
    Operator::OP_EQUAL,
    Operator::OP_NOT_EQUAL,
    Operator::OP_AND,
    Operator::OP_OR,
};

// CompareBools
//------------------------------------------------------------------------------
static bool CompareBools( const Function * function, bool lhs, bool rhs, const BFFToken * opToken, Operator op, bool & result )
{

    switch ( op )
    {

    // Supported
    case Operator::OP_EQUAL:                    result = (lhs == rhs); break;
    case Operator::OP_NOT_EQUAL:                result = (lhs != rhs); break;
    case Operator::OP_AND:                      result = (lhs && rhs); break;
    case Operator::OP_OR:                       result = (lhs || rhs); break;

    // Unsupported
    case Operator::OP_LESS_THAN:
    case Operator::OP_LESS_THAN_OR_EQUAL:
    case Operator::OP_GREATER_THAN:
    case Operator::OP_GREATER_THAN_OR_EQUAL:
    case Operator::OP_IN:
    case Operator::OP_NOT_IN:
        Error::Error_1255_UnexpectedOperator( opToken, function );
        return false;
    case Operator::OP_UNKNOWN:
        ASSERT( false );
        return false;
    }
    return true;
}

static const Array<Operator> IntOperators
{
    Operator::OP_EQUAL,
    Operator::OP_NOT_EQUAL,
    Operator::OP_LESS_THAN,
    Operator::OP_LESS_THAN_OR_EQUAL,
    Operator::OP_GREATER_THAN,
    Operator::OP_GREATER_THAN_OR_EQUAL,
};

// CompareInts
//------------------------------------------------------------------------------
static bool CompareInts( const Function * function, int32_t lhs, int32_t rhs, const BFFToken * opToken, const Operator op, bool & result )
{

    switch ( op )
    {
        // Supported
    case Operator::OP_EQUAL:                    result = (lhs == rhs); break;
    case Operator::OP_NOT_EQUAL:                result = (lhs != rhs); break;
    case Operator::OP_LESS_THAN:                result = (lhs < rhs); break;
    case Operator::OP_LESS_THAN_OR_EQUAL:       result = (lhs < rhs) || (lhs == rhs); break;
    case Operator::OP_GREATER_THAN:             result = (lhs > rhs); break;
    case Operator::OP_GREATER_THAN_OR_EQUAL:    result = (lhs > rhs) || (lhs == rhs); break;

        // Logic error
    case Operator::OP_IN:
    case Operator::OP_NOT_IN:
    case Operator::OP_AND:
    case Operator::OP_OR:
        Error::Error_1255_UnexpectedOperator( opToken, function );
        return false;

    case Operator::OP_UNKNOWN:
        ASSERT( false );
        return false;
    }
    return true;
}

static const Array<Operator> StringOperators
{
    Operator::OP_EQUAL,
    Operator::OP_NOT_EQUAL,
    Operator::OP_LESS_THAN,
    Operator::OP_LESS_THAN_OR_EQUAL,
    Operator::OP_GREATER_THAN,
    Operator::OP_GREATER_THAN_OR_EQUAL,
    Operator::OP_IN,
    Operator::OP_NOT_IN,
};

// CompareStrings
//------------------------------------------------------------------------------
static bool CompareStrings( const Function * function, const AString & lhs, const AString & rhs, const BFFToken * opToken, const Operator op, bool & result )
{

    switch ( op )
    {
        // Supported
    case Operator::OP_EQUAL:                    result = (lhs == rhs); break;
    case Operator::OP_NOT_EQUAL:                result = (lhs != rhs); break;
    case Operator::OP_LESS_THAN:                result = (lhs < rhs); break;
    case Operator::OP_LESS_THAN_OR_EQUAL:       result = (lhs < rhs) || (lhs == rhs); break;
    case Operator::OP_GREATER_THAN:             result = (lhs > rhs); break;
    case Operator::OP_GREATER_THAN_OR_EQUAL:    result = (lhs > rhs) || (lhs == rhs); break;

        // Logic error
    case Operator::OP_IN:
    case Operator::OP_NOT_IN:
        ASSERT( false ); // Should be hanlded in another function.
        return false;
    case Operator::OP_AND:
    case Operator::OP_OR:
        Error::Error_1255_UnexpectedOperator( opToken, function );
        return false;

    case Operator::OP_UNKNOWN:
        ASSERT( false );
        return false;
    }
    return true;
}

static bool ParseBooleanExp( const Function * function, BFFTokenRange & iter, bool endsOnCloseBracket, bool & expResult );

// IsExpEnd
// .a == .b
//         ^end
// (.a == .b) || (.c == .d)
//          ^end of sub exp
// iter is expected pointing to its own end or a close round bracket if `endsOnCloseBracket` is true.
//------------------------------------------------------------------------------
static bool IsExpEnd( BFFTokenRange & iter, bool endsOnCloseBracket )
{
    if ( iter.IsAtEnd() )
    {
        return true;
    }
    return endsOnCloseBracket && iter.GetCurrent()->IsRoundBracket( ')' );
}

// ParseOperator consumes 1 or 2 tokens (2 for "not in").
// iter is expected pointing to the operator.
//------------------------------------------------------------------------------
static bool ParseOperator( const Function * function, BFFTokenRange & iter, Operator & op, const Array<Operator> * supportedOps = nullptr )
{
    // Determine operator for more complex expression
    op = Operator::OP_UNKNOWN;

    // in
    if ( iter->IsKeyword( BFF_KEYWORD_IN ) )
    {
        op = Operator::OP_IN;
    }
    // not in
    else if ( iter->IsKeyword( BFF_KEYWORD_NOT ) )
    {
        iter++; // Consume additional keyword
        if ( iter->IsKeyword( BFF_KEYWORD_IN ) )
        {
            op = Operator::OP_NOT_IN;
        }
    }
    // ==
    else if ( iter->IsOperator( "==" ) )
    {
        op = Operator::OP_EQUAL;
    }
    // !=
    else if ( iter->IsOperator( "!=" ) )
    {
        op = Operator::OP_NOT_EQUAL;
    }
    // <=
    else if ( iter->IsOperator( "<=" ) )
    {
        op = Operator::OP_LESS_THAN_OR_EQUAL;
    }
    // <  (must be checked after <=)
    else if ( iter->IsOperator( "<" ) )
    {
        op = Operator::OP_LESS_THAN;
    }
    // >=
    else if ( iter->IsOperator( ">=" ) )
    {
        op = Operator::OP_GREATER_THAN_OR_EQUAL;
    }
    // >  (must be checked after >=)
    else if ( iter->IsOperator( ">" ) )
    {
        op = Operator::OP_GREATER_THAN;
    }
    else if ( iter->IsOperator( "&&" ) )
    {
        op = Operator::OP_AND;
    }
    else if ( iter->IsOperator( "||" ) )
    {
        op = Operator::OP_OR;
    }

    // Handle unrecognized operator
    if ( op == Operator::OP_UNKNOWN )
    {
        Error::Error_1042_UnknownOperator( iter.GetCurrent(), iter->GetValueString() );
        return false;
    }

    if ( ( supportedOps != nullptr ) && ( supportedOps->Find( op ) == nullptr ) )
    {
        Error::Error_1255_UnexpectedOperator( iter.GetCurrent(), function );
        return false;
    }

    iter++; // consume keyword or operator
    return true;
}

/*
Forms:
    bool-exp =
             | string-exp in string-set
             | string-exp not in string-set
    string-exp = string-literal | string-variable
    string-set = string-set-variable

    iter is expected pointing to the rhs.
*/
//------------------------------------------------------------------------------
bool ParseStringInArray( const Function * function, const BFFToken * lhsToken, Operator op, BFFTokenRange & iter, bool & expResult )
{
    ASSERT( ( op == Operator::OP_IN ) || ( op == Operator::OP_NOT_IN ) );

    AString lhs;
    if ( !GetString( function, lhsToken, lhs ) )
    {
        return false;
    }

    const BFFToken * rhsToken = iter.GetCurrent();
    iter++;

    const Array< AString > * rhsArray = nullptr;
    if ( !GetArrayOfString( function, rhsToken, rhsArray ) )
    {
        return false;
    }
    ASSERT( rhsArray != nullptr );

    bool conditionSuccess = ( rhsArray->Find( lhs ) != nullptr );

    if ( op == Operator::OP_NOT_IN )
    {
        conditionSuccess = !conditionSuccess;
    }

    expResult = conditionSuccess;
    return true;
}

/*
Forms:
    bool-exp =
             | string-set in string-set
             | string-set not in string-set
    string-set = string-set-variable

iter is expected pointing to the operator.
*/
//------------------------------------------------------------------------------
bool ParseStringArrayInArray( const Function * function, const BFFToken * lhsToken, BFFTokenRange & iter, bool & expResult )
{
    const BFFToken * opToken = iter.GetCurrent();
    Operator op = Operator::OP_UNKNOWN;
    if ( !ParseOperator( function, iter, op ) )
    {
        return false;
    }
    if ( ( op != Operator::OP_IN ) && ( op != Operator::OP_NOT_IN ) )
    {
        Error::Error_1255_UnexpectedOperator( opToken, function );
        return false;
    }

    const Array< AString > * lhsArray = nullptr;
    if ( !GetArrayOfString( function, lhsToken, lhsArray ) )
    {
        return false;
    }

    const BFFToken * rhsToken = iter.GetCurrent();
    iter++;

    const Array< AString > * rhsArray = nullptr;
    if ( !GetArrayOfString( function, rhsToken, rhsArray ) )
    {
        return false;
    }

    ASSERT( ( lhsArray != nullptr ) || ( rhsArray != nullptr ) );

    bool conditionSuccess = false;

    // Is any string in array?
    for ( const AString & testStr : *lhsArray )
    {
        if ( rhsArray->Find( testStr ) )
        {
            conditionSuccess = true;
            break;
        }
    }

    if ( op == Operator::OP_NOT_IN )
    {
        conditionSuccess = !conditionSuccess;
    }

    expResult = conditionSuccess;
    return true;
}

/*
Forms:
    bool-exp = true | false
             | (bool-exp)
             | bool-variable
             | ! bool-exp

iter is expected pointing to the operand.
*/
//------------------------------------------------------------------------------
static bool ParseUnaryBooleanOperand( const Function * function, BFFTokenRange & iter, bool & expResult )
{
    switch ( iter.GetCurrent()->GetType() )
    {
    case BFFTokenType::Boolean:        // true or false
    case BFFTokenType::Variable:       // .Var or ^Var
    {
        if ( !GetBool( function, iter.GetCurrent(), expResult ) )
        {
            return false;
        }
        iter++;
        return true;
    }
    case BFFTokenType::Operator:       // = += > ! == etc
        if ( iter->IsOperator( "!" ) )
        {
            iter++;
            bool subResult;
            if ( !ParseUnaryBooleanOperand( function, iter, subResult ) )
            {
                return false;
            }
            expResult = !subResult;
            return true;
        }
        break;
    case BFFTokenType::RoundBracket:   // ( or )
        if ( iter->IsRoundBracket( '(' ) )
        {
            iter++;
            bool result = ParseBooleanExp( function, iter, true, expResult );
            if ( !result )
            {
                return false;
            }
            ASSERT( iter->IsRoundBracket( ')' ) );
            iter++;
            return true;
        }
        break;
    default:
        break;
    }

    Error::Error_1255_UnexpectedOperator( iter.GetCurrent(), function );
    return false;
}

/*
Forms:
    bool-exp = true | false
             | (bool-exp)
             | bool-variable
             | ! bool-exp
             | bool-exp ==/!=/&&/|| bool-exp

iter is expected pointing to the operator.
*/
//------------------------------------------------------------------------------
static bool ParseBinaryBooleanExp( const Function * function, bool lhs, BFFTokenRange & iter, bool & expResult )
{
    const BFFToken * opToken = iter.GetCurrent();
    Operator op = Operator::OP_UNKNOWN;
    if ( !ParseOperator( function, iter, op, &BoolOperators ) )
    {
        return false;
    }

    bool rhs;
    switch ( iter.GetCurrent()->GetType() )
    {
    case BFFTokenType::Boolean:         // true, false
    case BFFTokenType::Variable:       // .Var or ^Var
    {
        if ( !GetBool( function, iter.GetCurrent(), rhs ) )
        {
            return false;
        }
        iter++; // Consume the current token.
        break;
    }
    case BFFTokenType::RoundBracket:
        if ( iter->IsRoundBracket( '(' ) )
        {
            iter++;
            if ( !ParseBooleanExp( function, iter, true, rhs ) )
            {
                return false;
            }
            ASSERT( iter.GetCurrent()->IsRoundBracket( ')' ) );
            iter++; // Consume the current token.
            break;
        }
        [[fallthrough]];
    default:
        Error::Error_1256_UnexpectedToken( iter.GetCurrent(), function );
        return false;
    }
    return CompareBools( function, lhs, rhs, opToken, op, expResult );
}

/*
Forms:
    bool-exp = 
             | number-exp ==/!=/>/>=/</<= number-exp
    number-exp = number-literal | number-variable

    iter is expected pointing to the operator.
*/
//------------------------------------------------------------------------------
static bool ParseIntComparisonExp( const Function * function, const BFFToken * lhsToken, BFFTokenRange & iter, bool & expResult )
{
    ASSERT( lhsToken->IsNumber() || lhsToken->IsVariable() );

    const BFFToken * opToken = iter.GetCurrent();
    Operator op = Operator::OP_UNKNOWN;
    if ( !ParseOperator( function, iter, op, &IntOperators ) )
    {
        return false;
    }

    // TODO: Support literal int arrays.
    switch ( iter.GetCurrent()->GetType() )
    {
    case BFFTokenType::Number:         // 123, -12
    case BFFTokenType::Variable:       // .Var or ^Var
    {
        int32_t lhs;
        if ( !GetInt( function, lhsToken, lhs ) )
        {
            return false;
        }
        int32_t rhs;
        if ( !GetInt( function, iter.GetCurrent(), rhs ) )
        {
            return false;
        }
        iter++;
        return CompareInts( function, lhs, rhs, opToken, op, expResult );
    }
    default:
        Error::Error_1256_UnexpectedToken( iter.GetCurrent(), function );
        return false;
    }
}

/*
Forms:
    bool-exp = 
             | string-exp ==/!= string-exp
             | string-exp in string-set
             | string-exp not in string-set
    string-exp = string-literal | string-variable
    string-set = string-set-variable

iter is expected pointing to the operator.
*/
//------------------------------------------------------------------------------
static bool ParseStringComparisonExp( const Function * function, const BFFToken * lhsToken, BFFTokenRange & iter, bool & expResult )
{
    ASSERT( lhsToken->IsString() || lhsToken->IsVariable() );

    const BFFToken * opToken = iter.GetCurrent();
    Operator op = Operator::OP_UNKNOWN;
    if ( !ParseOperator( function, iter, op, &StringOperators ) )
    {
        return false;
    }

    // TODO: Support literal string arrays.
    switch ( iter.GetCurrent()->GetType() )
    {
    case BFFTokenType::String:         // "Hello"
    case BFFTokenType::Variable:       // .Var or ^Var
    {
        AString lhs;
        if ( !GetString( function, lhsToken, lhs ) )
        {
            return false;
        }

        if ( ( op == Operator::OP_IN ) || ( op == Operator::OP_NOT_IN ) )
        {
            return ParseStringInArray( function, lhsToken, op, iter, expResult );
        }

        AString rhs;
        if ( !GetString( function, iter.GetCurrent(), rhs ) )
        {
            return false;
        }
        iter++;
        return CompareStrings( function, lhs, rhs, opToken, op, expResult );
    }
    default:
        Error::Error_1256_UnexpectedToken( iter.GetCurrent(), function );
        return false;
    }
}

/*
Forms:
    bool-exp = true | false
             | (bool-exp)
             | bool-variable
             | ! bool-exp
             | bool-exp ==/!=/&&/|| bool-exp
             | number-exp ==/!=/>/>=/</<= number-exp
             | string-exp ==/!= string-exp
             | string-exp in string-set
             | string-exp not in string-set
             | string-set in string-set
             | string-set not in string-set
    number-exp = number-literal | number-variable
    string-exp = string-literal | string-variable
    string-set = string-set-variable
*/
static bool ParseBooleanExp( const Function * function, BFFTokenRange & iter, bool endsOnCloseBracket, bool & expResult )
{
    switch ( iter.GetCurrent()->GetType() )
    {
    case BFFTokenType::Variable:       // .Var or ^Var
    {
        const BFFToken * varToken = iter.GetCurrent();
        const BFFVariable * var = GetVar( function, iter.GetCurrent() );
        if ( var == nullptr )
        {
            return false;
        }
        iter++;
        switch ( var->GetType() )
        {
        case BFFVariable::VAR_BOOL:
        {
            bool subResult = var->GetBool();
            while ( !IsExpEnd( iter, endsOnCloseBracket ) )
            {
                if ( !ParseBinaryBooleanExp( function, subResult, iter, subResult ) )
                {
                    return false;
                }
            }
            expResult = subResult;
            return true;
        }
        case BFFVariable::VAR_INT:
        {
            // TODO: Removed duplication of cases: VAR_INT, VAR_STRING, VAR_ARRAY_OF_STRINGS
            if ( ParseIntComparisonExp( function, varToken, iter, expResult ) )
            {
                if ( IsExpEnd( iter, endsOnCloseBracket ) )
                {
                    return true;
                }
                Error::Error_1002_MatchingClosingTokenNotFound( iter.GetCurrent(), function, ')' );
            }
            return false;
        }
        case BFFVariable::VAR_STRING:
        {
            if ( ParseStringComparisonExp( function, varToken, iter, expResult ) )
            {
                if ( IsExpEnd( iter, endsOnCloseBracket ) )
                {
                    return true;
                }
                Error::Error_1002_MatchingClosingTokenNotFound( iter.GetCurrent(), function, ')' );
            }
            return false;
        }
        case BFFVariable::VAR_ARRAY_OF_STRINGS:
        {
            if ( ParseStringArrayInArray( function, varToken, iter, expResult ) )
            {
                if ( IsExpEnd( iter, endsOnCloseBracket ) )
                {
                    return true;
                }
                Error::Error_1002_MatchingClosingTokenNotFound( iter.GetCurrent(), function, ')' );
            }
            return false;
        }
        default:
            ASSERT( false );
            return false;
        }
    }
    case BFFTokenType::Boolean:        // true or false
    {
        bool subResult = iter->GetBoolean();
        iter++;
        while ( !IsExpEnd( iter, endsOnCloseBracket ) )
        {
            if ( !ParseBinaryBooleanExp( function, subResult, iter, subResult ) )
            {
                return false;
            }
        }
        expResult = subResult;
        return true;
    }
    case BFFTokenType::Number:         // 12, 56, -102 etc
    {
        const BFFToken * varToken = iter.GetCurrent();
        iter++;
        return ParseIntComparisonExp( function, varToken, iter, expResult );
    }
    case BFFTokenType::String:         // "Hello"
    {
        const BFFToken * varToken = iter.GetCurrent();
        iter++;
        return ParseStringComparisonExp( function, varToken, iter, expResult );
    }
    case BFFTokenType::Operator:       // = += > ! == etc
        if ( iter->IsOperator( "!" ) )
        {
            iter++;
            bool subResult;
            if ( ParseUnaryBooleanOperand( function, iter, subResult ) )
            {
                expResult = !subResult;
                return true;
            }
            return false;
        }
        else
        {
            Error::Error_1255_UnexpectedOperator( iter.GetCurrent(), function );
            return false;
        }
        break;
    case BFFTokenType::RoundBracket:   // ( or )
        if ( iter->IsRoundBracket( '(' ) )
        {
            iter++;
            bool subResult;
            if ( !ParseBooleanExp( function, iter, true, subResult ) )
            {
                return false;
            }
            ASSERT( iter->IsRoundBracket( ')' ) );
            iter++;

            while ( !IsExpEnd( iter, true ) )
            {
                if ( !ParseBinaryBooleanExp( function, subResult, iter, subResult ) )
                {
                    return false;
                }
            }
            expResult = subResult;
            return true;
        }
        break;
    default:
        break;
    }

    Error::Error_1256_UnexpectedToken( iter.GetCurrent(), function );
    return false;
}

/*static*/ bool BFFBooleanExpParser::Parse( const Function * function, const BFFTokenRange & range, bool & expResult )
{
    // The `iter` will be used across all the functions calls without copying.
    BFFTokenRange iter( range );

    return ParseBooleanExp( function, iter, false, expResult );
}

