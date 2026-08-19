// DTLTOJsonParser
//------------------------------------------------------------------------------

// Includes
//------------------------------------------------------------------------------
#include "DTLTOJsonParser.h"

// FBuildCore
#include "Tools/FBuild/FBuildCore/FLog.h"

// Core
#include "Core/Strings/AStackString.h"
#include "Core/Tracing/Tracing.h"

// CONSTRUCTOR
//------------------------------------------------------------------------------
DTLTOJsonParser::DTLTOJsonParser( const AString & buffer )
    : m_Pos( buffer.Get() )
    , m_End( buffer.GetEnd() )
{
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
        FLOG_ERROR( "DTLTO: expected string" );
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
                    return false; // matcher emits its own error
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

// MatchArray
//------------------------------------------------------------------------------
template <typename ELEMENT_PARSER>
bool DTLTOJsonParser::MatchArray( ELEMENT_PARSER parseElement )
{
    if ( !MatchChar( '[' ) )
    {
        FLOG_ERROR( "DTLTO: expected '['" );
        return false;
    }

    if ( MatchChar( ']' ) )
    {
        return true;
    }

    for ( ;; )
    {
        if ( !parseElement() )
        {
            return false;
        }

        if ( MatchChar( ']' ) )
        {
            return true;
        }

        if ( !MatchChar( ',' ) )
        {
            FLOG_ERROR( "DTLTO: expected ',' or ']' in array" );
            return false;
        }
    }
}

// MatchStringArray
//------------------------------------------------------------------------------
bool DTLTOJsonParser::MatchStringArray( Array<AString> & out )
{
    return MatchArray( [ this, &out ]() -> bool {
        AStackString<> element;
        if ( !MatchString( element ) )
        {
            return false;
        }
        out.Append( element );
        return true;
    } );
}

// Parse
//------------------------------------------------------------------------------
bool DTLTOJsonParser::Parse()
{
    const PropertyMatcher matchers[] = {
        { "common", &DTLTOJsonParser::MatchCommonProp },
        { "jobs", &DTLTOJsonParser::MatchJobsProp },
    };

    return MatchObject( matchers );
}

// MatchCommonProp
//------------------------------------------------------------------------------
bool DTLTOJsonParser::MatchCommonProp()
{
    const PropertyMatcher matchers[] = {
        { "linker_output", &DTLTOJsonParser::MatchLinkerOutputProp },
        { "args", &DTLTOJsonParser::MatchArgsProp },
        { "inputs", &DTLTOJsonParser::MatchInputsProp },
    };

    if ( !MatchObject( matchers ) )
    {
        FLOG_ERROR( "DTLTO: failed to read property 'common'" );
        return false;
    }
    return true;
}

// MatchLinkerOutputProp
//------------------------------------------------------------------------------
bool DTLTOJsonParser::MatchLinkerOutputProp()
{
    AStackString<> propVal;
    if ( !MatchString( propVal ) )
    {
        FLOG_ERROR( "DTLTO: failed to read property 'linker_output'" );
        return false;
    }

    m_Data.m_LinkerOutput = propVal;
    return true;
}

// MatchStringArrayProp
//------------------------------------------------------------------------------
bool DTLTOJsonParser::MatchStringArrayProp( const char * propertyName, Array<AString> & out )
{
    if ( !MatchStringArray( out ) )
    {
        FLOG_ERROR( "DTLTO: failed to read property '%s'", propertyName );
        return false;
    }
    return true;
}

// MatchArgsProp
//------------------------------------------------------------------------------
bool DTLTOJsonParser::MatchArgsProp()
{
    return MatchStringArrayProp( "args", m_Data.m_CommonArgs );
}

// MatchInputsProp
//------------------------------------------------------------------------------
bool DTLTOJsonParser::MatchInputsProp()
{
    return MatchStringArrayProp( "inputs", m_Data.m_CommonInputs );
}

// MatchJobsProp
//------------------------------------------------------------------------------
bool DTLTOJsonParser::MatchJobsProp()
{
    const bool ok = MatchArray( [ this ]() -> bool {
        m_CurrentJob = &m_Data.m_Jobs.EmplaceBack();
        const PropertyMatcher matchers[] = {
            { "args", &DTLTOJsonParser::MatchJobArgsProp },
            { "inputs", &DTLTOJsonParser::MatchJobInputsProp },
            { "outputs", &DTLTOJsonParser::MatchJobOutputsProp },
        };
        return MatchObject( matchers );
    } );
    if ( !ok )
    {
        FLOG_ERROR( "DTLTO: failed to read property 'jobs'" );
        return false;
    }
    return true;
}

// MatchJobArgsProp
//------------------------------------------------------------------------------
bool DTLTOJsonParser::MatchJobArgsProp()
{
    return MatchStringArrayProp( "args", m_CurrentJob->m_Args );
}

// MatchJobInputsProp
//------------------------------------------------------------------------------
bool DTLTOJsonParser::MatchJobInputsProp()
{
    return MatchStringArrayProp( "inputs", m_CurrentJob->m_Inputs );
}

// MatchJobOutputsProp
//------------------------------------------------------------------------------
bool DTLTOJsonParser::MatchJobOutputsProp()
{
    return MatchStringArrayProp( "outputs", m_CurrentJob->m_Outputs );
}

//------------------------------------------------------------------------------
