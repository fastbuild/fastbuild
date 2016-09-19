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
    ~MemPoolBlock();

    void *  Alloc( size_t size );
    void    Free( void * ptr );

private:
    void AllocPage();

    struct FreeBlock
    {
        FreeBlock * m_Next;
    };

    // in-place linked list of free blocks
    FreeBlock * m_FreeBlockChain;

    // total number of active allocations
    #ifdef DEBUG
        uint32_t m_NumAllocations;
    #endif

    // internal control params
    size_t      m_BlockSize;
    size_t      m_BlockAlignment;

    // allocated pages
    enum { PAGE_SIZE = 64 * 1024 };
    Array< void * > m_Pages;
};

//------------------------------------------------------------------------------
