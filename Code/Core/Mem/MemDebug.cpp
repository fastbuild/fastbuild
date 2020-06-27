// MemDebug.cpp
//------------------------------------------------------------------------------

// Includes
//------------------------------------------------------------------------------
#include "MemDebug.h"

#if defined( MEM_DEBUG_ENABLED )

// Core
#include "Core/Env/Assert.h"
#include "Core/Env/Types.h"

// FillMem
//------------------------------------------------------------------------------
void MemDebug::FillMem( void * ptr, const size_t size, const uint64_t pattern )
{
    // this function assumes at least 64bit alignment
    ASSERT( uintptr_t( ptr ) % sizeof( uint64_t ) == 0 );

    // fill whole words
    const size_t numU64 = size / sizeof( uint64_t );
    uint64_t * it = static_cast< uint64_t * >( ptr );
    const uint64_t* end = it + numU64;
    while ( it != end )
    {
        *it = pattern;
        ++it;
    }

    // fill remaining bytes
    const size_t remainder = size - ( numU64 * sizeof( uint64_t ) );
    if ( remainder )
    {
        // assuming little-endian format
        const char * b = reinterpret_cast< const char * >( &pattern ) + 1;
        char * cit = static_cast< char * >( static_cast< void * >( it ) );
        switch( remainder )
        {
            case 7: *cit++ = *b++; *cit++ = *b++; *cit++ = *b++; *cit++ = *b++; *cit++ = *b++; *cit++ = *b++; *cit++ = *b++; break;
            case 6: *cit++ = *b++; *cit++ = *b++; *cit++ = *b++; *cit++ = *b++; *cit++ = *b++; *cit++ = *b++; break;
            case 5: *cit++ = *b++; *cit++ = *b++; *cit++ = *b++; *cit++ = *b++; *cit++ = *b++; break;
            case 4: *cit++ = *b++; *cit++ = *b++; *cit++ = *b++; *cit++ = *b++; break;
            case 3: *cit++ = *b++; *cit++ = *b++; *cit++ = *b++; break;
            case 2: *cit++ = *b++; *cit++ = *b++; break;
            case 1: *cit++ = *b++; break;
        }
    }
}

//------------------------------------------------------------------------------
#endif // MEM_DEBUG_ENABLED
