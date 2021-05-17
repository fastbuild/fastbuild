// BFFBooleanExpParser
//------------------------------------------------------------------------------

// Includes
//------------------------------------------------------------------------------
#include "BFFBooleanExpParser.h"

#include "Tools/FBuild/FBuildCore/Error.h"
#include "Tools/FBuild/FBuildCore/BFF/BFFKeywords.h"
#include "Tools/FBuild/FBuildCore/BFF/BFFParser.h"
#include "Tools/FBuild/FBuildCore/BFF/BFFStackFrame.h"
#include "Tools/FBuild/FBuildCore/BFF/BFFVariable.h"
#include "Tools/FBuild/FBuildCore/BFF/Tokenizer/BFFTokenRange.h"

#include "Core/Strings/AStackString.h"

namespace
{

// Forward Declarations
//------------------------------------------------------------------------------
static bool ParseBooleanExp( const Function * function, BFFTokenRange & iter, bool endsOnCloseBracket, bool & expResult );
static bool ParseSubBooleanExp( const Function * function, BFFTokenRange & iter, bool & expResult );
static bool ParseUnaryBooleanExp( const Function * function, BFFTokenRange & iter, bool & expResult );

// BoolOperator
//------------------------------------------------------------------------------
enum class BoolOperator
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

    AStackString<> varName;
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

// Meta of Types
// ---------------------------------------------------------------

// Bool
// ---------------------------------------------------------------
struct Bool
{
    typedef bool Type;
    typedef bool ReturnType;
    typedef bool & OutParamType;
    static const BFFVariable::VarType VarType = BFFVariable::VarType::VAR_BOOL;
};

// Number
// ---------------------------------------------------------------
struct Number
{
    typedef int32_t Type;
    typedef int32_t ReturnType;
    typedef int32_t & OutParamType;
    static const BFFVariable::VarType VarType = BFFVariable::VarType::VAR_INT;
};

// String
// ---------------------------------------------------------------
struct String
{
    typedef AString Type;
    typedef const AString & ReturnType;
    typedef AString & OutParamType;
    static const BFFVariable::VarType VarType = BFFVariable::VarType::VAR_STRING;
};

// ArrayOfString
// ---------------------------------------------------------------
struct ArrayOfString
{
    typedef Array<AString> Type;
    typedef const Array<AString> & ReturnType;
    typedef Array<AString> & OutParamType;
    //static const BFFVariable::VarType VarType = BFFVariable::VarType::VAR_ARRAY_OF_STRINGS;
};

// Is
// Normalized type checkers.
// ---------------------------------------------------------------
template < typename T, typename VarOrLiteral >
bool Is( const VarOrLiteral * token );

template <>
bool Is<Bool, BFFToken>( const BFFToken * token ) { return ( token != nullptr ) && token->IsBooelan(); }

template <>
bool Is<Number, BFFToken>( const BFFToken * token ) { return ( token != nullptr ) && token->IsNumber(); }

template <>
bool Is<String, BFFToken>( const BFFToken * token ) { return ( token != nullptr ) && token->IsString(); }

template <>
constexpr bool Is<ArrayOfString, BFFToken>( const BFFToken * /*token*/ ) { return false; } // No token represents an array of String.

template <>
bool Is<Bool, BFFVariable>( const BFFVariable * var ) { return ( var != nullptr ) && var->IsBool(); }

template <>
bool Is<Number, BFFVariable>( const BFFVariable * var ) { return ( var != nullptr ) && var->IsInt(); }

template <>
bool Is<String, BFFVariable>( const BFFVariable * var ) { return ( var != nullptr ) && var->IsString(); }

template <>
bool Is<ArrayOfString, BFFVariable>( const BFFVariable * var ) { return ( var != nullptr ) && var->IsArrayOfStrings(); }

// Get
// Normalized value getters.
// ---------------------------------------------------------------
template < typename T, typename VarOrLiteral >
typename T::ReturnType Get( const VarOrLiteral & varOrToken );

template <>
Bool::ReturnType Get<Bool, BFFToken>( const BFFToken & token ) { ASSERT( token.IsBooelan() ); return token.GetBoolean(); }

template <>
Number::ReturnType Get<Number, BFFToken>( const BFFToken & token ) { ASSERT( token.IsNumber() ); return token.GetValueInt(); }

template <>
String::ReturnType Get<String, BFFToken>( const BFFToken & token ) { ASSERT( token.IsString() ); return token.GetValueString(); }

template <>
ArrayOfString::ReturnType Get<ArrayOfString, BFFToken>( const BFFToken & /*token*/ )
{
    ASSERT( false ); // It should not be here, or something went wrong.
    static Array<AString> s_Empty;
    return s_Empty;
}

template <>
Bool::ReturnType Get<Bool, BFFVariable>( const BFFVariable & var ) { ASSERT( var.IsBool() ); return var.GetBool(); }

template <>
Number::ReturnType Get<Number, BFFVariable>( const BFFVariable & var ) { ASSERT( var.IsInt() ); return var.GetInt(); }

template <>
String::ReturnType Get<String, BFFVariable>( const BFFVariable & var ) { ASSERT( var.IsString() ); return var.GetString(); }

template <>
ArrayOfString::ReturnType Get<ArrayOfString, BFFVariable>( const BFFVariable & var ) { ASSERT( var.IsArrayOfStrings() ); return var.GetArrayOfStrings(); }

// Operand
// Since GetVar is a slightly heavy op, cache it.
// ---------------------------------------------------------------
class Operand
{
public:
    explicit Operand( const BFFToken & token, const Function * function )
        : m_Token( token )
        , m_Var( nullptr )
        , m_Valid( true )
        , m_Function( function )
    {
        if ( token.IsVariable() )
        {
            m_Var = ::GetVar( function, &token );
            if ( m_Var == nullptr )
            {
                m_Valid = false; // GetVar emitted error
            }
        }
    }
    explicit Operand( const Operand & ) = default;

