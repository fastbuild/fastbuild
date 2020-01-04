// BFFParser
//------------------------------------------------------------------------------

// Includes
//------------------------------------------------------------------------------
#include "BFFParser.h"
#include "BFFKeywords.h"
#include "BFFStackFrame.h"
#include "Tools/FBuild/FBuildCore/BFF/BFFFile.h"
#include "Tools/FBuild/FBuildCore/BFF/Tokenizer/BFFTokenizer.h"
#include "Tools/FBuild/FBuildCore/BFF/Tokenizer/BFFTokenRange.h"
#include "Tools/FBuild/FBuildCore/FBuild.h"
#include "Tools/FBuild/FBuildCore/Graph/NodeGraph.h"
#include "Tools/FBuild/FBuildCore/FLog.h"
#include "Tools/FBuild/FBuildCore/BFF/Functions/Function.h"
#include "Tools/FBuild/FBuildCore/FBuildVersion.h"

// Core
#include "Core/Containers/AutoPtr.h"
#include "Core/Env/Assert.h"
#include "Core/Env/Env.h"
#include "Core/FileIO/FileIO.h"
#include "Core/FileIO/FileStream.h"
#include "Core/FileIO/PathUtils.h"
#include "Core/Math/xxHash.h"
#include "Core/Profile/Profile.h"
#include "Core/Strings/AStackString.h"
#include "Core/Time/Timer.h"
#include "Core/Tracing/Tracing.h"

#include <stdio.h>

// CONSTRUCTOR
//------------------------------------------------------------------------------
BFFParser::BFFParser( NodeGraph & nodeGraph )
    : m_NodeGraph( nodeGraph )
{
}

// DESTRUCTOR
//------------------------------------------------------------------------------
BFFParser::~BFFParser() = default;

// ParseFromFile
//------------------------------------------------------------------------------
bool BFFParser::ParseFromFile( const char * fileName )
{
    PROFILE_FUNCTION

    // Tokenize file
    if ( m_Tokenizer.TokenizeFromFile( AStackString<>( fileName ) ) == false )
    {
        return false; // Tokenize will have emitted an error
    }

    const Array<BFFToken>& tokens = m_Tokenizer.GetTokens();
    if ( tokens.IsEmpty() )
    {
        return true; // An empty file is ok
    }

    CreateBuiltInVariables();

    // Walk tokens
    BFFTokenRange range( tokens.Begin(), tokens.End() );
    bool result = Parse( range );

    return result;
}

// ParseFromString
//------------------------------------------------------------------------------
bool BFFParser::ParseFromString( const char * fileName, const char * fileContents )
{
    PROFILE_FUNCTION

    // Tokenize string
    if ( m_Tokenizer.TokenizeFromString( AStackString<>( fileName ), AStackString<>( fileContents ) ) == false )
    {
        return false; // Tokenize will have emitted an error
    }

    const Array<BFFToken>& tokens = m_Tokenizer.GetTokens();
    if ( tokens.IsEmpty() )
    {
        return true; // An empty file is ok
    }

    CreateBuiltInVariables();

    // Walk tokens
    BFFTokenRange range( tokens.Begin(), tokens.End() );
    bool result = Parse( range );

    return result;
}

// Parse
//------------------------------------------------------------------------------
bool BFFParser::Parse( BFFTokenRange & iter )
{
    while ( iter.IsAtEnd() == false )
    {
        // Handle updating current bff path variable
        SetBuiltInVariable_CurrentBFFDir( iter->GetSourceFileName().Get() );

        const BFFToken * token = iter.GetCurrent();

        // Variable
        if ( token->IsVariable() )
        {
            if ( ParseNamedVariableDeclaration( iter ) == false )
            {
                return false;
            }
            continue;
        }

        // + or - operator
        if ( token->IsOperator( "+" ) || token->IsOperator( "-" ) )
        {
            // concatenation to/subtraction from last used variable
            if ( ParseUnnamedVariableModification( iter ) == false )
            {
                return false;
            }
            continue;
        }

        // New scope
        if ( token->IsOpeningCurlyBracket() )
        {
            // start an unnamed scope
            if ( ParseUnnamedScope( iter ) == false )
            {
                return false;
            }
            continue;
        }

        // Function
        if ( token->IsFunction() )
        {
            if ( ParseFunction( iter ) == false )
            {
                return false;
            }
            continue;
        }

        // TODO: Better error here
        Error::Error_1010_UnknownConstruct( token );
        return false;
    }

    return true; // cleanly hit end of file
}

// ParseNamedVariableName
//------------------------------------------------------------------------------
/*static*/ bool BFFParser::ParseVariableName( const BFFToken * iter, AString & name, bool & parentScope )
{
    const char * pos = iter->GetValueString().Get();

    // skip over the declaration symbol
    ASSERT( ( *pos == BFF_DECLARE_VAR_INTERNAL ) || ( *pos == BFF_DECLARE_VAR_PARENT ) );
    parentScope = ( *pos == BFF_DECLARE_VAR_PARENT );
    ++pos;

    // Variable name is "normalized" and parentScope bool is used by caller
    name = ".";

    // Variable of the ."String" form?
    if ( ( *pos == '"' ) || ( *pos == '\'' ) ) // TODO: Handle this as a new type in the BFFTokenizer?
    {
        // Tokenizer should ensure this is valid
        ASSERT( iter->GetValueString().EndsWith( *pos ) );
        ASSERT( iter->GetValueString().GetLength() >= 4 ); // at least one char inside: ."x" 

        // unescape and subsitute embedded variables
        AStackString<> value;
        if ( PerformVariableSubstitutions( iter, value ) == false )
        {
            return false;
        }

        // sanity check it is a sensible length
        if ( name.GetLength() + 1  > MAX_VARIABLE_NAME_LENGTH ) // +1 for '.' will be added 
        {
            Error::Error_1014_VariableNameIsTooLong( iter, (uint32_t)value.GetLength(), (uint32_t)MAX_VARIABLE_NAME_LENGTH );
            return false;
        }

        // Resolved variable name, minus quotes quotes
        name.Append( value.Get() + 2, value.GetLength() - 3 ); // 2 for leading ." and -1 for trailing "
    }
    else
    {
        // Variable name
        name += ( iter->GetValueString().Get() + 1 ); // 1 for leading . or ^
    }

    return true;
}

