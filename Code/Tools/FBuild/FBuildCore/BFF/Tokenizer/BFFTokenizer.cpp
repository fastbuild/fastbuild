// BFFTokenizer.cpp
//------------------------------------------------------------------------------

// Includes
//------------------------------------------------------------------------------
#include "BFFTokenizer.h"

// FBuildCore
#include "Tools/FBuild/FBuildCore/BFF/BFFKeywords.h"
#include "Tools/FBuild/FBuildCore/BFF/BFFParser.h"
#include "Tools/FBuild/FBuildCore/BFF/Functions/Function.h"
#include "Tools/FBuild/FBuildCore/BFF/Tokenizer/BFFTokenRange.h"
#include "Tools/FBuild/FBuildCore/Error.h"
#include "Tools/FBuild/FBuildCore/FBuild.h"
#include "Tools/FBuild/FBuildCore/Graph/NodeGraph.h"

// Core
#include "Core/FileIO/PathUtils.h"
#include "Core/Strings/AStackString.h"
#include "Core/Profile/Profile.h"

// Helpers
//------------------------------------------------------------------------------
namespace
{
    bool IsAtEndOfLine( const char c )      { return ( ( c == '\r' ) || ( c == '\n' ) || ( c == '\000' ) ); }
    bool IsWhiteSpace( const char c )       { return ( ( c == ' ' ) || ( c == '\r' ) || ( c == '\n' ) || ( c == '\t' ) ); }
    bool IsUppercaseLetter( const char c )  { return ( ( c >= 'A' ) && ( c <= 'Z' ) ); }
    bool IsLowercaseLetter( const char c )  { return ( ( c >= 'a' ) && ( c <= 'z' ) ); }
    bool IsUnderscore( const char c )       { return ( c == '_' ); }
    bool IsMinus( const char c )            { return ( c == '-' ); }
    bool IsNumber( const char c )           { return ( ( c >= '0' ) && ( c <= '9' ) ); }

    bool IsIdentifierStart( const char c )  { return ( IsUppercaseLetter( c ) || IsLowercaseLetter( c ) || IsUnderscore( c ) ); }
    bool IsIdentifier( const char c )       { return ( IsUppercaseLetter( c ) || IsLowercaseLetter( c ) || IsUnderscore( c ) || IsNumber( c ) ); }

    bool IsNumberStart( const char c )      { return ( ( c >= '0' ) && ( c <= '9' ) ) || IsMinus( c ); }
    bool IsStringStart( const char c )      { return ( ( c == '\'' ) || ( c == '"') ); }
    bool IsVariableStart( const char c )    { return ( ( c == '.' ) || ( c == '^' ) ); }
    bool IsVariable( const char c )         { return IsUppercaseLetter( c ) || IsLowercaseLetter( c ) || IsNumber( c ) || IsUnderscore( c );  }
    bool IsOperator( const char c )         { return ( ( c == '+' ) || ( c == '-' ) || ( c == '=' ) || ( c == '!' ) || ( c == '<' ) || ( c == '>' ) || ( c == '&' ) || ( c == '|' )); }
    bool IsComma( const char c )            { return ( c == ',' ); }
    bool IsDirective( const char c )        { return ( c == '#' ); }
    void SkipWhitespace( const char * & pos )
    {
        while ( IsWhiteSpace( *pos ) )
        {
            ++pos;
        }
    }
    void SkipWhitespaceOnCurrentLine( const char * & pos )
    {
        while ( ( *pos == ' ' ) || ( *pos == '\t' ) )
        {
            ++pos;
        }
    }
    bool IsAtCommentStart( const char * pos )
    {
        const char c = *pos;

        // fwd slash style comments
        if ( ( c == '/' ) && ( pos[ 1 ] == '/' ) )
        {
            return true;
        }

        // ; style comments
        return ( c == ';' );
    }
    void SkipToStartOfNextLine( const char * & pos, const char * end )
    {
        while ( pos < end )
        {
            if ( *pos == '\n' )
            {
                ++pos;
                return;
            }
            ++pos;
        }

    }
}

// CONSTRUCTOR
//------------------------------------------------------------------------------
BFFTokenizer::BFFTokenizer() = default;

// DESTRUCTOR
//------------------------------------------------------------------------------
BFFTokenizer::~BFFTokenizer()
{
    // Cleanup all the files we loaded
    for ( BFFFile * file : m_Files )
    {
        FDELETE( file );
    }
}

// TokenizeFromFile
//------------------------------------------------------------------------------
bool BFFTokenizer::TokenizeFromFile( const AString & fileName )
{
    const BFFToken * token = nullptr; // No token for the root
    const bool result = Tokenize( fileName, token );

    // Close the token stream
    if ( result )
    {
        ASSERT( m_Tokens.IsEmpty() || ( m_Tokens.Top().GetType() != BFFTokenType::EndOfFile ) );
        const BFFFile & firstFile = *m_Files[ 0 ];
        const char * end = firstFile.GetSourceFileContents().GetEnd();
        m_Tokens.EmplaceBack( firstFile, end, BFFTokenType::EndOfFile, AString::GetEmpty() );
    }

    return result;
}

