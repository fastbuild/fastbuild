// BFFIterator - track position in a BFF during parsing
//------------------------------------------------------------------------------
#pragma once
#ifndef FBUILD_BFF_BFFITERATOR_H
#define FBUILD_BFF_BFFITERATOR_H

// Includes
//------------------------------------------------------------------------------
#include "Core/Env/Assert.h"
#include "Core/Env/Types.h"
#include "Core/Strings/AString.h"

// Forward Declarations
//------------------------------------------------------------------------------

// BFFIterator
//------------------------------------------------------------------------------
class BFFIterator
{
public:
	inline BFFIterator()
		: m_Pos( nullptr )
		, m_MinPos( nullptr )
		, m_MaxPos( nullptr )
		//, m_FileName() // default constructed
		, m_FileTimeStamp( 0 ) {}
	inline BFFIterator( const char * pos, uint32_t dataSize, const char * fileName, uint64_t fileTimeStamp ) 
			: m_Pos( pos ) 
			, m_MinPos( pos )
			, m_MaxPos( pos + dataSize )
			, m_FileName( fileName )
			, m_FileTimeStamp( fileTimeStamp ) {}
	inline ~BFFIterator() {}

	void operator = ( const BFFIterator & iter );

	void operator ++ (int) { ASSERT( m_Pos < m_MaxPos ); m_Pos++; }
	void operator -- (int) { ASSERT( m_Pos > m_MinPos ); m_Pos--; }
	inline bool operator < ( const BFFIterator & other ) const { return ( m_Pos < other.m_Pos ); }
	inline bool operator > ( const BFFIterator & other ) const { return ( m_Pos > other.m_Pos ); }
	char operator *() const { return *m_Pos; }
	void SkipWhiteSpace();
	void SkipComment();
	void SkipString( char quote );
	void SkipWhiteSpaceAndComments();
	void SkipVariableName();
	void SkipFunctionName();
	void SkipDirectiveName();
	bool ParseToNext( char c );
	bool ParseToMatchingBrace( char openBrace, char closeBrace );
	bool ParseExactString( const char * string );

	inline size_t GetDistTo( const BFFIterator & other ) const { ASSERT( other.m_Pos >= m_Pos ); return (size_t)( other.m_Pos - m_Pos ); }

	// access to parse position for error reporting
	inline const char * GetCurrent() const { return m_Pos; }
	inline void SetMax( const char * max )
	{
		ASSERT( max >= m_Pos );
		ASSERT( max > m_MinPos );
		m_MaxPos = max;
	}

	bool IsAtComment() const;
	bool IsAtWhitespace() const;
	bool IsAtValidVariableNameCharacter() const;
	bool IsAtValidFunctionNameCharacter() const;
	bool IsAtValidDirectiveNameCharacter() const;
	inline bool IsAtEnd() const { ASSERT( m_Pos <= m_MaxPos ); return ( m_Pos == m_MaxPos ); }

	inline const AString & GetFileName() const { return m_FileName; }
	inline uint64_t GetFileTimeStamp() const { return m_FileTimeStamp; }
private:
	// use to report error position in bff file
	// (slow - only used during error reporting)
	friend class Error;
	void GetPosInfo( uint32_t & line,
					 uint32_t & column,
					 const char * & lineStart ) const;

	const char * m_Pos;
	const char * m_MinPos; // the 0th char of the file
	const char * m_MaxPos; // 1 past the end of parseable data
	AString		 m_FileName; // source file name
	uint64_t	 m_FileTimeStamp;
};

//------------------------------------------------------------------------------
#endif // FBUILD_BFF_BFFITERATOR_H
 