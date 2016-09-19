// BFFIterator
//------------------------------------------------------------------------------

// Includes
//------------------------------------------------------------------------------
#include "Tools/FBuild/FBuildCore/PrecompiledHeader.h"

#include "BFFIterator.h"
#include "Tools/FBuild/FBuildCore/BFF/BFFParser.h"

// operator =
//------------------------------------------------------------------------------
void BFFIterator::operator = ( const BFFIterator & iter )
{
    m_Pos = iter.m_Pos;
    m_MinPos = iter.m_MinPos;
    m_MaxPos = iter.m_MaxPos;
    m_FileName = iter.m_FileName;
    m_FileTimeStamp = iter.m_FileTimeStamp;
}

// SkipWhiteSpace
//------------------------------------------------------------------------------
void BFFIterator::SkipWhiteSpace()
{
    while ( !IsAtEnd() )
    {
        if ( IsAtWhitespace() )
        {
            (*this)++;
        }
        else
        {
            break;
        }
    }
}

// SkipComment
//------------------------------------------------------------------------------
void BFFIterator::SkipComment()
{
    // keep going until we hit the end of a line
    while ( !IsAtEnd() )
    {
        bool atLineEnd = (*m_Pos == '\n' );
        (*this)++;
        if ( atLineEnd )
        {
            break;
        }
    }
}

// SkipString
//------------------------------------------------------------------------------
void BFFIterator::SkipString( char quote )
{
    // start on open char (first ++ will handle this)
    ASSERT( *m_Pos == quote );

    while ( !IsAtEnd() )
    {
        m_Pos++;

        const char c = *m_Pos;

        // escape char?
        if ( c == '^' )
        {
            m_Pos++; // extra increment to skip escaped char
            continue;
        }

        // closing quote?
        if ( *m_Pos == quote )
        {
            return;
        }
    }
}

// SkipWhiteSpaceAndComments
//------------------------------------------------------------------------------
void BFFIterator::SkipWhiteSpaceAndComments()
{
    SkipWhiteSpace();
    while ( IsAtComment() )
    {
        SkipComment();
        SkipWhiteSpace();
    }
}

// SkipVariableName
//------------------------------------------------------------------------------
void BFFIterator::SkipVariableName()
{
    while ( !IsAtEnd() )
    {
        if ( !IsAtValidVariableNameCharacter() )
        {
            return;
        }

        // character still part of variable name
        (*this)++;
    }
}

// SkipFunctionName
//------------------------------------------------------------------------------
void BFFIterator::SkipFunctionName()
{
    while ( !IsAtEnd() )
    {
        if ( IsAtValidFunctionNameCharacter() )
        {
            // character still part of variable name
            (*this)++;
            continue;
        }

        break; // hit the end of a non-valid character for a function name
    }
}


// SkipDirectiveName
//------------------------------------------------------------------------------
void BFFIterator::SkipDirectiveName()
{
    while ( !IsAtEnd() )
    {
        if ( IsAtValidDirectiveNameCharacter() )
        {
            // character still part of directive name
            (*this)++;
            continue;
        }

        break; // hit the end of a non-valid character for a directive name
    }
}

// ParseToNext
//------------------------------------------------------------------------------
bool BFFIterator::ParseToNext( char c )
{
    if ( IsAtEnd() )
    {
        return false;
    }

    char prev = '\000';
    do
    {
        (*this)++;
        if ( *m_Pos == c )
        {
            if ( prev != '^' )
            {
                return true;
            }
        }
        prev = *m_Pos;
    } while ( !IsAtEnd() );

    return false;
}

