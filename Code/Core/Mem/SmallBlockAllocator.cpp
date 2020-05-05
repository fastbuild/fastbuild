// SmallBlockAllocator.cpp
//------------------------------------------------------------------------------

// Includes
//------------------------------------------------------------------------------
#include "SmallBlockAllocator.h"

// Core
#include "Core/Env/Types.h"
#include "Core/Math/Conversions.h"
#include "Core/Mem/MemDebug.h"
#include "Core/Mem/MemPoolBlock.h"
#include "Core/Process/Atomic.h"
#include "Core/Process/Mutex.h"
#if defined( DEBUG )
    #include "Core/Process/Thread.h"
#endif
#include "Core/Strings/AStackString.h"
#include "Core/Tracing/Tracing.h"

#if !defined( __WINDOWS__ )
    #include <sys/mman.h>
#endif

// Defines
//------------------------------------------------------------------------------
// An address with the MSB set is not a valid user-space address on Windows, Linux or OSX
// We can take advantage of this to test if an allocation being freed is a bucket
// allocation whether or not the SmallBlockAlloctor is initialized or not
#define MEM_BUCKETS_NOT_INITIALIZED (void *)( (uint64_t)1 << 63 )

// Static Data
//------------------------------------------------------------------------------
/*static*/ bool                                 SmallBlockAllocator::s_ThreadSafeAllocs( true );
#if defined( DEBUG )
    /*static*/ uint64_t                         SmallBlockAllocator::s_ThreadSafeAllocsDebugOwnerThread( 0 );
#endif
/*static*/ void *                               SmallBlockAllocator::s_BucketMemoryStart( MEM_BUCKETS_NOT_INITIALIZED );
/*static*/ uint32_t                             SmallBlockAllocator::s_BucketNextFreePageIndex( 0 );
/*static*/ uint64_t                             SmallBlockAllocator::s_BucketMemBucketMemory[ BUCKET_NUM_BUCKETS * sizeof( MemBucket ) / sizeof (uint64_t) ];
/*static*/ SmallBlockAllocator::MemBucket *     SmallBlockAllocator::s_Buckets( nullptr );
/*static*/ uint8_t                              SmallBlockAllocator::s_BucketMappingTable[ BUCKET_MAPPING_TABLE_SIZE ] = { 0 };

// InitBuckets
//------------------------------------------------------------------------------
NO_INLINE void SmallBlockAllocator::InitBuckets()
{
    ASSERT( s_BucketMemoryStart == MEM_BUCKETS_NOT_INITIALIZED );

    // Reserve the address space for the buckets to manage
    #if defined( __WINDOWS__ )
        s_BucketMemoryStart = ::VirtualAlloc( nullptr, BUCKET_ADDRESSSPACE_SIZE, MEM_RESERVE, PAGE_NOACCESS );
        ASSERT( s_BucketMemoryStart );
    #else
        s_BucketMemoryStart = ::mmap( nullptr, BUCKET_ADDRESSSPACE_SIZE, PROT_NONE, MAP_PRIVATE | MAP_ANON, -1, 0 );
        ASSERT( (size_t)s_BucketMemoryStart != (size_t)-1 );
    #endif

    // Construct the bucket structures in the reservedspace
    // (Done this way to avoid memory allocations which would be re-entrant)
    s_Buckets = reinterpret_cast< MemBucket * >( s_BucketMemBucketMemory );
    for ( size_t i = 0; i < BUCKET_NUM_BUCKETS; ++i )
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
        for ( uint32_t i = 0; i < BUCKET_NUM_BUCKETS; ++i )
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
    if ( s_BucketMemoryStart == MEM_BUCKETS_NOT_INITIALIZED )
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

    // Sanity check that we're being used safely
    #if defined( DEBUG )
        ASSERT( s_ThreadSafeAllocs || ( s_ThreadSafeAllocsDebugOwnerThread == (uint64_t)Thread::GetCurrentThreadId() ) );
    #endif

    // Alloc
    if ( s_ThreadSafeAllocs )
    {
        bucket.m_Mutex.Lock();
    }

    void * ptr = bucket.Alloc();

    if ( s_ThreadSafeAllocs )
    {
        bucket.m_Mutex.Unlock();
    }

    // Debug fill
    #if defined( MEM_FILL_NEW_ALLOCATIONS )
        if ( ptr )
        {
            MemDebug::FillMem( ptr, bucket.m_BlockSize, MemDebug::MEM_FILL_NEW_ALLOCATION_PATTERN );
        }
    #endif

    return ptr;
}

