// MemDebug.cpp
//------------------------------------------------------------------------------

// Includes
//------------------------------------------------------------------------------
#include "Core/PrecompiledHeader.h"

#include "MemDebug.h"

#if defined( MEM_DEBUG_ENABLED )

// Core
#include "Core/Env/Assert.h"

// FillMem
//------------------------------------------------------------------------------
void MemDebug::FillMem( void * ptr, const size_t size, const uint32_t pattern )
{
    // this function assumes at least 32bit alignment
    ASSERT( uintptr_t( ptr ) % sizeof( uint32_t ) == 0 );

    // fill whole words
    const size_t numWords = size / sizeof( uint32_t );
    uint32_t * it = static_cast< uint32_t * >( ptr );
    const uint32_t * end = it + numWords;
    while ( it != end )
    {
        *it = pattern;
        ++it;
    }

    // fill remaining bytes
    const size_t remainder =  size - ( numWords * sizeof( uint32_t ) );
    if ( remainder )
    {
        // assuming little-endian format
        char bytes[ 3 ] = { (char)( ( pattern & 0x000000FF ) ),
                            (char)( ( pattern & 0x0000FF00 ) >> 8 ),
                            (char)( ( pattern & 0x00FF0000 ) >> 16 ) };
        const char * b = bytes;
        char * cit = static_cast< char * >( static_cast< void * >( it ) );
        switch( remainder )
        {
            case 3: *cit = *b; ++cit; ++b;
            case 2: *cit = *b; ++cit; ++b;
            case 1: *cit = *b;
        }
    }
}

//------------------------------------------------------------------------------
#endif // MEM_DEBUG_ENABLED
