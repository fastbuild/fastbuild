// FunctionForEach
//------------------------------------------------------------------------------

// Includes
//------------------------------------------------------------------------------
#include "Tools/FBuild/FBuildCore/PrecompiledHeader.h"

#include "FunctionForEach.h"

#include "Tools/FBuild/FBuildCore/BFF/BFFIterator.h"
#include "Tools/FBuild/FBuildCore/BFF/BFFParser.h"
#include "Tools/FBuild/FBuildCore/BFF/BFFStackFrame.h"
#include "Tools/FBuild/FBuildCore/BFF/BFFVariable.h"
#include "Tools/FBuild/FBuildCore/FLog.h"

#include "Core/Strings/AStackString.h"

// CONSTRUCTOR
//------------------------------------------------------------------------------
FunctionForEach::FunctionForEach()
: Function( "ForEach" )
{
}

// AcceptsHeader
//------------------------------------------------------------------------------
/*virtual*/ bool FunctionForEach::AcceptsHeader() const
{
    return true;
}

// NeedsHeader
//------------------------------------------------------------------------------
/*virtual*/ bool FunctionForEach::NeedsHeader() const
{
    return true;
}

//------------------------------------------------------------------------------
/*virtual*/ bool FunctionForEach::ParseFunction(
                    NodeGraph & nodeGraph,
                    const BFFIterator & functionNameStart,
                    const BFFIterator * functionBodyStartToken,
                    const BFFIterator * functionBodyStopToken,
                    const BFFIterator * functionHeaderStartToken,
                    const BFFIterator * functionHeaderStopToken ) const
{
    // build array for each pair to loop through
    Array< AString >                localNames( 4, true );
    Array< const BFFVariable * >    arrayVars( 4, true );

    int loopLen = -1;

    // parse it all out
    BFFIterator pos( *functionHeaderStartToken );
    pos++; // skip opening token
    while ( pos < *functionHeaderStopToken )
    {
        pos.SkipWhiteSpace();
        if ( *pos != BFFParser::BFF_DECLARE_VAR_INTERNAL )
        {
            Error::Error_1200_ExpectedVar( pos, this );
            return false;
        }

        const BFFIterator arrayVarNameBegin = pos;
        AStackString< BFFParser::MAX_VARIABLE_NAME_LENGTH > localName;
        bool localParentScope = false; // always false thanks to the previous test
        if ( BFFParser::ParseVariableName( pos, localName, localParentScope ) == false )
        {
            return false;
        }
        ASSERT( false == localParentScope );

        localNames.Append( localName );

        pos.SkipWhiteSpace();

        // check for "in" token
        bool foundIn = false;
        if ( *pos == 'i' )
        {
            pos++;
            if ( *pos == 'n' )
            {
                foundIn = true;
            }
        }
        if ( foundIn == false )
        {
            Error::Error_1201_MissingIn( pos, this );
            return false;
        }
        pos++;
        pos.SkipWhiteSpace();

        if ( *pos != BFFParser::BFF_DECLARE_VAR_INTERNAL &&
             *pos != BFFParser::BFF_DECLARE_VAR_PARENT /* tolerant with parent vars */ )
        {
            Error::Error_1202_ExpectedVarFollowingIn( pos, this );
            return false;
        }

        const BFFIterator arrayNameStart( pos );
        AStackString< BFFParser::MAX_VARIABLE_NAME_LENGTH > arrayVarName;
        bool arrayParentScope = false;
        if ( BFFParser::ParseVariableName( pos, arrayVarName, arrayParentScope ) == false )
        {
            return false;
        }

        const BFFVariable * var = nullptr;
        BFFStackFrame * const arrayFrame = ( arrayParentScope )
            ? BFFStackFrame::GetParentDeclaration( arrayVarName, BFFStackFrame::GetCurrent()->GetParent(), var )
            : nullptr;

        if ( false == arrayParentScope )
        {
            var = BFFStackFrame::GetVar( arrayVarName, nullptr );
        }

        if ( ( arrayParentScope && ( nullptr == arrayFrame ) ) || ( var == nullptr ) )
        {
            Error::Error_1009_UnknownVariable( arrayNameStart, this );
            return false;
        }

        // it can be of any supported type
        if ( ( var->IsArrayOfStrings() == false ) && ( var->IsArrayOfStructs() == false ) )
        {
            Error::Error_1050_PropertyMustBeOfType( arrayVarNameBegin, this, arrayVarName.Get(), var->GetType(), BFFVariable::VAR_ARRAY_OF_STRINGS, BFFVariable::VAR_ARRAY_OF_STRUCTS );
            return false;
        }

        // is this the first variable?
        int thisArrayLen( -1 );
        if ( var->GetType() == BFFVariable::VAR_ARRAY_OF_STRINGS )
        {
            thisArrayLen = (int)var->GetArrayOfStrings().GetSize();
        }
        else if ( var->GetType() == BFFVariable::VAR_ARRAY_OF_STRUCTS )
        {
            thisArrayLen = (int)var->GetArrayOfStructs().GetSize();
        }
        else
        {
            ASSERT( false );
        }

        if ( loopLen == -1 )
        {
            // it will set the length of looping
            loopLen = thisArrayLen;
        }
        else
        {
            // variables after the first must match length of the first
            if ( loopLen != thisArrayLen )
            {
                Error::Error_1204_LoopVariableLengthsDiffer( arrayVarNameBegin, this, arrayVarName.Get(), (uint32_t)thisArrayLen, (uint32_t)loopLen );
                return false;
            }
        }
        arrayVars.Append( var );

        // skip optional separator
        pos.SkipWhiteSpace();
        if ( *pos == ',' )
        {
            pos++;
        }
    }

    ASSERT( localNames.GetSize() == arrayVars.GetSize() );

    // gracefully allow empty loops
    if ( loopLen < 1 )
    {
        return true;
    }

    // freeze the variable to avoid modifications while looping
    for ( uint32_t j=0; j<arrayVars.GetSize(); ++j )
    {
        arrayVars[ j ]->Freeze();
        FLOG_INFO( "Freezing loop array '%s' of type <%s>",
                   arrayVars[j]->GetName().Get(), BFFVariable::GetTypeName( arrayVars[j]->GetType() ) );
    }

    bool succeed = true;

    for ( uint32_t i=0; i<(uint32_t)loopLen; ++i )
    {
        // create local loop variables
        BFFStackFrame loopStackFrame;
        for ( uint32_t j=0; j<localNames.GetSize(); ++j )
        {
            if ( arrayVars[ j ]->GetType() == BFFVariable::VAR_ARRAY_OF_STRINGS )
            {
                BFFStackFrame::SetVarString( localNames[ j ], arrayVars[ j ]->GetArrayOfStrings()[ i ], &loopStackFrame );
            }
            else if ( arrayVars[ j ]->GetType() == BFFVariable::VAR_ARRAY_OF_STRUCTS )
            {
                BFFStackFrame::SetVarStruct( localNames[ j ], arrayVars[ j ]->GetArrayOfStructs()[ i ]->GetStructMembers(), &loopStackFrame );
            }
            else
            {
                ASSERT( false );
            }
        }

        // parse the function body
        BFFParser subParser( nodeGraph );
        BFFIterator subIter( *functionBodyStartToken );
        subIter++; // skip opening token
        subIter.SetMax( functionBodyStopToken->GetCurrent() ); // limit to closing token
        if ( subParser.Parse( subIter ) == false )
        {
            succeed = false;
            break;
        }

        // complete the function
        if ( Commit( nodeGraph, functionNameStart ) == false )
        {
            succeed = false;
            break;
        }
    }

    // unfreeze all array variables
    for ( uint32_t j=0; j<arrayVars.GetSize(); ++j )
    {
        arrayVars[ j ]->Unfreeze();
        FLOG_INFO( "Unfreezing loop array '%s' of type <%s>",
                   arrayVars[j]->GetName().Get(), BFFVariable::GetTypeName( arrayVars[j]->GetType() ) );
    }

    return succeed;
}

//------------------------------------------------------------------------------