// ParseUnnamedVariableModification
//------------------------------------------------------------------------------
bool BFFParser::ParseUnnamedVariableModification( BFFTokenRange & iter )
{
    ASSERT( iter->IsOperator() );

    const AString & varName = BFFStackFrame::GetCurrent()->GetLastVariableSeen();
    BFFStackFrame * frame = BFFStackFrame::GetCurrent()->GetLastVariableSeenFrame();

    // have we assigned a variable before?
    if ( varName.IsEmpty() )
    {
        Error::Error_1011_UnnamedModifcationMustFollowAssignment( iter.GetCurrent() );
        return false;
    }

    return ParseVariableDeclaration( iter, varName, frame );
}

// ParseNamedVariableDeclaration
//------------------------------------------------------------------------------
bool BFFParser::ParseNamedVariableDeclaration( BFFTokenRange & iter )
{
    const BFFToken * varToken = iter.GetCurrent();
    ASSERT( varToken->IsVariable() );

    bool parentScope = false;
    AStackString<> varName;
    if ( ParseVariableName( varToken, varName, parentScope ) == false )
    {
        return false; // ParseVariableName() would have display an error
    }
    iter++;

    // check if points to a previous declaration in a parent scope
    const BFFVariable * parentVar = nullptr;
    BFFStackFrame * frame = ( parentScope )
                          ? BFFStackFrame::GetParentDeclaration( varName, nullptr, parentVar )
                          : nullptr;

    if ( parentScope )
    {
        // check if a parent definition exists
        if ( nullptr == frame )
        {
            Error::Error_1009_UnknownVariable( varToken, nullptr, varName );
            return false;
        }

        ASSERT( nullptr != parentVar );

        // check if the parent definition is frozen
        if ( parentVar->Frozen() )
        {
            Error::Error_1060_CantModifyFrozenVar( varToken, nullptr, parentVar );
            return false;
        }
    }

    // Store "last seen" variable
    BFFStackFrame::GetCurrent()->SetLastVariableSeen( varName, frame );

    return ParseVariableDeclaration( iter, varName, frame );
}

// ParseVariableDeclaration
//------------------------------------------------------------------------------
bool BFFParser::ParseVariableDeclaration( BFFTokenRange & iter, const AString & varName, BFFStackFrame * frame )
{
    const BFFToken * opToken = iter.GetCurrent();

    // Variable must be followed by an operator
    if ( opToken->GetType() != BFFTokenType::Operator )
    {
        Error::Error_1016_UnexepectedCharFollowingVariableName( opToken );
        return false;
    }

    // Check this is an operator we support
    const char op = opToken->GetValueString()[ 0 ];
    if ( ( op != BFF_VARIABLE_ASSIGNMENT ) &&
         ( op != BFF_VARIABLE_CONCATENATION ) &&
         ( op != BFF_VARIABLE_SUBTRACTION ) )
    {
        Error::Error_1254_UnrecognizedOperator( opToken, nullptr ); // TODO: Check error
        return false;
    }

    iter++; // Consume operator

    // What operator type is this?
    const bool concat = ( op == BFF_VARIABLE_CONCATENATION );
    const bool subtract = ( op == BFF_VARIABLE_SUBTRACTION );

    const BFFToken * rhsToken = iter.GetCurrent();
    if ( rhsToken->IsString() )
    {
        iter++; // Consume the rhs
        return StoreVariableString( varName, rhsToken, opToken, frame );
    }
    else if ( rhsToken->IsOpeningCurlyBracket() ) // Open Scope
    {
        const BFFToken * openBraceToken = iter.GetCurrent();
        BFFTokenRange bracedRange;
        if ( FindBracedRange( iter, bracedRange ) == false )
        {
            Error::Error_1002_MatchingClosingTokenNotFound( openBraceToken, nullptr, '}' );
            return false;
        }
        return StoreVariableArray( varName, bracedRange, opToken, frame );
    }
    else if ( rhsToken->IsOpeningSquareBracket() ) // Open Struct
    {
        const BFFToken * openBraceToken = iter.GetCurrent();
        BFFTokenRange bracedRange;
        if ( FindBracedRange( iter, bracedRange ) == false )
        {
            Error::Error_1002_MatchingClosingTokenNotFound( openBraceToken, nullptr, ']' );
            return false;
        }
        return StoreVariableStruct( varName, bracedRange, opToken, frame );
    }
    else if ( rhsToken->IsNumber() )
    {
        // find existing
        const BFFVariable * var = BFFStackFrame::GetVar( varName, frame );

        // variable type must match
        if ( ( var != nullptr ) && !var->IsInt() )
        {
            Error::Error_1034_OperationNotSupported( rhsToken, var->GetType(), BFFVariable::VAR_INT, opToken );
            return false;
        }

        // variable must exist, if we are going to modify it
        if ( ( concat || subtract ) && ( var == nullptr ) )
        {
            Error::Error_1026_VariableNotFoundForModification( opToken, varName );
            return false;
        }

        const int i = rhsToken->GetValueInt();
        iter++; // Consume token
        int32_t newVal;
        if ( concat )
        {
            newVal = var->GetInt() + i;
        }
        else if ( subtract )
        {
            newVal = var->GetInt() - i;
        }
        else
        {
            newVal = i;
        }
        return StoreVariableInt( varName, newVal, frame );
    }
    else if ( rhsToken->IsBooelan() )
    {
        // find existing
        const BFFVariable * var = BFFStackFrame::GetVar( varName, frame );

        // are we concatenating?
        if ( concat || subtract )
        {
            // concatenation of bools not supported
            Error::Error_1027_CannotModify( opToken, varName, BFFVariable::VAR_BOOL, BFFVariable::VAR_ANY );
            return false;
        }
        else
        {
            // variable must be new or a bool
            if ( !( ( var == nullptr ) || ( var->IsBool() ) ) )
            {
                Error::Error_1034_OperationNotSupported( rhsToken, var->GetType(), BFFVariable::VAR_BOOL, opToken );
                return false;
            }
        }

        iter++; // Consume the rhs

        return StoreVariableBool( varName, rhsToken->GetBoolean(), frame );
    }
    else if ( rhsToken->IsVariable() )
    {
        iter++; // Consume the rhs
        return StoreVariableToVariable( varName, rhsToken, opToken, frame );
    }

    Error::Error_1017_UnexepectedCharInVariableValue( rhsToken );
    return false;
}

