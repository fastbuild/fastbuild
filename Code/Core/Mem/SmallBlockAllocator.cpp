// SmallBlockAllocator.cpp
//------------------------------------------------------------------------------

// Includes
//------------------------------------------------------------------------------
#include "Core/PrecompiledHeader.h"

#include "SmallBlockAllocator.h"

#if defined( SMALL_BLOCK_ALLOCATOR_ENABLED )

// Core
#include "Core/Env/Types.h"
#include "Core/Math/Conversions.h"
#include "Core/Mem/MemDebug.h"
#include "Core/Mem/MemPoolBlock.h"
#include "Core/Process/Mutex.h"
#include "Core/Strings/AStackString.h"
#include "Core/Tracing/Tracing.h"

#if !defined( __WINDOWS__ )
    #include <sys/mman.h>
#endif

// Static Data
//------------------------------------------------------------------------------
/*static*/ void *                               SmallBlockAllocator::s_BucketMemoryStart( nullptr );
/*static*/ void *                               SmallBlockAllocator::s_BucketMemoryEnd( nullptr );
/*static*/ void *                               SmallBlockAllocator::s_BucketNextFreePage( nullptr );
/*static*/ uint64_t                             SmallBlockAllocator::s_BucketMemBucketMemory[ BUCKET_NUM_BUCKETS * sizeof( MemBucket ) / sizeof (uint64_t) ];
/*static*/ SmallBlockAllocator::MemBucket *     SmallBlockAllocator::s_Buckets( nullptr );
/*static*/ uint8_t                              SmallBlockAllocator::s_BucketMappingTable[ BUCKET_MAPPING_TABLE_SIZE ] = { 0 };

// InitBuckets
//------------------------------------------------------------------------------
NO_INLINE void SmallBlockAllocator::InitBuckets()
{
    // Reserve the address space for the buckets to manage
    #if defined( __WINDOWS__ )
        s_BucketMemoryStart = ::VirtualAlloc( nullptr, BUCKET_ADDRESSSPACE_SIZE, MEM_RESERVE, PAGE_NOACCESS );
        ASSERT( s_BucketMemoryStart );
    #else
        s_BucketMemoryStart = ::mmap( nullptr, BUCKET_ADDRESSSPACE_SIZE, PROT_NONE, MAP_PRIVATE | MAP_ANON, -1, 0 );
        ASSERT( (ssize_t)s_BucketMemoryStart != (ssize_t)-1 );
    #endif
    s_BucketMemoryEnd = (char *)s_BucketMemoryStart + BUCKET_ADDRESSSPACE_SIZE;
    s_BucketNextFreePage = s_BucketMemoryStart;

    // Construct the bucket structures in the reservedspace
    // (Done this way to avoid memory allocations which would be re-entrant)
    s_Buckets = reinterpret_cast< MemBucket * >( s_BucketMemBucketMemory );
    for ( size_t i=0; i<BUCKET_NUM_BUCKETS; ++i )
    {
        const size_t size = BUCKET_ALIGNMENT * ( i + 1 );
        MemBucket * bucket = &s_Buckets[ i ];
        new (bucket) MemBucket( size, BUCKET_ALIGNMENT );
    }
}

// DumpStats
//------------------------------------------------------------------------------
#if defined( DEBUG )
    /*static*/ void SmallBlockAllocator::DumpStats()
    {
        // Keep a stack buffer to avoid allocations during formatting
        AStackString<5120> buffer;
        static_assert( BUCKET_NUM_BUCKETS * 70 < 5120, "Not enough space for DumpStats" );

        // Assumulate total stats as we loop
        uint32_t totalActive = 0;
        uint32_t totalActiveBytes = 0;
        uint32_t totalPeak = 0;
        uint32_t totalPeakBytes = 0;
        uint32_t totalLifetime = 0;
        buffer += "         | Active            | Peak              |\n";
        buffer += "    Size |      Num      Mem |      Num      Mem |   LifeTime\n";
        buffer += "-------------------------------------------------------------\n";

        // Print info for eeach bucket
        for ( uint32_t i=0; i<BUCKET_NUM_BUCKETS; ++i )
        {
            MemBucket & bucket = s_Buckets[ i ];
            const uint32_t numLive = bucket.m_NumActiveAllocations; //GetNumActiveAllocations();
            const uint32_t blockSize = bucket.m_BlockSize; //GetBlockSize();
            const uint32_t numPeak = bucket.m_PeakActiveAllocations;
            const uint32_t numLifetime = bucket.m_NumLifetimeAllocations;
            totalActive += numLive;
            totalActiveBytes += (numLive * blockSize);
            totalPeak += numPeak;
            totalPeakBytes += (numPeak * blockSize);
            totalLifetime += numLifetime;
            buffer.AppendFormat( "%8u | %8u %8u | %8u %8u | %10u\n", blockSize, numLive, (numLive * blockSize), numPeak, (numPeak * blockSize), numLifetime );

        }

        // Summary/Totals
        buffer += "-------------------------------------------------------------\n";
        buffer.AppendFormat( "         | %8u %8u | %8u %8u | %10u\n", totalActive, totalActiveBytes, totalPeak, totalPeakBytes, totalLifetime );

        OUTPUT( "%s", buffer.Get() );
    }
