// BFFParser
//------------------------------------------------------------------------------

// Includes
//------------------------------------------------------------------------------
#include "Tools/FBuild/FBuildCore/PrecompiledHeader.h"

#include "BFFParser.h"
#include "BFFIterator.h"
#include "BFFMacros.h"
#include "BFFStackFrame.h"
#include "Tools/FBuild/FBuildCore/FBuild.h"
#include "Tools/FBuild/FBuildCore/Graph/NodeGraph.h"
#include "Tools/FBuild/FBuildCore/FLog.h"
#include "Tools/FBuild/FBuildCore/BFF/Functions/Function.h"

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

// Static Data
//------------------------------------------------------------------------------
/*static*/ uint32_t BFFParser::s_Depth( 0 );

// CONSTRUCTOR
//------------------------------------------------------------------------------
BFFParser::BFFParser( NodeGraph & nodeGraph )
: m_SeenAVariable( false )
, m_LastVarFrame( nullptr )
, m_NodeGraph( nodeGraph )
{
    ++s_Depth;
}

// DESTRUCTOR
//------------------------------------------------------------------------------
BFFParser::~BFFParser()
{
    ASSERT( s_Depth > 0 );
    --s_Depth;
}

// Parse
//------------------------------------------------------------------------------
bool BFFParser::Parse( const char * dataWithSentinel,
                       uint32_t sizeExcludingSentinel,
                       const char * fileName,
                       uint64_t fileTimeStamp,
                       uint64_t fileDataHash,
                       bool pushStackFrame )
{
    PROFILE_FUNCTION

    // data should be 1 bytes larger than size, with a sentinel
    ASSERT( dataWithSentinel[ sizeExcludingSentinel ] == '\000' );

    if ( FBuild::IsValid() ) // cope with null for unit tests
    {
        // NOTE: filename may or may not be clean already - ok to do twice
        AStackString<> fileNameClean;
        NodeGraph::CleanPath( AStackString<>( fileName ), fileNameClean );
        m_NodeGraph.AddUsedFile( fileNameClean, fileTimeStamp, fileDataHash );
    }

    // parse it
    BFFIterator iter( dataWithSentinel, sizeExcludingSentinel, fileName, fileTimeStamp );
    if ( pushStackFrame )
    {
        BFFStackFrame stackFrame;
        return Parse( iter );
    }
    else
    {
        return Parse( iter );
    }
}

// Parse
//------------------------------------------------------------------------------
bool BFFParser::Parse( BFFIterator & iter )
{
    for (;;)
    {
        iter.SkipWhiteSpace();

        // is this a comment?
        if ( iter.IsAtComment() )
        {
            iter.SkipComment();
            continue;
        }

        const char c = *iter;
        switch ( c )
        {
            case BFF_DECLARE_VAR_INTERNAL:
            case BFF_DECLARE_VAR_PARENT:
            {
                if ( ParseNamedVariableDeclaration( iter ) == false )
                {
                    return false;
                }
                continue;
            }
            case BFF_VARIABLE_CONCATENATION:
            case BFF_VARIABLE_SUBTRACTION:
            {
                // concatenation to last used variable
                if ( ParseUnnamedVariableModification( iter ) == false )
                {
                    return false;
                }
                continue;
            }
            case BFF_SCOPE_OPEN:
            {
                // start an unnamed scope
                if ( ParseUnnamedScope( iter ) == false )
                {
                    return false;
                }
                continue;
            }
            case BFF_PREPROCESSOR_START:
            {
                if ( ParsePreprocessorDirective( iter ) == false )
                {
                    return false;
                }
                continue;
            }
            default:
            {
                if ( iter.IsAtValidFunctionNameCharacter() )
                {
                    if ( ParseFunction( iter ) == false )
                    {
                        return false;
                    }
                    continue;
                }
            }
        }

        iter.SkipWhiteSpace();
        if ( iter.IsAtEnd() == false )
        {
            Error::Error_1010_UnknownConstruct( iter );
            return false;
        }

        break;  // cleanly hit end of file
    }

    return true;
}

// ParseNamedVariableName
//------------------------------------------------------------------------------
/*static*/ bool BFFParser::ParseVariableName( BFFIterator & iter, AString & name, bool & parentScope )
{
    // skip over the declaration symbol
    ASSERT( *iter == BFF_DECLARE_VAR_INTERNAL ||
            *iter == BFF_DECLARE_VAR_PARENT );

    parentScope = ( *iter == BFF_DECLARE_VAR_PARENT );

    const BFFIterator varNameStart = iter; // include type token in var name
    iter++;

    // make sure we haven't hit the end of the file
    if ( iter.IsAtEnd() )
    {
        Error::Error_1012_UnexpectedEndOfFile( iter );
        return false;
    }

    if ( *iter == '\'' || *iter == '"' )
    {
        // parse the string
        const BFFIterator openToken = iter;
        iter.SkipString( *openToken );
        if ( *iter != *openToken )
        {
            Error::Error_1002_MatchingClosingTokenNotFound( openToken, nullptr, *openToken );
            return false;
        }
        BFFIterator stringStart = openToken;
        stringStart++;

        // unescape and subsitute embedded variables
        AStackString< 256 > value;
        if ( PerformVariableSubstitutions( stringStart, iter, value ) == false )
        {
            return false;
        }
        iter++; // skip close token

        BFFIterator varNameIter( value.Get(), value.GetLength(), iter.GetFileName(), iter.GetFileTimeStamp() );

        // sanity check it is a sensible length
        if ( value.GetLength() + 1/* '.' will be added */  > MAX_VARIABLE_NAME_LENGTH )
        {
            Error::Error_1014_VariableNameIsTooLong( varNameIter, (uint32_t)value.GetLength(), (uint32_t)MAX_VARIABLE_NAME_LENGTH );
            return false;
        }

        // sanity check it is a valid variable name
        while ( varNameIter.IsAtEnd() == false )
        {
            if ( varNameIter.IsAtValidVariableNameCharacter() == false )
            {
                Error::Error_1013_UnexpectedCharInVariableName( varNameIter, nullptr );
                return false;
            }
            varNameIter++;
        }

        // append '.' to variable name
        name = ".";
        name.Append( value );
    }
    else
    {
        // make sure immediately after the symbol starts a variable name
        if ( iter.IsAtValidVariableNameCharacter() == false )
        {
            Error::Error_1013_UnexpectedCharInVariableName( iter, nullptr );
            return false;
        }

        // find the end of the variable name
        iter.SkipVariableName();
        const BFFIterator varNameEnd = iter;

        // sanity check it is a sensible length
        size_t varNameLen = varNameStart.GetDistTo( varNameEnd );
        if ( varNameLen > MAX_VARIABLE_NAME_LENGTH )
        {
            Error::Error_1014_VariableNameIsTooLong( iter, (uint32_t)varNameLen, (uint32_t)MAX_VARIABLE_NAME_LENGTH );
            return false;
        }

        // store variable name
        name.Assign( varNameStart.GetCurrent(), varNameEnd.GetCurrent() );
    }

    ASSERT( name.GetLength() > 0 );
    if ( parentScope )
    {
        // exchange '^' with '.'
        ASSERT( BFF_DECLARE_VAR_PARENT == name[0] );
        name[0] = BFF_DECLARE_VAR_INTERNAL;
    }

    return true;
}

