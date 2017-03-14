// SmallBlockAllocator.h
//------------------------------------------------------------------------------
#pragma once

// Defines
//------------------------------------------------------------------------------
#define SMALL_BLOCK_ALLOCATOR_ENABLED

// Includes
//------------------------------------------------------------------------------
#include "Core/Mem/MemPoolBlock.h"
#include "Core/Process/Mutex.h"

// SmallBlockAllocator
//------------------------------------------------------------------------------
#if defined( SMALL_BLOCK_ALLOCATOR_ENABLED )
    class SmallBlockAllocator
    {
    public:
        // Attempt allocation. Returns nullptr if allocation can't be satisfied
        static void *   Alloc( size_t size, size_t align );

        // Attempt to free. Returns false if not a bucket owned allocation
        static bool     Free( void * ptr );

        // Hint when operating only on a single thread as we can greatly reduce allocation cost
        static void     SetSingleThreadedMode( bool singleThreadedMode );

        #if defined( DEBUG )
            static void DumpStats();
        #endif

    protected:
        static void InitBuckets();

        static const size_t BUCKET_MAX_ALLOC_SIZE = 256;
        static const size_t BUCKET_ALIGNMENT = sizeof( void * );
        static const size_t BUCKET_NUM_BUCKETS = ( BUCKET_MAX_ALLOC_SIZE / BUCKET_ALIGNMENT );
        static const size_t BUCKET_ADDRESSSPACE_SIZE = ( 200 * 1024 * 1024 );
        static const size_t BUCKET_NUM_PAGES = ( BUCKET_ADDRESSSPACE_SIZE / MemPoolBlock::MEMPOOLBLOCK_PAGE_SIZE );
        static const size_t BUCKET_MAPPING_TABLE_SIZE  = BUCKET_NUM_PAGES;

        class MemBucket : public MemPoolBlock
        {
        public:
            MemBucket( size_t size, size_t align ) : MemPoolBlock( size, align ) {}

        protected:
            virtual void *  AllocateMemoryForPage() override;

            friend class SmallBlockAllocator;
            Mutex           m_Mutex;
        };

        friend class MemBucket;

        // Single Threaded Mode
        static bool         s_ThreadSafeAllocs;
        #if defined( DEBUG )
            // When in single-threaded mode, catch unsafe use
            static uint64_t s_ThreadSafeAllocsDebugOwnerThread;
        #endif

        // Address space used by allocators
        static void *       s_BucketMemoryStart;
        static uint32_t     s_BucketNextFreePageIndex; // Next free memory page to commit

        // The actual buckets (using this placeholder memory to avoid static init issues)
        static uint64_t     s_BucketMemBucketMemory[ BUCKET_NUM_BUCKETS * sizeof( MemBucket ) / sizeof (uint64_t) ];
        static MemBucket *  s_Buckets;

        // A table to allow 0(1) conversion of any address to the bucket that owns it
        static uint8_t      s_BucketMappingTable[ BUCKET_MAPPING_TABLE_SIZE ];
    };
#endif

//------------------------------------------------------------------------------