#endif

// Alloc
//------------------------------------------------------------------------------
void * SmallBlockAllocator::Alloc( size_t size, size_t align )
{
    // Check we can support allocation of this size
    if ( size > BUCKET_MAX_ALLOC_SIZE )
    {
        return nullptr; // Too big
    }

    // Lazy initialization of buckets to support static allocations
    if ( s_BucketMemoryStart == nullptr )
    {
        InitBuckets();
    }

    // Handle 0 byte allocations
    size = Math::Max( size, BUCKET_ALIGNMENT );

    // Find the owning bucket
    const size_t alignedSize = Math::RoundUp<size_t>( size, BUCKET_ALIGNMENT );
    const size_t bucketIndex = ( ( alignedSize / BUCKET_ALIGNMENT ) - 1 );
    MemBucket & bucket  = s_Buckets[ bucketIndex ];
    ASSERT( bucket.m_BlockSize >= size );

    // Can bucket satisfy alignment requirement?
    if ( bucket.m_BlockAlignment < align )
    {
        return nullptr; // Can't satify alignment
    }

    // Alloc
    bucket.m_Mutex.Lock();

    void * ptr = bucket.Alloc( size );

    bucket.m_Mutex.Unlock();

    // Debug fill
    #if defined( MEM_FILL_NEW_ALLOCATIONS )
        MemDebug::FillMem( ptr, bucket.m_BlockSize, MemDebug::MEM_FILL_NEW_ALLOCATION_PATTERN );
    #endif

    return ptr;
}

// Free
//------------------------------------------------------------------------------
bool SmallBlockAllocator::Free( void * ptr )
{
    // Determine if this allocation belongs to the buckets
    const size_t pageIndex = ( ( (char *)ptr - (char *)s_BucketMemoryStart) / MemPoolBlock::PAGE_SIZE );
    if ( pageIndex >= BUCKET_MAPPING_TABLE_SIZE )
    {
        return false; // Not a bucket allocation
    }

    // Find the bucket using the page mapping table
    const size_t bucketIndex = s_BucketMappingTable[ pageIndex ];
    MemBucket & bucket = s_Buckets[ bucketIndex ];

    // Debug fill
    #if defined( MEM_FILL_FREED_ALLOCATIONS )
        MemDebug::FillMem( ptr, bucket.m_BlockSize, MemDebug::MEM_FILL_FREED_ALLOCATION_PATTERN );
    #endif

    // Free it
    bucket.m_Mutex.Lock();

    bucket.Free( ptr );

    bucket.m_Mutex.Unlock();

    return true;
}

// AllocateMemoryForPage
//------------------------------------------------------------------------------
/*virtual*/ void * SmallBlockAllocator::MemBucket::AllocateMemoryForPage()
{
    // Used up all pages?
    if ( SmallBlockAllocator::s_BucketNextFreePage >= SmallBlockAllocator::s_BucketMemoryEnd )
    {
        return nullptr; // No more free pages
    }

    // Consume the next free page
    void * newPage = SmallBlockAllocator::s_BucketNextFreePage;
    SmallBlockAllocator::s_BucketNextFreePage = ( (char *)SmallBlockAllocator::s_BucketNextFreePage + MemPoolBlock::PAGE_SIZE );

    // Commit the page
    #if defined( __WINDOWS__ )
        ::VirtualAlloc( newPage, MemPoolBlock::PAGE_SIZE, MEM_COMMIT, PAGE_READWRITE );
    #else
        VERIFY( ::mprotect( newPage, MemPoolBlock::PAGE_SIZE, PROT_READ | PROT_WRITE ) == 0 );
    #endif

    // Update page to bucket mapping table
    const size_t pageIndex = ( ( (char *)newPage - (char *)SmallBlockAllocator::s_BucketMemoryStart ) / MemPoolBlock::PAGE_SIZE );
    ASSERT( s_BucketMappingTable[ pageIndex ] ==  0 );
    const size_t bucketIndex = ( this - SmallBlockAllocator::s_Buckets );
    ASSERT( bucketIndex <= 255 );
    s_BucketMappingTable[ pageIndex ] = (uint8_t)bucketIndex;

    return newPage;
}

//------------------------------------------------------------------------------
#endif // SMALL_BLOCK_ALLOCATOR_ENABLED