// ParseFunction
//------------------------------------------------------------------------------
bool BFFParser::ParseFunction( BFFTokenRange & iter )
{
    const BFFToken * functionNameToken = iter.GetCurrent();
    ASSERT( functionNameToken->IsFunction() ); // Should only be called for a function
    iter++;

    // for variables to be used by this function
    BFFStackFrame stackFrame;

    // Is this a recognized function?
    const Function * func = Function::Find( functionNameToken->GetValueString() );
    if ( func == nullptr )
    {
        Error::Error_1015_UnknownFunction( functionNameToken );
        return false;
    }

    // Handle unique functions
    if ( func->IsUnique() && func->GetSeen() )
    {
        Error::Error_1020_FunctionCanOnlyBeInvokedOnce( functionNameToken, func );
        return false;
    }
    func->SetSeen();

    // Is there a header?
    BFFTokenRange headerRange;
    if ( iter->IsOpeningRoundBracket() )
    {
        // Check if this function can have a header
        if ( func->AcceptsHeader() == false )
        {
            Error::Error_1021_UnexpectedHeaderForFunction( iter.GetCurrent(), func );
            return false;
        }

        // Find limits of args inside brackets
        if ( FindBracedRange( iter, headerRange ) == false )
        {
            Error::Error_1022_MissingFunctionHeaderCloseToken( iter.GetCurrent(), func );
            return false;
        }
    }

    // Is a required header missing?
    if ( func->NeedsHeader() && ( headerRange.GetCurrent() == nullptr ) )
    {
        Error::Error_1023_FunctionRequiresAHeader( iter.GetCurrent(), func );
        return false;
    }

    // Handle function body
    BFFTokenRange bodyRange;
    if ( func->NeedsBody() )
    {
        // Check for body start
        if ( iter->IsOpeningCurlyBracket() == false )
        {
            Error::Error_1024_FunctionRequiresABody( iter.GetCurrent(), func );
            return false;
        }

        // Find close of body
        if ( FindBracedRange( iter, bodyRange ) == false )
        {
            Error::Error_1025_MissingScopeCloseToken( iter.GetCurrent(), func );
            return false;
        }
     }

    return func->ParseFunction( m_NodeGraph,
                                *this,
                                functionNameToken,
                                headerRange,
                                bodyRange );
}

// ParseUnnamedScope
//------------------------------------------------------------------------------
bool BFFParser::ParseUnnamedScope( BFFTokenRange & iter )
{
    // create stack for scope
    BFFStackFrame stackFrame;

    // Find limits of args inside brackets
    const BFFToken * openToken = iter.GetCurrent();
    ASSERT( openToken->IsOpeningCurlyBracket() );
    BFFTokenRange range;
    if ( FindBracedRange( iter, range ) == false )
    {
        const char expectedChar = '}';
        Error::Error_1002_MatchingClosingTokenNotFound( openToken, nullptr, expectedChar );
        return false;
    }

    // parse the scoped part
    if ( Parse( range ) == false )
    {
        return false;
    }

    return true;
}


// FindBracedRange
//------------------------------------------------------------------------------
bool BFFParser::FindBracedRange( BFFTokenRange & iter, BFFTokenRange & outBracedRange ) const
{
    // Take note of range begin
    const BFFToken * begin = iter.GetCurrent() + 1; // First token after brace
    if ( FindBracedRangeRecurse( iter ) == false)
    {
        return false;
    }

    // Return new range
    outBracedRange = BFFTokenRange( begin, iter.GetCurrent() );
    iter++; // Consume closing brace
    return true;
}

// FindBracedRangeRecurse
//------------------------------------------------------------------------------
bool BFFParser::FindBracedRangeRecurse( BFFTokenRange & iter ) const
{
    // Determine the matching close token
    const BFFToken * openToken = iter.GetCurrent();
    BFFTokenType closeTokenType;
    switch( openToken->GetType() )
    {
        case BFFTokenType::OpeningCurlyBracket:    closeTokenType = BFFTokenType::ClosingCurlyBracket; break;
        case BFFTokenType::OpeningRoundBracket:    closeTokenType = BFFTokenType::ClosingRoundBracket; break;
        case BFFTokenType::OpeningSquareBracket:   closeTokenType = BFFTokenType::ClosingSquareBracket; break;
        default: ASSERT(false); return false;
    }
    iter++;

    while ( iter.IsAtEnd() == false )
    {
        // Found the close bracket?
        if ( iter.GetCurrent()->GetType() == closeTokenType )
        {
            return true;
        }

        if ( iter.GetCurrent()->GetType() == openToken->GetType() )
        {
            // Recurse into nested open bracket
            if ( FindBracedRangeRecurse( iter ) == false )
            {
                return false;
            }
        }

        // Keep searching
        iter++;
    }

    return false;
}

