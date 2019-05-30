// MemDebug.h
//------------------------------------------------------------------------------
#pragma once

// Includes
//------------------------------------------------------------------------------
#include "Core/Env/Types.h"

// Placement new/delete
//------------------------------------------------------------------------------
// Fill new and freed memory in Debug builds unless:
//   * We are using MemorySanitizer, in which case it is better to not
//     initialize allocated memory because doing so will hide uninitialized reads
//     from MemorySanitizer.
#if !defined( __has_feature )
    #define __has_feature( ... ) 0
#endif
#if defined( DEBUG ) && !__has_feature( memory_sanitizer )
    #define MEM_DEBUG_ENABLED
#endif
#if defined( MEM_DEBUG_ENABLED )
    #define MEM_FILL_NEW_ALLOCATIONS
    #define MEM_FILL_FREED_ALLOCATIONS  // Will be applied where possible
#endif

// MemDebug
//------------------------------------------------------------------------------
#if defined( MEM_DEBUG_ENABLED )
    class MemDebug
    {
    public:
        // Patterns used are:
        // - Signalling floats
        // - Unaligned
        // - Unlikely to be valid addresses
        static const uint64_t MEM_FILL_NEW_ALLOCATION_PATTERN = 0x7F8BAAAD7F8BAAAD;
        static const uint64_t MEM_FILL_FREED_ALLOCATION_PATTERN = 0x7F8BDDDD7F8BDDDD;

        static void FillMem( void * ptr, const size_t size, const uint64_t pattern );
    };
#endif

//------------------------------------------------------------------------------