// Free
//------------------------------------------------------------------------------
bool SmallBlockAllocator::Free( void * ptr )
{
    // Determine if this allocation belongs to the buckets
    // Even if the buckets have nevere been initialized, this is safe
    // as it will result in a page index out of bounds since for any valid
    // pointer as no allocation can validly be outside of the user mode address space.
    const size_t pageIndex = (size_t)( ( (char *)ptr - (char *)s_BucketMemoryStart ) / MemPoolBlock::MEMPOOLBLOCK_PAGE_SIZE );
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

    // Sanity check that we're being used safely
    #if defined( DEBUG )
        ASSERT( s_ThreadSafeAllocs || ( s_ThreadSafeAllocsDebugOwnerThread == (uint64_t)Thread::GetCurrentThreadId() ) );
    #endif

    // Free it
    if ( s_ThreadSafeAllocs )
    {
        bucket.m_Mutex.Lock();
    }

    bucket.Free( ptr );

    if ( s_ThreadSafeAllocs )
    {
        bucket.m_Mutex.Unlock();
    }

    return true;
}

// SetSingleThreadedMode
//------------------------------------------------------------------------------
/*static*/ void SmallBlockAllocator::SetSingleThreadedMode( bool singleThreadedMode )
{
    if ( singleThreadedMode )
    {
        // Sanity check we're not already in single threaded mode
        ASSERT( s_ThreadSafeAllocs == true );
        ASSERT( s_ThreadSafeAllocsDebugOwnerThread == 0 );

        // Store the new owner thread for further safety checks
        #if defined( DEBUG )
            s_ThreadSafeAllocsDebugOwnerThread = (uint64_t)Thread::GetCurrentThreadId();
        #endif
    }
    else
    {
        // Sanity check we're in single threaded mode
        ASSERT( s_ThreadSafeAllocs == false );
        ASSERT( s_ThreadSafeAllocsDebugOwnerThread == (uint64_t)Thread::GetCurrentThreadId() );

        // Store the new owner thread for further safety checks
        #if defined( DEBUG )
            s_ThreadSafeAllocsDebugOwnerThread = 0;
        #endif
    }

    s_ThreadSafeAllocs = ( !singleThreadedMode );
}

// AllocateMemoryForPage
//------------------------------------------------------------------------------
/*virtual*/ void * SmallBlockAllocator::MemBucket::AllocateMemoryForPage()
{
    // Have we exhausted our page space?
    if ( AtomicLoadRelaxed( &SmallBlockAllocator::s_BucketNextFreePageIndex ) >= BUCKET_NUM_PAGES )
    {
        return nullptr;
    }

    // Grab the next page
    const uint32_t pageIndex = AtomicIncU32( &SmallBlockAllocator::s_BucketNextFreePageIndex ) - 1;

    // Handle edge case where two or more threads try to allocate the last page simultaneously
    if ( pageIndex >= BUCKET_NUM_PAGES )
    {
        return nullptr;
    }

    // Commit the page
    void * newPage = (void *)( ( (size_t)SmallBlockAllocator::s_BucketMemoryStart ) + ( (size_t)pageIndex * MemPoolBlock::MEMPOOLBLOCK_PAGE_SIZE ) );
    #if defined( __WINDOWS__ )
        VERIFY( ::VirtualAlloc( newPage, MemPoolBlock::MEMPOOLBLOCK_PAGE_SIZE, MEM_COMMIT, PAGE_READWRITE ) );
    #else
        VERIFY( ::mprotect( newPage, MemPoolBlock::MEMPOOLBLOCK_PAGE_SIZE, PROT_READ | PROT_WRITE ) == 0 );
    #endif

    // Update page to bucket mapping table
    ASSERT( s_BucketMappingTable[ pageIndex ] ==  0 );
    const size_t bucketIndex = (size_t)( this - SmallBlockAllocator::s_Buckets );
    ASSERT( bucketIndex <= 255 );
    s_BucketMappingTable[ pageIndex ] = (uint8_t)bucketIndex;

    return newPage;
}

//------------------------------------------------------------------------------
