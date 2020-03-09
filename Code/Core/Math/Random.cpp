// Random
//------------------------------------------------------------------------------

// Includes
//------------------------------------------------------------------------------
#include "Random.h"

#include "Core/Process/Process.h"
#if defined( __WINDOWS__ )
    #include "Core/Env/WindowsHeader.h"
    #include <Bcrypt.h>
#else
    #include <fcntl.h>
    #include <unistd.h>
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
    #if defined( __WINDOWS__ )
        HRESULT hr = BCryptGenRandom(
                NULL, 
                (BYTE*)&randomInts[0], 
                sizeof( randomInts ), 
                BCRYPT_USE_SYSTEM_PREFERRED_RNG
                );
        bool success = SUCCEEDED(hr);
    #elif defined( __LINUX__ ) || defined( __APPLE__ )
        int32_t devRandom = open( "/dev/urandom", O_RDONLY | O_CLOEXEC );
        bool success = true;
        for ( uint32_t randomIntIndex = 0; randomIntIndex < 2; ++randomIntIndex )
        {
            int32_t randnum;
            success = success &&
                      devRandom != -1 &&
                      read( devRandom, &randnum, sizeof randnum ) == sizeof randnum;
            if ( success )
            {
                randomInts[ randomIntIndex ] = randnum;
            }
        }
        if ( devRandom != -1 )
        {
            close( devRandom );
        }
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