// StoreVariableString
//------------------------------------------------------------------------------
bool BFFParser::StoreVariableString( const AString & name,
                                     const BFFToken * rhsString,
                                     const BFFToken * opToken,
                                     BFFStackFrame * frame )
{
    ASSERT( rhsString->IsString() );
    ASSERT( opToken->IsOperator() );

    // unescape and subsitute embedded variables
    AStackString< 2048 > value;
    if ( PerformVariableSubstitutions( rhsString, value ) == false )
    {
        return false;
    }

    // find existing
    const BFFVariable * var = BFFStackFrame::GetVar( name, frame );

    const bool dstIsEmpty = ( var == nullptr ) ||
                            ( var->IsArrayOfStrings() && var->GetArrayOfStrings().IsEmpty() ) ||
                            ( var->IsArrayOfStructs() && var->GetArrayOfStructs().IsEmpty() );

    // are we concatenating?
    if ( opToken->IsOperator( BFF_VARIABLE_CONCATENATION ) ||
         opToken->IsOperator( BFF_VARIABLE_SUBTRACTION ) )
    {
        // variable must exist
        if ( var == nullptr )
        {
            Error::Error_1026_VariableNotFoundForModification( opToken, name );
            return false;
        }

        // make sure types are compatible
        if ( var->IsString() )
        {
            // OK - can concat String to String
            AStackString< 1024 > finalValue( var->GetString() );
            if ( opToken->IsOperator( BFF_VARIABLE_CONCATENATION ) )
            {
                finalValue += value;
            }
            else
            {
                finalValue.Replace( value.Get(), "" );
            }

            BFFStackFrame::SetVarString( name, finalValue, frame );
            return true;
        }
        else if ( var->IsArrayOfStrings() || dstIsEmpty )
        {
            // OK - can concat String to ArrayOfStrings or to empty array
            StackArray<AString> finalValues;
            finalValues.SetCapacity( var->GetArrayOfStrings().GetSize() + 1 );
            if ( opToken->IsOperator( BFF_VARIABLE_CONCATENATION ) )
            {
                if ( !dstIsEmpty )
                {
                    finalValues = var->GetArrayOfStrings();
                }
                finalValues.Append( value );
            }
            else if ( !dstIsEmpty )
            {
                auto end = var->GetArrayOfStrings().End();
                for ( auto it=var->GetArrayOfStrings().Begin(); it!=end; ++it )
                {
                    if ( *it != value ) // remove equal strings
                    {
                        finalValues.Append( *it );
                    }
                }
            }

            BFFStackFrame::SetVarArrayOfStrings( name, finalValues, frame );
            return true;
        }
        else
        {
            Error::Error_1027_CannotModify( opToken, name, var->GetType(), BFFVariable::VAR_STRING );
            return false;
        }
    }
    else
    {
        // make sure types are compatible
        if ( ( var == nullptr ) || var->IsString() )
        {
            // OK - asigning to a new variable or to a string
            BFFStackFrame::SetVarString( name, value, frame );
            return true;
        }
        else if ( var->IsArrayOfStrings() || dstIsEmpty )
        {
            // OK - store new string as the single element of array
            StackArray<AString> values;
            values.Append( value );
            BFFStackFrame::SetVarArrayOfStrings( name, values, frame );
            return true;
        }
        else
        {
            const BFFToken * errorIter = opToken;
            errorIter--;
            Error::Error_1034_OperationNotSupported( errorIter, var->GetType(), BFFVariable::VAR_STRING, opToken );
            return false;
        }
    }
    return false;
}

