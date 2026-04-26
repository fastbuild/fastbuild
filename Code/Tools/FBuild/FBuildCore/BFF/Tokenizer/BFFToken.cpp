// BFFToken.cpp
//------------------------------------------------------------------------------

// Includes
//------------------------------------------------------------------------------
#include "BFFToken.h"

#include <stdio.h>

//------------------------------------------------------------------------------
namespace
{
    static const AString sBoolTrueString( "true" );
    static const AString sBoolFalseString( "false" );
    static const AString sCommaString( "," );
    static const AString sBraceStringRoundLeft( "(" );
    static const AString sBraceStringRoundRight( ")" );
    static const AString sBraceStringCurlyLeft( "{" );
    static const AString sBraceStringCurlyRight( "}" );
    static const AString sBraceStringSquareLeft( "[" );
    static const AString sBraceStringSquareRight( "]" );
}

// Static Data
//------------------------------------------------------------------------------
/*static*/ const BFFFile BFFToken::s_BuiltInFile( "<builtin>", AString::GetEmpty() );
/*static*/ const BFFToken BFFToken::s_BuiltInToken( InvalidType::eInvalid,
                                                    s_BuiltInFile,
                                                    s_BuiltInFile.GetSourceFileContents().Get() );

//------------------------------------------------------------------------------
const AString & BFFToken::GetValueString() const
{
    // Return already available string for things that have unique string
    // representations
    if ( IsString() )
    {
        return m_String; // String may be empty
    }

    // Variables and Functions should always have a valid string
    if ( IsVariable() || IsFunction() || IsIdentifier() )
    {
        ASSERT( m_String.IsEmpty() == false );
        return m_String;
    }

    // Anything below here should rarely (if ever) need to be retrieved as a
    // string. The most common use-case would be for error reporting

    // If anything has a cached string, return that
    if ( m_String.IsEmpty() == false )
    {
        return m_String;
    }

    // Use global keyword strings
    if ( IsKeyword() )
    {
        return BFFKeyword::GetString( m_KeywordType );
    }

    // Bools use global strings to reduce overhead
    if ( IsBoolean() )
    {
        return m_Boolean ? sBoolTrueString : sBoolFalseString;
    }

    // Format numbers on demand
    if ( IsNumber() )
    {
        m_String.Format( "%i", m_Integer );
        return m_String;
    }

    // Use global operator strings
    if ( IsOperator() )
    {
        return BFFOperator::GetString( m_OperatorType );
    }

    // Use global comma string
    if ( IsComma() )
    {
        return sCommaString;
    }

    // Use global brace strings
    if ( IsBrace() )
    {
        switch ( GetBraceType() )
        {
            case BraceType::eRoundLeft: return sBraceStringRoundLeft;
            case BraceType::eRoundRight: return sBraceStringRoundRight;
            case BraceType::eCurlyLeft: return sBraceStringCurlyLeft;
            case BraceType::eCurlyRight: return sBraceStringCurlyRight;
            case BraceType::eSquareLeft: return sBraceStringSquareLeft;
            case BraceType::eSquareRight: return sBraceStringSquareRight;
        }
        ASSERT( false );
    }

    // End of file
    if ( GetType() == BFFTokenType::EndOfFile )
    {
        return AString::GetEmpty();
    }

    // Something is terribly wrong
    ASSERT( false );
    return m_String;
}

// GetPosInfo
//------------------------------------------------------------------------------
void BFFToken::GetPosInfo( uint32_t & outLine,
                           uint32_t & outColumn,
                           const char *& outLineStart ) const
{
    ASSERT( m_SourcePos >= m_BFFFile.GetSourceFileContents().Get() );
    ASSERT( m_SourcePos <= m_BFFFile.GetSourceFileContents().GetEnd() ); // <= to allow pointing to EOF

    // count the number of lines from the start of the file to
    // the current iterator position
    const char * p = m_BFFFile.GetSourceFileContents().Get();
    outLineStart = p;
    outLine = 1;
    while ( p < m_SourcePos ) // loop to current iterator pos
    {
        bool atLineEnd = false;
        if ( *p == '\n' )
        {
            // \n line ending
            atLineEnd = true;
            p++;
        }
        else if ( *p == '\r' )
        {
            // \r or \r\n line endings
            atLineEnd = true;
            p++;

            // skip \n for \r\n line endings
            if ( *p == '\n' )
            {
                p++;
            }
        }
        else
        {
            p++;
        }
        if ( atLineEnd )
        {
            outLineStart = p;
            outLine++;
        }
    }
    outColumn = (uint32_t)( ( p - outLineStart ) + 1 );
}

//------------------------------------------------------------------------------
