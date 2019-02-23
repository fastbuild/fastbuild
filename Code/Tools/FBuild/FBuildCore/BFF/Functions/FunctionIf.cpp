// FunctionIf
//------------------------------------------------------------------------------

// Includes
//------------------------------------------------------------------------------
#include "FunctionIf.h"

#include "Tools/FBuild/FBuildCore/BFF/BFFIterator.h"
#include "Tools/FBuild/FBuildCore/BFF/BFFKeywords.h"
#include "Tools/FBuild/FBuildCore/BFF/BFFParser.h"
#include "Tools/FBuild/FBuildCore/BFF/BFFStackFrame.h"
#include "Tools/FBuild/FBuildCore/BFF/BFFVariable.h"
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
/*virtual*/ bool FunctionIf::ParseFunction(
                    NodeGraph & nodeGraph,
                    const BFFIterator & /*functionNameStart*/,
                    const BFFIterator * functionBodyStartToken,
                    const BFFIterator * functionBodyStopToken,
                    const BFFIterator * functionHeaderStartToken,
                    const BFFIterator * functionHeaderStopToken ) const
{
    // parse it all out
    BFFIterator pos( *functionHeaderStartToken );
    pos++; // skip opening token
    pos.SkipWhiteSpace();

    // Check for prefix not character
    bool negated = false;
    if ( *pos == '!' )
    {
        negated = true;
        pos++;
    }

    // Get first variable
    const BFFIterator lhsVarNameBegin( pos );
    const BFFVariable * lhsVar = GetVar( pos );
    if ( lhsVar == nullptr )
    {
        return false; // GetVar will have emitted error
    }

    // at end?
    if ( pos.GetCurrent() == functionHeaderStopToken->GetCurrent() )
    {
        return HandleSimpleBooleanExpression( nodeGraph, functionBodyStartToken, functionBodyStopToken, lhsVarNameBegin, lhsVar, negated );
    }

    // At end with negation previously seen means extraneous text
    if ( negated )
    {
        Error::Error_1002_MatchingClosingTokenNotFound( pos, this, ')' );
        return false;
    }

    // Determine operator for more complex expression
    Operator op = Operator::OP_UNKNOWN;
    const BFFIterator operatorIter( pos );

    // in
    if ( pos.ParseExactString( BFF_KEYWORD_IN ) )
    {
        op = Operator::OP_IN;
    }
    // not in
    else if ( pos.ParseExactString( BFF_KEYWORD_NOT ) )
    {
        pos.SkipWhiteSpace();
        if ( pos.ParseExactString( BFF_KEYWORD_IN ) )
        {
            op = Operator::OP_NOT_IN;
        }
    }
    // ==
    else if ( pos.ParseExactString( "==" ) )
    {
        op = Operator::OP_EQUAL;
    }
    // !=
    else if ( pos.ParseExactString( "!=" ) )
    {
        op = Operator::OP_NOT_EQUAL;
    }
    // <=
    else if ( pos.ParseExactString( "<=" ) )
    {
        op = Operator::OP_LESS_THAN_OR_EQUAL;
    }
    // <  (must be checked after <=)
    else if ( pos.ParseExactString( "<" ) )
    {
        op = Operator::OP_LESS_THAN;
    }
    // >=
    else if ( pos.ParseExactString( ">=" ) )
    {
        op = Operator::OP_GREATER_THAN_OR_EQUAL;
    }
    // >  (must be checked after >=)
    else if ( pos.ParseExactString( ">" ) )
    {
        op = Operator::OP_GREATER_THAN;
    }

    // Handle unrecognized operator
    if ( op == Operator::OP_UNKNOWN )
    {
        Error::Error_1042_UnknownOperator( pos, AString::GetEmpty() );
        return false;
    }

    pos.SkipWhiteSpace();

    const BFFIterator rhsVarNameBegin = pos;
    const BFFVariable * rhsVar = GetVar( pos );
    if ( rhsVar == nullptr )
    {
        return false; // GetVar will have emitted an error
    }

    // Make sure there are no extraneous tokens
    if ( pos.GetCurrent() != functionHeaderStopToken->GetCurrent() )
    {
        Error::Error_1002_MatchingClosingTokenNotFound(pos, this, ')');
        return false;
    }

    bool result;
    if ( ( op == Operator::OP_IN ) || ( op == Operator::OP_NOT_IN ) )
    {
        result = HandleIn( nodeGraph, functionBodyStartToken, functionBodyStopToken, lhsVarNameBegin, lhsVar, rhsVarNameBegin, rhsVar, op);
    }
    else
    {
        result = HandleSimpleCompare( nodeGraph, functionBodyStartToken, functionBodyStopToken, lhsVarNameBegin, lhsVar, rhsVarNameBegin, rhsVar, operatorIter, op );
    }

    // If not at the end of the header, there are extraneous tokens
    if ( pos.GetCurrent() != functionHeaderStopToken->GetCurrent() )
    {
        Error::Error_1002_MatchingClosingTokenNotFound( *functionHeaderStartToken, this, ')' );
        return false;
    }

    return result;
}

