// Mem.cpp
//------------------------------------------------------------------------------

// Includes
//------------------------------------------------------------------------------
#include "Core/PrecompiledHeader.h"

#include "Mem.h"
#include "Core/Env/Assert.h"
#include "Core/Env/Types.h"
#include "Core/Mem/MemTracker.h"

#include <stdlib.h>

// Defines
//------------------------------------------------------------------------------
#ifdef DEBUG
	#define MEM_FILL_NEW_ALLOCATIONS
	#define MEM_FILL_NEW_ALLOCATION_PATTERN ( 0x7F8BAAAD )
#endif

// FillMem
//------------------------------------------------------------------------------
#ifdef MEM_FILL_NEW_ALLOCATIONS
	void FillMem( void * ptr, size_t size, uint32_t pattern )
	{
		// this function assumes at least 32bit alignment
		ASSERT( uintptr_t( ptr ) % sizeof( uint32_t ) == 0 );

		// fill whole words
		const size_t numWords = size / sizeof( uint32_t );
		uint32_t * it = reinterpret_cast< uint32_t * >( ptr );
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
			char * cit = reinterpret_cast< char * >( it );
			switch( remainder )
			{
				case 3: *cit = *b; ++cit; ++b;
				case 2: *cit = *b; ++cit; ++b;
				case 1: *cit = *b;
			}
		}
	}
#endif

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
    #if defined( __LINUX__ ) || defined( __APPLE__ )
        void * mem( nullptr );
        VERIFY( posix_memalign( &mem, alignment, size ) == 0 );
	#else
		void * mem = _aligned_malloc( size, alignment );
	#endif

	#ifdef MEM_FILL_NEW_ALLOCATIONS
		FillMem( mem, size, MEM_FILL_NEW_ALLOCATION_PATTERN );
	#endif

	MEMTRACKER_ALLOC( mem, size, file, line );
	(void)file; (void)line; // TODO: strip args in release

	return mem;
}

// Free
//------------------------------------------------------------------------------
void Free( void * ptr )
{
	MEMTRACKER_FREE( ptr );

    #if defined( __LINUX__ ) || defined( __APPLE__ )
        free( ptr );
	#else
		_aligned_free( ptr );
	#endif
}

// Operators
//------------------------------------------------------------------------------
#if defined( __OSX__ )
	// TODO: resolve issue with Clang and inline new/delete
#else
	#if defined( MEMTRACKER_ENABLED )
		void * operator new( size_t size, const char * file, int line ) { return AllocFileLine( size, file, line ); }
		void * operator new[]( size_t size, const char * file, int line ) { return AllocFileLine( size, file, line ); }
		void operator delete( void * ptr, const char *, int ) { return Free( ptr ); }
		void operator delete[]( void * ptr, const char *, int ) { return Free( ptr ); }
	#endif
	void * operator new( size_t size ) { return Alloc( size ); }
	void * operator new[]( size_t size ) { return Alloc( size ); }
	void operator delete( void * ptr ) { Free( ptr ); }
	void operator delete[]( void * ptr ) { Free( ptr ); }
#endif

//------------------------------------------------------------------------------
