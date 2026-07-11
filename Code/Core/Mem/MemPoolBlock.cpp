// MemPoolBlock - Block-based memory allocator
//------------------------------------------------------------------------------

// Includes
//------------------------------------------------------------------------------
#include "MemPoolBlock.h"
#include "Core/Mem/Mem.h"

// CONSTRUCTOR
//------------------------------------------------------------------------------
MemPoolBlock::MemPoolBlock( size_t blockSize, size_t blockAlignment )
    : m_BlockSize( (uint32_t)blockSize )
    , m_BlockAlignment( (uint32_t)blockAlignment )
{
    ASSERT( blockSize >= sizeof( FreeBlock ) );
    ASSERT( blockSize <= kMemPoolBlockPageSize );
    ASSERT( blockAlignment >= 4 );
    ASSERT( blockAlignment <= kMemPoolBlockPageSize );
}

// DESTRUCTOR
//------------------------------------------------------------------------------
MemPoolBlock::~MemPoolBlock()
{
    // Ensure no memory leaks
    ASSERT( m_NumActiveAllocations == 0 );

    // free pages
    for ( void * page : m_Pages )
    {
        FREE( page );
    }
}

// Alloc
//------------------------------------------------------------------------------
void * MemPoolBlock::Alloc()
{
    if ( m_FreeBlockChain == nullptr )
    {
        if ( AllocPage() == false )
        {
            return nullptr;
        }
        ASSERT( m_FreeBlockChain );
    }

#if defined( ASSERTS_ENABLED )
    m_NumActiveAllocations++;
    if ( m_NumLifetimeAllocations < 0xFFFFFFFF )
    {
        ++m_NumLifetimeAllocations;
    }
    if ( m_NumActiveAllocations > m_PeakActiveAllocations )
    {
        m_PeakActiveAllocations = m_NumActiveAllocations;
    }
#endif

    // Take first block from free chain
    FreeBlock * block = m_FreeBlockChain;
    m_FreeBlockChain = block->m_Next;
    return block;
}

// Free
//------------------------------------------------------------------------------
void MemPoolBlock::Free( void * ptr )
{
    ASSERT( m_NumActiveAllocations > 0 );

    // Insert free block into head of chain
    FreeBlock * freeBlock = static_cast<FreeBlock *>( ptr );
    freeBlock->m_Next = m_FreeBlockChain;
    m_FreeBlockChain = freeBlock;

#if defined( ASSERTS_ENABLED )
    --m_NumActiveAllocations;
#endif
}

// AllocPage
//------------------------------------------------------------------------------
NO_INLINE bool MemPoolBlock::AllocPage()
{
    // allocate page from system
    void * newPage = AllocateMemoryForPage();
    if ( newPage == nullptr )
    {
        return false;
    }

    // sanity check page alignment can support block alignment
    ASSERT( ( (size_t)newPage % m_BlockAlignment ) == 0 );

    // divide page into blocks
    const size_t alignedSize( Math::RoundUp( (size_t)m_BlockSize, (size_t)m_BlockAlignment ) );
    const size_t numBlocksInPage( kMemPoolBlockPageSize / alignedSize );

    // build chain into new blocks
    FreeBlock * block = reinterpret_cast<FreeBlock *>( (size_t)newPage );
    FreeBlock * const firstBlock = block;
    for ( size_t i = 0; i < ( numBlocksInPage - 1 ); ++i )
    {
        FreeBlock * next = reinterpret_cast<FreeBlock *>( (size_t)block + alignedSize );
        block->m_Next = next;
        block = next;
    }

    // hook last block into main chain
    block->m_Next = m_FreeBlockChain;
    m_FreeBlockChain = firstBlock;

    return true;
}

// AllocateMemoryForPage
//------------------------------------------------------------------------------
/*virtual*/ void * MemPoolBlock::AllocateMemoryForPage()
{
    void * newPage = ALLOC( kMemPoolBlockPageSize );

    // track new page
    m_Pages.Append( newPage );

    return newPage;
}

//------------------------------------------------------------------------------