    Operand( Operand && ) = delete;
    Operand & operator = ( const Operand & ) = delete;
    Operand & operator = ( Operand && ) = delete;

    #if defined(ASSERTS_ENABLED)
        bool IsValid() const { return m_Valid; }
    #endif

    template < typename T >
    bool Is() const
    {
        ASSERT( m_Valid );
        return m_Valid && ( ::Is<T>( &m_Token ) || ::Is<T>( m_Var ) );
    }

    // A handy method for checking and setting error.
    template < typename T >
    bool Ensure() const
    {
        if ( Is<T>() )
        {
            return true;
        }

        Error::Error_1050_PropertyMustBeOfType( &m_Token, m_Function, m_Var->GetName().Get(), m_Var->GetType(), T::VarType );
        return false;
    }

    template < typename T >
    typename T::ReturnType GetValue() const
    {
        ASSERT( Is<T>() );
        return ( m_Var != nullptr ) ? Get<T>( *m_Var ) : Get<T>( m_Token );
    }

protected:
    const BFFToken & m_Token;
    const BFFVariable * m_Var;
    bool m_Valid;
    const Function * m_Function;
};

static const BoolOperator sBoolOperators[] =
{
    BoolOperator::OP_EQUAL,
    BoolOperator::OP_NOT_EQUAL,
    BoolOperator::OP_AND,
    BoolOperator::OP_OR
};

// OperandOf<T>
// ------------------------------------------------------------
template < typename T >
class OperandOf : public Operand
{
public:
    explicit OperandOf( const BFFToken & token, const Function * function )
        : Operand( token, function )
    {
        if( !Is<T>() )
        {
            m_Valid = false;
        }
    }
    explicit OperandOf( const Operand & operand )
        : Operand( operand )
    {
        m_Valid = m_Valid && Is<T>();
    }
    OperandOf( const OperandOf & ) = delete;
    OperandOf( OperandOf && ) = delete;
    OperandOf & operator = ( const OperandOf & ) = delete;
    OperandOf & operator = ( OperandOf && ) = delete;