// StoreVariableArray
//------------------------------------------------------------------------------
bool BFFParser::StoreVariableArray( const AString & name,
                                    const BFFTokenRange & tokenRange,
                                    const BFFToken * opToken,
                                    BFFStackFrame * frame )
{
    ASSERT( opToken->IsOperator( BFF_VARIABLE_ASSIGNMENT ) ||
            opToken->IsOperator( BFF_VARIABLE_CONCATENATION ) ||
            opToken->IsOperator( BFF_VARIABLE_SUBTRACTION ) );

    StackArray<AString> values;
    StackArray<const BFFVariable *> structValues;

    // find existing
    const BFFVariable * var = BFFStackFrame::GetVar( name, frame );

    // are we concatenating?
    if ( opToken->IsOperator( BFF_VARIABLE_CONCATENATION ) ||
         opToken->IsOperator( BFF_VARIABLE_SUBTRACTION ) )
    {
        // variable must exist
        if ( var == nullptr )
        {
            Error::Error_1026_VariableNotFoundForModification( opToken, name );
            return false;
        }

        // make sure existing is an array
        if ( var->IsArrayOfStrings() )
        {
            // get values to start with
            values = var->GetArrayOfStrings();
        }
        else if ( var->IsArrayOfStructs() )
        {
            // get values to start with
            structValues = var->GetArrayOfStructs();
        }
        else
        {
            // TODO:B Improve this error to handle ArrayOfStructs case
            Error::Error_1027_CannotModify( opToken, name, var->GetType(), BFFVariable::VAR_ARRAY_OF_STRINGS );
            return false;
        }
    }
    else
    {
        // variable must be new or array of some kind
        if ( !( ( var == nullptr ) || var->IsArrayOfStrings() || var->IsArrayOfStructs() ) )
        {
            // TODO:B Improve this error to handle ArrayOfStructs case
            Error::Error_1034_OperationNotSupported( opToken, var->GetType(), BFFVariable::VAR_ARRAY_OF_STRINGS, opToken );
            return false;
        }
    }

    BFFVariable::VarType varType = var ? var->GetType() : BFFVariable::VAR_ANY;
    if ( ( varType == BFFVariable::VAR_ARRAY_OF_STRINGS && var->GetArrayOfStrings().IsEmpty() ) ||
         ( varType == BFFVariable::VAR_ARRAY_OF_STRUCTS && var->GetArrayOfStructs().IsEmpty() ) )
    {
        varType = BFFVariable::VAR_ANY; // allow type of an empty array to be changed
    }

    // Parse array of variables
    const BFFToken * iter = tokenRange.GetBegin();
    const BFFToken * end = tokenRange.GetEnd();
    while ( iter < end )
    {
        if ( iter->IsString() )
        {
            // a quoted string

            // dest is consistent?
            if ( ( varType != BFFVariable::VAR_ARRAY_OF_STRINGS ) &&
                 ( varType != BFFVariable::VAR_ANY ) )
            {
                // Mixed types in vector
                Error::Error_1034_OperationNotSupported( iter,
                                                         varType,
                                                         BFFVariable::VAR_STRING,
                                                         opToken );
                return false;
            }

            // subtraction not supported on arrays
            if ( opToken->IsOperator( BFF_VARIABLE_SUBTRACTION ) )
            {
                Error::Error_1034_OperationNotSupported( iter,
                                                         varType,
                                                         BFFVariable::VAR_STRING,
                                                         opToken );
                return false;
            }

            // a string
            AStackString< 2048 > elementValue;

            // unescape and subsitute embedded variables
            if ( PerformVariableSubstitutions( iter, elementValue ) == false )
            {
                return false;
            }

            varType = BFFVariable::VAR_ARRAY_OF_STRINGS;
            values.Append( elementValue );
        }
        else if ( iter->IsVariable() )
        {
            // a variable
            AStackString<> srcVarName( iter->GetValueString() );

            // Determine stack frame to use for Src var
            BFFStackFrame * srcFrame = BFFStackFrame::GetCurrent();
            if ( iter->GetValueString()[ 0 ] == BFF_DECLARE_VAR_PARENT )
            {
                srcVarName[ 0 ] = BFF_DECLARE_VAR_INTERNAL;
                srcFrame = BFFStackFrame::GetCurrent()->GetParent();
            }

            // get the variable
            const BFFVariable * varSrc = srcFrame ? srcFrame->GetVariableRecurse( srcVarName ) : nullptr;
            if ( varSrc == nullptr )
            {
                Error::Error_1026_VariableNotFoundForModification( opToken, srcVarName );
                return false;
            }

            // subtraction not supported on arrays
            if ( opToken->IsOperator( BFF_VARIABLE_SUBTRACTION ) )
            {
                Error::Error_1034_OperationNotSupported( iter,
                                                         varType,
                                                         varSrc->GetType(),
                                                         opToken );
                return false;
            }

            if ( varSrc->IsArrayOfStrings() && varSrc->GetArrayOfStrings().IsEmpty() )
            {
                // appending empty array, nothing to do
            }
            else if ( varSrc->IsArrayOfStructs() && varSrc->GetArrayOfStructs().IsEmpty() )
            {
                // appending empty array, nothing to do
            }
            else if ( varSrc->IsString() || varSrc->IsArrayOfStrings() )
            {
                // dest is consistent?
                if ( ( varType != BFFVariable::VAR_ARRAY_OF_STRINGS ) &&
                     ( varType != BFFVariable::VAR_ANY ) )
                {
                    // inconsistency
                    Error::Error_1034_OperationNotSupported( iter,
                                                             varType,
                                                             varSrc->GetType(),
                                                             opToken );
                    return false;
                }

                varType = BFFVariable::VAR_ARRAY_OF_STRINGS;
                if ( varSrc->IsString() )
                {
                    values.Append( varSrc->GetString() );
                }
                else
                {
                    values.Append( varSrc->GetArrayOfStrings() );
                }
            }
            else if ( varSrc->IsStruct() || varSrc->IsArrayOfStructs() )
            {
                // dest is consistent?
                if ( ( varType != BFFVariable::VAR_ARRAY_OF_STRUCTS ) &&
                     ( varType != BFFVariable::VAR_ANY ) )
                {
                    // inconsistency
                    Error::Error_1034_OperationNotSupported( iter,
                                                             varType,
                                                             varSrc->GetType(),
                                                             opToken );
                    return false;
                }

                varType = BFFVariable::VAR_ARRAY_OF_STRUCTS;
                if ( varSrc->IsStruct() )
                {
                    structValues.Append( varSrc );
                }
                else
                {
                    structValues.Append( varSrc->GetArrayOfStructs() );
                }
            }
            else
            {
                Error::Error_1050_PropertyMustBeOfType( iter, nullptr, name.Get(),
                                                        varType,
                                                        BFFVariable::VAR_STRING,
                                                        BFFVariable::VAR_STRUCT );
                return false;
            }
        }
        else
        {
            Error::Error_1001_MissingStringStartToken( iter, nullptr );
            return false;
        }

        iter++; // Entry has been handled

        if ( iter->IsComma() ) // comma seperators are optional
        {
            iter++;
        }

        // continue looking for more vars...
    }

    // should only have one populated array
    ASSERT( values.IsEmpty() || structValues.IsEmpty() );

    // if array is empty then try to preserve it's type
    if ( varType == BFFVariable::VAR_ANY )
    {
        ASSERT( values.IsEmpty() && structValues.IsEmpty() );
        varType = var ? var->GetType() : BFFVariable::VAR_ARRAY_OF_STRINGS;
    }

    // Register this variable
    if ( varType == BFFVariable::VAR_ARRAY_OF_STRUCTS )
    {
        // structs
        BFFStackFrame::SetVarArrayOfStructs( name, structValues, frame );
    }
    else
    {
        ASSERT( varType == BFFVariable::VAR_ARRAY_OF_STRINGS );
        // strings
        BFFStackFrame::SetVarArrayOfStrings( name, values, frame );
    }

    return true;
}

