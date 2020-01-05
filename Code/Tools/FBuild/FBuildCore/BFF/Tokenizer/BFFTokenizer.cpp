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
    bool IsOperator( const char c )         { return ( ( c == '+' ) || ( c == '-' ) || ( c == '=' ) || ( c == '!' ) || ( c == '<' ) || ( c == '>' ) ); }
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
    PROFILE_FUNCTION

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

    // Parse a previously seen file again?
    if ( fileToParse )
    {
        return Tokenize( *fileToParse, fileToParse->GetSourceFileContents().Get(), fileToParse->GetSourceFileContents().GetEnd() );
    }

    // A file seen for the first time
    BFFFile * newFile = FNEW( BFFFile() );
    if ( newFile->Load( cleanFileName ) == false )
    {
        FDELETE( newFile );
        return false; // Load will have emitted an error
    }
    m_Files.Append( newFile );

    // Recursively tokenize
    return Tokenize( newFile );
}

// TokenizeFromString
//------------------------------------------------------------------------------
bool BFFTokenizer::TokenizeFromString( const AString & fileName, const AString & fileContents )
{
    PROFILE_FUNCTION

    // Canonicalize path
    AStackString<> cleanFileName;
    NodeGraph::CleanPath( fileName, cleanFileName );

    // A file seen for the first time
    BFFFile * newFile = FNEW( BFFFile( fileName.Get(), fileContents ) );
    m_Files.Append( newFile );

    // Recursively tokenize
    return Tokenize( newFile );
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
        char c = *pos;
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
                return false; // HandleDirective will have emitted an error
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
            m_Tokens.Append( BFFToken( file, tokenStart, BFFTokenType::String, string ) );
            continue;
        }

        // Two character operator?
        if ( ( c == '=' ) || ( c == '!' ) || ( c == '<' ) || ( c == '>' ) )
        {
            if ( ( pos + 1 ) < end )
            {
                AStackString<> opString;
                opString += pos[ 0 ];
                opString += pos[ 1 ];

                if ( ( opString == "==" ) ||
                     ( opString == "!=" ) ||
                     ( opString == "<=" ) ||
                     ( opString == ">=" ) )
                {
                    m_Tokens.Append( BFFToken( file, pos, BFFTokenType::Operator, pos, pos + 2 ) );
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
                m_Tokens.Append( BFFToken( file, tokenStart, BFFTokenType::Operator, pos, pos + 1 ) );
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
            m_Tokens.Append( BFFToken( file, tokenStart, BFFTokenType::Number, tokenStart, pos ) );
            continue;
        }

        // Comma
        if ( IsComma( c ) )
        {
            ++pos;
            m_Tokens.Append( BFFToken( file, tokenStart, BFFTokenType::Comma, tokenStart, pos ) );
            continue;
        }

        // Round Brackets?
        if ( ( c == '(' ) || ( c == ')' ) )
        {
            m_Tokens.Append( BFFToken( file, tokenStart, BFFTokenType::RoundBracket, pos, pos + 1 ) );
            pos++;
            continue;
        }

        // Curly Brackets?
        if ( ( c == '{' ) || ( c == '}' ) )
        {
            m_Tokens.Append( BFFToken( file, tokenStart, BFFTokenType::CurlyBracket, pos, pos + 1 ) );
            pos++;
            continue;
        }

        // Square Brackets?
        if ( ( c == '[' ) || ( c == ']' ) )
        {
            m_Tokens.Append( BFFToken( file, tokenStart, BFFTokenType::SquareBracket, pos, pos + 1 ) );
            pos++;
            continue;
        }

        // # directive
        if ( IsDirective( c ) )
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

    m_Tokens.Append( BFFToken( file, end, BFFTokenType::EndOfFile, AString::GetEmpty() ) );
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
        m_Tokens.Append( BFFToken( file, idStart, BFFTokenType::Boolean, true ) );
        return true;
    }
    if ( identifier == BFF_KEYWORD_FALSE )
    {
        m_Tokens.Append( BFFToken( file, idStart, BFFTokenType::Boolean, false ) );
        return true;
    }

    // - Keywords
    if ( ( identifier == BFF_KEYWORD_DEFINE ) ||
         ( identifier == BFF_KEYWORD_ELSE ) ||
         ( identifier == BFF_KEYWORD_EXISTS ) ||
         ( identifier == BFF_KEYWORD_IF ) ||
         ( identifier == BFF_KEYWORD_IMPORT ) ||
         ( identifier == BFF_KEYWORD_INCLUDE ) ||
         ( identifier == BFF_KEYWORD_IN ) ||
         ( identifier == BFF_KEYWORD_NOT ) ||
         ( identifier == BFF_KEYWORD_ONCE ) ||
         ( identifier == BFF_KEYWORD_UNDEF ) )
    {
        m_Tokens.Append( BFFToken( file, idStart, BFFTokenType::Keyword, identifier ) );
        return true;
    }

    // - Functions
    if ( Function::Find( identifier ) )
    {
        m_Tokens.Append( BFFToken( file, idStart, BFFTokenType::Function, identifier ) );
        return true;
    }

    // Unspecified Identifier
    m_Tokens.Append( BFFToken( file, idStart, BFFTokenType::Identifier, identifier ) );
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

    m_Tokens.Append( BFFToken( file, variableStart, BFFTokenType::Variable, variableName ) );
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
    if ( Tokenize( file, argsStart, argsEnd ) == false )
    {
        return false; // Tokenize will have emitted an error
    }

    // Remove directive args tokens (the directive parsing consumes them)
    const size_t numArgTokens = m_Tokens.GetSize() - numTokens;
    StackArray<BFFToken> args;
    for ( size_t i = 0; i < numArgTokens; ++i )
    {
        if ( m_Tokens[ numTokens + i ].GetType() != BFFTokenType::EndOfFile )
        {
            args.Append( Move( m_Tokens[ numTokens + i ] ) );
        }
    }
    for ( size_t i = 0; i < numArgTokens; ++i )
    {
        m_Tokens.Pop(); // Avoiding use of SetSize as this requires a default constructor
    }

    // # must be followed by something
    if ( args.IsEmpty() )
    {
        // TODO:C Improve error
        const BFFToken error( file, argsStart, BFFTokenType::Invalid, AStackString<>( "???" ) );
        Error::Error_1030_UnknownDirective( &error, AStackString<>( "???" ) );
        return false;
    }

    // Check keywords that are valid directives
    const BFFToken & directiveToken = args[ 0 ];
    const AString & directive = directiveToken.GetValueString();
    if ( directiveToken.IsKeyword() )
    {
        if ( directive == "define" )        { return HandleDirective_Define( file, pos, end, args ); }
        else if ( directive == "else" )     { return HandleDirective_Else( file, pos, end, args ); }
        else if ( directive == "if" )       { return HandleDirective_If( file, pos, end, args ); }
        else if ( directive == "import" )   { return HandleDirective_Import( file, pos, end, args ); }
        else if ( directive == "include" )  { return HandleDirective_Include( file, pos, end, args ); }
        else if ( directive == "once" )     { return HandleDirective_Once( file, pos, end, args ); }
        else if ( directive == "undef" )    { return HandleDirective_Undef( file, pos, end, args ); }

        // TODO:C We should have an error for unused args here

        // fall through to error handling
    }

    const BFFToken error( file, argsStart, BFFTokenType::Invalid, argsStart );
    Error::Error_1030_UnknownDirective( &error, directive );
    return false;
}