// ParseUnnamedVariableModification
//------------------------------------------------------------------------------
bool BFFParser::ParseUnnamedVariableModification( BFFIterator & iter )
{
    ASSERT( ( *iter == BFF_VARIABLE_CONCATENATION ) ||
            ( *iter == BFF_VARIABLE_SUBTRACTION ) );

    // have we assigned a variable before?
    if ( m_SeenAVariable == false )
    {
        Error::Error_1011_UnnamedModifcationMustFollowAssignment( iter );
        return false;
    }

    return ParseVariableDeclaration( iter, m_LastVarName, m_LastVarFrame );
}

// ParseNamedVariableDeclaration
//------------------------------------------------------------------------------
bool BFFParser::ParseNamedVariableDeclaration( BFFIterator & iter )
{
    const BFFIterator varNameStart( iter );

    bool parentScope = false;
    if ( ParseVariableName( iter, m_LastVarName, parentScope ) == false )
    {
        return false; // ParseVariableName() would have display an error
    }

    // find the start of the assignment
    iter.SkipWhiteSpaceAndComments();
    if ( iter.IsAtEnd() )
    {
        Error::Error_1012_UnexpectedEndOfFile( iter );
        return false;
    }

    // check if points to a previous declaration in a parent scope
    const BFFVariable * parentVar = nullptr;
    m_LastVarFrame = ( parentScope )
        ? BFFStackFrame::GetParentDeclaration( m_LastVarName, nullptr, parentVar )
        : nullptr;

    if ( parentScope )
    {
        // check if a parent definition exists
        if ( nullptr == m_LastVarFrame )
        {
            Error::Error_1009_UnknownVariable( varNameStart, nullptr );
            return false;
        }

        ASSERT( nullptr != parentVar );

        // check if the parent definition is frozen
        if ( parentVar->Frozen() )
        {
            Error::Error_1060_CantModifyFrozenVar( varNameStart, nullptr, parentVar );
            return false;
        }
    }

    return ParseVariableDeclaration( iter, m_LastVarName, m_LastVarFrame );
}

