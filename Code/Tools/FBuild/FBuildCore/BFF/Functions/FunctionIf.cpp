// FunctionIf
//------------------------------------------------------------------------------

// Includes
//------------------------------------------------------------------------------
#include "FunctionIf.h"

#include "Tools/FBuild/FBuildCore/BFF/BFFKeywords.h"
#include "Tools/FBuild/FBuildCore/BFF/BFFParser.h"
#include "Tools/FBuild/FBuildCore/BFF/BFFStackFrame.h"
#include "Tools/FBuild/FBuildCore/BFF/BFFVariable.h"
#include "Tools/FBuild/FBuildCore/BFF/Tokenizer/BFFTokenRange.h"
#include "Tools/FBuild/FBuildCore/FLog.h"

#include "Core/Strings/AStackString.h"

// CONSTRUCTOR
//------------------------------------------------------------------------------
FunctionIf::FunctionIf()
: Function( "If" )
{
}

// AcceptsHeader
//------------------------------------------------------------------------------
/*virtual*/ bool FunctionIf::AcceptsHeader() const
{
    return true;
}

// NeedsHeader
//------------------------------------------------------------------------------
/*virtual*/ bool FunctionIf::NeedsHeader() const
{
    return true;
}

// ParseFunction
//------------------------------------------------------------------------------
/*virtual*/ bool FunctionIf::ParseFunction( NodeGraph & /*nodeGraph*/,
                                            BFFParser & parser,
                                            const BFFToken * /*functionNameStart*/,
                                            const BFFTokenRange & headerRange,
                                            const BFFTokenRange & bodyRange ) const
{
    // Iterate the args
    BFFTokenRange iter( headerRange );

    if ( iter.IsAtEnd() )
    {
        Error::Error_1007_ExpectedVariable( iter.GetCurrent(), this );
        return false;
    }

    // Handle single boolean expression
    // - Check for negation
    bool negated = false;
    if ( iter->IsOperator( "!" ) )
    {
        negated = true;
        iter++;
    }

    if ( iter.IsAtEnd() )
    {
        Error::Error_1007_ExpectedVariable( iter.GetCurrent(), this );
        return false;
    }

    // Get first variable
    const BFFToken * lhsToken = iter.GetCurrent();
    const BFFVariable * lhsVar = GetVar( lhsToken );
    if ( lhsVar == nullptr )
    {
        return false; // GetVar will have emitted error
    }
    iter++; // Consume lhs

    // at end?
    if ( iter.IsAtEnd() )
    {
        return HandleSimpleBooleanExpression( parser, headerRange, bodyRange, lhsToken, lhsVar, negated );
    }

    // At end with negation previously seen means extraneous text
    if ( negated )
    {
        Error::Error_1002_MatchingClosingTokenNotFound( iter.GetCurrent(), this, ')' ); // TODO:C Better error
        return false;
    }

    // Determine operator for more complex expression
    Operator op = Operator::OP_UNKNOWN;
    const BFFToken * opToken = iter.GetCurrent();

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

    // Handle unrecognized operator
    if ( op == Operator::OP_UNKNOWN )
    {
        Error::Error_1042_UnknownOperator( iter.GetCurrent(), iter->GetValueString() );
        return false;
    }
    iter++; // consume keyword or operator

    // Get rhs
    const BFFToken * rhsToken = iter.GetCurrent();
    const BFFVariable * rhsVar = GetVar( rhsToken );
    if ( rhsVar == nullptr )
    {
        return false; // GetVar will have emitted an error
    }
    iter++; // consume rhs

    // Make sure there are no extraneous tokens
    if ( iter.IsAtEnd() == false )
    {
        Error::Error_1002_MatchingClosingTokenNotFound( iter.GetCurrent(), this, ')' ); // TODO:C Better error
        return false;
    }

    bool result;
    if ( ( op == Operator::OP_IN ) || ( op == Operator::OP_NOT_IN ) )
    {
        result = HandleIn( parser, bodyRange, lhsToken, lhsVar, rhsToken, rhsVar, op);
    }
    else
    {
        result = HandleSimpleCompare( parser, bodyRange, lhsToken, lhsVar, rhsToken, rhsVar, opToken, op );
    }

    // If not at the end of the header, there are extraneous tokens
    if ( iter.IsAtEnd() == false )
    {
        Error::Error_1002_MatchingClosingTokenNotFound( iter.GetCurrent(), this, ')' ); // TODO:C Better error
        return false;
    }

    return result;
}

