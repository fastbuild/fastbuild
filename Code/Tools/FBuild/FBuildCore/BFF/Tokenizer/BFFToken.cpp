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

//------------------------------------------------------------------------------