// StoreVariableStruct
//------------------------------------------------------------------------------
bool BFFParser::StoreVariableStruct( const AString & name,
                                     const BFFTokenRange & tokenRange,
                                     const BFFToken * operatorToken,
                                     BFFStackFrame * frame )
{
    // find existing
    const BFFVariable * var = BFFStackFrame::GetVar( name, frame );

    // are we concatenating?
    ASSERT( operatorToken->IsOperator() );
    const char opChar = operatorToken->GetValueString()[ 0 ];
    if ( ( opChar == BFF_VARIABLE_CONCATENATION ) ||
         ( opChar == BFF_VARIABLE_SUBTRACTION ) )
    {
        // concatenation of structs not supported
        Error::Error_1027_CannotModify( operatorToken, name, BFFVariable::VAR_STRUCT, BFFVariable::VAR_ANY );
        return false;
    }
    else
    {
        // variable must be new or a struct
        if ( !( ( var == nullptr ) || var->IsStruct() ) )
        {
            Error::Error_1034_OperationNotSupported( operatorToken, var->GetType(), BFFVariable::VAR_STRUCT, operatorToken );
            return false;
        }
    }

    // create stack frame to capture variables
    BFFStackFrame stackFrame;

    // parse all the variables in the scope
    BFFTokenRange range( tokenRange );
    if ( Parse( range ) == false )
    {
        return false; // error will be emitted by Parse
    }

    // get variables defined in the scope
    Array<BFFVariable *> & structMembers = stackFrame.GetLocalVariables();

    // Register this variable
    BFFStackFrame::SetVarStruct( name, Move( structMembers ), frame ? frame : stackFrame.GetParent() );

    return true;
}

// StoreVariableBool
//------------------------------------------------------------------------------
bool BFFParser::StoreVariableBool( const AString & name, bool value, BFFStackFrame * frame )
{
    // Register this variable
    BFFStackFrame::SetVarBool( name, value, frame );

    return true;
}

// StoreVariableInt
//------------------------------------------------------------------------------
bool BFFParser::StoreVariableInt( const AString & name, int value, BFFStackFrame * frame )
{
    BFFStackFrame::SetVarInt( name, value, frame );

    return true;
}