// HandleIn
//------------------------------------------------------------------------------
bool FunctionIf::HandleIn( BFFParser & parser,
                           const BFFTokenRange & bodyRange,
                           const BFFToken * lhsVarIter,
                           const BFFVariable * lhsVar,
                           const BFFToken * rhsVarIter,
                           const BFFVariable * rhsVar,
                           const Operator op ) const
{
    ASSERT( ( op == Operator::OP_IN ) || ( op == Operator::OP_NOT_IN ) );

    // it can be of any supported type
    if ( rhsVar->IsArrayOfStrings() == false )
    {
        Error::Error_1050_PropertyMustBeOfType( rhsVarIter, this, rhsVar->GetName().Get(), rhsVar->GetType(), BFFVariable::VAR_ARRAY_OF_STRINGS );
        return false;
    }

    const Array< AString > & listArray = rhsVar->GetArrayOfStrings();

    bool conditionSuccess = false;

    // now iterate over the first set of strings to see if any match the second set of strings

    if ( lhsVar->IsString() )
    {
        // Is string in array?
        conditionSuccess = ( listArray.Find( lhsVar->GetString() ) != nullptr );
    }
    else if ( lhsVar->IsArrayOfStrings() )
    {
        // Is any string in array?
        for ( const AString & testStr : lhsVar->GetArrayOfStrings() )
        {
            if ( listArray.Find( testStr ) )
            {
                conditionSuccess = true;
                break;
            }
        }
    }
    else
    {
        Error::Error_1050_PropertyMustBeOfType( lhsVarIter, this, lhsVar->GetName().Get(), lhsVar->GetType(), BFFVariable::VAR_ARRAY_OF_STRINGS, BFFVariable::VAR_STRING );
        return false;
    }

    if ( op == Operator::OP_NOT_IN )
    {
        conditionSuccess = !conditionSuccess;
    }

    // Parse if condition met
    if ( conditionSuccess )
    {
        // parse the function body
        BFFTokenRange range( bodyRange );
        return parser.Parse( range );
    }
    else
    {
        return true;
    }
}

// HandleSimpleBooleanExpression
//------------------------------------------------------------------------------
bool FunctionIf::HandleSimpleBooleanExpression( BFFParser & parser,
                                                const BFFTokenRange & /*headerRange*/,
                                                const BFFTokenRange & bodyRange,
                                                const BFFToken * testVarIter,
                                                const BFFVariable * testVar,
                                                const bool negated ) const
{
    // Check type
    if ( testVar->IsBool() == false )
    {
        Error::Error_1050_PropertyMustBeOfType( testVarIter, this, testVar->GetName().Get(), testVar->GetType(), BFFVariable::VAR_BOOL );
        return false;
    }

    // Get result
    bool result = testVar->GetBool();
    result = negated ? (!result) : result;

    // Parse body if condition is true
    if ( result )
    {
        // parse the function body
        BFFTokenRange range( bodyRange );
        return parser.Parse( range );
    }
    return true;
}

