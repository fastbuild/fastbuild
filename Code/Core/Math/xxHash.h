// xxHash.h
//------------------------------------------------------------------------------
#pragma once

// Includes
//------------------------------------------------------------------------------
#include "Core/Env/Types.h"
#include "Core/Strings/AString.h"

// avoid including xxhash header directly
extern "C"
{
    unsigned int XXH32( const void * input, size_t length, unsigned seed );
    unsigned long long XXH64( const void * input, size_t length, unsigned long long seed );
};

// xxHash
//------------------------------------------------------------------------------
class xxHash
{
public:
    inline static uint32_t  Calc32( const void * buffer, size_t len );
    inline static uint64_t  Calc64( const void * buffer, size_t len );

    inline static uint32_t  Calc32( const AString & string ) { return Calc32( string.Get(), string.GetLength() ); }
    inline static uint64_t  Calc64( const AString & string ) { return Calc64( string.Get(), string.GetLength() ); }
private:
    enum { XXHASH_SEED = 0x0 }; // arbitrarily chosen random seed
};

// Calc32
//------------------------------------------------------------------------------
/*static*/ uint32_t xxHash::Calc32( const void * buffer, size_t len )
{
    return XXH32( buffer, len, XXHASH_SEED );
}

// Calc64
//------------------------------------------------------------------------------
/*static*/ uint64_t xxHash::Calc64( const void * buffer, size_t len )
{
    return XXH64( buffer, len, XXHASH_SEED );
}

//------------------------------------------------------------------------------
