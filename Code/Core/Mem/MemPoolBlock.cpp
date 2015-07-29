// MemPoolBlock - Block-based memory allocator
//------------------------------------------------------------------------------

// Includes
//------------------------------------------------------------------------------
#include "Core/PrecompiledHeader.h"

#include "MemPoolBlock.h"
#include "Core/Mem/Mem.h"

// CONSTRUCTOR
//------------------------------------------------------------------------------
MemPoolBlock::MemPoolBlock( size_t blockSize, size_t blockAlignment )
	: m_FreeBlockChain( nullptr )
	#ifdef DEBUG
		, m_NumAllocations( 0 )
	#endif
	, m_BlockSize( blockSize )
	, m_BlockAlignment( blockAlignment )
	, m_Pages( 8, true )
{
	ASSERT( blockSize >= sizeof( FreeBlock ) );
	ASSERT( blockSize <= PAGE_SIZE );
	ASSERT( blockAlignment >= 4 );
	ASSERT( blockAlignment <= PAGE_SIZE );
}

// DESTRUCTOR 
//------------------------------------------------------------------------------
MemPoolBlock::~MemPoolBlock()
{
	// Ensure no memory leaks
	#ifdef DEBUG
		ASSERT( m_NumAllocations == 0 );
	#endif

	// free pages
	void ** end = m_Pages.End();
	for ( void ** it=m_Pages.Begin(); it != end; ++it )
	{
		void * page = *it;
		FREE( page );
	}
}

// Alloc
//------------------------------------------------------------------------------
void * MemPoolBlock::Alloc( size_t size )
{
	// Only permitted to call with supported sizes
	ASSERT( size <= m_BlockSize ); (void)size;

	if ( m_FreeBlockChain == nullptr )
	{
		AllocPage();
		ASSERT( m_FreeBlockChain );
	}

	#ifdef DEBUG
		m_NumAllocations++;
	#endif

	// Take first block from free chain
	FreeBlock * block = m_FreeBlockChain;
	m_FreeBlockChain = block->m_Next;
	return reinterpret_cast< void * >( block );
}

// Free
//------------------------------------------------------------------------------
void MemPoolBlock::Free( void * ptr )
{
	#ifdef DEBUG
		ASSERT( m_NumAllocations > 0 );
	#endif

	// Insert free block into head of chain
	FreeBlock * freeBlock = reinterpret_cast< FreeBlock * >( ptr );
	freeBlock->m_Next = m_FreeBlockChain;
	m_FreeBlockChain = freeBlock;

	#ifdef DEBUG
		--m_NumAllocations;
	#endif
}

// AlocPage
//------------------------------------------------------------------------------
void MemPoolBlock::AllocPage()
{
	// allocate page from system
	const size_t pageSize( PAGE_SIZE );
	void * newPage = ALLOC( pageSize );

	// sanity check page alignment can support block alignment
	ASSERT( ( (size_t)newPage % m_BlockAlignment ) == 0 );

	// divide page into blocks
	#if defined( __APPLE__ ) || defined( __LINUX__ ) // TODO: Fix this weirdness
		const size_t alignedSize( Math::RoundUp( (size_t)m_BlockSize, (size_t)m_BlockAlignment ) );
	#else
		const size_t alignedSize( Math::RoundUp( m_BlockSize, m_BlockAlignment ) );
	#endif
	const size_t numBlocksInPage( pageSize / alignedSize );

	// build chain into new blocks
	FreeBlock * const firstBlock = reinterpret_cast< FreeBlock * >( (size_t)newPage );
	FreeBlock * block = reinterpret_cast< FreeBlock * >( (size_t)newPage );
	for ( size_t i=0; i<( numBlocksInPage - 1 ); ++i )	
	{
		FreeBlock * next = reinterpret_cast< FreeBlock * >( (size_t)block + alignedSize );
		block->m_Next = next;
		block = next;
	}

	// hook last block into master chain
	block->m_Next = m_FreeBlockChain;
	m_FreeBlockChain = firstBlock;

	// track new page
	m_Pages.Append( newPage );
}

//------------------------------------------------------------------------------