// HandleSimpleCompare
//------------------------------------------------------------------------------
bool FunctionIf::HandleSimpleCompare( BFFParser & parser,
                                      const BFFTokenRange & bodyRange,
                                      const BFFToken * lhsVarIter,
                                      const BFFVariable * lhsVar,
                                      const BFFToken * rhsVarIter,
                                      const BFFVariable * rhsVar,
                                      const BFFToken * operatorIter,
                                      const Operator op ) const
{
    // Check types are equal
    if ( lhsVar->GetType() != rhsVar->GetType() )
    {
        // Report that rhs is not of same type as lhs
        Error::Error_1050_PropertyMustBeOfType( rhsVarIter, this, rhsVar->GetName().Get(), rhsVar->GetType(), lhsVar->GetType() );
        return false;
    }

    // Check types are supported
    bool result = false;
    if ( lhsVar->GetType() == BFFVariable::VAR_STRING )
    {
        const AString & lhs = lhsVar->GetString();
        const AString & rhs = rhsVar->GetString();
        switch ( op )
        {
            // Supported
            case Operator::OP_EQUAL:                    result = ( lhs == rhs ); break;
            case Operator::OP_NOT_EQUAL:                result = ( lhs != rhs ); break;
            case Operator::OP_LESS_THAN:                result = ( lhs < rhs ); break;
            case Operator::OP_LESS_THAN_OR_EQUAL:       result = ( lhs < rhs ) || ( lhs == rhs ); break;
            case Operator::OP_GREATER_THAN:             result = ( lhs > rhs ); break;
            case Operator::OP_GREATER_THAN_OR_EQUAL:    result = ( lhs > rhs ) || ( lhs == rhs ); break;

            // Logic error
            case Operator::OP_IN:
            case Operator::OP_NOT_IN:
            case Operator::OP_UNKNOWN: ASSERT( false ); // Should be impossible
        }
    }
    else if ( lhsVar->GetType() == BFFVariable::VAR_BOOL )
    {
        const bool lhs = lhsVar->GetBool();
        const bool rhs = rhsVar->GetBool();
        switch ( op )
        {
            // Supported
            case Operator::OP_EQUAL:                    result = ( lhs == rhs ); break;
            case Operator::OP_NOT_EQUAL:                result = ( lhs != rhs ); break;

            // Unsupported
            case Operator::OP_LESS_THAN:
            case Operator::OP_LESS_THAN_OR_EQUAL:
            case Operator::OP_GREATER_THAN:
            case Operator::OP_GREATER_THAN_OR_EQUAL:
            {
                Error::Error_1034_OperationNotSupported( rhsVarIter, lhsVar->GetType(), rhsVar->GetType(), operatorIter );
                return false;
            }

            // Logic error
            case Operator::OP_IN:
            case Operator::OP_NOT_IN:
            case Operator::OP_UNKNOWN: ASSERT( false ); // Should be impossible
        }
    }
    else if ( lhsVar->GetType() == BFFVariable::VAR_INT )
    {
        const int32_t & lhs = lhsVar->GetInt();
        const int32_t & rhs = rhsVar->GetInt();
        switch ( op )
        {
            // Supported
            case Operator::OP_EQUAL:                    result = ( lhs == rhs ); break;
            case Operator::OP_NOT_EQUAL:                result = ( lhs != rhs ); break;
            case Operator::OP_LESS_THAN:                result = ( lhs < rhs ); break;
            case Operator::OP_LESS_THAN_OR_EQUAL:       result = ( lhs < rhs ) || ( lhs == rhs ); break;
            case Operator::OP_GREATER_THAN:             result = ( lhs > rhs ); break;
            case Operator::OP_GREATER_THAN_OR_EQUAL:    result = ( lhs > rhs ) || ( lhs == rhs ); break;

            // Logic error
            case Operator::OP_IN:
            case Operator::OP_NOT_IN:
            case Operator::OP_UNKNOWN: ASSERT( false ); // Should be impossible
        }
    }
    else
    {
        Error::Error_1050_PropertyMustBeOfType( lhsVarIter, this, lhsVar->GetName().Get(), lhsVar->GetType(), BFFVariable::VAR_STRING, BFFVariable::VAR_BOOL );
        return false;
    }

    // Parse body if condition is true
    if ( result )
    {
        // parse the function body
        BFFTokenRange range( bodyRange );
        return parser.Parse( range );
    }
    return true;
}

// GetVar
//------------------------------------------------------------------------------
const BFFVariable * FunctionIf::GetVar( const BFFToken * token ) const
{
    if ( token->IsVariable() == false )
    {
        Error::Error_1200_ExpectedVar( token, this );
        return nullptr;
    }

    AStackString< BFFParser::MAX_VARIABLE_NAME_LENGTH > varName;
    bool varParentScope = false;
    if ( BFFParser::ParseVariableName( token, varName, varParentScope ) == false )
    {
        return nullptr; // ParseVariableName will have emitted error
    }

    const BFFVariable * var = nullptr;
    const BFFStackFrame * varFrame = ( varParentScope ) ? BFFStackFrame::GetParentDeclaration( varName, BFFStackFrame::GetCurrent()->GetParent(), var )
                                                        : nullptr;
    if ( false == varParentScope )
    {
        var = BFFStackFrame::GetVar( varName, nullptr );
    }

    if ( ( varParentScope && ( nullptr == varFrame ) ) || ( var == nullptr ) )
    {
        Error::Error_1009_UnknownVariable( token, this, varName );
        return nullptr;
    }

    return var;
}

//------------------------------------------------------------------------------