// HandleIn
//------------------------------------------------------------------------------
bool FunctionIf::HandleIn( NodeGraph & nodeGraph,
                           const BFFIterator * functionBodyStartToken,
                           const BFFIterator * functionBodyStopToken,
                           const BFFIterator & lhsVarIter,
                           const BFFVariable * lhsVar,
                           const BFFIterator & rhsVarIter,
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
        BFFParser subParser( nodeGraph );
        BFFIterator subIter( *functionBodyStartToken );
        subIter++; // skip opening token
        subIter.SetMax( functionBodyStopToken->GetCurrent() ); // limit to closing token
        return subParser.Parse( subIter );
    }
    else
    {
        return true;
    }
}

// HandleSimpleBooleanExpression
//------------------------------------------------------------------------------
bool FunctionIf::HandleSimpleBooleanExpression( NodeGraph & nodeGraph,
                                                const BFFIterator * functionBodyStartToken,
                                                const BFFIterator * functionBodyStopToken,
                                                const BFFIterator & testVarIter,
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
        BFFParser subParser( nodeGraph );
        BFFIterator subIter( *functionBodyStartToken );
        subIter++; // skip opening token
        subIter.SetMax( functionBodyStopToken->GetCurrent() ); // limit to closing token
        return subParser.Parse( subIter );
    }
    return true;
}


// HandleSimpleCompare
//------------------------------------------------------------------------------
bool FunctionIf::HandleSimpleCompare( NodeGraph & nodeGraph,
                                      const BFFIterator * functionBodyStartToken,
                                      const BFFIterator * functionBodyStopToken,
                                      const BFFIterator & lhsVarIter,
                                      const BFFVariable * lhsVar,
                                      const BFFIterator & rhsVarIter,
                                      const BFFVariable * rhsVar,
                                      const BFFIterator & operatorIter,
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
        BFFParser subParser( nodeGraph );
        BFFIterator subIter( *functionBodyStartToken );
        subIter++; // skip opening token
        subIter.SetMax( functionBodyStopToken->GetCurrent() ); // limit to closing token
        return subParser.Parse( subIter );
    }
    return true;
}

// GetVar
//------------------------------------------------------------------------------
const BFFVariable * FunctionIf::GetVar( BFFIterator & pos ) const
{
    if ( ( *pos != BFFParser::BFF_DECLARE_VAR_INTERNAL ) &&
         ( *pos != BFFParser::BFF_DECLARE_VAR_PARENT ) )
    {
        Error::Error_1200_ExpectedVar( pos, this );
        return nullptr;
    }

    const BFFIterator varNameBegin( pos );
    AStackString< BFFParser::MAX_VARIABLE_NAME_LENGTH > varName;
    bool varParentScope = false;
    if ( BFFParser::ParseVariableName( pos, varName, varParentScope ) == false )
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
        Error::Error_1009_UnknownVariable( varNameBegin, this, varName );
        return nullptr;
    }

    pos.SkipWhiteSpace();
    return var;
}

//------------------------------------------------------------------------------