// Tokenize
//------------------------------------------------------------------------------
bool BFFTokenizer::Tokenize( const AString & fileName, const BFFToken * token )
{
    PROFILE_FUNCTION;

    // The root bff doesn't have a token
    ASSERT( token || ( m_Depth == 0 ) );

    // Canonicalize path
    AStackString<> cleanFileName;
    NodeGraph::CleanPath( fileName, cleanFileName );

    // Have we seen this file before?
    const BFFFile * fileToParse = nullptr;
    for ( const BFFFile * previousInclude : m_Files )
    {
        if ( PathUtils::ArePathsEqual( previousInclude->GetFileName(), cleanFileName ) )
        {
            // Already seen and should only be parsed once?
            if ( previousInclude->IsParseOnce() )
            {
                return true;
            }

            // Already included, but can be included again
            fileToParse = previousInclude;
            break;
        }
    }

    // A file seen for the first time?
    if ( fileToParse == nullptr )
    {
        // Load the new file
        BFFFile * newFile = FNEW( BFFFile() );
        if ( newFile->Load( cleanFileName, token ) == false )
        {
            FDELETE( newFile );
            return false; // Load will have emitted an error
        }
        m_Files.Append( newFile );

        // use the new file
        fileToParse = newFile;
    }

    // Recursively tokenize
    return Tokenize( fileToParse );
}

// TokenizeFromString
//------------------------------------------------------------------------------
bool BFFTokenizer::TokenizeFromString( const AString & fileName, const AString & fileContents )
{
    PROFILE_FUNCTION;

    // Canonicalize path
    AStackString<> cleanFileName;
    NodeGraph::CleanPath( fileName, cleanFileName );

    // A file seen for the first time
    BFFFile * newFile = FNEW( BFFFile( cleanFileName.Get(), fileContents ) );
    m_Files.Append( newFile );

    // Recursively tokenize
    const bool result = Tokenize( newFile );

    // Close the token stream
    if ( result )
    {
        ASSERT( m_Tokens.IsEmpty() || ( m_Tokens.Top().GetType() != BFFTokenType::EndOfFile ) );
        const BFFFile & firstFile = *m_Files[ 0 ];
        const char * end = firstFile.GetSourceFileContents().GetEnd();
        m_Tokens.EmplaceBack( firstFile, end, BFFTokenType::EndOfFile, AString::GetEmpty() );
    }

    return result;
}

// Tokenize
//------------------------------------------------------------------------------
bool BFFTokenizer::Tokenize( const BFFFile * file )
{
    ASSERT( m_Files.Find( file ) );

    // Tokenize the stream
    const char * pos = file->GetSourceFileContents().Get();
    const char * end = file->GetSourceFileContents().GetEnd();
    return Tokenize( *file, pos, end );
}