// HandleDirective_Define
//------------------------------------------------------------------------------
bool BFFTokenizer::HandleDirective_Define( const BFFFile & /*file*/, const char * & /*pos*/, const char * /*end*/, const Array<BFFToken> & args )
{
    ASSERT( args[ 0 ].IsKeyword( "define" ) );

    const AString & directive = args[ 0 ].GetValueString();

    BFFTokenRange range( args.Begin(), args.End() );
    range++; // consume directive

    // Check for end
    if ( range.IsAtEnd() || ( range->IsIdentifier() == false ) )
    {
        // TODO:C A better error
        Error::Error_1031_UnexpectedCharFollowingDirectiveName( &args[ 0 ], directive, '?' );
        return false;
    }

    const BFFToken * identifierToken = range.GetCurrent();
    const AString & identifier = range->GetValueString();
    range++; // consume identifier

    // Define the token
    if ( BFFMacros::Get().Define( identifier ) == false )
    {
        // Attempting to overwrite an existing token
        Error::Error_1038_OverwritingTokenInDefine( identifierToken );
        return false;
    }

    return true;
}

// HandleDirective_Else
//------------------------------------------------------------------------------
bool BFFTokenizer::HandleDirective_Else( const BFFFile & /*file*/, const char * & /*pos*/, const char * /*end*/, const Array<BFFToken> & args )
{
    ASSERT( args[ 0 ].IsKeyword( "else" ) );

    // Finding the else directive is handled by ParseIfDirective, so if we hit one
    // by itself, that's an error
    Error::Error_1041_ElseWithoutIf( &args[ 0 ] );
    return false;
}

