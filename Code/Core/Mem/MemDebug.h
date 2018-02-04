// MemDebug.h
//------------------------------------------------------------------------------
#pragma once

// Includes
//------------------------------------------------------------------------------
#include "Core/Env/Types.h"

// Placement new/delete
//------------------------------------------------------------------------------
#if defined( DEBUG )
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
        static const uint32_t MEM_FILL_NEW_ALLOCATION_PATTERN = 0x7F8BAAAD;
        static const uint32_t MEM_FILL_FREED_ALLOCATION_PATTERN = 0x7F8BDDDD;

        static void FillMem( void * ptr, const size_t size, const uint32_t pattern );
    };
#endif

//------------------------------------------------------------------------------