// Tokenize
//------------------------------------------------------------------------------
bool BFFTokenizer::Tokenize( const BFFFile & file, const char * pos, const char * end )
{
    while ( pos < end )
    {
        // Skip whitespace
        const char c = *pos;
        if ( IsWhiteSpace( c ) )
        {
            SkipWhitespace( pos );
            continue;
        }

        // Comment?
        if ( IsAtCommentStart( pos ) )
        {
            SkipToStartOfNextLine( pos, end );
            continue;
        }

        // Take note of the start of the token
        const char * tokenStart = pos;

        // Identifier?
        if ( IsIdentifierStart( c ) )
        {
            if ( HandleIdentifier( pos, end, file ) == false )
            {
                return false; // HandleIdentifier will have emitted an error
            }
            continue;
        }

        // Variable?
        if ( IsVariableStart( c ) )
        {
            if ( HandleVariable( pos, end, file ) == false )
            {
                return false; // HandleVariable will have emitted an error
            }
            continue;
        }

        // String
        if ( IsStringStart( c ) )
        {
            AStackString<> string;
            if ( GetQuotedString( file, pos, string ) == false )
            {
                return false; // GetQuotedString will have emitted an error
            }
            m_Tokens.EmplaceBack( file, tokenStart, BFFTokenType::String, string );
            continue;
        }

        // Two character operator?
        if ( ( c == '=' ) || ( c == '!' ) || ( c == '<' ) || ( c == '>' ) || ( c == '&' ) || ( c == '|' ) )
        {
            if ( ( pos + 1 ) < end )
            {
                AStackString<> opString;
                opString += pos[ 0 ];
                opString += pos[ 1 ];

                if ( ( opString == "==" ) ||
                     ( opString == "!=" ) ||
                     ( opString == "<=" ) ||
                     ( opString == ">=" ) ||
                     ( opString == "&&" ) ||
                     ( opString == "||" ) )
                {
                    m_Tokens.EmplaceBack( file, pos, BFFTokenType::Operator, pos, pos + 2 );
                    pos += 2;
                    continue;
                }
            }
        }

        // Operator?
        if ( IsOperator( c ) )
        {
            // One char operator?

            // Check before numbers to disambiguate '-'
            // TODO:B Improve negated number check (this doesn't handle .x-7 )
            const bool isNegatedNumber = ( ( c == '-' ) && IsNumber( pos[ 1 ] ) );
            if ( isNegatedNumber == false )
            {
                m_Tokens.EmplaceBack( file, tokenStart, BFFTokenType::Operator, pos, pos + 1 );
                pos++;
                continue;
            }

            // Fall through
        }

        // Number?
        if ( IsNumberStart( c ) )
        {
            ++pos;
            while( IsNumber( *pos ) )
            {
                ++pos;
            }
            m_Tokens.EmplaceBack( file, tokenStart, BFFTokenType::Number, tokenStart, pos );
            continue;
        }

        // Comma
        if ( IsComma( c ) )
        {
            ++pos;
            m_Tokens.EmplaceBack( file, tokenStart, BFFTokenType::Comma, tokenStart, pos );
            continue;
        }

        // Round Brackets?
        if ( ( c == '(' ) || ( c == ')' ) )
        {
            m_Tokens.EmplaceBack( file, tokenStart, BFFTokenType::RoundBracket, pos, pos + 1 );
            pos++;
            continue;
        }

        // Curly Brackets?
        if ( ( c == '{' ) || ( c == '}' ) )
        {
            m_Tokens.EmplaceBack( file, tokenStart, BFFTokenType::CurlyBracket, pos, pos + 1 );
            pos++;
            continue;
        }

        // Square Brackets?
        if ( ( c == '[' ) || ( c == ']' ) )
        {
            m_Tokens.EmplaceBack( file, tokenStart, BFFTokenType::SquareBracket, pos, pos + 1 );
            pos++;
            continue;
        }

        // # directive (non-recursive)
        if ( IsDirective( c ) && ( m_ParsingDirective == false ) )
        {
            if ( HandleDirective( pos, end, file ) == false )
            {
                return false; // HandleDirective will have emitted an error
            }
            continue;
        }

        // Invalid input - record problem character
        const BFFToken error( file, tokenStart, BFFTokenType::Invalid, pos, pos + 1 );
        Error::Error_1010_UnknownConstruct( &error );
        return false;
    }

    return true;
}

// HandleIdentifier
//------------------------------------------------------------------------------
bool BFFTokenizer::HandleIdentifier( const char * & pos, const char * /*end*/, const BFFFile & file )
{
    // Should be called pointing to start of identifier
    ASSERT( IsIdentifierStart( *pos ) );

    // Extract identifier
    const char * idStart = pos;
    ++pos;
    while( IsIdentifier( *pos ) )
    {
        ++pos;
    }
    const AStackString<> identifier( idStart, pos );

    // TODO:C Check valid character follows:
    //   - whitespace
    //   - comma
    //   - brace

    // Elevate keyword identifiers

    // - Booleans
    if ( identifier == BFF_KEYWORD_TRUE )
    {
        m_Tokens.EmplaceBack( file, idStart, BFFTokenType::Boolean, true );
        return true;
    }
    if ( identifier == BFF_KEYWORD_FALSE )
    {
        m_Tokens.EmplaceBack( file, idStart, BFFTokenType::Boolean, false );
        return true;
    }

    // - Keywords
    if ( ( identifier == BFF_KEYWORD_DEFINE ) ||
         ( identifier == BFF_KEYWORD_ELSE ) ||
         ( identifier == BFF_KEYWORD_EXISTS ) ||
         ( identifier == BFF_KEYWORD_FILE_EXISTS ) ||
         ( identifier == BFF_KEYWORD_FUNCTION ) ||
         ( identifier == BFF_KEYWORD_IF ) ||
         ( identifier == BFF_KEYWORD_IMPORT ) ||
         ( identifier == BFF_KEYWORD_INCLUDE ) ||
         ( identifier == BFF_KEYWORD_IN ) ||
         ( identifier == BFF_KEYWORD_NOT ) ||
         ( identifier == BFF_KEYWORD_ONCE ) ||
         ( identifier == BFF_KEYWORD_UNDEF ) )
    {
        m_Tokens.EmplaceBack( file, idStart, BFFTokenType::Keyword, identifier );
        return true;
    }

    // - Functions
    if ( Function::Find( identifier ) )
    {
        m_Tokens.EmplaceBack( file, idStart, BFFTokenType::Function, identifier );
        return true;
    }

    // Unspecified Identifier
    m_Tokens.EmplaceBack( file, idStart, BFFTokenType::Identifier, identifier );
    return true;
}

