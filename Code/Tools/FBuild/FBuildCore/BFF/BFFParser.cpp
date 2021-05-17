// BFFParser
//------------------------------------------------------------------------------

// Includes
//------------------------------------------------------------------------------
#include "BFFParser.h"
#include "BFFKeywords.h"
#include "BFFStackFrame.h"
#include "Tools/FBuild/FBuildCore/BFF/BFFFile.h"
#include "Tools/FBuild/FBuildCore/BFF/Functions/Function.h"
#include "Tools/FBuild/FBuildCore/BFF/Tokenizer/BFFTokenizer.h"
#include "Tools/FBuild/FBuildCore/BFF/Tokenizer/BFFTokenRange.h"
#include "Tools/FBuild/FBuildCore/FBuild.h"
#include "Tools/FBuild/FBuildCore/Graph/NodeGraph.h"
#include "Tools/FBuild/FBuildCore/Helpers/BuildProfiler.h"
#include "Tools/FBuild/FBuildCore/FLog.h"
#include "Tools/FBuild/FBuildCore/FBuildVersion.h"

// Core
#include "Core/Env/Assert.h"
#include "Core/FileIO/PathUtils.h"
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
    PROFILE_FUNCTION;
    BuildProfilerScope buildProfileScope( "ParseBFF" );

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
    return Parse( range );
}

// ParseFromString
//------------------------------------------------------------------------------
bool BFFParser::ParseFromString( const char * fileName, const char * fileContents )
{
    PROFILE_FUNCTION;

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
    return Parse( range );
}

