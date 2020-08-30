// Mem.cpp
//------------------------------------------------------------------------------

// Includes
//------------------------------------------------------------------------------
#include "Mem.h"
#include "Core/Env/Assert.h"
#include "Core/Env/Types.h"
#include "Core/Mem/MemDebug.h"
#include "Core/Mem/MemTracker.h"
#include "Core/Mem/SmallBlockAllocator.h"

#include <stdlib.h>

// Alloc
//------------------------------------------------------------------------------
void * Alloc( size_t size )
{
    void * mem( nullptr );

    #if defined( SMALL_BLOCK_ALLOCATOR_ENABLED )
        mem = SmallBlockAllocator::Alloc( size, sizeof( void * ) );
        if ( mem == nullptr )
    #endif
    {
        #if defined( __LINUX__ ) || defined( __APPLE__ )
            #if defined( __clang__ )
                // Clang has a bug where class alignment is incorrectly reported, which
                // results in unsafe mixes of alignment and SSE instruction use, so we
                // enforce a minimum 16 byte alignment
                // Last seen in Apple LLVM version 10.0.0 (clang-1000.11.45.5) but exists in
                // other versions as well
                VERIFY( posix_memalign( &mem, 16, size ) == 0 );
            #else
                VERIFY( posix_memalign( &mem, sizeof( void * ), size ) == 0 );
            #endif
        #else
            mem = _aligned_malloc( size, sizeof( void * ) );
            __assume( mem );
        #endif

        #ifdef MEM_FILL_NEW_ALLOCATIONS
            MemDebug::FillMem( mem, size, MemDebug::MEM_FILL_NEW_ALLOCATION_PATTERN );
        #endif
    }

    return mem;
}

// Alloc
//------------------------------------------------------------------------------
void * Alloc( size_t size, size_t alignment )
{
    #if defined( __clang__ )
        // Clang has a bug where class alignment is incorrectly reported, which
        // results in unsafe mixes of alignment and SSE instruction use, so we
        // enforce a minimum 16 byte alignment
        // Last seen in Apple LLVM version 10.0.0 (clang-1000.11.45.5) but exists in
        // other versions as well
        alignment = ( alignment < 16 ) ? 16 : alignment;
    #endif

    void * mem;

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

    return mem;
}

// AllocFileLine
//------------------------------------------------------------------------------
#if defined( MEMTRACKER_ENABLED )
    void * AllocFileLine( size_t size, const char * file, int line )
    {
        void * mem = Alloc( size );
        MEMTRACKER_ALLOC( mem, size, file, line );
        return mem;
    }
#endif

// AllocFileLine
//------------------------------------------------------------------------------
#if defined( MEMTRACKER_ENABLED )
    void * AllocFileLine( size_t size, size_t alignment, const char * file, int line )
    {
        void * mem = Alloc( size, alignment );
        MEMTRACKER_ALLOC( mem, size, file, line );
        return mem;
    }
#endif

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
    // Via FNEW macros
    void * operator new( size_t size, const char * file, int line ) { return AllocFileLine( size, file, line ); }
    void * operator new[]( size_t size, const char * file, int line ) { return AllocFileLine( size, file, line ); }
    void operator delete( void * ptr, const char *, int ) { Free( ptr ); }
    void operator delete[]( void * ptr, const char *, int ) { Free( ptr ); }

    // Directly called
    void * operator new( size_t size ) { return AllocFileLine( size, "Unknown", 0 ); }
    void * operator new[]( size_t size ) { return AllocFileLine( size, "Unknown", 0 ); }
    void operator delete( void * ptr ) NOEXCEPT { Free( ptr ); }
    void operator delete[]( void * ptr ) NOEXCEPT { Free( ptr ); }
#else
    #if !__has_feature( address_sanitizer ) && !__has_feature( memory_sanitizer ) && !__has_feature( thread_sanitizer ) && !defined( __SANITIZE_ADDRESS__ )
        void * operator new( size_t size ) { return Alloc( size ); }
        void * operator new[]( size_t size ) { return Alloc( size ); }
        void operator delete( void * ptr ) NOEXCEPT { Free( ptr ); }
        void operator delete[]( void * ptr ) NOEXCEPT { Free( ptr ); }
    #endif
#endif
//------------------------------------------------------------------------------
