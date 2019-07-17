// Mem.h
//------------------------------------------------------------------------------
#pragma once

// Includes
//------------------------------------------------------------------------------
#include "Core/Env/Types.h"
#include "Core/Mem/MemTracker.h"

// Placement new/delete
//------------------------------------------------------------------------------
#define INPLACE_NEW new
inline void * operator new( size_t, void * ptr ) { return ptr; }
inline void * operator new[]( size_t, void * ptr ) { return ptr; }
inline void operator delete( void *, void * ) {}
inline void operator delete[]( void *, void * ) {}

// new/delete
//------------------------------------------------------------------------------
#if defined( MEMTRACKER_ENABLED )
    #define FNEW( code )        new ( __FILE__, __LINE__ ) code
    #define FNEW_ARRAY( code )  new ( __FILE__, __LINE__ ) code
    #define FDELETE             delete
    #define FDELETE_ARRAY       delete[]

    #define ALLOC( ... )        ::AllocFileLine( __VA_ARGS__, __FILE__, __LINE__ )
    #define FREE( ptr )         ::Free( ptr )
#else
    #define FNEW( code )        new code
    #define FNEW_ARRAY( code )  new code
    #define FDELETE             delete
    #define FDELETE_ARRAY       delete[]

    #define ALLOC( ... )        ::Alloc( __VA_ARGS__ )
    #define FREE( ptr )         ::Free( ptr )
#endif

// Alloc/Free
//------------------------------------------------------------------------------
#if !defined( MEMTRACKER_ENABLED )
    // Functions are private when MemTracker is enabled
    void * Alloc( size_t size );
    void * Alloc( size_t size, size_t alignment );
#else
    void * AllocFileLine( size_t size, const char * file, int line );
    void * AllocFileLine( size_t size, size_t alignment, const char * file, int line );
#endif
void Free( void * ptr );

// global new/delete
//------------------------------------------------------------------------------
#if defined( MEMTRACKER_ENABLED )
    void * operator new( size_t size, const char * file, int line );
    void * operator new[]( size_t size, const char * file, int line );
    void operator delete( void * ptr, const char *, int );
    void operator delete[]( void * ptr, const char *, int );
#endif
#if !defined( __has_feature )
    #define __has_feature( ... ) 0
#endif
#if !__has_feature( address_sanitizer ) && !__has_feature( memory_sanitizer ) && !__has_feature( thread_sanitizer ) && !defined( __SANITIZE_ADDRESS__ )
void * operator new( size_t size );
void * operator new[]( size_t size );
void operator delete( void * ptr ) NOEXCEPT;
void operator delete[]( void * ptr ) NOEXCEPT;
#endif

//------------------------------------------------------------------------------
