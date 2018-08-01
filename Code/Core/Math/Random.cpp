// Random
//------------------------------------------------------------------------------

// Includes
//------------------------------------------------------------------------------
#include "Core/PrecompiledHeader.h"

#include "Random.h"

#include "Core/Process/Process.h"
#if defined( __WINDOWS__ )
    #include <Bcrypt.h>
#endif
#include <time.h>

// CONSTRUCTOR
//------------------------------------------------------------------------------
Random::Random()
{
    static uint32_t offset( 0 );
    m_Seed = (uint32_t)time( nullptr ) + offset;
    offset++;
}

// GetRand
//------------------------------------------------------------------------------
uint32_t Random::GetRand()
{
    m_Seed = m_Seed * 1103515245 + 12345;
    return( (uint32_t)( m_Seed/ ( ( CORE_RAND_MAX + 1 ) * 2 ) ) % ( CORE_RAND_MAX + 1 ) );
}

// GetCryptRandom
//------------------------------------------------------------------------------
/*static*/ bool Random::GetCryptRandom( uint32_t ( & randomInts )[ 2 ] )
{
    bool success = false;
    #if defined( __WINDOWS__ )
        HRESULT hr = BCryptGenRandom(
                NULL, 
                (BYTE*)&randomInts[0], 
                sizeof( randomInts ), 
                BCRYPT_USE_SYSTEM_PREFERRED_RNG
                );
        success = SUCCEEDED(hr);
    #elif defined( __LINUX__ ) || defined( __APPLE__ )
        GetRand( randomInts[ 0 ] );
        GetRand( randomInts[ 1 ] );
        success = true;
    #else
        #error Unknown platform
    #endif
    return success;
}

// GetCryptRandomString
//------------------------------------------------------------------------------
/*static*/ bool Random::GetCryptRandomString( AString & randomString )
{
    bool success = false;
    uint32_t randomInts[2];
    if ( GetCryptRandom( randomInts ) )
    {
        randomString.Format(
            "%08X_%08X%08X",
            Process::GetCurrentId(),
            randomInts[ 0 ], randomInts[ 1 ] );
    }
    return success;
}

//------------------------------------------------------------------------------
