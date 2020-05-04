// MemPoolBlock - Block-based memory allocator
//------------------------------------------------------------------------------
#pragma once

// Array
//------------------------------------------------------------------------------
#include "Core/Containers/Array.h"

// MemPoolBlock
//------------------------------------------------------------------------------
class MemPoolBlock
{
public:
    MemPoolBlock( size_t blockSize, size_t blockAlignment );
    virtual ~MemPoolBlock();

    void *  Alloc();
    void    Free( void * ptr );

    enum { MEMPOOLBLOCK_PAGE_SIZE = 64 * 1024 };

protected:
    bool    AllocPage();

    virtual void * AllocateMemoryForPage();

    struct FreeBlock
    {
        FreeBlock * m_Next;
    };

    // in-place linked list of free blocks
    FreeBlock * m_FreeBlockChain            = nullptr;

    // debug active allocations
    #if defined( DEBUG )
        uint32_t m_NumActiveAllocations     = 0;
        uint32_t m_NumLifetimeAllocations   = 0;
        uint32_t m_PeakActiveAllocations    = 0;
    #endif

    // internal control params
    uint32_t    m_BlockSize                 = 0;
    uint32_t    m_BlockAlignment            = 0;

    // allocated pages
    Array< void * > m_Pages;
};

//------------------------------------------------------------------------------