// HandleVariable
//------------------------------------------------------------------------------
bool BFFTokenizer::HandleVariable( const char * & pos, const char * /*end*/, const BFFFile & file )
{
    const char * variableStart = pos; // Includes . or ^
    ASSERT( ( *pos == '.' ) || ( *pos == '^' ) );
    ++pos; // Consume . or ^

    if ( IsStringStart( *pos ) )
    {
        AStackString<> variable;
        if ( GetQuotedString( file, pos, variable ) == false )
        {
            return false; // GetQuotedString will have emitted an error
        }

        // We want to keep the ./^ and surrounding quotes, so we discard "variable"

        // TODO:B Check for Error_1013_UnexpectedCharInVariableName
    }
    else
    {
        // .Blah style
        while( IsVariable( *pos ) )
        {
            ++pos;
        }
    }

    const AStackString<> variableName( variableStart, pos );

    // If variable name is missing
    if ( variableName.GetLength() <= 1 ) // includes '.' or '^'
    {
        // TODO:C Improve error
        const BFFToken error( file, pos, BFFTokenType::Invalid, AStackString<>( "???" ) );
        Error::Error_1017_UnexepectedCharInVariableValue( &error );
        return false;
    }

    m_Tokens.EmplaceBack( file, variableStart, BFFTokenType::Variable, variableName );
    return true;
}

// HandleDirective
//------------------------------------------------------------------------------
bool BFFTokenizer::HandleDirective( const char * & pos, const char * end, const BFFFile & file )
{
    // Consume the directive start character
    ASSERT( *pos == '#' );
    ++pos;

    // Take note of the number of non-directive tokens
    const size_t numTokens = m_Tokens.GetSize();

    // Find the limits of the current line (directives cannot span lines)
    const char * argsStart = pos;
    SkipToStartOfNextLine( pos, end );
    const char * argsEnd = pos;

    // Tokenize the line
    m_ParsingDirective = true;
    if ( Tokenize( file, argsStart, argsEnd ) == false )
    {
        return false; // Tokenize will have emitted an error
    }
    m_ParsingDirective = false;

    // Remove directive args tokens (the directive parsing consumes them)
    const size_t numArgTokens = m_Tokens.GetSize() - numTokens;
    StackArray<BFFToken> args;
    for ( size_t i = 0; i < numArgTokens; ++i )
    {
        args.Append( Move( m_Tokens[ numTokens + i ] ) );
    }
    for ( size_t i = 0; i < numArgTokens; ++i )
    {
        m_Tokens.Pop(); // Avoiding use of SetSize as this requires a default constructor
    }

    // Terminate arg stream so handlers can safely manage invalid cases
    args.EmplaceBack( file, pos, BFFTokenType::EndOfFile, AString::GetEmpty() );

    BFFTokenRange argsRange( args.Begin(), args.End() );

    // Check keywords that are valid directives
    const BFFToken & directiveToken = args[ 0 ];
    const AString & directive = directiveToken.GetValueString();
    if ( directiveToken.IsKeyword() )
    {
        const char * directiveName = nullptr;
        bool result = false;
        if ( directive == "define" )        { directiveName = "define";     result = HandleDirective_Define( file, pos, end, argsRange ); }
        else if ( directive == "else" )     { directiveName = "else";       result = HandleDirective_Else( file, pos, end, argsRange ); }
        else if ( directive == "if" )       { directiveName = "if";         result = HandleDirective_If( file, pos, end, argsRange ); }
        else if ( directive == "import" )   { directiveName = "import";     result = HandleDirective_Import( file, pos, end, argsRange ); }
        else if ( directive == "include" )  { directiveName = "include";    result = HandleDirective_Include( file, pos, end, argsRange ); }
        else if ( directive == "once" )     { directiveName = "once";       result = HandleDirective_Once( file, pos, end, argsRange ); }
        else if ( directive == "undef" )    { directiveName = "undef";      result = HandleDirective_Undef( file, pos, end, argsRange ); }

        // Did we see a recognized directive?
        if ( directiveName )
        {
            // If it failed, we already emitted an error
            if ( result == false )
            {
                return false; // Handle_* will have emitted an error
            }

            // If it passed, all tokens should have been consumed. This avoids each handler
            // having to have the same boilerplate logic to check for extraneous tokens.
            if ( argsRange->GetType() != BFFTokenType::EndOfFile )
            {
                Error::Error_1045_ExtraneousTokenFollowingDirective( argsRange.GetCurrent(), directiveName );
                return false;
            }

            return true;
        }

        // fall through to error handling
    }

    const BFFToken error( file, argsStart, BFFTokenType::Invalid, argsStart, argsStart + 1 );
    Error::Error_1030_UnknownDirective( &error, directive );
    return false;
}

