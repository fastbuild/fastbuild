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

//------------------------------------------------------------------------------
/*virtual*/ bool FunctionIf::ParseFunction(
                    NodeGraph & nodeGraph,
                    const BFFIterator & functionNameStart,
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

    if ( ( *pos != BFFParser::BFF_DECLARE_VAR_INTERNAL ) &&
         ( *pos != BFFParser::BFF_DECLARE_VAR_PARENT ) )
    {
        Error::Error_1200_ExpectedVar( pos, this );
        return false;
    }

    const BFFIterator testVarNameBegin = pos;
    AStackString< BFFParser::MAX_VARIABLE_NAME_LENGTH > testVarName;
    bool testVarParentScope = false;
    if ( BFFParser::ParseVariableName( pos, testVarName, testVarParentScope ) == false )
    {
        return false;
    }
        
    const BFFVariable * testVar = nullptr;
    BFFStackFrame * const testVarFrame = ( testVarParentScope )
        ? BFFStackFrame::GetParentDeclaration( testVarName, BFFStackFrame::GetCurrent()->GetParent(), testVar )
        : nullptr;

    if ( false == testVarParentScope )
    {
        testVar = BFFStackFrame::GetVar( testVarName, nullptr );
    }

    if ( ( testVarParentScope && ( nullptr == testVarFrame ) ) || ( testVar == nullptr ) )
    {
        Error::Error_1009_UnknownVariable( testVarNameBegin, this, testVarName );
        return false;
    }

    pos.SkipWhiteSpace();

    // check for optional "not" token
    const bool foundNot = pos.ParseExactString( "not" );
    if ( foundNot )
    {
        pos.SkipWhiteSpace();
    }

    // check for required "in" token
    if ( pos.ParseExactString( "in" ) == false )
    {
        Error::Error_1201_MissingIn( pos, this );
        return false;
    }
    pos.SkipWhiteSpace();

    if ( ( *pos != BFFParser::BFF_DECLARE_VAR_INTERNAL ) &&
         ( *pos != BFFParser::BFF_DECLARE_VAR_PARENT ) /* tolerant with parent vars */ )
    {
        Error::Error_1202_ExpectedVarFollowingIn( pos, this );
        return false;
    }

    const BFFIterator listVarNameBegin( pos );
    AStackString< BFFParser::MAX_VARIABLE_NAME_LENGTH > listVarName;
    bool listParentScope = false;
    if ( BFFParser::ParseVariableName( pos, listVarName, listParentScope ) == false )
    {
        return false;
    }

    const BFFVariable * listVar = nullptr;
    BFFStackFrame * const arrayFrame = ( listParentScope )
        ? BFFStackFrame::GetParentDeclaration( listVarName, BFFStackFrame::GetCurrent()->GetParent(), listVar )
        : nullptr;

    if ( false == listParentScope )
    {
        listVar = BFFStackFrame::GetVar( listVarName, nullptr );
    }

    if ( ( listParentScope && ( nullptr == arrayFrame ) ) || ( listVar == nullptr ) )
    {
        Error::Error_1009_UnknownVariable( listVarNameBegin, this, listVarName );
        return false;
    }

    // it can be of any supported type
    if ( listVar->IsArrayOfStrings() == false )
    {
        Error::Error_1050_PropertyMustBeOfType( listVarNameBegin, this, listVarName.Get(), listVar->GetType(), BFFVariable::VAR_ARRAY_OF_STRINGS );
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
        Error::Error_1050_PropertyMustBeOfType( testVarNameBegin, this, listVarName.Get(), listVar->GetType(), BFFVariable::VAR_ARRAY_OF_STRINGS, BFFVariable::VAR_STRING );
        return false;
    }

    if ( foundNot )
    {
        conditionSuccess = !conditionSuccess;
    }

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
        if ( subParser.Parse( subIter ) == false )
        {
            return false;
        }

        return Commit( nodeGraph, functionNameStart );
    }
    else
    {
        return true;
    }
}

//------------------------------------------------------------------------------
