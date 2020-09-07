// MemPoolBlock - Block-based memory allocator
//------------------------------------------------------------------------------

// Includes
//------------------------------------------------------------------------------
#include "MemPoolBlock.h"
#include "Core/Mem/Mem.h"

// CONSTRUCTOR
//------------------------------------------------------------------------------
MemPoolBlock::MemPoolBlock( size_t blockSize, size_t blockAlignment )
    : m_FreeBlockChain( nullptr )
    #ifdef DEBUG
        , m_NumActiveAllocations( 0 )
        , m_NumLifetimeAllocations( 0 )
        , m_PeakActiveAllocations( 0 )
    #endif
    , m_BlockSize( (uint32_t)blockSize )
    , m_BlockAlignment( (uint32_t)blockAlignment )
    , m_Pages( 0, true )
{
    ASSERT( blockSize >= sizeof( FreeBlock ) );
    ASSERT( blockSize <= MEMPOOLBLOCK_PAGE_SIZE );
    ASSERT( blockAlignment >= 4 );
    ASSERT( blockAlignment <= MEMPOOLBLOCK_PAGE_SIZE );
}

// DESTRUCTOR
//------------------------------------------------------------------------------
MemPoolBlock::~MemPoolBlock()
{
    // Ensure no memory leaks
    #ifdef DEBUG
        ASSERT( m_NumActiveAllocations == 0 );
    #endif

    // free pages
    const void * const * end = m_Pages.End();
    for ( void ** it = m_Pages.Begin(); it != end; ++it )
    {
        void * page = *it;
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

    #ifdef DEBUG
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
    return static_cast< void * >( block );
}

// Free
//------------------------------------------------------------------------------
void MemPoolBlock::Free( void * ptr )
{
    #ifdef DEBUG
        ASSERT( m_NumActiveAllocations > 0 );
    #endif

    // Insert free block into head of chain
    FreeBlock * freeBlock = static_cast< FreeBlock * >( ptr );
    freeBlock->m_Next = m_FreeBlockChain;
    m_FreeBlockChain = freeBlock;

    #ifdef DEBUG
        --m_NumActiveAllocations;
    #endif
}

// AlocPage
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
    const size_t numBlocksInPage( MEMPOOLBLOCK_PAGE_SIZE / alignedSize );

    // build chain into new blocks
    FreeBlock * block = reinterpret_cast< FreeBlock * >( (size_t)newPage );
    FreeBlock * const firstBlock = block;
    for ( size_t i = 0; i < ( numBlocksInPage - 1 ); ++i )
    {
        FreeBlock * next = reinterpret_cast< FreeBlock * >( (size_t)block + alignedSize );
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
    void * newPage = ALLOC( MEMPOOLBLOCK_PAGE_SIZE );

    // track new page
    m_Pages.Append( newPage );

    return newPage;
}

//------------------------------------------------------------------------------