// HandleDirective_Define
//------------------------------------------------------------------------------
bool BFFTokenizer::HandleDirective_Define( const BFFFile & /*file*/, const char * & /*pos*/, const char * /*end*/, BFFTokenRange & argsIter )
{
    ASSERT( argsIter->IsKeyword( "define" ) );
    argsIter++;

    // Check for end
    if ( argsIter->IsIdentifier() == false )
    {
        // TODO:C A better error
        Error::Error_1031_UnexpectedCharFollowingDirectiveName( argsIter.GetCurrent(), "define", '?' );
        return false;
    }

    // Define the token
    const AString & identifier = argsIter->GetValueString();
    if ( BFFMacros::Get().Define( identifier ) == false )
    {
        // Attempting to overwrite an existing token
        Error::Error_1038_OverwritingTokenInDefine( argsIter.GetCurrent() );
        return false;
    }
    argsIter++; // consume identifier

    return true;
}

// HandleDirective_Else
//------------------------------------------------------------------------------
bool BFFTokenizer::HandleDirective_Else( const BFFFile & /*file*/, const char * & /*pos*/, const char * /*end*/, BFFTokenRange & argsIter )
{
    ASSERT( argsIter->IsKeyword( "else" ) );

    // Finding the else directive is handled by ParseIfDirective, so if we hit one
    // by itself, that's an error
    Error::Error_1041_ElseWithoutIf( argsIter.GetCurrent() );
    return false;
}

// HandleDirective_If
//------------------------------------------------------------------------------
bool BFFTokenizer::HandleDirective_If( const BFFFile & file, const char * & pos, const char * end, BFFTokenRange & argsIter )
{
    ASSERT( argsIter->IsKeyword( "if" ) );
    argsIter++;

    enum { IF_NONE = 1, IF_AND = 2, IF_OR = 4, IF_NEGATE = 8 };
    bool ranOnce = false;
    uint8_t operatorHistory[ BFFParser::MAX_OPERATOR_HISTORY ];   // Record any expression operators into an array in order to process the operator precedence after we finish parsing the line
    uint32_t numOperators = 0;

    while ( !ranOnce || ( ranOnce && ( argsIter->IsOperator( "&&" ) || argsIter->IsOperator( "||" ) ) ) )
    {
        uint32_t ifOperator = IF_NONE;
        if ( argsIter->IsOperator( "&&" ) || argsIter->IsOperator( "||" ) )
        {
            // check if this has been run once, to avoid
            // expressions like #if &&a
            if ( !ranOnce )
            {
                Error::Error_1046_IfExpressionCannotStartWithBooleanOperator( argsIter.GetCurrent() );
                return false;
            }

            ifOperator = ( argsIter->IsOperator( "&&" ) ? IF_AND : IF_OR );
            argsIter++; // consume operator
        }

        // Check for negation operator
        if ( argsIter->IsOperator( '!' ) )
        {
            ifOperator |= IF_NEGATE; // the condition will be inverted
            argsIter++; // consume negation operator
        }

        bool result;

        // Keyword or identifier?
        if ( argsIter->IsKeyword( BFF_KEYWORD_EXISTS ) )
        {
            argsIter++; // consume keyword
            if ( HandleDirective_IfExists( argsIter, result ) == false )
            {
                return false;
            }
        }
        else if ( argsIter->IsKeyword( BFF_KEYWORD_FILE_EXISTS ) )
        {
            argsIter++; // consume keyword
            if ( HandleDirective_IfFileExists( file, argsIter, result ) == false )
            {
                return false;
            }
        }
        else if ( argsIter->IsIdentifier() )
        {
            if ( HandleDirective_IfDefined( argsIter, result ) == false )
            {
                return false;
            }
        }
        else
        {
            // TODO:C A better error
            Error::Error_1031_UnexpectedCharFollowingDirectiveName( argsIter.GetCurrent(), "if", '?' );
            return false;
        }

        // Negate result to handle "#if !"
        if ( ifOperator & IF_NEGATE )
        {
            result = !( result );
        }

        // First expression in sequence?
        if ( !ranOnce )
        {
            const uint8_t r = ( result ? 1u : 0u );
            operatorHistory[ numOperators++ ] = r;
            ranOnce = true;
        }
        else
        {
            // Record boolean operator
            uint8_t r = (uint8_t)( ( ifOperator & IF_AND ) ? IF_AND : IF_OR );

            // Merge in result
            r |= ( result ? 1u : 0u );

            // Store to history
            operatorHistory[ numOperators++ ] = r;
            
            // Check for excessive complexity
            if ( numOperators == BFFParser::MAX_OPERATOR_HISTORY )
            {
                Error::Error_1047_IfExpressionTooComplex( argsIter.GetCurrent() );
                return false;
            }            
        }
    }
    
    // Apply any && operators. Any valid expression isn't going to end with an operator, so we don't check that.
    for ( uint32_t i = 0; i < ( numOperators - 1 ); i++ )
    {
        if ( operatorHistory[ i + 1 ] & IF_AND )
        {
            // Do the AND operation and store it in the right hand operator being tested
            operatorHistory[ i + 1 ] = ( uint8_t )( operatorHistory[ i ] & operatorHistory[ i + 1 ] );
            // Clear the left hand operator, its job is now done
            operatorHistory[ i ] = 0;
        }
    }
    // Apply any || operators
    bool result = false;
    for ( uint32_t i = 0; i < numOperators; i++ )
    {
        result |= ( operatorHistory[ i ] & 1 );
    }

    // take note of start of "true" block
    const char * ifBlockBegin = pos;

    // Find #else or #endif
    bool hasElseBlock( false );
    const char * ifBlockEnd;
    if ( !ParseToEndIf( pos, end, file, true, ifBlockEnd, &hasElseBlock ) ) // Allow else
    {
        return false; // ParseToEndIf will have emitted an error
    }
    if ( hasElseBlock )
    {
        // Find end of else block
        const char * elseBlockBegin = pos;
        const char * elseBlockEnd;
        if ( !ParseToEndIf( pos, end, file, false, elseBlockEnd, nullptr ) ) // Must be endif
        {
            return false; // ParseToEndIf will have emitted an error
        }

        if ( result == true )
        {
            // Parse If -> Else
            if ( Tokenize( file, ifBlockBegin, ifBlockEnd ) == false )
            {
                return false; // Parse will have emitted an error
            }
        }
        else
        {
            // Parse Else -> EndIf
            if ( Tokenize( file, elseBlockBegin, elseBlockEnd ) == false )
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
            if ( Tokenize( file, ifBlockBegin, ifBlockEnd ) == false )
            {
                return false; // Parse will have emitted an error
            }
        }
    }
    return true;
}