// StoreVariableToVariable
//------------------------------------------------------------------------------
bool BFFParser::StoreVariableToVariable( const AString & dstName, const BFFToken * rhsToken, const BFFToken * operatorToken, BFFStackFrame * dstFrame )
{
    AStackString< MAX_VARIABLE_NAME_LENGTH > srcName;

    bool srcParentScope = false;
    if ( ParseVariableName( rhsToken, srcName, srcParentScope ) == false )
    {
        return false;
    }

    // find src var
    const BFFVariable * varSrc = nullptr;
    BFFStackFrame * const srcFrame = ( srcParentScope )
        ? BFFStackFrame::GetParentDeclaration( srcName, nullptr, varSrc )
        : nullptr;

    if ( !srcParentScope )
    {
        varSrc = BFFStackFrame::GetVar( srcName, nullptr );
    }

    if ( ( srcParentScope && nullptr == srcFrame ) || ( nullptr == varSrc ) )
    {
        Error::Error_1009_UnknownVariable( rhsToken, nullptr, srcName );
        return false;
    }

    // find dst var
    const BFFVariable * varDst = BFFStackFrame::GetVar( dstName, dstFrame );

    const char opChar = operatorToken->GetValueString()[ 0 ];
    const bool concat = ( opChar== BFF_VARIABLE_CONCATENATION );
    const bool subtract = ( opChar == BFF_VARIABLE_SUBTRACTION );

    // concatenation?
    if ( concat || subtract )
    {
        // can only concatenate to existing vars
        if ( varDst == nullptr )
        {
            Error::Error_1026_VariableNotFoundForModification( operatorToken, dstName );
            return false;
        }
    }
    else
    {
        // self-assignment?
        if ( varDst == varSrc )
        {
            // It's only self-assignment if the dstVar is at the same scope.
            const BFFVariable * var = (dstFrame ? dstFrame : BFFStackFrame::GetCurrent())->GetLocalVar( dstName );
            if ( var )
            {
                return true;
            }

            // If at a parent scope, continue to lower logic which will create the
            // shadow copy at the current scope.
        }
    }

    // if dst exists, types must match
    BFFVariable::VarType srcType = varSrc->GetType();
    BFFVariable::VarType dstType;
    if ( varDst )
    {
        dstType = varDst->GetType();
    }
    else
    {
        ASSERT( concat == false );
        dstType = srcType;
    }

    // handle supported types

    if ( srcType != dstType )
    {
        const bool dstIsEmpty = ( varDst == nullptr ) ||
            ( dstType == BFFVariable::VAR_ARRAY_OF_STRINGS && varDst->GetArrayOfStrings().IsEmpty() ) ||
            ( dstType == BFFVariable::VAR_ARRAY_OF_STRUCTS && varDst->GetArrayOfStructs().IsEmpty() );
        const bool srcIsEmpty =
            ( srcType == BFFVariable::VAR_ARRAY_OF_STRINGS && varSrc->GetArrayOfStrings().IsEmpty() ) ||
            ( srcType == BFFVariable::VAR_ARRAY_OF_STRUCTS && varSrc->GetArrayOfStructs().IsEmpty() );

        // Mismatched - is there a supported conversion?

        // String to ArrayOfStrings or empty array
        if ( ( dstType == BFFVariable::VAR_ARRAY_OF_STRINGS || dstIsEmpty ) &&
             ( srcType == BFFVariable::VAR_STRING ) )
        {
            StackArray<AString> values;
            values.SetCapacity( varDst->GetArrayOfStrings().GetSize() + 1 );
            if ( concat )
            {
                if ( !dstIsEmpty )
                {
                    values.Append( varDst->GetArrayOfStrings() );
                }
                values.Append( varSrc->GetString() );
            }
            else if ( subtract )
            {
                if ( dstIsEmpty == false )
                {
                    auto end = varDst->GetArrayOfStrings().End();
                    for ( auto it = varDst->GetArrayOfStrings().Begin(); it!=end; ++it )
                    {
                        if ( *it != varSrc->GetString() ) // remove equal strings
                        {
                            values.Append( *it );
                        }
                    }
                }
            }
            else
            {
                values.Append( varSrc->GetString() );
            }

            BFFStackFrame::SetVarArrayOfStrings( dstName, values, dstFrame );
            return true;
        }

        // Struct to ArrayOfStructs or empty array, assignment or concatenation
        if ( ( dstType == BFFVariable::VAR_ARRAY_OF_STRUCTS || dstIsEmpty ) &&
             ( srcType == BFFVariable::VAR_STRUCT ) &&
             !subtract )
        {
            uint32_t num = (uint32_t)( 1 + ( ( concat && !dstIsEmpty ) ? varDst->GetArrayOfStructs().GetSize() : 0 ) );
            StackArray<const BFFVariable *> values;
            values.SetCapacity( num );
            if ( concat && !dstIsEmpty )
            {
                values.Append( varDst->GetArrayOfStructs() );
            }
            values.Append( varSrc );

            BFFStackFrame::SetVarArrayOfStructs( dstName, values, dstFrame );
            return true;
        }

        // Empty array to ArrayOfStrings, assignment or concatenation
        if ( dstType == BFFVariable::VAR_ARRAY_OF_STRINGS && srcIsEmpty && !subtract )
        {
            if ( concat )
            {
                // Avoid self-assignment by checking that the destination variable is not in destination scope.
                const BFFVariable * var = ( dstFrame ? dstFrame : BFFStackFrame::GetCurrent() )->GetLocalVar( dstName );
                if ( varDst != var )
                {
                    BFFStackFrame::SetVarArrayOfStrings( dstName, varDst->GetArrayOfStrings(), dstFrame );
                }
            }
            else
            {
                BFFStackFrame::SetVarArrayOfStrings( dstName, Array< AString >(), dstFrame );
            }
            return true;
        }

        // Empty array to ArrayOfStructs, assignment or concatenation
        if ( dstType == BFFVariable::VAR_ARRAY_OF_STRUCTS && srcIsEmpty && !subtract )
        {
            if ( concat )
            {
                // Avoid self-assignment by checking that the destination variable is not in destination scope.
                const BFFVariable * var = ( dstFrame ? dstFrame : BFFStackFrame::GetCurrent() )->GetLocalVar( dstName );
                if ( varDst != var )
                {
                    BFFStackFrame::SetVarArrayOfStructs( dstName, varDst->GetArrayOfStructs(), dstFrame );
                }
            }
            else
            {
                BFFStackFrame::SetVarArrayOfStructs(dstName, Array< const BFFVariable * >(), dstFrame);
            }
            return true;
        }

        // ArrayOfStrings to empty array, assignment or concatenation
        if ( dstIsEmpty && srcType == BFFVariable::VAR_ARRAY_OF_STRINGS && !subtract )
        {
            BFFStackFrame::SetVarArrayOfStrings( dstName, varSrc->GetArrayOfStrings(), dstFrame );
            return true;
        }

        // ArrayOfStructs to empty array, assignment or concatenation
        if ( dstIsEmpty && srcType == BFFVariable::VAR_ARRAY_OF_STRUCTS && !subtract )
        {
            BFFStackFrame::SetVarArrayOfStructs( dstName, varSrc->GetArrayOfStructs(), dstFrame );
            return true;
        }
    }
    else
    {
        // Matching Src and Dst

        if ( srcType == BFFVariable::VAR_STRING )
        {
            if ( concat )
            {
                AStackString< 2048 > finalValue(varDst->GetString());
                finalValue += varSrc->GetString();
                BFFStackFrame::SetVarString( dstName, finalValue, dstFrame );
            }
            else if ( subtract )
            {
                AStackString< 2048 > finalValue(varDst->GetString());
                finalValue.Replace( varSrc->GetString().Get(), "" );
                BFFStackFrame::SetVarString( dstName, finalValue, dstFrame );
            }
            else
            {
                BFFStackFrame::SetVarString( dstName, varSrc->GetString(), dstFrame );
            }
            return true;
        }

        if ( srcType == BFFVariable::VAR_ARRAY_OF_STRINGS && !subtract )
        {
            if ( concat )
            {
                const unsigned int num = (unsigned int)( varSrc->GetArrayOfStrings().GetSize() + varDst->GetArrayOfStrings().GetSize() );
                StackArray<AString> values;
                values.SetCapacity( num );
                values.Append( varDst->GetArrayOfStrings() );
                values.Append( varSrc->GetArrayOfStrings() );
                BFFStackFrame::SetVarArrayOfStrings( dstName, values, dstFrame );
            }
            else
            {
                BFFStackFrame::SetVarArrayOfStrings( dstName, varSrc->GetArrayOfStrings(), dstFrame );
            }
            return true;
        }

        if ( srcType == BFFVariable::VAR_ARRAY_OF_STRUCTS && !subtract)
        {
            if ( concat )
            {
                const unsigned int num = (unsigned int)( varSrc->GetArrayOfStructs().GetSize() + varDst->GetArrayOfStructs().GetSize() );
                StackArray<const BFFVariable *> values;
                values.SetCapacity( num );
                values.Append( varDst->GetArrayOfStructs() );
                values.Append( varSrc->GetArrayOfStructs() );
                BFFStackFrame::SetVarArrayOfStructs( dstName, values, dstFrame );
            }
            else
            {
                BFFStackFrame::SetVarArrayOfStructs( dstName, varSrc->GetArrayOfStructs(), dstFrame );
            }
            return true;
        }

        if ( srcType == BFFVariable::VAR_INT )
        {
            int32_t newVal;
            if ( concat )
            {
                newVal = varDst->GetInt() + varSrc->GetInt();
            }
            else if ( subtract )
            {
                newVal = varDst->GetInt() - varSrc->GetInt();
            }
            else
            {
                newVal = varSrc->GetInt();
            }
            return StoreVariableInt( dstName, newVal, dstFrame );
        }

        if ( ( srcType == BFFVariable::VAR_BOOL ) && !concat && !subtract )
        {
            return StoreVariableBool( dstName, varSrc->GetBool(), dstFrame );
        }

        if ( ( srcType == BFFVariable::VAR_STRUCT ) && !subtract )
        {
            const Array< const BFFVariable * > & srcMembers = varSrc->GetStructMembers();
            if ( concat )
            {
                BFFVariable *const newVar = BFFStackFrame::ConcatVars( dstName, varDst, varSrc, dstFrame, operatorToken );
                if ( newVar == nullptr )
                {
                    return false; // ConcatVars will have emitted an error
                }
            }
            else
            {
                // Register this variable
                BFFStackFrame::SetVarStruct( dstName, srcMembers, dstFrame );
            }
            return true;
        }
    }

    Error::Error_1034_OperationNotSupported( rhsToken,
                                             varDst ? varDst->GetType() : varSrc->GetType(),
                                             varSrc->GetType(),
                                             operatorToken );
    return false;
}

