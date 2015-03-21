// Murmur3.h
//------------------------------------------------------------------------------
#pragma once
#ifndef CORE_MATH_MURMUR3_H
#define CORE_MATH_MURMUR3_H

// Includes
//------------------------------------------------------------------------------
#include "Core/Env/Types.h"
#include "Core/Math/MurmurHash3/MurmurHash3.h"
#include "Core/Strings/AString.h"

// Murmur3
//------------------------------------------------------------------------------
class Murmur3
{
public:
	inline static uint32_t	Calc32( const void * buffer, size_t len );
	inline static uint64_t	Calc64( const void * buffer, size_t len );
	inline static uint64_t Calc128( const void * buffer, size_t len, uint64_t & other );

	inline static uint32_t	Calc32( const AString & string ) { return Calc32( string.Get(), string.GetLength() ); }
	inline static uint64_t	Calc64( const AString & string ) { return Calc64( string.Get(), string.GetLength() ); }
	inline static uint64_t	Calc128( const AString & string, uint64_t & other )	{ return Calc128( string.Get(), string.GetLength(), other ); }
private:
	enum { MURMUR3_SEED = 0x65cc95f0 }; // arbitrarily chosen random seed
};

// Calc32
//------------------------------------------------------------------------------
/*static*/ uint32_t Murmur3::Calc32( const void * buffer, size_t len )
{
	uint32_t hash;
	MurmurHash3_x86_32( buffer, (int)len, 0, &hash );
	return hash;
}

// Calc64
//------------------------------------------------------------------------------
/*static*/ uint64_t	Murmur3::Calc64( const void * buffer, size_t len )
{
	uint64_t a1, a2;
	a1 = Murmur3::Calc128( buffer, len, a2 );
	return ( a1 ^ a2 ); // xor merge
}

// Calc128
//------------------------------------------------------------------------------
/*static*/ uint64_t Murmur3::Calc128( const void * buffer, size_t len, uint64_t & other )
{
	uint64_t hash[ 2 ];

	// using the x64 version - slower than CRC32 on x86, but 4x faster than CRC32 on x64
	MurmurHash3_x64_128( buffer, (int)len, MURMUR3_SEED, &hash );
	other = hash[ 0 ];
	return hash[ 1 ];
}

//------------------------------------------------------------------------------
#endif // CORE_MATH_MURMUR3_H