// HandleDirective_IfExists
//------------------------------------------------------------------------------
bool BFFTokenizer::HandleDirective_IfExists( BFFTokenRange & iter, bool & outResult )
{
    // Expect open bracket
    if ( iter->IsRoundBracket( '(' ) == false )
    {
        Error::Error_1031_UnexpectedCharFollowingDirectiveName( iter.GetCurrent(), "exists", '(' );
        return false;
    }
    iter++; // consume open (

    // Expect identifier
    if ( iter->IsIdentifier() == false )
    {
        // TODO:C Better error?
        Error::Error_1030_UnknownDirective( iter.GetCurrent(), iter->GetValueString() );
        return false;
    }
    const AString & varName = iter->GetValueString();
    iter++; // consume string value

    // Expect close bracket
    if ( iter->IsRoundBracket( ')' ) == false )
    {
        Error::Error_1031_UnexpectedCharFollowingDirectiveName( iter.GetCurrent(), "exists", ')' );
        return false;
    }
    iter++; // consume close )

    // look for varName in system environment
    AStackString<> varValue;
    uint32_t varHash = 0;
    const bool optional = true;
    // TODO:C Move ImportEnvironmentVar to BFFTokenizer
    FBuild::Get().ImportEnvironmentVar( varName.Get(), optional, varValue, varHash );
    outResult = ( varHash != 0 ); // a hash of 0 means the env var was not found
    return true;
}

// HandleDirective_IfFileExists
//------------------------------------------------------------------------------
bool BFFTokenizer::HandleDirective_IfFileExists( const BFFFile & file, BFFTokenRange & iter, bool & outResult )
{
    // Expect open bracket
    if ( iter->IsRoundBracket( '(' ) == false )
    {
        Error::Error_1031_UnexpectedCharFollowingDirectiveName( iter.GetCurrent(), "file_exists", '(' );
        return false;
    }
    iter++; // consume open (

    // Expect string
    if ( iter->IsString() == false )
    {
        // TODO:C Better error?
        Error::Error_1030_UnknownDirective( iter.GetCurrent(), iter->GetValueString() );
        return false;
    }
    const AString & fileName = iter->GetValueString();
    iter++; // consume string value

    // Expect close bracket
    if ( iter->IsRoundBracket( ')' ) == false )
    {
        Error::Error_1031_UnexpectedCharFollowingDirectiveName( iter.GetCurrent(), "file_exists", ')' );
        return false;
    }
    iter++; // consume close )

    AStackString<> includePath( fileName );
    ExpandIncludePath( file, includePath );

    // check if file exists
    outResult = FBuild::Get().AddFileExistsCheck( includePath );
    return true;
}

// HandleDirective_IfDefined
//------------------------------------------------------------------------------
bool BFFTokenizer::HandleDirective_IfDefined( BFFTokenRange & iter,
                                              bool & outResult )
{
    ASSERT( iter->IsIdentifier() );
    const AString & identifier = iter->GetValueString();
    iter++; // consume identifier

    outResult = m_Macros.IsDefined( identifier );
    return true;
}