// HandleDirective_If
//------------------------------------------------------------------------------
bool BFFTokenizer::HandleDirective_If( const BFFFile & file, const char * & pos, const char * end, const Array<BFFToken> & args )
{
    ASSERT( args[ 0 ].IsKeyword( "if" ) );

    const AString & directive = args[ 0 ].GetValueString();

    BFFTokenRange range( args.Begin(), args.End() );
    range++; // consume directive

    // Check for end
    if ( range.IsAtEnd() )
    {
        // TODO:C A better error
        Error::Error_1031_UnexpectedCharFollowingDirectiveName( &args[ 0 ], directive, '?' );
        return false;
    }

    // Check for negation operator
    bool negate = false;
    if ( range->IsOperator( "!" ) )
    {
        negate = true;
        range++; // consume negation operator

        // Check for end
        if ( range.IsAtEnd() )
        {
            // TODO:C A better error
            Error::Error_1031_UnexpectedCharFollowingDirectiveName( &args[ 0 ], directive, '?' );
            return false;
        }
    }

    // Keyword or identifier?
    bool result = false;
    if ( range->IsKeyword() && ( range->GetValueString() == BFF_KEYWORD_EXISTS ) )
    {
        range++; // consume keyword
        if ( HandleDirective_IfExists( range, result ) == false )
        {
            return false;
        }
    }
    else if ( range->IsIdentifier() )
    {
        if ( HandleDirective_IfDefined( range, result ) == false )
        {
            return false;
        }
    }
    else
    {
        // TODO:C A better error
        Error::Error_1031_UnexpectedCharFollowingDirectiveName( &args[ 0 ], directive, '?' );
        return false;
    }

    // Negate result to handle "#if !"
    if ( negate )
    {
        result = !( result );
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
        Error::Error_1031_UnexpectedCharFollowingDirectiveName( iter.GetCurrent(), AStackString<>( "exists" ), '(' );
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
        Error::Error_1031_UnexpectedCharFollowingDirectiveName( iter.GetCurrent(), AStackString<>( "exists" ), ')' );
        return false;
    }
    iter++; // consume close )

    // look for varName in system environment
    AStackString<> varValue;
    uint32_t varHash = 0;
    bool optional = true;
    // TODO:C Move ImportEnvironmentVar to BFFTokenizer
    FBuild::Get().ImportEnvironmentVar( varName.Get(), optional, varValue, varHash );
    outResult = ( varHash != 0 ); // a hash of 0 means the env var was not found
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
                return true;;
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
bool BFFTokenizer::HandleDirective_Import( const BFFFile & file, const char * & pos, const char * /*end*/, const Array<BFFToken> & args )
{
    BFFTokenRange iter( args.Begin(), args.End() );
    ASSERT( iter->IsKeyword( "import" ) );
    iter++;

    // #import takes exactly one literal arg
    if ( iter.IsAtEnd() )
    {
        const AString & directive = args[ 0 ].GetValueString();
        Error::Error_1031_UnexpectedCharFollowingDirectiveName( &args[ 0 ], directive, '?' );// TODO:A A better error
        return false;
    }
    if ( iter->IsIdentifier() == false )
    {
        const AString & directive = args[ 0 ].GetValueString();
        const BFFToken & badToken = args[ 1 ];
        Error::Error_1031_UnexpectedCharFollowingDirectiveName( &badToken, directive, '?' ); // TODO:B A better error
        return false;
    }
    const AString & envVarToImport( args[ 1 ].GetValueString() );

    // TODO:B Check validity of macro arg

    // TODO:B Check for extraneous args

    // look for varName in system environment
    AStackString<> varValue;
    uint32_t varHash = 0;
    bool optional = false;
    // TODO:C Move ImportEnvironmentVar to BFFTokenizer
    if ( FBuild::Get().ImportEnvironmentVar( envVarToImport.Get(), optional, varValue, varHash ) == false )
    {
        Error::Error_1009_UnknownVariable( &args[ 1 ], nullptr, envVarToImport );
        return false;
    }

    // Inject variable declaration
    AStackString<> varName( "." );
    varName += envVarToImport;
    m_Tokens.Append( BFFToken( file, pos, BFFTokenType::Variable, varName ) );
    m_Tokens.Append( BFFToken( file, pos, BFFTokenType::Operator, AStackString<>( "=" ) ) );
    m_Tokens.Append( BFFToken( file, pos, BFFTokenType::String, varValue ) );

    return true;
}

// HandleDirective_Include
//------------------------------------------------------------------------------
bool BFFTokenizer::HandleDirective_Include( const BFFFile & file, const char * & /*pos*/, const char * /*end*/, const Array<BFFToken> & args )
{
    BFFTokenRange iter( args.Begin(), args.End() );
    ASSERT( iter->IsKeyword( "include" ) );
    iter++;

    // #include takes exactly one string arg
    if ( iter.IsAtEnd() )
    {
        const AString & directive = args[ 0 ].GetValueString();
        Error::Error_1031_UnexpectedCharFollowingDirectiveName( &args[ 0 ], directive, '?' );// TODO:A A better error
        return false;
    }
    if ( iter->IsString() == false )
    {
        const AString & directive = args[ 0 ].GetValueString();
        const BFFToken & badToken = args[ 1 ];
        Error::Error_1031_UnexpectedCharFollowingDirectiveName( &badToken, directive, '?' ); // TODO:B A better error
        return false;
    }
    AStackString<> include( args[ 1 ].GetValueString() );

    // TODO:B Check for empty string

    // NOTE: Substitutions are not supported on includes

    // Includes are relative to current file, unless full paths
    if ( PathUtils::IsFullPath( include ) == false )
    {
        // Determine path from current file
        const AString & currentFile = file.GetFileName();
        const char * lastSlash = currentFile.FindLast( NATIVE_SLASH );
        lastSlash = lastSlash ? lastSlash : currentFile.FindLast( OTHER_SLASH );
        lastSlash = lastSlash ? ( lastSlash + 1 ): currentFile.Get(); // file only, truncate to empty
        AStackString<> tmp( currentFile.Get(), lastSlash );
        tmp += include;
        include = tmp;
    }

    // Check include depth to detect cyclic includes
    m_Depth++;
    if ( m_Depth >= 128 )
    {
        Error::Error_1035_ExcessiveDepthComplexity( &args[ 0 ] );
        return false;
    }

    // Recursively tokenize
    const bool result = TokenizeFromFile( include );

    --m_Depth;

    return result;
}

// HandleDirective_Once
//------------------------------------------------------------------------------
bool BFFTokenizer::HandleDirective_Once( const BFFFile & file, const char * & /*pos*/, const char * /*end*/, const Array<BFFToken> & args )
{
    ASSERT( m_Files.Find( &file ) ); // Must be a file we're tracking

    ASSERT( args[ 0 ].IsKeyword( "once" ) );

    // #once takes no additional args
    if ( args.GetSize() != 1 )
    {
        const AString & directive = args[ 0 ].GetValueString();
        const BFFToken & badToken = args[ 1 ];
        Error::Error_1031_UnexpectedCharFollowingDirectiveName( &badToken, directive, badToken.GetValueString()[ 0 ] );
        return false;
    }

    ASSERT( file.IsParseOnce() == false ); // Shouldn't be parsing a second time

    file.SetParseOnce();
    return true;
}

// HandleDirective_Undef
//------------------------------------------------------------------------------
bool BFFTokenizer::HandleDirective_Undef( const BFFFile & /*file*/, const char * & /*pos*/, const char * /*end*/, const Array<BFFToken> & args )
{
    ASSERT( args[ 0 ].IsKeyword( "undef" ) );

    const AString & directive = args[ 0 ].GetValueString();

    BFFTokenRange range( args.Begin(), args.End() );
    range++; // consume directive

    // Check for end
    if ( range.IsAtEnd() || ( range->IsIdentifier() == false ) )
    {
        // TODO:C A better error
        Error::Error_1031_UnexpectedCharFollowingDirectiveName( &args[ 0 ], directive, '?' );
        return false;
    }

    const BFFToken * identifierToken = range.GetCurrent();
    const AString & identifier = range->GetValueString();
    range++; // consume identifier

    // Undefine the token
    if ( BFFMacros::Get().Undefine( identifier ) == false )
    {
        if ( identifierToken->GetValueString().BeginsWith( "__" ) )
        {
            Error::Error_1040_UndefOfBuiltInTokenNotAllowed( identifierToken );
        }
        else
        {
            Error::Error_1039_UnknownTokenInUndef( identifierToken );
        }
        return false;
    }

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

//------------------------------------------------------------------------------
