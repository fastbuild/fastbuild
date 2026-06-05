// DTLTOJsonParser
//------------------------------------------------------------------------------

// Includes
//------------------------------------------------------------------------------
#include "DTLTOJsonParser.h"

// FBuildCore
#include "Tools/FBuild/FBuildCore/FLog.h"

// Core
#include "Core/Env/ErrorFormat.h"
#include "Core/FileIO/FileStream.h"
#include "Core/Strings/AStackString.h"
#include "Core/Tracing/Tracing.h"

// CONSTRUCTOR
//------------------------------------------------------------------------------
DTLTOJsonParser::DTLTOJsonParser( const char * fileName )
    : m_FileName( fileName )
{
}

// Load
//------------------------------------------------------------------------------
bool DTLTOJsonParser::Load()
{
    FileStream f;
    if ( !f.Open( m_FileName.Get(), FileStream::READ_ONLY ) )
    {
        FLOG_ERROR( "DTLTO: failed to open '%s'. Error: %s", m_FileName.Get(), LAST_ERROR_STR );
        return false;
    }

    if ( !f.ReadIntoString( m_Buffer ) )
    {
        FLOG_ERROR( "DTLTO: failed to read '%s'", m_FileName.Get() );
        return false;
    }

    m_Pos = m_Buffer.Get();
    m_End = m_Buffer.GetEnd();
    return true;
}

// SkipWhitespaces
//------------------------------------------------------------------------------
void DTLTOJsonParser::SkipWhitespaces()
{
    for ( char c = Peek(); ( c == ' ' ) || ( c == '\t' ) || ( c == '\r' ) || ( c == '\n' ); c = Peek() )
    {
        Consume();
    }
}

// MatchChar
//------------------------------------------------------------------------------
bool DTLTOJsonParser::MatchChar( char c )
{
    SkipWhitespaces();
    if ( Peek() != c )
    {
        return false;
    }
    Consume();
    return true;
}

// MatchString
//------------------------------------------------------------------------------
bool DTLTOJsonParser::MatchString( AString & result )
{
    SkipWhitespaces();
    if ( Peek() != '"' )
    {
        return false;
    }
    Consume();
    result.Clear();
    for ( ;; )
    {
        const char c = Consume();
        if ( c == '"' )
        {
            return true;
        }
        if ( c == EndOfInput )
        {
            FLOG_ERROR( "DTLTO: unterminated string" );
            return false;
        }
        if ( c != '\\' )
        {
            result += c;
            continue;
        }

        const char esc = Consume();
        switch ( esc )
        {
            case '"': result += '"'; break;
            case '\\': result += '\\'; break;
            case '/': result += '/'; break;
            case 'b': result += '\b'; break;
            case 'f': result += '\f'; break;
            case 'n': result += '\n'; break;
            case 'r': result += '\r'; break;
            case 't': result += '\t'; break;
            case EndOfInput:
                FLOG_ERROR( "DTLTO: truncated escape at end of input" );
                return false;
            default:
                FLOG_ERROR( "DTLTO: unknown escape '\\%c' in string (\\uXXXX not yet supported)", esc );
                return false;
        }
    }
}

// MatchObject
//------------------------------------------------------------------------------
template <size_t NUM_MATCHERS>
bool DTLTOJsonParser::MatchObject( const PropertyMatcher ( &matchers )[ NUM_MATCHERS ] )
{
    if ( !MatchChar( '{' ) )
    {
        FLOG_ERROR( "DTLTO: expected '{'" );
        return false;
    }

    for ( ;; )
    {
        // Match property name
        AStackString<64> key;
        if ( !MatchString( key ) )
        {
            FLOG_ERROR( "DTLTO: expected property name" );
            return false;
        }

        if ( !MatchChar( ':' ) )
        {
            FLOG_ERROR( "DTLTO: expected ':' after property '%s'", key.Get() );
            return false;
        }

        // Search among known properties
        bool found = false;
        for ( size_t i = 0; i < NUM_MATCHERS; ++i )
        {
            const PropertyMatcher & matcher = matchers[ i ];
            if ( key == matcher.m_PropertyName )
            {
                if ( !( this->*( matcher.m_Matcher ) )() )
                {
                    FLOG_ERROR( "DTLTO: failed to parse property value for '%s'", key.Get() );
                    return false;
                }
                found = true;
                break;
            }
        }

        if ( !found )
        {
            FLOG_ERROR( "DTLTO: unknown property '%s'", key.Get() );
            return false;
        }

        if ( MatchChar( '}' ) )
        {
            return true;
        }

        if ( !MatchChar( ',' ) )
        {
            FLOG_ERROR( "DTLTO: expected ',' after value for property '%s'", key.Get() );
            return false;
        }
    }
}

// Parse
//------------------------------------------------------------------------------
bool DTLTOJsonParser::Parse()
{
    const PropertyMatcher matchers[] = {
        { "common", &DTLTOJsonParser::MatchCommonProp },
        // TODO: { "jobs", &DTLTOJsonParser::MatchJobsProp },
    };

    return MatchObject( matchers );
}

// MatchCommonProp
//------------------------------------------------------------------------------
bool DTLTOJsonParser::MatchCommonProp()
{
    const PropertyMatcher matchers[] = {
        { "linker_output", &DTLTOJsonParser::MatchLinkerOutputProp },
        // TODO: { "args", &DTLTOJsonParser::MatchArgsProp },
        // TODO: { "inputs", &DTLTOJsonParser::MatchInputsProp },
    };

    return MatchObject( matchers );
}

// MatchLinkerOutputProp
//------------------------------------------------------------------------------
bool DTLTOJsonParser::MatchLinkerOutputProp()
{
    AStackString<> propVal;
    if ( !MatchString( propVal ) )
    {
        FLOG_ERROR( "DTLTO: expected string for 'linker_output'" );
        return false;
    }

    m_Data.m_LinkerOutput = propVal;
    return true;
}

//------------------------------------------------------------------------------
