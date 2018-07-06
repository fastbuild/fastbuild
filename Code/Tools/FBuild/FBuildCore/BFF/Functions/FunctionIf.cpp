// FunctionIf
//------------------------------------------------------------------------------

// Includes
//------------------------------------------------------------------------------
#include "Tools/FBuild/FBuildCore/PrecompiledHeader.h"

#include "FunctionIf.h"

#include "Tools/FBuild/FBuildCore/BFF/BFFIterator.h"
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
    bool conditionSuccess = false;

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
    const BFFIterator testVarNameBegin( pos );
    const BFFVariable * testVar = GetVar( pos );
    if ( testVar == nullptr )
    {
        return false; // GetVar will have emitted error
    }

    // at end?
    if ( pos.GetCurrent() == functionHeaderStopToken->GetCurrent() )
    {
        return HandleSimpleBooleanExpression( nodeGraph, functionBodyStartToken, functionBodyStopToken, testVarNameBegin, testVar, negated );
    }

    // check for optional "not" token
    bool foundNot = false;
    bool foundIn = false;
    bool foundEquals = false;
    bool foundNotEquals = false;
    if ( negated == false )
    {
        foundNot = pos.ParseExactString( "not" );
        if ( foundNot )
        {
            pos.SkipWhiteSpace();
        }

        // check for "in"
        foundIn = pos.ParseExactString( "in" );
        if ( foundIn )
        {
            pos.SkipWhiteSpace();
        }

        // Check for == and !=
        if ( ( foundNot == false ) && ( foundIn == false ) )
        {
            foundEquals = pos.ParseExactString( "==" );
            if ( !foundEquals )
            {
                foundNotEquals = pos.ParseExactString( "!=" );
            }
        }
        if ( foundEquals || foundNotEquals )
        {
            pos.SkipWhiteSpace();
        }
    }

    const BFFIterator listVarNameBegin = pos;
    const BFFVariable * listVar = GetVar( pos );
    if ( listVar == nullptr )
    {
        return false; // GetVar will have emitted an error
    }

    if ( foundEquals || foundNotEquals )
    {
        return HandleSimpleCompare( nodeGraph, functionBodyStartToken, functionBodyStopToken, testVarNameBegin, testVar, listVarNameBegin, listVar, foundNotEquals );
    }

    // it can be of any supported type
    if ( listVar->IsArrayOfStrings() == false )
    {
        Error::Error_1050_PropertyMustBeOfType( listVarNameBegin, this, listVar->GetName().Get(), listVar->GetType(), BFFVariable::VAR_ARRAY_OF_STRINGS );
        return false;
    }

    const Array< AString > & listArray = listVar->GetArrayOfStrings();

    // now iterate over the first set of strings to see if any match the second set of strings

    if ( testVar->IsString() )
    {
        // Is string in array?
        conditionSuccess = ( listArray.Find( testVar->GetString() ) != nullptr );
    }
    else if ( testVar->IsArrayOfStrings() )
    {
        // Is any string in array?
        for ( const AString & testStr : testVar->GetArrayOfStrings() )
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
        Error::Error_1050_PropertyMustBeOfType( testVarNameBegin, this, listVar->GetName().Get(), listVar->GetType(), BFFVariable::VAR_ARRAY_OF_STRINGS, BFFVariable::VAR_STRING );
        return false;
    }

    if ( foundNot )
    {
        conditionSuccess = !conditionSuccess;
    }
    ASSERT( negated == false ); // Should have reported error (! not allowed for "in")

    pos.SkipWhiteSpace();

    // If not at the end of the header, there are extraneous tokens
    if ( pos.GetCurrent() != functionHeaderStopToken->GetCurrent() )
    {
        Error::Error_1002_MatchingClosingTokenNotFound( *functionHeaderStartToken, this, ')' );
        return false;
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
                                      const bool negated ) const
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
        result = ( lhsVar->GetString() == rhsVar->GetString() );
    }
    else if ( lhsVar->GetType() == BFFVariable::VAR_BOOL )
    {
        result = ( lhsVar->GetBool() == rhsVar->GetBool() );
    }
    else
    {
        Error::Error_1050_PropertyMustBeOfType( lhsVarIter, this, lhsVar->GetName().Get(), lhsVar->GetType(), BFFVariable::VAR_STRING, BFFVariable::VAR_BOOL );
        return false;
    }

    // Handle negation
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