// PerformVariableSubstitutions
//------------------------------------------------------------------------------
/*static*/ bool BFFParser::PerformVariableSubstitutions( const BFFToken * inputToken,
                                                         AString & value )
{
    AStackString< 4096 > output;

    const char * src = inputToken->GetValueString().Get();
    const char * end = inputToken->GetValueString().GetEnd();

    // Ignore opening character for variables (which might be a parent scope ^)
    if ( inputToken->IsVariable() )
    {
        ASSERT( ( *src == '.' ) || ( *src == '^' ) );
        output += *src;
        ++src;
    }

    while ( src < end )
    {
        switch ( *src )
        {
            case '^':
            {
                src++; // skip escape char
                if ( src < end )
                {
                    output += *src; // append escaped character
                }
                break;
            }
            case '$':
            {
                src++; // skip opening $

                // find matching $
                const char *startName( src );
                const char * endName = nullptr;
                while ( src < end )
                {
                    if ( *src == '$' )
                    {
                        endName = src;
                        break;
                    }
                    src++;
                }
                if ( ( endName == nullptr ) ||
                     ( ( endName - startName ) < 1 ) )
                {
                    Error::Error_1028_MissingVariableSubstitutionEnd( inputToken ); // TODO: Improve error positioning
                    return false;
                }
                AStackString< MAX_VARIABLE_NAME_LENGTH > varName( startName, endName );
                const BFFVariable * var = BFFStackFrame::GetVarAny( varName );
                if ( var == nullptr )
                {
                    Error::Error_1009_UnknownVariable( inputToken, nullptr, varName ); // TODO: Improve error positioning
                    return false;
                }
                if ( var->IsBool() == true )
                {
                    output += ( ( var->GetBool() ) ? BFF_KEYWORD_TRUE : BFF_KEYWORD_FALSE );
                }
                else if ( var->IsInt() == true )
                {
                    output.AppendFormat( "%i", var->GetInt() );
                }
                else if ( var->IsString() == true )
                {
                    output += var->GetString();
                }
                else
                {
                    Error::Error_1029_VariableForSubstitutionIsNotAString( inputToken, varName, var->GetType() ); // TODO: Improve error positioning
                    return false;
                }
                break;
            }
            default:
            {
                output += *src;
                break;
            }
        }
        src++;
    }

    value = output;
    return true;
}

// CreateBuiltInVariables
//------------------------------------------------------------------------------
void BFFParser::CreateBuiltInVariables()
{
    // Handle special case in tests
    if ( FBuild::IsValid() == false )
    {
        return;
    }

    // TODO:B Add a mechanism to mark these variables as read-only

    // _WORKING_DIR_
    {
        AStackString<> varName( "._WORKING_DIR_" );
        ASSERT( BFFStackFrame::GetVarAny( AStackString<>( varName.Get() + 1 ) ) == nullptr );
        BFFStackFrame::SetVarString( varName, FBuild::Get().GetWorkingDir(), &m_BaseStackFrame );
        // TODO:B Add a mechanism to mark variable as read-only
    }

    // _FASTBUILD_VERSION_STRING_
    {
        AStackString<> varName( "._FASTBUILD_VERSION_STRING_" );
        ASSERT( BFFStackFrame::GetVarAny( AStackString<>( varName.Get() + 1 ) ) == nullptr );
        BFFStackFrame::SetVarString( varName, AStackString<>(FBUILD_VERSION_STRING), &m_BaseStackFrame );
        // TODO:B Add a mechanism to mark variable as read-only
    }

    // _FASTBUILD_VERSION_NUMBER_
    {
        AStackString<> varName( "._FASTBUILD_VERSION_" );
        ASSERT( BFFStackFrame::GetVarAny( AStackString<>( varName.Get() + 1 ) ) == nullptr );
        BFFStackFrame::SetVarInt( varName, (int32_t)FBUILD_VERSION, &m_BaseStackFrame );
        // TODO:B Add a mechanism to mark variable as read-only
    }
}

// SetBuiltInVariable_CurrentBFFDir
//------------------------------------------------------------------------------
void BFFParser::SetBuiltInVariable_CurrentBFFDir( const char * fileName )
{
    // Handle special case in tests
    if ( FBuild::IsValid() == false )
    {
        return;
    }

    // Early out if file has not changed.
    // - Note that this is the path without processing below so we can avoid
    //   doing that work if the path has not changed (i.e. the value stored here
    //   is not the same as the contents of the variable)
    if ( m_CurrentBFFDir == fileName )
    {
        return;
    }
    m_CurrentBFFDir = fileName;

    // Get absolute path to bff
    AStackString<> fullPath( fileName );
    NodeGraph::CleanPath( fullPath );

    // Get path to bff relative to working dir
    AStackString<> relativePath;
    PathUtils::GetRelativePath( FBuild::Get().GetWorkingDir(), fullPath, relativePath );

    // Get dir part only
    const char * lastSlash = relativePath.FindLast( NATIVE_SLASH );
    lastSlash = lastSlash ? lastSlash : relativePath.Get(); // handle no sub dirs
    relativePath.SetLength( (uint32_t)( lastSlash - relativePath.Get() ) );

    // Set the variable - always in the base scope
    const AStackString<> varName( "._CURRENT_BFF_DIR_" );
    BFFStackFrame::SetVarString( varName, relativePath, &m_BaseStackFrame );
    // TODO:B Add a mechanism to mark variable as read-only
}

//------------------------------------------------------------------------------
