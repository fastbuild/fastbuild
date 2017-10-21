// Mem.cpp
//------------------------------------------------------------------------------

// Includes
//------------------------------------------------------------------------------
#include "Core/PrecompiledHeader.h"

#include "Mem.h"
#include "Core/Env/Assert.h"
#include "Core/Env/Types.h"
#include "Core/Mem/MemDebug.h"
#include "Core/Mem/MemTracker.h"
#include "Core/Mem/SmallBlockAllocator.h"

#include <stdlib.h>

// Defines
//------------------------------------------------------------------------------

// Alloc
//------------------------------------------------------------------------------
void * Alloc( size_t size )
{
    return AllocFileLine( size, sizeof( void * ), "Unknown", 0 );
}

// Alloc
//------------------------------------------------------------------------------
void * Alloc( size_t size, size_t alignment )
{
    return AllocFileLine( size, alignment, "Unknown", 0 );
}

// AllocFileLine
//------------------------------------------------------------------------------
void * AllocFileLine( size_t size, const char * file, int line )
{
    return AllocFileLine( size, sizeof( void * ), file, line );
}

// AllocFileLine
//------------------------------------------------------------------------------
void * AllocFileLine( size_t size, size_t alignment, const char * file, int line )
{
    void * mem( nullptr );

    #if defined( SMALL_BLOCK_ALLOCATOR_ENABLED )
        mem = SmallBlockAllocator::Alloc( size, alignment );
        if ( mem == nullptr )
    #endif
    {
        #if defined( __LINUX__ ) || defined( __APPLE__ )
            VERIFY( posix_memalign( &mem, alignment, size ) == 0 );
        #else
            mem = _aligned_malloc( size, alignment );
            __assume( mem );
        #endif

        #ifdef MEM_FILL_NEW_ALLOCATIONS
            MemDebug::FillMem( mem, size, MemDebug::MEM_FILL_NEW_ALLOCATION_PATTERN );
        #endif
    }

    MEMTRACKER_ALLOC( mem, size, file, line );
    (void)file; (void)line; // TODO: strip args in release

    return mem;
}

// Free
//------------------------------------------------------------------------------
void Free( void * ptr )
{
    if ( ptr == nullptr )
    {
        return;
    }

    MEMTRACKER_FREE( ptr );

    #if defined( SMALL_BLOCK_ALLOCATOR_ENABLED )
        if ( SmallBlockAllocator::Free( ptr ) == false )
    #endif
    {
        #if defined( __LINUX__ ) || defined( __APPLE__ )
            free( ptr );
        #else
            _aligned_free( ptr );
        #endif
    }
}

// Operators
//------------------------------------------------------------------------------
#if defined( MEMTRACKER_ENABLED )
    void * operator new( size_t size, const char * file, int line ) { return AllocFileLine( size, file, line ); }
    void * operator new[]( size_t size, const char * file, int line ) { return AllocFileLine( size, file, line ); }
    void operator delete( void * ptr, const char *, int ) { Free( ptr ); }
    void operator delete[]( void * ptr, const char *, int ) { Free( ptr ); }
#endif
#if !__has_feature( address_sanitizer ) && !__has_feature( memory_sanitizer ) && !__SANITIZE_ADDRESS__
void * operator new( size_t size ) { return Alloc( size ); }
void * operator new[]( size_t size ) { return Alloc( size ); }
void operator delete( void * ptr ) NOEXCEPT { Free( ptr ); }
void operator delete[]( void * ptr ) NOEXCEPT { Free( ptr ); }
#endif

//------------------------------------------------------------------------------