// ParseToMatchingBrace
//------------------------------------------------------------------------------
bool BFFIterator::ParseToMatchingBrace( char openBrace, char closeBrace )
{
    ASSERT( *m_Pos == openBrace );

    do
    {
        m_Pos++;
        SkipWhiteSpaceAndComments();

        // hit a nested brace?
        if ( *m_Pos == openBrace )
        {
            if ( ParseToMatchingBrace( openBrace, closeBrace ) == false )
            {
                return false;
            }
            continue;
        }

        // hit a string?
        if ( ( *m_Pos == '\'' ) || ( *m_Pos == '"' ) )
        {
            SkipString( *m_Pos );
        }

        // hit the close brace?
        if ( *m_Pos == closeBrace )
        {
            return true;
        }

        // a regular charater.... keep searching

    } while ( !IsAtEnd() );

    return false;
}

// ParseExactString
//------------------------------------------------------------------------------
bool BFFIterator::ParseExactString( const char * string )
{
    const char * const originalPos = m_Pos;

    const char * stringPos = string; // stores pointer to the next character to be checked for presence in our sequence
    while ( !IsAtEnd() && ( *stringPos != '\000' ) )
    {
        if ( *m_Pos != *stringPos )
        {
            break;
    }
        m_Pos++;
        stringPos++;
    }

    if ( *stringPos == '\000' )
    {
        // we are at the end of string, that means all characters have successfuly matched
        return true;
    }
    else
    {
        // we are not at the end of string, that means either our sequence ended, or there is non matching character
        // restore our pos before returning
        m_Pos = originalPos;
        return false;
    }
}

// IsAtValidVariableNameCharacter
//------------------------------------------------------------------------------
bool BFFIterator::IsAtValidVariableNameCharacter() const
{
    const char c = *m_Pos;
    return ( ( ( c >= 'a' ) && ( c <= 'z' ) ) ||
             ( ( c >= 'A' ) && ( c <= 'Z' ) ) ||
             ( ( c >= '0' ) && ( c <= '9' ) ) ||
             ( c == '_' ) );
}

// IsAtValidFunctionNameCharacter
//------------------------------------------------------------------------------
bool BFFIterator::IsAtValidFunctionNameCharacter() const
{
    const char c = *m_Pos;
    return ( ( ( c >= 'a' ) && ( c <= 'z' ) ) ||
             ( ( c >= 'A' ) && ( c <= 'Z' ) ) ||
             ( ( c >= '0' ) && ( c <= '9' ) ) ||
             ( c == '_' ) );
}

// IsAtValidDirectiveNameCharacter
//------------------------------------------------------------------------------
bool BFFIterator::IsAtValidDirectiveNameCharacter() const
{
    const char c = *m_Pos;
    return ( ( c >= 'a' ) && ( c <= 'z' ) );
}

// IsAtWhitespace
//------------------------------------------------------------------------------
bool BFFIterator::IsAtWhitespace() const
{
    char c = *m_Pos;
    return ( ( c == '\n' ) ||
             ( c == '\r' ) ||
             ( c == '\t' ) ||
             ( c == ' ' ) );
}

// IsAtComment
//------------------------------------------------------------------------------
bool BFFIterator::IsAtComment() const
{
    const char c = *m_Pos;
    if ( c == BFFParser::BFF_COMMENT_SEMICOLON )
    {
        return true;
    }
    if ( ( c == BFFParser::BFF_COMMENT_SLASH ) && ( *( m_Pos + 1  ) == BFFParser::BFF_COMMENT_SLASH ) )
    {
        return true;
    }
    return false;
}

// GetPosInfo
//------------------------------------------------------------------------------
void BFFIterator::GetPosInfo( uint32_t & line,
                              uint32_t & column,
                              const char * & lineStart ) const
{
    // count the number of lines from the start of the file to
    // the current iterator position
    const char * p = m_MinPos;
    lineStart = m_MinPos;
    line = 1;
    while ( p < m_Pos ) // loop to current iterator pos
    {
        bool atLineEnd = ( *p == '\n' );
        p++;
        if ( atLineEnd )
        {
            lineStart = p;
            line++;
        }
    }
    column = (uint32_t)( ( p - lineStart ) + 1 );
}

//------------------------------------------------------------------------------
