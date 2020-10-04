// FunctionForEach
//------------------------------------------------------------------------------

// Includes
//------------------------------------------------------------------------------
#include "FunctionForEach.h"

#include "Tools/FBuild/FBuildCore/BFF/BFFKeywords.h"
#include "Tools/FBuild/FBuildCore/BFF/BFFParser.h"
#include "Tools/FBuild/FBuildCore/BFF/BFFStackFrame.h"
#include "Tools/FBuild/FBuildCore/BFF/BFFVariable.h"
#include "Tools/FBuild/FBuildCore/BFF/Tokenizer/BFFTokenRange.h"
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
/*virtual*/ bool FunctionForEach::ParseFunction( NodeGraph & /*nodeGraph*/,
                                                 BFFParser & parser,
                                                 const BFFToken * /*functionNameStart*/,
                                                 const BFFTokenRange & headerRange,
                                                 const BFFTokenRange & bodyRange ) const
{
    // build array for each pair to loop through
    StackArray<AString> localNames;
    StackArray<const BFFVariable *> arrayVars;

    int loopLen = -1;

    // parse it all out
    BFFTokenRange headerIter = headerRange;
    while ( headerIter.IsAtEnd() == false )
    {
        // Expect a dest variable which will be locally created
        {
            const BFFToken * localVarToken = headerIter.GetCurrent();
            headerIter++;
            if ( localVarToken->IsVariable() == false )
            {
                Error::Error_1200_ExpectedVar( localVarToken, this );
                return false;
            }
            // TODO: Check for . and not ^

            // Resolve the name of the local variable
            AStackString< BFFParser::MAX_VARIABLE_NAME_LENGTH > localName;
            bool localParentScope = false; // always false thanks to the previous test
            if ( BFFParser::ParseVariableName( localVarToken, localName, localParentScope ) == false )
            {
                return false;
            }
            ASSERT( false == localParentScope );

            localNames.Append( localName );
        }

        // check for required "in" token
        {
            const BFFToken * inToken = headerIter.GetCurrent();
            headerIter++;
            if ( ( inToken->IsKeyword() == false ) || ( inToken->GetValueString() != "in" ) )
            {
                Error::Error_1201_MissingIn( inToken, this );
                return false;
            }
        }

        // Expect a source variable
        {
            const BFFToken * srcVarToken = headerIter.GetCurrent();
            headerIter++;
            if ( srcVarToken->IsVariable() == false )
            {
                Error::Error_1202_ExpectedVarFollowingIn( srcVarToken, this );
                return false;
            }

            AStackString< BFFParser::MAX_VARIABLE_NAME_LENGTH > arrayVarName;
            bool arrayParentScope = false;
            if ( BFFParser::ParseVariableName( srcVarToken, arrayVarName, arrayParentScope ) == false )
            {
                return false;
            }

            const BFFVariable * var = nullptr;
            const BFFStackFrame * const arrayFrame = ( arrayParentScope )
                ? BFFStackFrame::GetParentDeclaration( arrayVarName, BFFStackFrame::GetCurrent()->GetParent(), var )
                : nullptr;

            if ( false == arrayParentScope )
            {
                var = BFFStackFrame::GetVar( arrayVarName, nullptr );
            }

            if ( ( arrayParentScope && ( nullptr == arrayFrame ) ) || ( var == nullptr ) )
            {
                Error::Error_1009_UnknownVariable( srcVarToken, this, arrayVarName );
                return false;
            }

            // it can be of any supported type
            if ( ( var->IsArrayOfStrings() == false ) && ( var->IsArrayOfStructs() == false ) )
            {
                Error::Error_1050_PropertyMustBeOfType( srcVarToken, this, arrayVarName.Get(), var->GetType(), BFFVariable::VAR_ARRAY_OF_STRINGS, BFFVariable::VAR_ARRAY_OF_STRUCTS );
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
                    Error::Error_1204_LoopVariableLengthsDiffer( srcVarToken, this, arrayVarName.Get(), (uint32_t)thisArrayLen, (uint32_t)loopLen );
                    return false;
                }
            }
            arrayVars.Append( var );
        }

        // skip optional separator
        if ( headerIter->IsComma() )
        {
            headerIter++;
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
        BFFTokenRange range( bodyRange );
        if ( parser.Parse( range ) == false )
        {
            succeed = false;
            break;
        }
    }

    // unfreeze all array variables
    for ( uint32_t j=0; j<arrayVars.GetSize(); ++j )
    {
        arrayVars[ j ]->Unfreeze();
    }

    return succeed;
}

//------------------------------------------------------------------------------