// ParseVariableDeclaration
//------------------------------------------------------------------------------
bool BFFParser::ParseVariableDeclaration( BFFIterator & iter, const AString & varName, BFFStackFrame * frame )
{
    m_SeenAVariable = true;

    // look for an appropriate operator
    BFFIterator operatorIter( iter );
    const bool concat = ( *operatorIter == BFF_VARIABLE_CONCATENATION );
    const bool subtract = ( *operatorIter == BFF_VARIABLE_SUBTRACTION );
    if ( *iter != BFF_VARIABLE_ASSIGNMENT && !concat && !subtract )
    {
        Error::Error_1016_UnexepectedCharFollowingVariableName( iter );
        return false;
    }

    // skip the assignment symbol and whitespace
    iter++;
    iter.SkipWhiteSpaceAndComments();
    if ( iter.IsAtEnd() )
    {
        Error::Error_1012_UnexpectedEndOfFile( iter );
        return false;
    }

    char openToken = *iter;
    char closeToken = 0;
    bool ok = false;
    if ( ( openToken == '"' ) || ( openToken == '\'' ) )
    {
        closeToken = openToken;
        ok = true;
    }
    else if ( openToken == BFF_SCOPE_OPEN )
    {
        closeToken = BFF_SCOPE_CLOSE;
        ok = true;
    }
    else if ( openToken == BFF_STRUCT_OPEN )
    {
        closeToken = BFF_STRUCT_CLOSE;
        ok = true;
    }
    else if ( ( openToken >= '0' ) && ( openToken <= '9' ) )
    {
        // integer value?
        BFFIterator startIntValue( iter );
        while ( iter.IsAtEnd() == false )
        {
            iter++;
            if ( ( *iter < '0' ) || ( *iter > '9' ) )
            {
                break; // end of integer
            }
        }
        if ( startIntValue.GetDistTo( iter ) > 10 )
        {
            Error::Error_1018_IntegerValueCouldNotBeParsed( startIntValue );
            return false;
        }
        AStackString<> intAsString( startIntValue.GetCurrent(), iter.GetCurrent() );
        int i = 0;
        if ( sscanf( intAsString.Get(), "%i", &i ) != 1 )
        {
            Error::Error_1018_IntegerValueCouldNotBeParsed( startIntValue );
            return false;
        }

        // find existing
        const BFFVariable * var = BFFStackFrame::GetVar( varName, frame );

        // variable type must match
        if ( ( var != nullptr ) && !var->IsInt() )
        {
            Error::Error_1034_OperationNotSupported( startIntValue, var->GetType(), BFFVariable::VAR_INT, operatorIter );
            return false;
        }

        // variable must exist, if we are going to modify it
        if ( ( concat || subtract ) && ( var == nullptr ) )
        {
            Error::Error_1026_VariableNotFoundForModification( operatorIter, varName );
            return false;
        }

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
    else if ( ( *iter == 't' ) || ( *iter == 'f' ) )
    {
        // bool value?
        BFFIterator startBoolValue( iter );

        bool value = false; // just to silence C4701 from MSVC
        ok = false;
        if ( iter.ParseExactString( "true" ) )
        {
            value = true;
            ok = true;
        }
        else if ( iter.ParseExactString( "false" ) )
        {
            value = false;
            ok = true;
        }

        if ( ok )
        {
            // find existing
            const BFFVariable * var = BFFStackFrame::GetVar( varName, frame );

            // are we concatenating?
            if ( concat || subtract )
            {
                // concatenation of bools not supported
                Error::Error_1027_CannotModify( operatorIter, varName, BFFVariable::VAR_BOOL, BFFVariable::VAR_ANY );
                return false;
            }
            else
            {
                // variable must be new or a bool
                if ( !( ( var == nullptr ) || ( var->IsBool() ) ) )
                {
                    Error::Error_1034_OperationNotSupported( startBoolValue, var->GetType(), BFFVariable::VAR_BOOL, operatorIter );
                    return false;
                }
            }

            return StoreVariableBool( varName, value, frame );
        }
    }
    else if ( *iter == BFF_DECLARE_VAR_INTERNAL ||
              *iter == BFF_DECLARE_VAR_PARENT )
    {
        return StoreVariableToVariable( varName, iter, operatorIter, frame );
    }

    if ( !ok )
    {
        Error::Error_1017_UnexepectedCharInVariableValue( iter );
        return false;
    }

    bool result = false;

    // find closing token
    BFFIterator openTokenPos( iter );
    BFFIterator openTokenForError( iter ); // take note of opening token pos for possible error
    openTokenPos++; // more start iter to first char of value
    if ( openToken == BFF_SCOPE_OPEN )
    {
        if ( iter.ParseToMatchingBrace( openToken, closeToken ) )
        {
            result = StoreVariableArray( varName, openTokenPos, iter, operatorIter, frame );
        }
        else
        {
            Error::Error_1002_MatchingClosingTokenNotFound( openTokenForError, nullptr, closeToken );
        }
    }
    else if ( openToken == BFF_STRUCT_OPEN )
    {
        if ( iter.ParseToMatchingBrace( openToken, closeToken ) )
        {
            result = StoreVariableStruct( varName, openTokenPos, iter, operatorIter, frame );
        }
        else
        {
            Error::Error_1002_MatchingClosingTokenNotFound( openTokenForError, nullptr, closeToken );
        }
    }
    else
    {
        ASSERT( ( openToken == '\'' ) || ( openToken == '"' ) );
        iter.SkipString( closeToken );
        if ( *iter == closeToken )
        {
            result = StoreVariableString( varName, openTokenPos, iter, operatorIter, frame );
        }
        else
        {
            Error::Error_1002_MatchingClosingTokenNotFound( openTokenForError, nullptr, closeToken );
        }
    }

    if ( result )
    {
        iter++; // skip over the end token
    }

    // StoreVariable will have emitted an error if there was one
    return result;
}

// ParseFunction
//------------------------------------------------------------------------------
bool BFFParser::ParseFunction( BFFIterator & iter )
{
    ASSERT( iter.IsAtValidFunctionNameCharacter() );

    // for variables to be used by this function
    BFFStackFrame stackFrame;

    BFFIterator functionNameStart( iter );
    iter.SkipFunctionName();
    if ( iter.IsAtEnd() )
    {
        Error::Error_1012_UnexpectedEndOfFile( iter );
        return false;
    }

    // check length
    if ( functionNameStart.GetDistTo( iter ) > MAX_FUNCTION_NAME_LENGTH )
    {
        // if it's too long, then it can't be a valid function
        Error::Error_1015_UnknownFunction( functionNameStart );
        return false;
    }

    // store function name
    AStackString<MAX_FUNCTION_NAME_LENGTH> functionName( functionNameStart.GetCurrent(), iter.GetCurrent() );
    const Function * func = Function::Find( functionName );
    if ( func == nullptr )
    {
        Error::Error_1015_UnknownFunction( functionNameStart );
        return false;
    }
    iter.SkipWhiteSpace();

    if ( func->IsUnique() && func->GetSeen() )
    {
        Error::Error_1020_FunctionCanOnlyBeInvokedOnce( functionNameStart, func );
        return false;
    }
    func->SetSeen();

    FLOG_INFO( "Function call '%s'", functionName.Get() );

    // header, or body?
    bool hasHeader = false;
    BFFIterator functionArgsStartToken( iter );
    BFFIterator functionArgsStopToken( iter );
    if ( *iter == BFF_FUNCTION_ARGS_OPEN )
    {
        // can this function accept a header?
        if ( func->AcceptsHeader() == false )
        {
            Error::Error_1021_UnexpectedHeaderForFunction( iter, func );
            return false;
        }

        // args
        if ( iter.ParseToMatchingBrace( BFF_FUNCTION_ARGS_OPEN, BFF_FUNCTION_ARGS_CLOSE ) == false )
        {
            Error::Error_1022_MissingFunctionHeaderCloseToken( functionArgsStartToken, func );
            return false;
        }
        functionArgsStopToken = iter;
        hasHeader = true;
        iter++; // skip over closing token
        iter.SkipWhiteSpaceAndComments();
    }

    if ( func->NeedsHeader() && ( hasHeader == false ) )
    {
        Error::Error_1023_FunctionRequiresAHeader( iter, func );
        return false;
    }

    // some functions have no body
    bool hasBody = false;

    BFFIterator functionBodyStartToken( iter );
    BFFIterator functionBodyStopToken( iter );
    if ( func->NeedsBody() )
    {
        // find body
        if ( *iter != BFF_SCOPE_OPEN )
        {
            Error::Error_1024_FunctionRequiresABody( functionNameStart, func );
            return false;
        }

        if ( iter.ParseToMatchingBrace( BFF_SCOPE_OPEN, BFF_SCOPE_CLOSE ) == false )
        {
            Error::Error_1025_MissingScopeCloseToken( functionBodyStartToken, func );
            return false;
        }

        functionBodyStopToken = iter;
        iter++;
        hasBody = true;
    }

    return func->ParseFunction( m_NodeGraph,
                                functionNameStart,
                                hasBody ? &functionBodyStartToken : nullptr,
                                hasBody ? &functionBodyStopToken : nullptr,
                                hasHeader ? &functionArgsStartToken : nullptr,
                                hasHeader ? &functionArgsStopToken : nullptr );
}

// ParseUnnamedScope
//------------------------------------------------------------------------------
bool BFFParser::ParseUnnamedScope( BFFIterator & iter )
{
    // find the matching bracket
    BFFIterator scopeStart( iter );
    if ( iter.ParseToMatchingBrace( BFF_SCOPE_OPEN, BFF_SCOPE_CLOSE ) == false )
    {
        Error::Error_1025_MissingScopeCloseToken( scopeStart, nullptr );
        return false;
    }

    // create stack for scope
    BFFStackFrame stackFrame;

    // parse the scoped part
    BFFParser subParser( m_NodeGraph );
    BFFIterator subIter( scopeStart );
    subIter++; // skip opening token
    subIter.SetMax( iter.GetCurrent() ); // limit to closing token
    if ( subParser.Parse( subIter ) == false )
    {
        return false;
    }

    iter++; // skip closing }

    return true;
}

// ParsePreprocessorDirective
//------------------------------------------------------------------------------
bool BFFParser::ParsePreprocessorDirective( BFFIterator & iter )
{
    const BFFIterator directiveStart( iter );

    // skip directive start token
    ASSERT( *iter == BFF_PREPROCESSOR_START );
    iter++;

    // allow whitepace before directive name
    iter.SkipWhiteSpace();

    // start of directive name
    BFFIterator directiveStartIter( iter );

    // find end of directive
    while ( iter.IsAtValidDirectiveNameCharacter() )
    {
        iter++;
    }
    BFFIterator directiveEndIter( iter );

    iter.SkipWhiteSpace();

    // determine directive
    AStackString< MAX_DIRECTIVE_NAME_LENGTH > directive( directiveStartIter.GetCurrent(), directiveEndIter.GetCurrent() );
    if ( directive == "include" )
    {
        return ParseIncludeDirective( iter );
    }
    else if ( directive == "once" )
    {
        m_NodeGraph.SetCurrentFileAsOneUse();
        return true;
    }
    else if ( directive == "define" )
    {
        return ParseDefineDirective( iter );
    }
    else if ( directive == "undef" )
    {
        return ParseUndefDirective( iter );
    }
    else if ( directive == "if" )
    {
        return ParseIfDirective( directiveStart, iter );
    }
    else if ( directive == "else" )
    {
        return ParseElseDirective( directiveStartIter );
    }
    else if ( directive == "endif" )
    {
        return ParseEndIfDirective( directiveStartIter );
    }
    else if ( directive == "import" )
    {
        return ParseImportDirective( directiveStart, iter );
    }

    // unknown
    Error::Error_1030_UnknownDirective( directiveStartIter, directive );
    return false;
}

// ParseIncludeDirective
//------------------------------------------------------------------------------
bool BFFParser::ParseIncludeDirective( BFFIterator & iter )
{
    // Sanity check include depth to detect cyclic includes
    if ( s_Depth >= 128 )
    {
        Error::Error_1035_ExcessiveDepthComplexity( iter );
        return false;
    }

    // we expect a " quoted string
    if ( *iter != '"' )
    {
        Error::Error_1031_UnexpectedCharFollowingDirectiveName( iter, AStackString<>( "include" ), '"' );
        return false;
    }

    BFFIterator stringStart( iter );
    stringStart++; // first actual character

    // find end of string
    if ( iter.ParseToNext( '"' ) == false )
    {
        Error::Error_1012_UnexpectedEndOfFile( iter );
        return false;
    }

    // unescape and substitute variables
    AStackString<> include;
    if ( PerformVariableSubstitutions( stringStart, iter, include ) == false )
    {
        return false;
    }

    iter++; // skip closing quote before returning

    FLOG_INFO( "Including: %s\n", include.Get() );

    // open include

    // 1) Try current directory
    AStackString<> includeToUse;
    if (PathUtils::IsFullPath(include) == false)
    {
        AStackString<> fileName( iter.GetFileName() );
        const char * lastSlash = fileName.FindLast( NATIVE_SLASH );
        lastSlash = lastSlash ? lastSlash : fileName.FindLast( OTHER_SLASH );
        lastSlash = lastSlash ? ( lastSlash + 1 ): fileName.Get(); // file only, truncate to empty
        includeToUse.Assign( fileName.Get(), lastSlash );
    }
    includeToUse += include;
    AStackString<> includeToUseClean;
    NodeGraph::CleanPath( includeToUse, includeToUseClean );
    FileStream f;
    if ( f.Open( includeToUseClean.Get(), FileStream::READ_ONLY ) == false )
    {
        Error::Error_1032_UnableToOpenInclude( stringStart, includeToUseClean );
        return false;
    }

    // check if include uses "once" pragma
    if ( m_NodeGraph.IsOneUseFile( includeToUseClean ) )
    {
        // already seen, and uses #once : don't include again
        return true;
    }

    uint64_t includeTimeStamp = FileIO::GetFileLastWriteTime( includeToUseClean );

    // read content of include
    const uint32_t fileSize = (uint32_t)f.GetFileSize();
    AutoPtr< char > mem( (char *)ALLOC( fileSize + 1 ) );
    if ( f.Read( mem.Get(), fileSize ) != fileSize )
    {
        Error::Error_1033_ErrorReadingInclude( stringStart, include, Env::GetLastErr() );
        return false;
    }
    const uint64_t includeDataHash = xxHash::Calc64( mem.Get(), fileSize );
    mem.Get()[ fileSize ] = '\000'; // sentinel
    BFFParser parser( m_NodeGraph );
    const bool pushStackFrame = false; // include is treated as if injected at this point
    return parser.Parse( mem.Get(), fileSize, includeToUseClean.Get(), includeTimeStamp, includeDataHash, pushStackFrame );
}

// ParseDefineDirective
//------------------------------------------------------------------------------
bool BFFParser::ParseDefineDirective( BFFIterator & iter )
{
    if ( iter.IsAtEnd() )
    {
        Error::Error_1012_UnexpectedEndOfFile( iter );
        return false;
    }

    // parse out token
    const BFFIterator tokenStart( iter );
    iter.SkipVariableName();
    if ( tokenStart.GetCurrent() == iter.GetCurrent() )
    {
        Error::Error_1007_ExpectedVariable( iter, nullptr );
        return false;
    }
    const BFFIterator tokenEnd( iter );

    AStackString<> token( tokenStart.GetCurrent(), tokenEnd.GetCurrent() );

    if ( BFFMacros::Get().Define( token ) == false )
    {
        Error::Error_1038_OverwritingTokenInDefine( tokenStart );
        return false;
    }

    FLOG_INFO( "Define macro <%s>", token.Get() );

    return true;
}

// ParseUndefDirective
//------------------------------------------------------------------------------
bool BFFParser::ParseUndefDirective( BFFIterator & iter )
{
    if ( iter.IsAtEnd() )
    {
        Error::Error_1012_UnexpectedEndOfFile( iter );
        return false;
    }

    // parse out token
    const BFFIterator tokenStart( iter );
    iter.SkipVariableName();
    if ( tokenStart.GetCurrent() == iter.GetCurrent() )
    {
        Error::Error_1007_ExpectedVariable( iter, nullptr );
        return false;
    }
    const BFFIterator tokenEnd( iter );

    AStackString<> token( tokenStart.GetCurrent(), tokenEnd.GetCurrent() );

    if ( BFFMacros::Get().Undefine( token ) == false )
    {
        if ( token.BeginsWith( "__" ) )
        {
            Error::Error_1040_UndefOfBuiltInTokenNotAllowed( tokenStart );
        }
        else
        {
            Error::Error_1039_UnknownTokenInUndef( tokenStart );
        }
        return false;
    }

    FLOG_INFO( "Undefine macro <%s>", token.Get() );

    return true;
}

// ParseIfDirective
//------------------------------------------------------------------------------
bool BFFParser::ParseIfDirective( const BFFIterator & directiveStart, BFFIterator & iter )
{
    if ( iter.IsAtEnd() )
    {
        Error::Error_1012_UnexpectedEndOfFile( iter );
        return false;
    }

    bool negate = false;
    if ( *iter == '!' )
    {
        negate = true; // the condition will be inverted
        iter++; // skip '!'
        iter.SkipWhiteSpace(); // allow whitepace after '!'
        if ( iter.IsAtEnd() )
        {
            Error::Error_1012_UnexpectedEndOfFile( iter );
            return false;
        }
    }

    // Parse and evaluate condition
    bool result;
    if ( !ParseIfCondition( directiveStart, iter, result ) )
    {
        return false; // ParseIfCondition will have emitted an error
    }

    // #if !
    if ( negate )
    {
        result = !( result );
    }

    // Find #endif
    BFFIterator startIfBlock( iter );
    BFFIterator endIfBlock( iter );
    bool hasElseBlock( false );
    if ( !ParseToEndIf( endIfBlock, iter, true, &hasElseBlock ) ) // Allow else
    {
        return false; // ParseToEndIf will have emitted an error
    }
    if ( hasElseBlock )
    {
        // Find end of else block
        BFFIterator startElseBlock( iter );
        BFFIterator endElseBlock( iter );
        if ( !ParseToEndIf( endElseBlock, iter, false, nullptr ) ) // Must be endif
        {
            return false; // ParseToEndIf will have emitted an error
        }

        if ( result == true )
        {
            // Parse If -> Else
            BFFIterator subIter( startIfBlock );
            subIter.SetMax( endIfBlock.GetCurrent() );
            if ( Parse( subIter ) == false )
            {
                return false; // Parse will have emitted an error
            }
        }
        else
        {
            // Parse Else -> EndIf
            BFFIterator subIter( startElseBlock );
            subIter.SetMax( endElseBlock.GetCurrent() );
            if ( Parse( subIter ) == false )
            {
                return false; // Parse will have emitted an error
            }
        }
    }
    else
    {
        if ( result == true )
        {
            // Parse If -> EndIf
            BFFIterator subIter( startIfBlock );
            subIter.SetMax( endIfBlock.GetCurrent() );
            if ( Parse( subIter ) == false )
            {
                return false; // Parse will have emitted an error
            }
        }
    }

    return true;
}

// ParseIfCondition
//------------------------------------------------------------------------------
bool BFFParser::ParseIfCondition( const BFFIterator & directiveStart, BFFIterator & iter, bool & result )
{
    const BFFIterator variableOrOperatorStart( iter );
    iter.SkipVariableName();
    if ( variableOrOperatorStart.GetCurrent() == iter.GetCurrent() )
    {
        Error::Error_1007_ExpectedVariable( directiveStart, nullptr );
        return false;
    }
    const BFFIterator variableOrOperatorEnd( iter );

    AStackString<> variableOrOperator( variableOrOperatorStart.GetCurrent(), variableOrOperatorEnd.GetCurrent() );
    iter.SkipWhiteSpace();

    // Check whether this is an #if operator by first looking for an opening parenthesis.
    // For compatibility with previous versions we allow macros to have the same names as
    // #if operators (e.g. exists) and only parse them as operators if we find a brace.
    if ( *iter == BFF_FUNCTION_ARGS_OPEN )
    {
        if ( variableOrOperator == "exists" )
        {
            return ParseIfExistsCondition( iter, result );
        }

        Error::Error_1042_UnknownOperator( variableOrOperatorStart, variableOrOperator );
        return false;
    }
    else
    {
        // Evaluate the condition
        if ( CheckIfCondition( variableOrOperatorStart, variableOrOperatorEnd, result ) == false )
        {
            return false; // CheckIfCondition will have emitted an error
        }
    }

    return true;
}

// ParseIfExistsCondition
//------------------------------------------------------------------------------
bool BFFParser::ParseIfExistsCondition( BFFIterator & iter, bool & result )
{
    const BFFIterator openToken = iter;
    iter++; // skip over opening token
    iter.SkipWhiteSpace();

    const BFFIterator varNameStart = iter;
    iter.SkipVariableName();
    const BFFIterator varNameEnd = iter;
    if ( *iter != BFF_FUNCTION_ARGS_CLOSE )
    {
        Error::Error_1002_MatchingClosingTokenNotFound( openToken, nullptr, BFF_FUNCTION_ARGS_CLOSE );
        return false;
    }
    iter++; // skip over closing token
    iter.SkipWhiteSpaceAndComments();

    // sanity check it is a sensible length
    size_t varNameLen = varNameStart.GetDistTo( varNameEnd );
    if ( varNameLen == 0 )
    {
        Error::Error_1007_ExpectedVariable( openToken, nullptr );
        return false;
    }
    if ( varNameLen > MAX_VARIABLE_NAME_LENGTH )
    {
        Error::Error_1014_VariableNameIsTooLong( iter, (uint32_t)varNameLen, (uint32_t)MAX_VARIABLE_NAME_LENGTH );
        return false;
    }
    AStackString<> varName( varNameStart.GetCurrent(), varNameEnd.GetCurrent() );

    // look for varName in system environment
    AStackString<> varValue;
    uint32_t varHash = 0;
    bool optional = true;
    FBuild::Get().ImportEnvironmentVar( varName.Get(), optional, varValue, varHash );
    result = ( varHash != 0 ); // a hash of 0 means the env var was not found

    return true;
}

// ParseElseDirective
//------------------------------------------------------------------------------
bool BFFParser::ParseElseDirective( const BFFIterator& directiveStart )
{
    // Finding the else directive is handled by ParseIfDirective, so if we hit one
    // by itself, that's an error
    Error::Error_1041_ElseWithoutIf( directiveStart );
    return false;
}

// ParseToEndIf
//------------------------------------------------------------------------------
bool BFFParser::ParseToEndIf( BFFIterator & directiveIter, BFFIterator & iter, bool allowElse, bool * outIsElse )
{
    // Advance iterator past entire #if block
    size_t depth = 1; // handle nested ifs
    while ( depth > 0 )
    {
        // did we hit the end of the file?
        if ( iter.IsAtEnd() )
        {
            Error::Error_1012_UnexpectedEndOfFile( iter ); // TODO:B better error for this?
            return false;
        }

        // find the next preprocessor directive
        iter.SkipWhiteSpace();
        if ( *iter == BFF_PREPROCESSOR_START )
        {
            // Store position of directive for caller
            directiveIter = iter;

            iter++; // skip #
            iter.SkipWhiteSpace(); // allow whitespace between # and directive
            const BFFIterator directiveNameStart( iter );
            while ( iter.IsAtValidDirectiveNameCharacter() )
            {
                iter++;
            }
            const BFFIterator directiveNameEnd( iter );
            AStackString<> directiveName( directiveNameStart.GetCurrent(), directiveNameEnd.GetCurrent() );
            if ( directiveName == "endif" )
            {
                --depth;
            }
            else if ( directiveName == "if" )
            {
                ++depth;
            }
            else if ( ( depth == 1 ) && ( directiveName == "else" ) )
            {
                if ( allowElse == false )
                {
                    Error::Error_1041_ElseWithoutIf( directiveNameStart );
                    return false;
                }
                if ( outIsElse )
                {
                    *outIsElse = true;
                }
                return true;
            }

            // continue to skip rest of line....
        }

        // skip rest of line
        while ( ( iter.IsAtEnd() == false ) &&
                ( *iter != '\r' ) &&
                ( *iter != '\n' ) )
        {
            iter++;
        }
    }

    if ( outIsElse )
    {
        *outIsElse = false;
    }

    return true;
}

// ParseEndIfDirective
//------------------------------------------------------------------------------
bool BFFParser::ParseEndIfDirective( const BFFIterator & directiveStart )
{
    // Finding the else directive is handled by ParseIfDirective, so if we hit one
    // by itself, that's an error
    Error::Error_1037_EndIfWithoutIf( directiveStart );
    return false;
}

// CheckIfCondition
//------------------------------------------------------------------------------
bool BFFParser::CheckIfCondition( const BFFIterator & conditionStart, const BFFIterator & conditionEnd, bool & result )
{
    // trim condition
    AStackString<> condition( conditionStart.GetCurrent(), conditionEnd.GetCurrent() );

    result = BFFMacros::Get().IsDefined( condition );

    return true;
}

// ParseImportDirective
//------------------------------------------------------------------------------
bool BFFParser::ParseImportDirective( const BFFIterator & directiveStart, BFFIterator & iter )
{
    iter.SkipWhiteSpace();

    // make sure we haven't hit the end of the file
    if ( iter.IsAtEnd() )
    {
        Error::Error_1012_UnexpectedEndOfFile( directiveStart );
        return false;
    }

    // make sure this is a variable name
    if ( iter.IsAtValidVariableNameCharacter() == false )
    {
        Error::Error_1013_UnexpectedCharInVariableName( iter, nullptr );
        return false;
    }

    // find the end of the variable name
    const BFFIterator varNameStart( iter );
    iter.SkipVariableName();
    if ( iter.IsAtEnd() )
    {
        Error::Error_1012_UnexpectedEndOfFile( iter );
        return false;
    }
    const BFFIterator varNameEnd( iter );

    // sanity check it is a sensible length
    size_t varNameLen = varNameStart.GetDistTo( varNameEnd );
    if ( varNameLen > MAX_VARIABLE_NAME_LENGTH )
    {
        Error::Error_1014_VariableNameIsTooLong( iter, (uint32_t)varNameLen, (uint32_t)MAX_VARIABLE_NAME_LENGTH );
        return false;
    }
    AStackString<> varName( varNameStart.GetCurrent(), varNameEnd.GetCurrent() );

    // look for varName in system environment
    AStackString<> varValue;
    uint32_t varHash = 0;
    bool optional = false;
    if ( FBuild::Get().ImportEnvironmentVar( varName.Get(), optional, varValue, varHash ) == false )
    {
        Error::Error_1009_UnknownVariable( varNameStart, nullptr );
        return false;
    }

    // add the dot to variable name
    varName = ".";
    varName.Append( varNameStart.GetCurrent(), varNameLen );

    // import variable in current scope
    BFFStackFrame::SetVarString( varName, varValue, nullptr );
    FLOG_INFO( "Imported <string> variable '%s' with value '%s' from system environment", varName.Get(), varValue.Get() );

    return true;
}

// StoreVariableString
//------------------------------------------------------------------------------
bool BFFParser::StoreVariableString( const AString & name,
                                     const BFFIterator & valueStart, const BFFIterator & valueEnd,
                                     const BFFIterator & operatorIter,
                                     BFFStackFrame * frame )
{
    // unescape and subsitute embedded variables
    AStackString< 2048 > value;
    if ( PerformVariableSubstitutions( valueStart, valueEnd, value ) == false )
    {
        return false;
    }

    // find existing
    const BFFVariable * var = BFFStackFrame::GetVar( name, frame );

    const bool dstIsEmpty = ( var == nullptr ) ||
        ( var->IsArrayOfStrings() && var->GetArrayOfStrings().IsEmpty() ) ||
        ( var->IsArrayOfStructs() && var->GetArrayOfStructs().IsEmpty() );

    // are we concatenating?
    if ( ( *operatorIter == BFF_VARIABLE_CONCATENATION ) ||
         ( *operatorIter == BFF_VARIABLE_SUBTRACTION ) )
    {
        // variable must exist
        if ( var == nullptr )
        {
            Error::Error_1026_VariableNotFoundForModification( operatorIter, name );
            return false;
        }

        // make sure types are compatible
        if ( var->IsString() )
        {
            // OK - can concat String to String
            AStackString< 1024 > finalValue( var->GetString() );
            if ( *operatorIter == BFF_VARIABLE_CONCATENATION )
            {
                finalValue += value;
            }
            else
            {
                finalValue.Replace( value.Get(), "" );
            }

            BFFStackFrame::SetVarString( name, finalValue, frame );
            FLOG_INFO( "%s '%s' %s <String> variable '%s' with result '%s'",
                        ( *operatorIter == BFF_VARIABLE_CONCATENATION ) ? "Appending" : "Removing",
                        value.Get(),
                        ( *operatorIter == BFF_VARIABLE_CONCATENATION ) ? "to" : "from",
                        name.Get(),
                        finalValue.Get() );
            return true;
        }
        else if ( var->IsArrayOfStrings() || dstIsEmpty )
        {
            // OK - can concat String to ArrayOfStrings or to empty array
            Array< AString > finalValues( var->GetArrayOfStrings().GetSize() + 1, false );
            if ( *operatorIter == BFF_VARIABLE_CONCATENATION )
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
            FLOG_INFO( "%s '%s' %s <ArrayOfStrings> variable '%s' with result of %i items",
                        ( *operatorIter == BFF_VARIABLE_CONCATENATION ) ? "Appending" : "Removing",
                        value.Get(),
                        ( *operatorIter == BFF_VARIABLE_CONCATENATION ) ? "to" : "from",
                        name.Get(),
                        finalValues.GetSize() );
            return true;
        }
        else
        {
            Error::Error_1027_CannotModify( operatorIter, name, var->GetType(), BFFVariable::VAR_STRING );
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
            FLOG_INFO( "Registered <string> variable '%s' with value '%s'", name.Get(), value.Get() );
            return true;
        }
        else if ( var->IsArrayOfStrings() || dstIsEmpty )
        {
            // OK - store new string as the single element of array
            Array< AString > values( 1, false );
            values.Append( value );
            BFFStackFrame::SetVarArrayOfStrings( name, values, frame );
            FLOG_INFO( "Registered <ArrayOfStrings> variable '%s' with 1 element: '%s'", name.Get(), value.Get() );
            return true;
        }
        else
        {
            auto errorIter = valueStart;
            errorIter--;
            Error::Error_1034_OperationNotSupported( errorIter, var->GetType(), BFFVariable::VAR_STRING, operatorIter );
            return false;
        }
    }
    return false;
}

// StoreVariableArray
//------------------------------------------------------------------------------
bool BFFParser::StoreVariableArray( const AString & name,
                                    const BFFIterator & valueStart, const BFFIterator & valueEnd,
                                    const BFFIterator & operatorIter,
                                    BFFStackFrame * frame )
{
    Array< AString > values( 32, true );
    Array< const BFFVariable * > structValues( 32, true );

    // find existing
    const BFFVariable * var = BFFStackFrame::GetVar( name, frame );

    // are we concatenating?
    if ( ( *operatorIter == BFF_VARIABLE_CONCATENATION ) ||
         ( *operatorIter == BFF_VARIABLE_SUBTRACTION ) )
    {
        // variable must exist
        if ( var == nullptr )
        {
            Error::Error_1026_VariableNotFoundForModification( operatorIter, name );
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
            Error::Error_1027_CannotModify( operatorIter, name, var->GetType(), BFFVariable::VAR_ARRAY_OF_STRINGS );
            return false;
        }
    }
    else
    {
        // variable must be new or array of some kind
        if ( !( ( var == nullptr ) || var->IsArrayOfStrings() || var->IsArrayOfStructs() ) )
        {
            // TODO:B Improve this error to handle ArrayOfStructs case
            Error::Error_1034_OperationNotSupported( valueStart, var->GetType(), BFFVariable::VAR_ARRAY_OF_STRINGS, operatorIter );
            return false;
        }
    }

    const bool dstIsEmpty = ( var == nullptr ) ||
        ( var->IsArrayOfStrings() && var->GetArrayOfStrings().IsEmpty() ) ||
        ( var->IsArrayOfStructs() && var->GetArrayOfStructs().IsEmpty() );

    // Parse array of variables
    BFFIterator iter( valueStart );
    for (;;)
    {
        iter.SkipWhiteSpaceAndComments();

        // end?
        if ( iter.GetCurrent() == valueEnd.GetCurrent() )
        {
            break;
        }

        const char c = *iter;
        if ( ( c == '"' ) || ( c == '\'' ) )
        {
            // a quoted string

            // dest is consistent?
            // if it started empty it shouldn't contain structs at this point, otherwise it must be ArrayOfStrings
            if ( !( dstIsEmpty ? structValues.IsEmpty() : var->IsArrayOfStrings() ) )
            {
                // Mixed types in vector
                Error::Error_1034_OperationNotSupported( iter,
                                                         var->GetType(),
                                                         BFFVariable::VAR_STRING,
                                                         operatorIter );
                return false;
            }

            // subtraction not supported on arrays
            if ( *operatorIter == BFF_VARIABLE_SUBTRACTION )
            {
                Error::Error_1034_OperationNotSupported( iter, var->GetType(), BFFVariable::VAR_STRING, operatorIter );
                return false;
            }

            // a string
            BFFIterator elementValueStart( iter );
            iter.SkipString( c );
            ASSERT( iter.GetCurrent() <= valueEnd.GetCurrent() ); // should not be in this function if string is not terminated
            elementValueStart++; // move to start of actual content
            AStackString< 2048 > elementValue;

            // unescape and subsitute embedded variables
            if ( PerformVariableSubstitutions( elementValueStart, iter, elementValue ) == false )
            {
                return false;
            }

            values.Append( elementValue );

            iter++; // pass closing quote
        }
        else if ( c == BFF_DECLARE_VAR_INTERNAL ||
                  c == BFF_DECLARE_VAR_PARENT )
        {
            const BFFIterator elementStartValue = iter;

            // a variable
            AStackString< MAX_VARIABLE_NAME_LENGTH > srcName;
            bool parentScope = false; // ignored, the behavior is the same
            if ( ParseVariableName( iter, srcName, parentScope ) == false )
            {
                return false;
            }

            // Determine stack frame to use for Src var
            BFFStackFrame * srcFrame = BFFStackFrame::GetCurrent();
            if ( c == BFF_DECLARE_VAR_PARENT )
            {
                srcFrame = BFFStackFrame::GetCurrent()->GetParent();
            }

            // get the variable
            const BFFVariable * varSrc = srcFrame->GetVariableRecurse( srcName );
            if ( varSrc == nullptr )
            {
                Error::Error_1026_VariableNotFoundForModification( operatorIter, srcName );
                return false;
            }

            // subtraction not supported on arrays
            if ( *operatorIter == BFF_VARIABLE_SUBTRACTION )
            {
                Error::Error_1034_OperationNotSupported( elementStartValue, var->GetType(), varSrc->GetType(), operatorIter );
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
                // if it started empty it shouldn't contain structs at this point, otherwise it must be ArrayOfStrings
                if ( !( dstIsEmpty ? structValues.IsEmpty() : var->IsArrayOfStrings() ) )
                {
                    // inconsistency
                    Error::Error_1034_OperationNotSupported( elementStartValue,
                                                             var->GetType(),
                                                             varSrc->GetType(),
                                                             operatorIter );
                    return false;
                }

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
                // if it started empty it shouldn't contain strings at this point, otherwise it must be ArrayOfStructs
                if ( !( dstIsEmpty ? values.IsEmpty() : var->IsArrayOfStructs() ) )
                {
                    // inconsistency
                    Error::Error_1034_OperationNotSupported( elementStartValue,
                                                             var->GetType(),
                                                             varSrc->GetType(),
                                                             operatorIter );
                    return false;
                }

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
                                                        varSrc->GetType(),
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

        iter.SkipWhiteSpaceAndComments();
        if ( *iter == ',' ) // comma seperators are optional
        {
            iter++;
        }

        // continue looking for more vars...
    }

    // should only have one populated array
    ASSERT( values.IsEmpty() || structValues.IsEmpty() );

    // determine type of resulting variable
    BFFVariable::VarType varType = BFFVariable::VAR_ARRAY_OF_STRINGS; // default (for new empty variables)
    if ( !dstIsEmpty )
    {
        // if variable wasn't empty, use its type
        varType = var->GetType();
    }
    else if ( structValues.IsEmpty() == false )
    {
        // using type of value: ArrayOfStructs
        varType = BFFVariable::VAR_ARRAY_OF_STRUCTS;
    }
    else if ( values.IsEmpty() == false )
    {
        // using type of value: ArrayOfStrings
        varType = BFFVariable::VAR_ARRAY_OF_STRINGS;
    }

    // check if selected type correctly
    ASSERT ( varType == BFFVariable::VAR_ARRAY_OF_STRINGS || varType == BFFVariable::VAR_ARRAY_OF_STRUCTS );

    // Register this variable
    if ( varType == BFFVariable::VAR_ARRAY_OF_STRUCTS )
    {
        // structs
        BFFStackFrame::SetVarArrayOfStructs( name, structValues, frame );
        FLOG_INFO( "Registered <ArrayOfStructs> variable '%s' with %u elements", name.Get(), structValues.GetSize() );
    }
    else if ( varType == BFFVariable::VAR_ARRAY_OF_STRINGS )
    {
        // strings
        BFFStackFrame::SetVarArrayOfStrings( name, values, frame );
        FLOG_INFO( "Registered <ArrayOfStrings> variable '%s' with %u elements", name.Get(), values.GetSize() );
    }

    return true;
}

// StoreVariableStruct
//------------------------------------------------------------------------------
bool BFFParser::StoreVariableStruct( const AString & name,
                                     const BFFIterator & valueStart, const BFFIterator & valueEnd,
                                     const BFFIterator & operatorIter,
                                     BFFStackFrame * frame )
{
    // find existing
    const BFFVariable * var = BFFStackFrame::GetVar( name, frame );

    // are we concatenating?
    if ( ( *operatorIter == BFF_VARIABLE_CONCATENATION ) ||
         ( *operatorIter == BFF_VARIABLE_SUBTRACTION ) )
    {
        // concatenation of structs not supported
        Error::Error_1027_CannotModify( operatorIter, name, BFFVariable::VAR_STRUCT, BFFVariable::VAR_ANY );
        return false;
    }
    else
    {
        // variable must be new or a struct
        if ( !( ( var == nullptr ) || var->IsStruct() ) )
        {
            Error::Error_1034_OperationNotSupported( valueStart, var->GetType(), BFFVariable::VAR_STRUCT, operatorIter );
            return false;
        }
    }

    // create stack frame to capture variables
    BFFStackFrame stackFrame;

    // parse all the variables in the scope
    BFFParser subParser( m_NodeGraph );
    BFFIterator subIter( valueStart );
    subIter.SetMax( valueEnd.GetCurrent() ); // limit to closing token
    if ( subParser.Parse( subIter ) == false )
    {
        return false; // error will be emitted by Parse
    }

    // get variables defined in the scope
    const Array< const BFFVariable * > & structMembers = stackFrame.GetLocalVariables();

    // Register this variable
    BFFStackFrame::SetVarStruct( name, structMembers, frame ? frame : stackFrame.GetParent() );
    FLOG_INFO( "Registered <struct> variable '%s' with %u members", name.Get(), structMembers.GetSize() );

    return true;
}


// StoreVariableBool
//------------------------------------------------------------------------------
bool BFFParser::StoreVariableBool( const AString & name, bool value, BFFStackFrame * frame )
{
    // Register this variable
    BFFStackFrame::SetVarBool( name, value, frame );

    FLOG_INFO( "Registered <bool> variable '%s' with value '%s'", name.Get(), value ? "true" : "false" );

    return true;
}

// StoreVariableInt
//------------------------------------------------------------------------------
bool BFFParser::StoreVariableInt( const AString & name, int value, BFFStackFrame * frame )
{
    BFFStackFrame::SetVarInt( name, value, frame );

    FLOG_INFO( "Registered <int> variable '%s' with value '%i'", name.Get(), value );

    return true;
}

// StoreVariableToVariable
//------------------------------------------------------------------------------
bool BFFParser::StoreVariableToVariable( const AString & dstName, BFFIterator & iter, const BFFIterator & operatorIter, BFFStackFrame * dstFrame )
{
    AStackString< MAX_VARIABLE_NAME_LENGTH > srcName;

    bool srcParentScope = false;
    const BFFIterator varNameSrcStart( iter ); // Take note of start of var
    if ( ParseVariableName( iter, srcName, srcParentScope ) == false )
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
        Error::Error_1009_UnknownVariable( varNameSrcStart, nullptr );
        return false;
    }

    // find dst var
    const BFFVariable * varDst = BFFStackFrame::GetVar( dstName, dstFrame );

    const bool concat = ( *operatorIter == BFF_VARIABLE_CONCATENATION );
    const bool subtract = ( *operatorIter == BFF_VARIABLE_SUBTRACTION );

    // concatenation?
    if ( concat || subtract )
    {
        // can only concatenate to existing vars
        if ( varDst == nullptr )
        {
            Error::Error_1026_VariableNotFoundForModification( operatorIter, dstName );
            return false;
        }
    }

    // if dst exists, types must match
    BFFVariable::VarType srcType = varSrc->GetType();
    BFFVariable::VarType dstType = BFFVariable::VAR_ANY;
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
            Array< AString > values( varDst->GetArrayOfStrings().GetSize() + 1, false );
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
            FLOG_INFO( "Registered <ArrayOfStrings> variable '%s' with %u elements", dstName.Get(), values.GetSize() );
            return true;
        }

        // Struct to ArrayOfStructs or empty array, assignment or concatenation
        if ( ( dstType == BFFVariable::VAR_ARRAY_OF_STRUCTS || dstIsEmpty ) &&
             ( srcType == BFFVariable::VAR_STRUCT ) &&
             !subtract )
        {
            uint32_t num = (uint32_t)( 1 + ( ( concat && !dstIsEmpty ) ? varDst->GetArrayOfStructs().GetSize() : 0 ) );
            Array< const BFFVariable * > values( num, false );
            if ( concat && !dstIsEmpty )
            {
                values.Append( varDst->GetArrayOfStructs() );
            }
            values.Append( varSrc );

            BFFStackFrame::SetVarArrayOfStructs( dstName, values, dstFrame );
            FLOG_INFO( "Registered <ArrayOfStructs> variable '%s' with %u elements", dstName.Get(), num );
            return true;
        }

        // Empty array to ArrayOfStrings, assignment or concatenation
        if ( dstType == BFFVariable::VAR_ARRAY_OF_STRINGS && srcIsEmpty && !subtract )
        {
            if ( concat )
            {
                BFFStackFrame::SetVarArrayOfStrings( dstName, varDst->GetArrayOfStrings(), dstFrame );
                FLOG_INFO( "Registered <ArrayOfStrings> variable '%s' with %u elements", dstName.Get(), (unsigned int)varDst->GetArrayOfStrings().GetSize() );
            }
            else
            {
                BFFStackFrame::SetVarArrayOfStrings( dstName, Array< AString >(), dstFrame );
                FLOG_INFO( "Registered <ArrayOfStrings> variable '%s' with 0 elements", dstName.Get() );
            }
            return true;
        }

        // Empty array to ArrayOfStructs, assignment or concatenation
        if ( dstType == BFFVariable::VAR_ARRAY_OF_STRUCTS && srcIsEmpty && !subtract )
        {
            if ( concat )
            {
                BFFStackFrame::SetVarArrayOfStructs( dstName, varDst->GetArrayOfStructs(), dstFrame );
                FLOG_INFO( "Registered <ArrayOfStructs> variable '%s' with %u elements", dstName.Get(), (unsigned int)varDst->GetArrayOfStructs().GetSize() );
            }
            else
            {
                BFFStackFrame::SetVarArrayOfStructs(dstName, Array< const BFFVariable * >(), dstFrame);
                FLOG_INFO( "Registered <ArrayOfStructs> variable '%s' with 0 elements", dstName.Get() );
            }
            return true;
        }

        // ArrayOfStrings to empty array, assignment or concatenation
        if ( dstIsEmpty && srcType == BFFVariable::VAR_ARRAY_OF_STRINGS && !subtract )
        {
            BFFStackFrame::SetVarArrayOfStrings( dstName, varSrc->GetArrayOfStrings(), dstFrame );
            FLOG_INFO( "Registered <ArrayOfStrings> variable '%s' with %u elements", dstName.Get(), (unsigned int)varSrc->GetArrayOfStrings().GetSize() );
            return true;
        }

        // ArrayOfStructs to empty array, assignment or concatenation
        if ( dstIsEmpty && srcType == BFFVariable::VAR_ARRAY_OF_STRUCTS && !subtract )
        {
            BFFStackFrame::SetVarArrayOfStructs( dstName, varSrc->GetArrayOfStructs(), dstFrame );
            FLOG_INFO( "Registered <ArrayOfStructs> variable '%s' with %u elements", dstName.Get(), (unsigned int)varSrc->GetArrayOfStructs().GetSize() );
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
                FLOG_INFO( "Appending '%s' (value of %s) to <String> variable '%s' with result '%s'",
                            varSrc->GetString().Get(),
                            srcName.Get(),
                            dstName.Get(),
                            finalValue.Get() );
            }
            else if ( subtract )
            {
                AStackString< 2048 > finalValue(varDst->GetString());
                finalValue.Replace( varSrc->GetString().Get(), "" );
                BFFStackFrame::SetVarString( dstName, finalValue, dstFrame );
                FLOG_INFO( "Removing '%s' (value of %s) from <String> variable '%s' with result '%s'",
                            varSrc->GetString().Get(),
                            srcName.Get(),
                            dstName.Get(),
                            finalValue.Get() );
            }
            else
            {
                BFFStackFrame::SetVarString( dstName, varSrc->GetString(), dstFrame );
                FLOG_INFO( "Registered <string> variable '%s' with value '%s'", dstName.Get(), varSrc->GetString().Get() );
            }
            return true;
        }

        if ( srcType == BFFVariable::VAR_ARRAY_OF_STRINGS && !subtract )
        {
            if ( concat )
            {
                const unsigned int num = (unsigned int)( varSrc->GetArrayOfStrings().GetSize() + varDst->GetArrayOfStrings().GetSize() );
                Array< AString > values( num, false );
                values.Append( varDst->GetArrayOfStrings() );
                values.Append( varSrc->GetArrayOfStrings() );
                BFFStackFrame::SetVarArrayOfStrings( dstName, values, dstFrame );
                FLOG_INFO( "Registered <ArrayOfStrings> variable '%s' with %u elements", dstName.Get(), num );
            }
            else
            {
                BFFStackFrame::SetVarArrayOfStrings( dstName, varSrc->GetArrayOfStrings(), dstFrame );
                FLOG_INFO( "Registered <ArrayOfStrings> variable '%s' with %u elements", dstName.Get(), (unsigned int)varSrc->GetArrayOfStrings().GetSize() );
            }
            return true;
        }

        if ( srcType == BFFVariable::VAR_ARRAY_OF_STRUCTS && !subtract)
        {
            if ( concat )
            {
                const unsigned int num = (unsigned int)( varSrc->GetArrayOfStructs().GetSize() + varDst->GetArrayOfStructs().GetSize() );
                Array< const BFFVariable * > values( num, false );
                values.Append( varDst->GetArrayOfStructs() );
                values.Append( varSrc->GetArrayOfStructs() );
                BFFStackFrame::SetVarArrayOfStructs( dstName, values, dstFrame );
                FLOG_INFO( "Registered <ArrayOfStructs> variable '%s' with %u elements", dstName.Get(), num );
            }
            else
            {
                BFFStackFrame::SetVarArrayOfStructs( dstName, varSrc->GetArrayOfStructs(), dstFrame );
                FLOG_INFO( "Registered <ArrayOfStructs> variable '%s' with %u elements", dstName.Get(), (unsigned int)varSrc->GetArrayOfStructs().GetSize() );
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
                BFFVariable *const newVar = BFFStackFrame::ConcatVars( dstName, varSrc, varDst, dstFrame );
                FLOG_INFO( "Registered <struct> variable '%s' with %u members", dstName.Get(), newVar->GetStructMembers().GetSize() );
            }
            else
            {
                // Register this variable
                BFFStackFrame::SetVarStruct( dstName, srcMembers, dstFrame );
                FLOG_INFO( "Registered <struct> variable '%s' with %u members", dstName.Get(), srcMembers.GetSize() );
            }
            return true;
        }
    }

    Error::Error_1034_OperationNotSupported( varNameSrcStart,
                                             varDst ? varDst->GetType() : varSrc->GetType(),
                                             varSrc->GetType(),
                                             operatorIter );
    return false;
}