    bool EnsureValid() const { return Ensure<T>(); }
    typename T::ReturnType GetValue() const { return Operand::GetValue<T>(); }
};

// CompareBools
//------------------------------------------------------------------------------
static bool CompareBools( const Function * function, bool lhs, bool rhs, const BFFToken * opToken, BoolOperator op, bool & result )
{
    switch ( op )
    {
        // Supported
        case BoolOperator::OP_EQUAL:        result = ( lhs == rhs ); break;
        case BoolOperator::OP_NOT_EQUAL:    result = ( lhs != rhs ); break;
        case BoolOperator::OP_AND:          result = ( lhs && rhs ); break;
        case BoolOperator::OP_OR:           result = ( lhs || rhs ); break;

        // Unsupported
        case BoolOperator::OP_LESS_THAN:
        case BoolOperator::OP_LESS_THAN_OR_EQUAL:
        case BoolOperator::OP_GREATER_THAN:
        case BoolOperator::OP_GREATER_THAN_OR_EQUAL:
        case BoolOperator::OP_IN:
        case BoolOperator::OP_NOT_IN:
        {
            Error::Error_1070_UnexpectedOperator( opToken, function );
            return false;
        }
        case BoolOperator::OP_UNKNOWN:
        {
            ASSERT( false );
            return false;
        }
    }
    return true;
}

static const BoolOperator sIntOperators[] =
{
    BoolOperator::OP_EQUAL,
    BoolOperator::OP_NOT_EQUAL,
    BoolOperator::OP_LESS_THAN,
    BoolOperator::OP_LESS_THAN_OR_EQUAL,
    BoolOperator::OP_GREATER_THAN,
    BoolOperator::OP_GREATER_THAN_OR_EQUAL
};

// CompareInts
//------------------------------------------------------------------------------
static bool CompareInts( const Function * function, int32_t lhs, int32_t rhs, const BFFToken * opToken, const BoolOperator op, bool & result )
{

    switch ( op )
    {
        // Supported
        case BoolOperator::OP_EQUAL:                    result = ( lhs == rhs ); break;
        case BoolOperator::OP_NOT_EQUAL:                result = ( lhs != rhs ); break;
        case BoolOperator::OP_LESS_THAN:                result = ( lhs < rhs ); break;
        case BoolOperator::OP_LESS_THAN_OR_EQUAL:       result = ( lhs < rhs ) || ( lhs == rhs ); break;
        case BoolOperator::OP_GREATER_THAN:             result = ( lhs > rhs ); break;
        case BoolOperator::OP_GREATER_THAN_OR_EQUAL:    result = ( lhs > rhs ) || ( lhs == rhs ); break;

        // Logic error
        case BoolOperator::OP_IN:
        case BoolOperator::OP_NOT_IN:
        case BoolOperator::OP_AND:
        case BoolOperator::OP_OR:
        {
            Error::Error_1070_UnexpectedOperator( opToken, function );
            return false;
        }
        case BoolOperator::OP_UNKNOWN:
        {
            ASSERT( false );
            return false;
        }
    }
    return true;
}

static const BoolOperator sStringOperators[]
{
    BoolOperator::OP_EQUAL,
    BoolOperator::OP_NOT_EQUAL,
    BoolOperator::OP_LESS_THAN,
    BoolOperator::OP_LESS_THAN_OR_EQUAL,
    BoolOperator::OP_GREATER_THAN,
    BoolOperator::OP_GREATER_THAN_OR_EQUAL,
    BoolOperator::OP_IN,
    BoolOperator::OP_NOT_IN
};

// CompareStrings
//------------------------------------------------------------------------------
static bool CompareStrings( const Function * function, const AString & lhs, const AString & rhs, const BFFToken * opToken, const BoolOperator op, bool & result )
{

    switch ( op )
    {
        // Supported
        case BoolOperator::OP_EQUAL:                    result = ( lhs == rhs ); break;
        case BoolOperator::OP_NOT_EQUAL:                result = ( lhs != rhs ); break;
        case BoolOperator::OP_LESS_THAN:                result = ( lhs < rhs ); break;
        case BoolOperator::OP_LESS_THAN_OR_EQUAL:       result = ( lhs < rhs ) || ( lhs == rhs ); break;
        case BoolOperator::OP_GREATER_THAN:             result = ( lhs > rhs ); break;
        case BoolOperator::OP_GREATER_THAN_OR_EQUAL:    result = ( lhs > rhs ) || ( lhs == rhs ); break;

        // Logic error
        case BoolOperator::OP_IN:
        case BoolOperator::OP_NOT_IN:
        {
            ASSERT( false ); // Should be hanlded in another function.
            return false;
        }
        case BoolOperator::OP_AND:
        case BoolOperator::OP_OR:
        {
            Error::Error_1070_UnexpectedOperator( opToken, function );
            return false;
        }
        case BoolOperator::OP_UNKNOWN:
        {
            ASSERT( false );
            return false;
        }
    }
    return true;
}

// IsExpEnd
// .a == .b
//         ^end
// (.a == .b) || (.c == .d)
//          ^end of sub exp
// iter is expected pointing to its own end or a close round bracket if 'endsOnCloseBracket' is true.
//------------------------------------------------------------------------------
static bool IsExpEnd( BFFTokenRange & iter, bool endsOnCloseBracket )
{
    if ( iter.IsAtEnd() )
    {
        return true;
    }
    return endsOnCloseBracket && iter.GetCurrent()->IsRoundBracket( ')' );
}

// ExpectExpEnd
// A helper function for exp ending checking and error reporting.
//------------------------------------------------------------------------------
static bool ExpectExpEnd( const Function * function, BFFTokenRange & iter, bool endsOnCloseBracket )
{
    if ( IsExpEnd( iter, endsOnCloseBracket ) )
    {
        return true;
    }
    if ( endsOnCloseBracket )
    {
        Error::Error_1002_MatchingClosingTokenNotFound( iter.GetCurrent(), function, ')' );
    }
    else
    {
        Error::Error_1071_UnexpectedToken( iter.GetCurrent(), function );
    }
    return false;
}

// ParseOperator
// iter is expected pointing to the operator.
// consumes 1 or 2 tokens (2 for "not in").
//------------------------------------------------------------------------------
static bool ParseOperator( const Function * function,
                           BFFTokenRange & iter,
                           BoolOperator & op,
                           const BoolOperator * supportedOps = nullptr,
                           const size_t numSupportedOps = 0 )
{
    // Determine operator for more complex expression
    op = BoolOperator::OP_UNKNOWN;

    // in
    if ( iter->IsKeyword( BFF_KEYWORD_IN ) )
    {
        op = BoolOperator::OP_IN;
    }
    // not in
    else if ( iter->IsKeyword( BFF_KEYWORD_NOT ) )
    {
        iter++; // Consume additional keyword
        if ( iter->IsKeyword( BFF_KEYWORD_IN ) )
        {
            op = BoolOperator::OP_NOT_IN;
        }
    }
    // ==
    else if ( iter->IsOperator( "==" ) )
    {
        op = BoolOperator::OP_EQUAL;
    }
    // !=
    else if ( iter->IsOperator( "!=" ) )
    {
        op = BoolOperator::OP_NOT_EQUAL;
    }
    // <=
    else if ( iter->IsOperator( "<=" ) )
    {
        op = BoolOperator::OP_LESS_THAN_OR_EQUAL;
    }
    // <  (must be checked after <=)
    else if ( iter->IsOperator( "<" ) )
    {
        op = BoolOperator::OP_LESS_THAN;
    }
    // >=
    else if ( iter->IsOperator( ">=" ) )
    {
        op = BoolOperator::OP_GREATER_THAN_OR_EQUAL;
    }
    // >  (must be checked after >=)
    else if ( iter->IsOperator( ">" ) )
    {
        op = BoolOperator::OP_GREATER_THAN;
    }
    else if ( iter->IsOperator( "&&" ) )
    {
        op = BoolOperator::OP_AND;
    }
    else if ( iter->IsOperator( "||" ) )
    {
        op = BoolOperator::OP_OR;
    }

    // Handle unrecognized operator
    if ( op == BoolOperator::OP_UNKNOWN )
    {
        Error::Error_1042_UnknownOperator( iter.GetCurrent(), iter->GetValueString() );
        return false;
    }

    if ( supportedOps != nullptr )
    {
        ASSERT( numSupportedOps > 0 );
        bool found = false;
        for ( size_t i = 0; i < numSupportedOps; ++i )
        {
            if ( supportedOps[ i ] == op )
            {
                found = true;
                break;
            }
        }
        if ( !found )
        {
            Error::Error_1070_UnexpectedOperator( iter.GetCurrent(), function );
            return false;
        }
    }

    iter++; // consume keyword or operator
    return true;
}

// ParseStringInArray
// Forms:
//    bool-exp =
//             | string-exp in string-set
//             | string-exp not in string-set
//    string-exp = string-literal | string-variable
//    string-set = string-set-variable
//
// iter is expected pointing to the rhs.
//------------------------------------------------------------------------------
static bool ParseStringInArray( const Function * function, const OperandOf<String> & lhs, BoolOperator op, BFFTokenRange & iter, bool & expResult )
{
    ASSERT( lhs.IsValid() );
    ASSERT( ( op == BoolOperator::OP_IN ) || ( op == BoolOperator::OP_NOT_IN ) );

    const BFFToken * rhsToken = iter.GetCurrent();
    iter++;

    OperandOf<ArrayOfString> rhs( *rhsToken, function );
    const Array< AString > & rhsArray = rhs.GetValue();

    const AString & lhsValue = lhs.GetValue();
    bool conditionSuccess = ( rhsArray.Find( lhsValue ) != nullptr );

    if ( op == BoolOperator::OP_NOT_IN )
    {
        conditionSuccess = !conditionSuccess;
    }

    expResult = conditionSuccess;
    return true;
}

// ParseStringArrayInArray
// Forms:
//    bool-exp =
//             | string-set in string-set
//             | string-set not in string-set
//    string-set = string-set-variable
//
// iter is expected pointing to the operator.
//------------------------------------------------------------------------------
static bool ParseStringArrayInArray( const Function * function, const OperandOf<ArrayOfString> & lhs, BFFTokenRange & iter, bool & expResult )
{
    ASSERT( lhs.IsValid() );

    const BFFToken * opToken = iter.GetCurrent();
    BoolOperator op = BoolOperator::OP_UNKNOWN;
    if ( !ParseOperator( function, iter, op ) )
    {
        return false;
    }
    if ( ( op != BoolOperator::OP_IN ) && ( op != BoolOperator::OP_NOT_IN ) )
    {
        Error::Error_1070_UnexpectedOperator( opToken, function );
        return false;
    }

    const BFFToken * rhsToken = iter.GetCurrent();
    iter++;

    const OperandOf<ArrayOfString> rhs( *rhsToken, function );

    bool conditionSuccess = false;

    const Array<AString> & lhsArray = lhs.GetValue();
    const Array<AString> & rhsArray = rhs.GetValue();

    // Is any string in array?
    for ( const AString & testStr : lhsArray )
    {
        if ( rhsArray.Find( testStr ) )
        {
            conditionSuccess = true;
            break;
        }
    }

    if ( op == BoolOperator::OP_NOT_IN )
    {
        conditionSuccess = !conditionSuccess;
    }

    expResult = conditionSuccess;
    return true;
}

// ParseUnaryBooleanOperand
// Forms:
//    bool-exp = true | false
//             | (bool-exp)
//             | bool-variable
//             | ! bool-exp
//
// 'iter' is expected pointing to the operand.
//------------------------------------------------------------------------------
static bool ParseUnaryBooleanOperand( const Function * function, BFFTokenRange & iter, bool & expResult )
{
    switch ( iter.GetCurrent()->GetType() )
    {
        case BFFTokenType::Boolean:        // true or false
        case BFFTokenType::Variable:       // .Var or ^Var
        {
            OperandOf<Bool> operand( *iter.GetCurrent(), function );

            if ( !operand.EnsureValid() )
            {
                return false;
            }
            expResult = operand.GetValue();
            iter++;
            return true;
        }
        case BFFTokenType::Operator:       // = += > ! == etc
        {
            return ParseUnaryBooleanExp( function, iter, expResult );
        }
        case BFFTokenType::RoundBracket:   // ( or )
        {
            return ParseSubBooleanExp( function, iter, expResult );
        }
        default:
        {
            break;
        }
    }

    Error::Error_1071_UnexpectedToken( iter.GetCurrent(), function );
    return false;
}

// ParseUnaryBooleanExp
// Forms:
//    bool-exp = true | false
//             | (bool-exp)
//             | bool-variable
//             | ! bool-exp
//
// 'iter' is expected pointing to the operator.
//------------------------------------------------------------------------------
static bool ParseUnaryBooleanExp( const Function * function, BFFTokenRange & iter, bool & expResult )
{
    if ( !iter->IsOperator( "!" ) )
    {
        Error::Error_1070_UnexpectedOperator( iter.GetCurrent(), function );
        return false;
    }

    iter++;
    bool subResult;
    if ( !ParseUnaryBooleanOperand( function, iter, subResult ) )
    {
        return false;
    }
    expResult = !subResult;
    return true;
}

// ParseBinaryBooleanExp
// Forms:
//    bool-exp = true | false
//             | (bool-exp)
//             | bool-variable
//             | ! bool-exp
//             | bool-exp ==/!=/&&/|| bool-exp
//
// iter is expected pointing to the operator.
//------------------------------------------------------------------------------
static bool ParseBinaryBooleanExp( const Function * function, bool lhs, BFFTokenRange & iter, bool & expResult )
{
    const BFFToken * opToken = iter.GetCurrent();
    BoolOperator op = BoolOperator::OP_UNKNOWN;
    if ( !ParseOperator( function, iter, op, sBoolOperators, ARRAY_SIZE(sBoolOperators) ) )
    {
        return false;
    }

    bool rhs;
    switch ( iter.GetCurrent()->GetType() )
    {
        case BFFTokenType::Boolean:         // true, false
        case BFFTokenType::Variable:       // .Var or ^Var
        {
            OperandOf<Bool> rhsOperand( *iter.GetCurrent(), function );
            if ( !rhsOperand.EnsureValid() )
            {
                return false;
            }
            iter++; // Consume the current token.
            rhs = rhsOperand.GetValue();
            break;
        }
        case BFFTokenType::Operator:
        {
            if ( !ParseUnaryBooleanExp( function, iter, rhs ) )
            {
                return false;
            }
            break;
        }
        case BFFTokenType::RoundBracket:
        {
            if ( !ParseSubBooleanExp( function, iter, rhs ) )
            {
                return false;
            }
            break;
        }
        default:
        {
            Error::Error_1071_UnexpectedToken( iter.GetCurrent(), function );
            return false;
        }
    }
    return CompareBools( function, lhs, rhs, opToken, op, expResult );
}

// ParseIntComparisonExp
// Forms:
//    bool-exp = 
//             | number-exp ==/!=/>/>=/</<= number-exp
//    number-exp = number-literal | number-variable
//
// iter is expected pointing to the operator.
//------------------------------------------------------------------------------
static bool ParseIntComparisonExp( const Function * function, const OperandOf<Number> & lhs, BFFTokenRange & iter, bool & expResult )
{
    ASSERT( lhs.IsValid() );

    const BFFToken * opToken = iter.GetCurrent();
    BoolOperator op = BoolOperator::OP_UNKNOWN;
    if ( !ParseOperator( function, iter, op, sIntOperators, ARRAY_SIZE( sIntOperators ) ) )
    {
        return false;
    }

    // TODO: Support literal int arrays.
    OperandOf<Number> rhs( *iter.GetCurrent(), function );
    if ( !rhs.EnsureValid() )
    {
        return false;
    }
    iter++; // Consume rhs.
    return CompareInts( function, lhs.GetValue(), rhs.GetValue(), opToken, op, expResult );
}

// ParseStringComparisonExp
// Forms:
//    bool-exp = 
//             | string-exp ==/!= string-exp
//             | string-exp in string-set
//             | string-exp not in string-set
//    string-exp = string-literal | string-variable
//    string-set = string-set-variable
//
// iter is expected pointing to the operator.
//------------------------------------------------------------------------------
static bool ParseStringComparisonExp( const Function * function, const OperandOf<String> & lhs, BFFTokenRange & iter, bool & expResult )
{
    ASSERT( lhs.IsValid() );

    const BFFToken * opToken = iter.GetCurrent();
    BoolOperator op = BoolOperator::OP_UNKNOWN;
    if ( !ParseOperator( function, iter, op, sStringOperators, ARRAY_SIZE( sStringOperators ) ) )
    {
        return false;
    }

    // TODO: Support literal string arrays.
    switch ( iter.GetCurrent()->GetType() )
    {
        case BFFTokenType::String:         // "Hello"
        case BFFTokenType::Variable:       // .Var or ^Var
        {
            if ( ( op == BoolOperator::OP_IN ) || ( op == BoolOperator::OP_NOT_IN ) )
            {
                return ParseStringInArray( function, lhs, op, iter, expResult );
            }

            OperandOf<String> rhs(*iter.GetCurrent(), function);
            if ( !rhs.EnsureValid() )
            {
                return false;
            }
            iter++;
            return CompareStrings( function, lhs.GetValue(), rhs.GetValue(), opToken, op, expResult );
        }
        default:
        {
            Error::Error_1071_UnexpectedToken( iter.GetCurrent(), function );
            return false;
        }
    }
}

// ParseSubBooleanExp
// Forms:
//    bool-exp = (bool-exp)
//
// 'iter' is expected pointing to the opening round bracket.
//------------------------------------------------------------------------------
static bool ParseSubBooleanExp( const Function * function, BFFTokenRange & iter, bool & expResult )
{
    ASSERT( iter->IsRoundBracket( '(' ) );
    iter++;

    if ( !ParseBooleanExp( function, iter, true, expResult ) )
    {
        return false;
    }
    if ( !iter->IsRoundBracket( ')' ) )
    {
        Error::Error_1002_MatchingClosingTokenNotFound( iter.GetCurrent(), function, ')' );
        return false;
    }
    iter++;
    return true;
}

// ParseBooleanExp
// Forms:
//    bool-exp = true | false
//             | (bool-exp)
//             | bool-variable
//             | ! bool-exp
//             | bool-exp ==/!=/&&/|| bool-exp
//             | number-exp ==/!=/>/>=/</<= number-exp
//             | string-exp ==/!= string-exp
//             | string-exp in string-set
//             | string-exp not in string-set
//             | string-set in string-set
//             | string-set not in string-set
//    number-exp = number-literal | number-variable
//    string-exp = string-literal | string-variable
//    string-set = string-set-variable
//------------------------------------------------------------------------------
static bool ParseBooleanExp( const Function * function, BFFTokenRange & iter, bool endsOnCloseBracket, bool & expResult )
{
    const BFFToken * const token = iter.GetCurrent();

    switch ( token->GetType() )
    {
        case BFFTokenType::Variable:       // .Var or ^Var
        case BFFTokenType::Boolean:        // true or false
        case BFFTokenType::Number:         // 12, 56, -102 etc
        case BFFTokenType::String:         // "Hello"
        {
            Operand operand( *token, function );
            iter++;
            if ( operand.Is<Bool>() )
            {
                // Form: bool-value = bool-literal | bool-variable
                // Expect: bool-value [ ||/&& bool-exp ] [ ||/&& bool-exp ] ...
                expResult = operand.GetValue<Bool>();
                while ( !IsExpEnd( iter, endsOnCloseBracket ) )
                {
                    if ( !ParseBinaryBooleanExp( function, expResult, iter, expResult ) )
                    {
                        return false;
                    }
                }
                return true;
            }
            else if ( operand.Is<Number>() )
            {
                // Form: number-value = number-literal | number-variable
                // Expect: number-value, number-value <op> number-value
                if ( !ParseIntComparisonExp( function, OperandOf<Number>( operand ), iter, expResult ) )
                {
                    return false;
                }
            }
            else if ( operand.Is<String>() )
            {
                // Form: string-value = string-literal | string-variable
                // Expect: string-value, string-value <op> string-value
                if ( !ParseStringComparisonExp( function, OperandOf<String>( operand ), iter, expResult ) )
                {
                    return false;
                }
            }
            else if ( operand.Is<ArrayOfString>() )
            {
                // Expect: array-of-string-variable [not] in array-of-string-variable
                if ( !ParseStringArrayInArray( function, OperandOf<ArrayOfString>( operand ), iter, expResult ) )
                {
                    return false;
                }
            }
            else {
                // TODO: Update the following error.
                // Error::Error_1008_VariableOfWrongType was one of the options but we expected multiple types here,
                // where the 1008 function accept a single expected type.
                Error::Error_1071_UnexpectedToken( iter.GetCurrent(), function );
                return false;
            }

            return ExpectExpEnd( function, iter, endsOnCloseBracket );
        }
        case BFFTokenType::Operator:       // = += > ! == etc
        {
            // Expect: !bool-exp [ ||/&& bool-exp ] [ ||/&& bool-exp ] ...
            if ( !ParseUnaryBooleanExp( function, iter, expResult ) )
            {
                return false;
            }
            while ( !IsExpEnd( iter, endsOnCloseBracket ) )
            {
                if ( !ParseBinaryBooleanExp( function, expResult, iter, expResult ) )
                {
                    return false;
                }
            }
            return true;
        }
        case BFFTokenType::RoundBracket:   // ( or )
        {
            // Expect: (bool-exp) [ ||/&& bool-exp ] [ ||/&& bool-exp ] ...
            if ( !ParseSubBooleanExp( function, iter, expResult ) )
            {
                return false;
            }

            while ( !IsExpEnd( iter, true ) )
            {
                if ( !ParseBinaryBooleanExp( function, expResult, iter, expResult ) )
                {
                    return false;
                }
            }
            return true;
        }
        default:
        {
            break;
        }
    }

    Error::Error_1071_UnexpectedToken( iter.GetCurrent(), function );
    return false;
}

}

// Parse
//------------------------------------------------------------------------------
/*static*/ bool BFFBooleanExpParser::Parse( const Function * function, const BFFTokenRange & range, bool & expResult )
{
    BFFTokenRange iter( range );
    return ParseBooleanExp( function, iter, false, expResult );
}

//------------------------------------------------------------------------------