// Parse
//------------------------------------------------------------------------------
bool BFFParser::Parse( BFFTokenRange & iter )
{
    // Check for excessive depth (deeply recusive function calls for example)
    if ( BFFStackFrame::GetDepth() >= 128 )
    {
        Error::Error_1035_ExcessiveDepthComplexity( iter.GetCurrent() );
        return false;
    }

    while ( iter.IsAtEnd() == false )
    {
        // Handle updating current bff path variable
        SetBuiltInVariable_CurrentBFFDir( iter->GetSourceFile() );

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
        if ( token->IsCurlyBracket( '{' ) )
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

        // User-defined function (declaration)
        if ( token->IsKeyword( BFF_KEYWORD_FUNCTION ) )
        {
            if ( ParseUserFunctionDeclaration( iter ) == false )
            {
                return false;
            }
            continue;
        }

        // User-defined function (invocation)
        if ( token->IsIdentifier() )
        {
            // Is it a used function?
            const BFFUserFunction * func = GetUserFunction( token->GetValueString() );
            if ( func )
            {
                // Try and invoke it
                if ( ParseUserFunctionCall( iter, *func ) == false )
                {
                    return false;
                }
                continue;
            }

            // Not a user function - fallthrough
        }

        // End of File
        if ( token->GetType() == BFFTokenType::EndOfFile )
        {
            iter++;
            ASSERT( iter.IsAtEnd() ); // Should always be last arg
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
    // Variable must be followed by an operator
    if ( iter->IsOperator() == false )
    {
        Error::Error_1044_OperatorExpected( iter.GetCurrent() );
        return false;
    }
    const BFFToken * opToken = iter.GetCurrent();
    iter++; // Consume operator

    // Check this is an operator we support
    if ( ( opToken->IsOperator( BFF_VARIABLE_ASSIGNMENT ) == false ) &&
         ( opToken->IsOperator( BFF_VARIABLE_CONCATENATION ) == false ) &&
         ( opToken->IsOperator( BFF_VARIABLE_SUBTRACTION ) == false ) )
    {
        Error::Error_1034_OperationNotSupported( opToken, BFFVariable::VAR_ANY, BFFVariable::VAR_ANY, opToken );
        return false;
    }

    // What operator type is this?
    const bool concat = opToken->IsOperator( BFF_VARIABLE_CONCATENATION );
    const bool subtract = opToken->IsOperator( BFF_VARIABLE_SUBTRACTION );

    const BFFToken * rhsToken = iter.GetCurrent();
    if ( rhsToken->IsString() )
    {
        iter++; // Consume the rhs
        return StoreVariableString( varName, rhsToken, opToken, frame );
    }
    else if ( rhsToken->IsCurlyBracket( '{' ) ) // Open Scope
    {
        BFFTokenRange bracedRange;
        if ( FindBracedRange( iter, bracedRange ) == false )
        {
            return false; // FindBracedRange will have emitted an error
        }
        return StoreVariableArray( varName, bracedRange, opToken, frame );
    }
    else if ( rhsToken->IsSquareBracket( '[' ) ) // Open Struct
    {
        BFFTokenRange bracedRange;
        if ( FindBracedRange( iter, bracedRange ) == false )
        {
            return false; // FindBracedRange will have emitted an error
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
    if ( iter->IsRoundBracket( '(' ) )
    {
        // Check if this function can have a header
        if ( func->AcceptsHeader() == false )
        {
            Error::Error_1021_UnexpectedHeaderForFunction( iter.GetCurrent(), func );
            return false;
        }

        // Find limits of args inside brackets
        if ( FindBracedRange( iter, headerRange, func ) == false )
        {
            return false; // FindBracedRange will have emitted an error
        }
    }

    // Is a required header missing?
    if ( func->NeedsHeader() && headerRange.IsEmpty() )
    {
        Error::Error_1023_FunctionRequiresAHeader( iter.GetCurrent(), func );
        return false;
    }

    // Handle function body
    BFFTokenRange bodyRange;
    if ( func->NeedsBody() )
    {
        // Check for body start
        if ( iter->IsCurlyBracket( '{' ) == false )
        {
            Error::Error_1024_FunctionRequiresABody( iter.GetCurrent(), func );
            return false;
        }

        // Find close of body
        if ( FindBracedRange( iter, bodyRange, func ) == false )
        {
            return false; // FindBracedRange will have emitted an error
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
    ASSERT( iter->IsCurlyBracket( '{' ) );

    // create stack for scope
    BFFStackFrame stackFrame;

    // Find limits of args inside brackets
    BFFTokenRange range;
    if ( FindBracedRange( iter, range ) == false )
    {
        return false; // FindBracedRange will have emitted an error
    }

    // parse the scoped part
    if ( Parse( range ) == false )
    {
        return false;
    }

    return true;
}

// ParseUserFunctionDeclaration
//------------------------------------------------------------------------------
bool BFFParser::ParseUserFunctionDeclaration( BFFTokenRange & iter )
{
    ASSERT( iter->IsKeyword( BFF_KEYWORD_FUNCTION) );
    iter++;

    // Get function name
    if ( iter->IsIdentifier() == false )
    {
        Error::Error_1107_ExpectedFunctionNameFollowingFunctionKeyword( iter.GetCurrent() );
        return false;
    }
    const BFFToken * functionName = iter.GetCurrent();
    iter++;

    // Check function is not already declared
    if ( FBuild::Get().GetUserFunctions().FindFunction( functionName->GetValueString() ) )
    {
        Error::Error_1108_FunctionAlreadyDefined( functionName );
        return false;
    }

    // Find parameter declaration range
    if ( iter-> IsRoundBracket( '(' ) == false )
    {
        Error::Error_1023_FunctionRequiresAHeader( iter.GetCurrent(), nullptr );
        return false;
    }
    BFFTokenRange header;
    if ( FindBracedRange( iter, header ) == false )
    {
        return false; // FindBracedRange will have emitted an error
    }

    // Validate/parse argument declarations (ok to be empty)
    StackArray< const BFFToken * > arguments;
    while ( header.IsAtEnd() == false )
    {
        // Get parameter name
        if ( header->IsVariable() == false )
        {
            Error::Error_1007_ExpectedVariable( header.GetCurrent(), nullptr );
            return false;
        }
        const BFFToken * newArg = header.GetCurrent();
        header++;

        // TODO:B Support default values for arguments

        // Check arg is not already defined
        for ( const BFFToken * existingArg : arguments )
        {
            if ( newArg->GetValueString() == existingArg->GetValueString() )
            {
                Error::Error_1109_FunctionArgumentAlreadyDefined( newArg );
                return false;
            }
        }

        // Store arg
        arguments.Append( newArg );

        // Allow optional commas between args
        if ( header->IsComma() )
        {
            header++;
        }
    }

    // Find body
    if ( iter->IsCurlyBracket( '{' ) == false )
    {
        Error::Error_1024_FunctionRequiresABody( iter.GetCurrent(), nullptr );
        return false;
    }
    BFFTokenRange bodyRange;
    if ( FindBracedRange( iter, bodyRange ) == false )
    {
        return false; // FindBracedRange will have emitted an error
    }

    // Store function
    FBuild::Get().GetUserFunctions().AddFunction( functionName->GetValueString(), arguments, bodyRange );
    return true;
}

// ParseUserFunctionCall
//------------------------------------------------------------------------------
bool BFFParser::ParseUserFunctionCall( BFFTokenRange & iter, const BFFUserFunction & function )
{
    ASSERT( iter->IsIdentifier() ); // Pointing to the function name
    const BFFToken * functionToken = iter.GetCurrent();
    iter++;

    // Find arguments block
    if ( iter->IsRoundBracket( '(' ) == false )
    {
        Error::Error_1110_ExpectedArgumentBlockForFunctionCall( iter.GetCurrent() );
        return false;
    }
    BFFTokenRange argsIter;
    if ( FindBracedRange( iter, argsIter ) == false )
    {
        return false; // FindBracedRange will have emitted an error
    }

    // Get arguments
    StackArray< const BFFToken * > arguments;
    while ( argsIter.IsAtEnd() == false )
    {
        // Get parameter
        if ( argsIter->IsString() ||
             argsIter->IsBooelan() ||
             argsIter->IsNumber() )
        {
            // Literal value
            arguments.Append( argsIter.GetCurrent() );
        }
        else if ( argsIter->IsVariable() )
        {
            // Variable
            arguments.Append( argsIter.GetCurrent() );
        }
        else
        {
            Error::Error_1112_FunctionCallExpectedArgument( argsIter.GetCurrent() );
            return false;
        }
        argsIter++;

        // Allow optional commas between args
        if ( argsIter->IsComma() )
        {
            argsIter++;
        }
    }

    // Check arguments against function signature
    const Array< const BFFToken * > & expectedArgs = function.GetArgs();
    if ( arguments.GetSize() != expectedArgs.GetSize() )
    {
        Error::Error_1111_FunctionCallArgumentMismatch( functionToken,
                                                        (uint32_t)arguments.GetSize(),
                                                        (uint32_t)expectedArgs.GetSize() );
        return false;
    }

    // Function call has its own stack frame
    BFFStackFrame frame;

    // Push args into function call stack frame
    const size_t numArgs = arguments.GetSize();
    for ( size_t i = 0; i < numArgs; ++i )
    {
        const BFFToken * expectedArg = expectedArgs[ i ];
        const AString & argName = expectedArg->GetValueString();
        const BFFToken * arg = arguments[ i ];
        if ( arg->IsString() )
        {
            // unescape and subsitute embedded variables
            AStackString< 2048 > value;
            if ( PerformVariableSubstitutions( arg, value ) == false )
            {
                return false;
            }
            BFFStackFrame::SetVarString( argName, value, &frame );
        }
        else if ( arg->IsBooelan() )
        {
            BFFStackFrame::SetVarBool( argName, arg->GetBoolean(), &frame );
        }
        else if ( arg->IsNumber() )
        {
            BFFStackFrame::SetVarInt( argName, arg->GetValueInt(), &frame );
        }
        else
        {
            ASSERT( arg->IsVariable() );

            // a variable, possibly with substitutions
            AStackString<> srcVarName;
            bool srcParentScope;
            if ( ParseVariableName( arg, srcVarName, srcParentScope ) == false )
            {
                return false; // ParseVariableName will have emitted an error
            }

            // Determine stack frame to use for Src var
            BFFStackFrame * srcFrame = BFFStackFrame::GetCurrent();
            if ( srcParentScope )
            {
                srcVarName[ 0 ] = BFF_DECLARE_VAR_INTERNAL;
                srcFrame = BFFStackFrame::GetCurrent()->GetParent();
            }

            // get the variable
            const BFFVariable * varSrc = srcFrame ? srcFrame->GetVariableRecurse( srcVarName ) : nullptr;
            if ( varSrc == nullptr )
            {
                Error::Error_1009_UnknownVariable( arg, nullptr, srcVarName );
                return false;
            }

            // Set in function frame with argument name
            BFFStackFrame::SetVar( varSrc, argName, &frame );
        }
    }

    // Isolate stack frame (function calls cannot access anything outside
    // of their own scope)
    frame.DisconnectStackChain();

    // Invoke function
    BFFTokenRange functionImplementation( function.GetBodyTokenRange() );
    if ( Parse( functionImplementation ) == false )
    {
        return false; // Parse will have emitted an error
    }

    return true;
}

// FindBracedRange
//------------------------------------------------------------------------------
bool BFFParser::FindBracedRange( BFFTokenRange & iter, BFFTokenRange & outBracedRange, const Function * function ) const
{
    // Determine the matching close character
    const BFFToken * openToken = iter.GetCurrent();
    char closeTokenChar;
    switch( openToken->GetType() )
    {
        case BFFTokenType::CurlyBracket:    closeTokenChar = '}'; break;
        case BFFTokenType::RoundBracket:    closeTokenChar = ')'; break;
        case BFFTokenType::SquareBracket:   closeTokenChar = ']'; break;
        default:                            ASSERT(false); return false;
    }

    // Take note of range begin
    const BFFToken * begin = iter.GetCurrent() + 1; // First token after brace
    if ( FindBracedRangeRecurse( iter ) == false)
    {
        Error::Error_1002_MatchingClosingTokenNotFound( openToken, function, closeTokenChar );
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
    // Determine the matching close character
    const BFFToken * openToken = iter.GetCurrent();
    char closeTokenChar;
    switch( openToken->GetType() )
    {
        case BFFTokenType::CurlyBracket:    closeTokenChar = '}'; break;
        case BFFTokenType::RoundBracket:    closeTokenChar = ')'; break;
        case BFFTokenType::SquareBracket:   closeTokenChar = ']'; break;
        default: ASSERT(false); return false;
    }
    iter++;

    while ( iter.IsAtEnd() == false )
    {
        // found bracket?
        if ( iter.GetCurrent()->GetType() == openToken->GetType() )
        {
            // Is it a close bracket?
            if ( iter.GetCurrent()->GetValueString()[ 0 ] == closeTokenChar )
            {
                return true;
            }

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
                for ( const AString & it : var->GetArrayOfStrings() )
                {
                    if ( it != value ) // remove equal strings
                    {
                        finalValues.Append( it );
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
    BFFTokenRange iter( tokenRange );
    while ( iter.IsAtEnd() == false )
    {
        if ( iter->IsString() )
        {
            // a quoted string

            // dest is consistent?
            if ( ( varType != BFFVariable::VAR_ARRAY_OF_STRINGS ) &&
                 ( varType != BFFVariable::VAR_ANY ) )
            {
                // Mixed types in vector
                Error::Error_1034_OperationNotSupported( iter.GetCurrent(),
                                                         varType,
                                                         BFFVariable::VAR_STRING,
                                                         opToken );
                return false;
            }

            // subtraction not supported on arrays
            if ( opToken->IsOperator( BFF_VARIABLE_SUBTRACTION ) )
            {
                Error::Error_1034_OperationNotSupported( iter.GetCurrent(),
                                                         varType,
                                                         BFFVariable::VAR_STRING,
                                                         opToken );
                return false;
            }

            // a string
            AStackString< 2048 > elementValue;

            // unescape and subsitute embedded variables
            if ( PerformVariableSubstitutions( iter.GetCurrent(), elementValue ) == false )
            {
                return false;
            }

            varType = BFFVariable::VAR_ARRAY_OF_STRINGS;
            values.Append( elementValue );
        }
        else if ( iter->IsVariable() )
        {
            // a variable, possibly with substitutions
            AStackString<> srcVarName;
            bool srcParentScope;
            if ( ParseVariableName( iter.GetCurrent(), srcVarName, srcParentScope ) == false )
            {
                return false; // ParseVariableName will have emitted an error
            }

            // Determine stack frame to use for Src var
            BFFStackFrame * srcFrame = BFFStackFrame::GetCurrent();
            if ( srcParentScope )
            {
                srcVarName[ 0 ] = BFF_DECLARE_VAR_INTERNAL;
                srcFrame = BFFStackFrame::GetCurrent()->GetParent();
            }

            // get the variable
            const BFFVariable * varSrc = srcFrame ? srcFrame->GetVariableRecurse( srcVarName ) : nullptr;
            if ( varSrc == nullptr )
            {
                Error::Error_1009_UnknownVariable( iter.GetCurrent(), nullptr, srcVarName );
                return false;
            }

            // subtraction not supported on arrays
            if ( opToken->IsOperator( BFF_VARIABLE_SUBTRACTION ) )
            {
                Error::Error_1034_OperationNotSupported( iter.GetCurrent(),
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
                    Error::Error_1034_OperationNotSupported( iter.GetCurrent(),
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
                    Error::Error_1034_OperationNotSupported( iter.GetCurrent(),
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
                Error::Error_1050_PropertyMustBeOfType( iter.GetCurrent(), nullptr, name.Get(),
                                                        varType,
                                                        BFFVariable::VAR_STRING,
                                                        BFFVariable::VAR_STRUCT );
                return false;
            }
        }
        else
        {
            Error::Error_1001_MissingStringStartToken( iter.GetCurrent(), nullptr );
            return false;
        }

        iter++; // Entry has been handled

        if ( iter->IsComma() ) // comma separators are optional
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
    const BFFStackFrame * const srcFrame = ( srcParentScope )
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
    const BFFVariable::VarType srcType = varSrc->GetType();
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
                    for ( const AString & it : varDst->GetArrayOfStrings() )
                    {
                        if ( it != varSrc->GetString() ) // remove equal strings
                        {
                            values.Append( it );
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
            const uint32_t num = (uint32_t)( 1 + ( ( concat && !dstIsEmpty ) ? varDst->GetArrayOfStructs().GetSize() : 0 ) );
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
                const BFFVariable * const newVar = BFFStackFrame::ConcatVars( dstName, varDst, varSrc, dstFrame, operatorToken );
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
void BFFParser::SetBuiltInVariable_CurrentBFFDir( const BFFFile & file )
{
    // Early out if file has not changed.
    if ( m_CurrentBFFFile == &file )
    {
        return;
    }
    m_CurrentBFFFile = &file;

    // Handle special case in tests
    if ( FBuild::IsValid() == false )
    {
        return;
    }

    // Get absolute path to bff
    AStackString<> fullPath( file.GetFileName() );
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

// GetUserFunction
//------------------------------------------------------------------------------
BFFUserFunction * BFFParser::GetUserFunction( const AString & name )
{
    // Handle no FBuild (for tests)
    if ( FBuild::IsValid() == false )
    {
        return nullptr;
    }

    return FBuild::Get().GetUserFunctions().FindFunction( name );
}

//------------------------------------------------------------------------------