// ParseToEndIf
//------------------------------------------------------------------------------
bool BFFTokenizer::ParseToEndIf( const char * & pos, const char * end, const BFFFile & file, bool allowElse, const char * & outBlockEnd, bool * outIsElse )
{
    const char * blockEnd = nullptr;

    // Advance iterator past entire #if block
    size_t depth = 1; // handle nested ifs. start with 1 for the #if we're matching
    while ( depth > 0 )
    {
        SkipWhitespaceOnCurrentLine( pos );

        // did we hit the end of the file?
        if ( pos == end )
        {
            BFFToken error( file, pos, BFFTokenType::Invalid, pos, pos + 1 );
            Error::Error_1012_UnexpectedEndOfFile( &error ); // TODO:C better error for this?
            return false;
        }

        // find the next preprocessor directive
        if ( *pos == BFFParser::BFF_PREPROCESSOR_START )
        {
            blockEnd = pos;
            ++pos; // Consume #
            SkipWhitespaceOnCurrentLine( pos );

            // get directive
            AStackString<> directiveName;
            if ( GetDirective( file, pos, directiveName ) == false )
            {
                return false; // GetDirective will have emitted an error
            }

            if ( directiveName == BFF_KEYWORD_ENDIF )
            {
                --depth;
            }
            else if ( directiveName == BFF_KEYWORD_IF )
            {
                ++depth;
            }
            else if ( ( depth == 1 ) && ( directiveName == BFF_KEYWORD_ELSE ) )
            {
                if ( allowElse == false )
                {
                    BFFToken error( file, pos, BFFTokenType::Invalid, pos, pos + 1 );
                    Error::Error_1041_ElseWithoutIf( &error );
                    return false;
                }
                if ( outIsElse )
                {
                    *outIsElse = true;
                }
                outBlockEnd = blockEnd;
                return true;
            }

            // continue searching....
        }
        // continue searching....

        SkipToStartOfNextLine( pos, end );
    }

    if ( outIsElse )
    {
        *outIsElse = false;
    }
    outBlockEnd = blockEnd;

    return true;
}

// HandleDirective_Import
//------------------------------------------------------------------------------
bool BFFTokenizer::HandleDirective_Import( const BFFFile & file, const char * & pos, const char * /*end*/, BFFTokenRange & argsIter )
{
    ASSERT( argsIter->IsKeyword( "import" ) );
    argsIter++;

    // #import expects a literal arg
    if ( argsIter->IsIdentifier() == false )
    {
        Error::Error_1031_UnexpectedCharFollowingDirectiveName( argsIter.GetCurrent(), "import", '?' ); // TODO:B A better error
        return false;
    }
    const AString & envVarToImport( argsIter->GetValueString() );

    // TODO:B Check validity of macro arg

    // look for varName in system environment
    AStackString<> varValue;
    uint32_t varHash = 0;
    const bool optional = false;
    // TODO:C Move ImportEnvironmentVar to BFFTokenizer
    if ( FBuild::IsValid() ) // TODO:B Remove singleton validity check when ImportEnvironmentVar moved to BFFTokenizer
    {
        if ( FBuild::Get().ImportEnvironmentVar( envVarToImport.Get(), optional, varValue, varHash ) == false )
        {
            Error::Error_1009_UnknownVariable( argsIter.GetCurrent(), nullptr, envVarToImport );
            return false;
        }
    }
    argsIter++;

    // We must escape ^ and $ so they won't be interpretted as special chars
    varValue.Replace( "^", "^^" );
    varValue.Replace( "$", "^$" );

    // Inject variable declaration
    AStackString<> varName( "." );
    varName += envVarToImport;
    m_Tokens.EmplaceBack( file, pos, BFFTokenType::Variable, varName );
    m_Tokens.EmplaceBack( file, pos, BFFTokenType::Operator, AStackString<>( "=" ) );
    m_Tokens.EmplaceBack( file, pos, BFFTokenType::String, varValue );

    return true;
}

// HandleDirective_Include
//------------------------------------------------------------------------------
bool BFFTokenizer::HandleDirective_Include( const BFFFile & file, const char * & /*pos*/, const char * /*end*/, BFFTokenRange & argsIter )
{
    ASSERT( argsIter->IsKeyword( "include" ) );

    // Check include depth to detect cyclic includes
    m_Depth++;
    if ( m_Depth >= 128 )
    {
        Error::Error_1035_ExcessiveDepthComplexity( argsIter.GetCurrent() );
        return false;
    }
    argsIter++;

    // #include expects a string arg for the include path
    if ( argsIter->IsString() == false )
    {
        Error::Error_1031_UnexpectedCharFollowingDirectiveName( argsIter.GetCurrent(), "include", '?' ); // TODO:B A better error
        return false;
    }
    AStackString<> include( argsIter->GetValueString() );

    // TODO:B Check for empty string

    ExpandIncludePath( file, include );

    // Recursively tokenize
    const bool result = Tokenize( include, argsIter.GetCurrent() );
    argsIter++;

    --m_Depth;

    return result;
}

