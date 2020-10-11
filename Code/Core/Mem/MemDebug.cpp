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
    // this function assumes at least 64 bit alignment
    ASSERT( uintptr_t( ptr ) % sizeof( uint64_t ) == 0 );

    // fill 64 bits at a tim
    const size_t numU64 = size / sizeof( uint64_t );
    uint64_t * it = static_cast< uint64_t * >( ptr );
    const uint64_t* end = it + numU64;
    while ( it != end )
    {
        *it = pattern;
        ++it;
    }

    // fill remaining bytes
    size_t remainder = size & ( sizeof( uint64_t ) - 1 );
    if (remainder)
    {
        const char * src = reinterpret_cast< const char * >( &pattern );
        char * dst = static_cast< char * >( static_cast< void * >( it ) );
        while (remainder)
        {
            *dst = *src;
            ++dst;
            ++src;
            --remainder;
        }
    }
}

//------------------------------------------------------------------------------
#endif // MEM_DEBUG_ENABLED
