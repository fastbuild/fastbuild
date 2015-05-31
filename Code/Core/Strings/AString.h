// AString.h
//------------------------------------------------------------------------------
#pragma once
#ifndef CORE_STRINGS_ASTRING_H
#define CORE_STRINGS_ASTRING_H

// Includes
//------------------------------------------------------------------------------
#include "Core/Containers/Array.h"
#include "Core/Env/Assert.h"
#include "Core/Env/Types.h"

// Typedefs
//------------------------------------------------------------------------------
#ifndef va_list
    #if defined( __WINDOWS__ )
        typedef char * va_list; // avoid stdlib include
    #elif defined( __APPLE__ ) || defined( __LINUX__ )
        #include <stdarg.h>
    #endif
#endif

// AString
//------------------------------------------------------------------------------
class AString
{
public:
	explicit AString();
	explicit AString( uint32_t reserve );
	explicit AString( const AString & string );
	explicit AString( const char * string );
	explicit AString( const char * start, const char * end );
	~AString();

	inline uint32_t		GetLength() const	{ return m_Length; }
	inline uint32_t		GetReserved() const { return ( m_ReservedAndFlags & RESERVED_MASK ); }
	inline bool			IsEmpty() const		{ return ( m_Length == 0 ); }

	// C-style compatibility
	inline char *		Get()				{ return m_Contents; }
	inline const char * Get() const			{ return m_Contents; }
	inline char *		GetEnd()			{ return ( m_Contents + m_Length ); }
	inline const char *	GetEnd() const		{ return ( m_Contents + m_Length ); }
	inline char &		operator [] ( size_t index )		{ ASSERT( index < m_Length ); return m_Contents[ index ]; }
	inline const char & operator [] ( size_t index )  const { ASSERT( index < m_Length ); return m_Contents[ index ]; }

	// a pre-constructed global empty string for convenience
	static const AString & GetEmpty() { return s_EmptyAString; }

	// assignment
	inline AString & operator = ( const char * string ) { Assign( string ); return *this; }
	inline AString & operator = ( const AString & string ) { Assign( string ); return *this; }
	void Assign( const char * string );
	void Assign( const char * start, const char * end );
	void Assign( const AString & string );
	void Clear();
	void SetReserved( size_t capacity );

	// manually set length - NOTE: caller is responsible for making string contents valid
	void SetLength( uint32_t len );

	// concatenation
	AString & operator += ( char c );
	AString & operator += ( const char * string );
	AString & operator += ( const AString & string );
	inline void Append( const AString & string ) { this->operator +=( string ); }
	void Append( const char * string, size_t len );
	void AppendFormat( const char * fmtString, ... );

	// comparison
	bool operator == ( const char * other ) const;
	bool operator == ( const AString & other ) const;
	inline bool operator != ( const char * other ) const { return !(*this == other ); }
	inline bool operator != ( const AString & other ) const { return !(*this == other ); }
	int32_t CompareI( const AString & other ) const;
	inline bool operator < ( const AString & other ) const { return ( CompareI( other ) < 0 ); }

	inline bool MemoryMustBeFreed() const { return ( ( m_ReservedAndFlags & MEM_MUST_BE_FREED_FLAG ) == MEM_MUST_BE_FREED_FLAG ); }

	void Format( const char * fmtString, ... );
	void VFormat( const char * fmtString, va_list arg );

	void Tokenize( Array< AString > & tokens, char splitChar = ' ' ) const;

	// transformations
	uint32_t Replace( char from, char to, uint32_t maxReplaces = 0 );
	uint32_t Replace( const char * from, const char * to, uint32_t maxReplaces = 0 );
	void ToLower();

	// searching
	const char *	Find( char c, const char * startPos = nullptr ) const;
	char *			Find( char c, char * startPos = nullptr ) { return const_cast< char *>( ((const AString *)this)->Find( c, startPos ) ); }
	const char *	Find( const char * subString ) const;
	char *			Find( const char * subString ) { return const_cast< char *>( ((const AString *)this)->Find( subString ) ); }
	const char *	FindI( const char * subString ) const;
	const char *	FindLast( char c ) const;
	char *			FindLast( char c ) { return const_cast< char *>( ((const AString *)this)->FindLast( c ) ); }
	bool			EndsWith( char c ) const;
	bool			EndsWith( const char * string ) const;
    bool            EndsWith( const AString & other ) const;
	bool			EndsWithI( const char * other ) const;
	bool			EndsWithI( const AString & other ) const;
	bool			BeginsWith( char c ) const;
	bool			BeginsWith( const char * string ) const;
	bool			BeginsWith( const AString & string ) const;
	bool			BeginsWithI( const char * string ) const;
	bool			BeginsWithI( const AString & string ) const;

	// pattern matching
	static bool		Match( const char * pattern, const char * string );
	inline bool		Matches( const char * pattern ) const { return Match( pattern, m_Contents ); }
	static bool		MatchI( const char * pattern, const char * string );
	inline bool		MatchesI( const char * pattern ) const { return MatchI( pattern, m_Contents ); }

	// string manipulation helpers
	static void Copy( const char * src, char * dst, size_t len );
	static size_t StrLen( const char * string );
	static int32_t StrNCmp( const char * a, const char * b, size_t num );
	static int32_t StrNCmpI( const char * a, const char * b, size_t num );

protected:
	enum { MEM_MUST_BE_FREED_FLAG	= 0x00000001 };
	enum { RESERVED_MASK			= 0xFFFFFFFE };

	inline void SetReserved( uint32_t reserved, bool mustFreeMemory )
	{ 
		ASSERT( ( reserved & MEM_MUST_BE_FREED_FLAG ) == 0 ); // ensure reserved does not use lower bit
		m_ReservedAndFlags = ( reserved ^ ( mustFreeMemory ? MEM_MUST_BE_FREED_FLAG : 0 ) );
	}
	NO_INLINE void Grow( uint32_t newLen );		// Grow capacity, transferring existing string data (for concatenation)
	NO_INLINE void GrowNoCopy( uint32_t newLen ); // Grow capacity, discarding existing string data (for assignment/construction)

	char *		m_Contents;			// always points to valid null terminated string (even when empty)
	uint32_t	m_Length;			// length in characters
	uint32_t	m_ReservedAndFlags;	// reserved space in characters (even) and least significant bit used for static flag

	static const char * const   s_EmptyString;
	static const AString	s_EmptyAString;
};

//------------------------------------------------------------------------------
#endif // CORE_STRINGS_ASTRING_H