// HandleDirective_Once
//------------------------------------------------------------------------------
bool BFFTokenizer::HandleDirective_Once( const BFFFile & file, const char * & /*pos*/, const char * /*end*/, BFFTokenRange & argsIter )
{
    ASSERT( m_Files.Find( &file ) ); // Must be a file we're tracking

    ASSERT( argsIter->IsKeyword( "once" ) );
    argsIter++;

    ASSERT( file.IsParseOnce() == false ); // Shouldn't be parsing a second time

    file.SetParseOnce();
    return true;
}

// HandleDirective_Undef
//------------------------------------------------------------------------------
bool BFFTokenizer::HandleDirective_Undef( const BFFFile & /*file*/, const char * & /*pos*/, const char * /*end*/, BFFTokenRange & argsIter )
{
    ASSERT( argsIter->IsKeyword( "undef" ) );
    argsIter++; // consume directive

    // Expect an identifier
    if ( argsIter->IsIdentifier() == false )
    {
        // TODO:C A better error
        Error::Error_1031_UnexpectedCharFollowingDirectiveName( argsIter.GetCurrent(), "undef", '?' );
        return false;
    }

    // Undefine the token
    const AString & identifier = argsIter->GetValueString();
    if ( BFFMacros::Get().Undefine( identifier ) == false )
    {
        if ( identifier.BeginsWith( "__" ) )
        {
            Error::Error_1040_UndefOfBuiltInTokenNotAllowed( argsIter.GetCurrent() );
        }
        else
        {
            Error::Error_1039_UnknownTokenInUndef( argsIter.GetCurrent() );
        }
        return false;
    }
    argsIter++; // consume identifier

    return true;
}

// GetQuotedString
//------------------------------------------------------------------------------
bool BFFTokenizer::GetQuotedString( const BFFFile & file, const char * & pos, AString & outString ) const
{
    ASSERT( IsStringStart( *pos ) );

    // Take note of quote (we'll look for the matching one)
    const char * openQuotePos = pos;
    const char openChar = *pos;
    ++pos; // Consume open quote

    // Parse to end of string
    const char * stringStart = pos; // String starts inside quotes
    bool escapeNext = false;
    for ( ;; )
    {
        const char c = *pos;

        // String must end on the same line
        if ( IsAtEndOfLine( c ) )
        {
            const BFFToken error( file, openQuotePos, BFFTokenType::Invalid, openQuotePos, openQuotePos + 1 );
            Error::Error_1002_MatchingClosingTokenNotFound( &error, nullptr, openChar );
            return false;
        }

        // Handle escape char
        if ( escapeNext == false )
        {
            // Found closing quote?
            if ( c == openChar )
            {
                break;
            }

            // Escape next char
            escapeNext = ( c == '^' );
        }
        else
        {
            escapeNext = false;
        }

        ++pos;
    }

    // String is everything between the quotes (including escape chars which are untouched)
    outString.Assign( stringStart, pos );
    ++pos; // consume closing quote
    return true;
}

// GetDirective
//------------------------------------------------------------------------------
bool BFFTokenizer::GetDirective( const BFFFile & file, const char * & pos, AString & outDirectiveName ) const
{
    // Find end of range of valid directive characters
    const char * directiveNameStart = pos;
    if ( IsLowercaseLetter( *pos ) ) // Must start with lowercase letter
    {
        while( IsLowercaseLetter( *pos ) || IsUnderscore( *pos ) )
        {
            ++pos;
        }
    }

    outDirectiveName.Assign( directiveNameStart, pos );

    // Report invalid directive
    if ( outDirectiveName.IsEmpty() )
    {
        // TODO:C More specific error?
        const BFFToken error( file, pos, BFFTokenType::Invalid, directiveNameStart, pos );
        Error::Error_1030_UnknownDirective( &error, outDirectiveName );
        return false;
    }

    return true;
}

// ExpandIncludePath
//------------------------------------------------------------------------------
void BFFTokenizer::ExpandIncludePath( const BFFFile & file, AString & includePath ) const
{
    // Includes are relative to current file, unless full paths
    if ( PathUtils::IsFullPath( includePath ) == false )
    {
        // Determine path from current file
        const AString & currentFile = file.GetFileName();
        const char * lastSlash = currentFile.FindLast( NATIVE_SLASH );
        lastSlash = lastSlash ? lastSlash : currentFile.FindLast( OTHER_SLASH );
        lastSlash = lastSlash ? ( lastSlash + 1 ): currentFile.Get(); // file only, truncate to empty
        AStackString<> tmp( currentFile.Get(), lastSlash );
        tmp += includePath;
        includePath = tmp;
    }
}

//------------------------------------------------------------------------------