// PerformVariableSubstitutions
//------------------------------------------------------------------------------
/*static*/ bool BFFParser::PerformVariableSubstitutions( const BFFIterator & startIter,
                                              const BFFIterator & endIter,
                                              AString & value )
{
    AStackString< 4096 > output;

    BFFIterator src( startIter );
    BFFIterator end( endIter );

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
                BFFIterator firstToken( src );
                src++; // skip opening $

                // find matching $
                BFFIterator startName( src );
                const char * endName = nullptr;
                while ( src < end )
                {
                    if ( *src == '$' )
                    {
                        endName = src.GetCurrent();
                        break;
                    }
                    src++;
                }
                if ( ( endName == nullptr ) ||
                     ( ( endName - startName.GetCurrent() ) < 1 ) )
                {
                    Error::Error_1028_MissingVariableSubstitutionEnd( firstToken );
                    return false;
                }
                AStackString< MAX_VARIABLE_NAME_LENGTH > varName( startName.GetCurrent(), endName );
                const BFFVariable * var = BFFStackFrame::GetVarAny( varName );
                if ( var == nullptr )
                {
                    Error::Error_1009_UnknownVariable( startName, nullptr );
                    return false;
                }
                if ( var->IsBool() == true )
                {
                    output += ( ( var->GetBool() ) ? "true" : "false" );
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
                    Error::Error_1029_VariableForSubstitutionIsNotAString( startName, varName, var->GetType() );
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

//------------------------------------------------------------------------------
