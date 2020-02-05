// BFFToken.cpp
//------------------------------------------------------------------------------

// Includes
//------------------------------------------------------------------------------
#include "BFFToken.h"

#include <stdio.h>

// CONSTRUCTOR
//------------------------------------------------------------------------------
BFFToken::BFFToken( const BFFFile & file,
                    const char * sourcePos,
                    BFFTokenType type,
                    const char * valueStart,
                    const char * valueEnd )
    : m_Type( type )
    , m_Boolean( false )
    , m_Integer( 0 )
    , m_String( valueStart, valueEnd )
    , m_BFFFile( file )
    , m_SourcePos( sourcePos )
{
    if ( type == BFFTokenType::Number )
    {
        #if defined( __WINDOWS__ )
            VERIFY( sscanf_s( m_String.Get(), "%i", &m_Integer ) == 1 );
        #else
            VERIFY( sscanf( m_String.Get(), "%i", &m_Integer ) == 1 );
        #endif
    }
}

// CONSTRUCTOR
//------------------------------------------------------------------------------
BFFToken::BFFToken( const BFFFile & file, const char * sourcePos, BFFTokenType type, const AString & stringValue )
    : m_Type( type )
    , m_String( stringValue )
    , m_BFFFile( file )
    , m_SourcePos( sourcePos )
{
}

// CONSTRUCTOR
//------------------------------------------------------------------------------
BFFToken::BFFToken( const BFFFile & file, const char * sourcePos, BFFTokenType type, const bool boolValue )
    : m_Type( type )
    , m_Boolean( boolValue )
    , m_String( boolValue ? "true" : "false" )
    , m_BFFFile( file )
    , m_SourcePos( sourcePos )
{
}

// CONSTRUCTOR (Move)
//------------------------------------------------------------------------------
BFFToken::BFFToken( BFFToken && other )
    : m_Type( other.m_Type )
    , m_Boolean( other.m_Boolean )
    , m_Integer( other.m_Integer )
    , m_String( Move( other.m_String ) )
    , m_BFFFile( other.m_BFFFile )
    , m_SourcePos( other.m_SourcePos )
{
}

// GetPosInfo
//------------------------------------------------------------------------------
void BFFToken::GetPosInfo( uint32_t & outLine,
                           uint32_t & outColumn,
                           const char * & outLineStart ) const
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
