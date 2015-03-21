// AStackString.h
//------------------------------------------------------------------------------
#pragma once
#ifndef CORE_STRINGS_ASTACKSTRING_H
#define CORE_STRINGS_ASTACKSTRING_H

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
	explicit AStackString( const AStackString & string );
	explicit AStackString( const char * string );
	explicit AStackString( const char * start, const char * end );
	inline ~AStackString() {}

	AStackString< RESERVED > & operator = ( const char * string ) { Assign( string ); return *this; }
	AStackString< RESERVED > & operator = ( const AString & string ) { Assign( string ); return *this; }

private:
	char m_Storage[ RESERVED + 1 ];
};

// CONSTRUCTOR
//------------------------------------------------------------------------------
template < int RESERVED >
AStackString< RESERVED >::AStackString()
{
	CTASSERT( ( RESERVED % 2 ) == 0 );
	m_Contents = m_Storage;
	SetReserved( RESERVED, false );
	m_Storage[ 0 ] = '\0';
}

// CONSTRUCTOR
//------------------------------------------------------------------------------
template < int RESERVED >
AStackString< RESERVED >::AStackString( const AString & string )
{
	CTASSERT( ( RESERVED % 2 ) == 0 );
	m_Contents = m_Storage;
	SetReserved( RESERVED, false );
	Assign( string );
}

// CONSTRUCTOR (const AString &)
//------------------------------------------------------------------------------
template < int RESERVED >
AStackString< RESERVED >::AStackString( const AStackString & string )
{
	CTASSERT( ( RESERVED % 2 ) == 0 );
	m_Contents = m_Storage;
	SetReserved( RESERVED, false );
	Assign( string );
}

// CONSTRUCTOR (const char *)
//------------------------------------------------------------------------------
template < int RESERVED >
AStackString< RESERVED >::AStackString( const char * string )
{
	CTASSERT( ( RESERVED % 2 ) == 0 );
	m_Contents = m_Storage;
	SetReserved( RESERVED, false );
	Assign( string );
}

// CONSTRUCTOR( const char *, const char *)
//------------------------------------------------------------------------------
template < int RESERVED >
AStackString< RESERVED >::AStackString( const char * start, const char * end )
{
	CTASSERT( ( RESERVED % 2 ) == 0 );
	m_Contents = m_Storage;
	SetReserved( RESERVED, false );
	Assign( start, end );
}

//------------------------------------------------------------------------------
#endif // #define CORE_STRINGS_ASTACKSTRING_H
