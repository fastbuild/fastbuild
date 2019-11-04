// AStackString.h
//------------------------------------------------------------------------------
#pragma once

// Includes
//------------------------------------------------------------------------------
#include "AString.h"

// AStackString<>
//------------------------------------------------------------------------------
template <int RESERVED = 256 >
class AStackString : public AString
{
public:
    explicit AStackString();
    explicit AStackString( const AString & string );
    explicit AStackString( AString && string );
    explicit AStackString( const AStackString & string );
    explicit AStackString( AStackString && string );
    explicit AStackString( const char * string );
    explicit AStackString( const char * start, const char * end );
    inline ~AStackString() = default;

    AStackString< RESERVED > & operator = ( const char * string ) { Assign( string ); return *this; }
    AStackString< RESERVED > & operator = ( const AString & string ) { Assign( string ); return *this; }
    AStackString< RESERVED > & operator = ( AString && string ) { Assign( Move( string ) ); return *this; }
    AStackString< RESERVED > & operator = ( const AStackString & string ) { Assign( string ); return *this; }
    AStackString< RESERVED > & operator = ( AStackString && string ) { Assign( Move( string ) ); return *this; }

private:
    char m_Storage[ RESERVED + 1 ];
};

// CONSTRUCTOR
//------------------------------------------------------------------------------
template < int RESERVED >
AStackString< RESERVED >::AStackString()
{
    static_assert( ( RESERVED % 2 ) == 0, "Capacity must be multiple of 2" );
    m_Contents = m_Storage;
    SetReserved( RESERVED, false );
    m_Storage[ 0 ] = '\0';
}

// CONSTRUCTOR
//------------------------------------------------------------------------------
template < int RESERVED >
AStackString< RESERVED >::AStackString( const AString & string )
{
    static_assert( ( RESERVED % 2 ) == 0, "Capacity must be multiple of 2" );
    m_Contents = m_Storage;
    SetReserved( RESERVED, false );
    Assign( string );
}

// CONSTRUCTOR (AString &&)
//------------------------------------------------------------------------------
template < int RESERVED >
AStackString< RESERVED >::AStackString( AString && string )
    : AString()
{
    static_assert( ( RESERVED % 2 ) == 0, "Capacity must be multiple of 2" );
    m_Contents = m_Storage;
    SetReserved( RESERVED, false );
    Assign( Move( string ) );
}

// CONSTRUCTOR (const AString &)
//------------------------------------------------------------------------------
template < int RESERVED >
AStackString< RESERVED >::AStackString( const AStackString & string )
    : AString()
{
    static_assert( ( RESERVED % 2 ) == 0, "Capacity must be multiple of 2" );
    m_Contents = m_Storage;
    SetReserved( RESERVED, false );
    Assign( string );
}

// CONSTRUCTOR (AStackString &&)
//------------------------------------------------------------------------------
template < int RESERVED >
AStackString< RESERVED >::AStackString( AStackString && string )
    : AString()
{
    static_assert( ( RESERVED % 2 ) == 0, "Capacity must be multiple of 2" );
    m_Contents = m_Storage;
    SetReserved( RESERVED, false );
    Assign( Move( string ) );
}

// CONSTRUCTOR (const char *)
//------------------------------------------------------------------------------
template < int RESERVED >
AStackString< RESERVED >::AStackString( const char * string )
{
    static_assert( ( RESERVED % 2 ) == 0, "Capacity must be multiple of 2" );
    m_Contents = m_Storage;
    SetReserved( RESERVED, false );
    Assign( string );
}

// CONSTRUCTOR( const char *, const char *)
//------------------------------------------------------------------------------
template < int RESERVED >
AStackString< RESERVED >::AStackString( const char * start, const char * end )
{
    static_assert( ( RESERVED % 2 ) == 0, "Capacity must be multiple of 2" );
    m_Contents = m_Storage;
    SetReserved( RESERVED, false );
    Assign( start, end );
}

//------------------------------------------------------------------------------
