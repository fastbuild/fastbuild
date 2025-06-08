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
    // xxHash
    unsigned int XXH32( const void * input, size_t length, unsigned seed );
    unsigned long long XXH64( const void * input, size_t length, unsigned long long seed );

    // xxhash3
    unsigned long long xxHashLib_XXH3_64bits( const void * input, size_t length );
    struct xxHashLib_XXH3_state_s;
    void xxHashLib_XXH3_64bits_reset( xxHashLib_XXH3_state_s * state );
    void xxHashLib_XXH3_64bits_update( xxHashLib_XXH3_state_s * state, const void * data, size_t dataSize );
    uint64_t xxHashLib_XXH3_64bits_digest( xxHashLib_XXH3_state_s * state );
};

// xxHash
//------------------------------------------------------------------------------
class xxHash
{
public:
    static uint32_t Calc32( const void * buffer, size_t len );
    static uint64_t Calc64( const void * buffer, size_t len );

    static uint32_t Calc32( const AString & string ) { return Calc32( string.Get(), string.GetLength() ); }
    static uint64_t Calc64( const AString & string ) { return Calc64( string.Get(), string.GetLength() ); }
private:
    enum { XXHASH_SEED = 0x0 }; // arbitrarily chosen random seed
};

// xxHash3
//------------------------------------------------------------------------------
class xxHash3
{
public:
    static uint64_t Calc64( const void * buffer, size_t len );

    static uint64_t Calc64( const AString & string ) { return Calc64( string.Get(), string.GetLength() ); }
};

// xxHash3Accumulator
//------------------------------------------------------------------------------
#if defined( ASSERTS_ENABLED )
    PRAGMA_DISABLE_PUSH_MSVC( 4324 ) // structure was padded due to alignment specifier
#endif
class alignas( 64 ) xxHash3Accumulator
{
public:
    // Create an accumulator
    xxHash3Accumulator()
    {
        xxHashLib_XXH3_64bits_reset( reinterpret_cast<xxHashLib_XXH3_state_s *>( m_State ) );
    }

    // Add data in as many blocks as necessary
    void AddData( const void * data, size_t dataSize )
    {
        ASSERT( mFinalized == false );
        xxHashLib_XXH3_64bits_update( reinterpret_cast<xxHashLib_XXH3_state_s *>( m_State ),
                                      data,
                                      dataSize );
    }

    // Generate the final hash once and get the result
    uint64_t Finalize64()
    {
        ASSERT( mFinalized == false );
        #if defined( ASSERTS_ENABLED )
            mFinalized = true;
        #endif
        return xxHashLib_XXH3_64bits_digest( reinterpret_cast<xxHashLib_XXH3_state_s *>( m_State ) );
    }

protected:
    uint64_t m_State[ 72 ]; // See XXH3_state_s - note required alignas(64) on class
    #if defined( ASSERTS_ENABLED )
        bool mFinalized = false;
    #endif
};
#if defined( ASSERTS_ENABLED )
    PRAGMA_DISABLE_POP_MSVC
#endif

// Calc32
//------------------------------------------------------------------------------
/*static*/ inline uint32_t xxHash::Calc32( const void * buffer, size_t len )
{
    return XXH32( buffer, len, XXHASH_SEED );
}

// Calc64
//------------------------------------------------------------------------------
/*static*/ inline uint64_t xxHash::Calc64( const void * buffer, size_t len )
{
    return XXH64( buffer, len, XXHASH_SEED );
}

// Calc64 (xxHash3)
//------------------------------------------------------------------------------
/*static*/ inline uint64_t xxHash3::Calc64( const void * buffer, size_t len )
{
    return xxHashLib_XXH3_64bits( buffer, len );
}

//------------------------------------------------------------------------------
