// TestSmallBlockAllocator.cpp
//------------------------------------------------------------------------------

// Includes
//------------------------------------------------------------------------------
#include "TestFramework/TestGroup.h"

#include "Core/Containers/Array.h"
#include "Core/Math/Random.h"
#include "Core/Mem/Mem.h"
#include "Core/Mem/SmallBlockAllocator.h"
#include "Core/Process/Thread.h"
#include "Core/Time/Timer.h"
#include "Core/Tracing/Tracing.h"

// System
#include <stdlib.h>

// Handle GCC -ffreestanding environment
#if defined( __STDC_HOSTED__ ) && ( __STDC_HOSTED__ == 0 )
    extern "C"
    {
        void * malloc( size_t size );
        void free( void * ptr );
    }
#endif

// TestSmallBlockAllocator
//------------------------------------------------------------------------------
class TestSmallBlockAllocator : public TestGroup
{
private:
    DECLARE_TESTS

    void SingleThreaded() const;
    void MultiThreaded() const;

    // struct for managing threads
    class ThreadInfo
    {
    public:
        Thread m_Thread;
        Array<uint32_t> * m_AllocationSizes = nullptr;
        uint32_t m_RepeatCount = 0;
        float m_TimeTaken = 0.0f;
    };

    // Helper functions
    static void     GetRandomAllocSizes( const uint32_t numAllocs, Array<uint32_t> & allocSizes );
    static float    AllocateFromSystemAllocator( const Array<uint32_t> & allocSizes, const uint32_t repeatCount );
    static float    AllocateFromSmallBlockAllocator( const Array<uint32_t> & allocSizes, const uint32_t repeatCount );
    static uint32_t ThreadFunction_System( void * userData );
    static uint32_t ThreadFunction_SmallBlock( void * userData );
};

// Register Tests
//------------------------------------------------------------------------------
REGISTER_TESTS_BEGIN( TestSmallBlockAllocator )
    REGISTER_TEST( SingleThreaded )
    REGISTER_TEST( MultiThreaded )
REGISTER_TESTS_END

// SingleThreaded
//------------------------------------------------------------------------------
void TestSmallBlockAllocator::SingleThreaded() const
{
    #if defined( DEBUG )
        const uint32_t numAllocs( 10 * 1000 );
    #else
        const uint32_t numAllocs( 100 * 1000 );
    #endif
    const uint32_t repeatCount( 10 );

    Array<uint32_t> allocSizes;
    GetRandomAllocSizes( numAllocs, allocSizes );

    const float time1 = AllocateFromSystemAllocator( allocSizes, repeatCount );
    const float time2 = AllocateFromSmallBlockAllocator( allocSizes, repeatCount );

    // output
    OUTPUT( "System (malloc)     : %2.3fs - %u allocs @ %u allocs/sec\n", (double)time1, ( numAllocs * repeatCount ), (uint32_t)( float( numAllocs * repeatCount ) / time1 ) );
    OUTPUT( "SmallBlockAllocator : %2.3fs - %u allocs @ %u allocs/sec\n", (double)time2, ( numAllocs * repeatCount ), (uint32_t)( float( numAllocs * repeatCount ) / time2 ) );
}

// MultiThreaded
//------------------------------------------------------------------------------
void TestSmallBlockAllocator::MultiThreaded() const
{
    #if defined( DEBUG )
        const uint32_t numAllocs( 10 * 1000 );
    #else
        const uint32_t numAllocs( 100 * 1000 );
    #endif
    const uint32_t repeatCount( 10 );

    Array<uint32_t> allocSizes;
    GetRandomAllocSizes( numAllocs, allocSizes );

    float time1( 0.0f );
    float time2( 0.0f );
    const size_t numThreads = 4;

    // System Allocator
    {
        // Create some threads
        ThreadInfo info[ numThreads ];
        for ( size_t i = 0; i < numThreads; ++i )
        {
            info[ i ].m_AllocationSizes = &allocSizes;
            info[ i ].m_RepeatCount = repeatCount;
            info[ i ].m_Thread.Start( ThreadFunction_System, "SmallBlock", (void *)&info[ i ] );
        }

        // Join the threads
        for ( size_t i = 0; i < numThreads; ++i )
        {
            info[ i ].m_Thread.Join();
            time1 += info[ i ].m_TimeTaken;
        }
    }

    // Small Block Allocator
    {
        // Create some threads
        ThreadInfo info[ numThreads ];
        for ( size_t i = 0; i < numThreads; ++i )
        {
            info[ i ].m_AllocationSizes = &allocSizes;
            info[ i ].m_RepeatCount = repeatCount;
            info[ i ].m_Thread.Start( ThreadFunction_SmallBlock, "SmallBlock", (void *)&info[ i ] );
        }

        // Join the threads
        for ( size_t i = 0; i < numThreads; ++i )
        {
            info[ i ].m_Thread.Join();
            time2 += info[ i ].m_TimeTaken;
        }
        time2 /= numThreads;
    }

    // output
    OUTPUT( "System (malloc)        : %2.3fs - %u allocs @ %u allocs/sec\n", (double)time1, ( numAllocs * repeatCount ), (uint32_t)( float( numAllocs * repeatCount ) / time1 ) );
    OUTPUT( "SmallBlockAllocator    : %2.3fs - %u allocs @ %u allocs/sec\n", (double)time2, ( numAllocs * repeatCount ), (uint32_t)( float( numAllocs * repeatCount ) / time2 ) );
}

// GetRandomAllocSizes
//------------------------------------------------------------------------------
/*static*/ void TestSmallBlockAllocator::GetRandomAllocSizes( const uint32_t numAllocs, Array<uint32_t> & allocSizes )
{
    const size_t maxSize( 256 ); // max supported size of block allocator

    allocSizes.SetCapacity( numAllocs );

    Random r;
    r.SetSeed( 0 ); // Deterministic between runs by using a consistent seed

    for ( size_t i = 0; i < numAllocs; ++i )
    {
        allocSizes.Append( r.GetRandIndex( maxSize ) );
    }
}

// AllocateFromSystemAllocator
//------------------------------------------------------------------------------
/*static*/ float TestSmallBlockAllocator::AllocateFromSystemAllocator( const Array<uint32_t> & allocSizes, const uint32_t repeatCount )
{
    const size_t numAllocs = allocSizes.GetSize();

    Array<void *> allocs;
    allocs.SetCapacity( numAllocs );
    const Timer timer;

    for ( size_t r = 0; r < repeatCount; ++r )
    {
        // use malloc
        for ( uint32_t i = 0; i < numAllocs; ++i )
        {
            PRAGMA_DISABLE_PUSH_MSVC( 26408 ) // Memory subsystem is allowed to call malloc
            uint32_t * mem = (uint32_t *)malloc( allocSizes[ i ] );
            PRAGMA_DISABLE_POP_MSVC
            allocs.Append( mem );
        }

        // use free
        for ( uint32_t i = 0; i < numAllocs; ++i )
        {
            void * mem = allocs[ i ];
            PRAGMA_DISABLE_PUSH_MSVC( 26408 ) // Memory subsystem is allowed to call free
            free( mem );
            PRAGMA_DISABLE_POP_MSVC
        }

        allocs.Clear();
    }

    return timer.GetElapsed();
}

// AllocateFromSmallBlockAllocator
//------------------------------------------------------------------------------
/*static*/ float TestSmallBlockAllocator::AllocateFromSmallBlockAllocator( const Array<uint32_t> & allocSizes, const uint32_t repeatCount )
{
    const size_t numAllocs = allocSizes.GetSize();

    Array<void *> allocs;
    allocs.SetCapacity( numAllocs );
    const Timer timer;

    for ( size_t r = 0; r < repeatCount; ++r )
    {
        // Use ALLOC
        for ( uint32_t i = 0; i < numAllocs; ++i )
        {
            uint32_t * mem = (uint32_t *)ALLOC( allocSizes[ i ] );
            allocs.Append( mem );
        }

        // Use FREE
        for ( uint32_t i = 0; i < numAllocs; ++i )
        {
            void * mem = allocs[ i ];
            FREE( mem );
        }

        allocs.Clear();
    }

    return timer.GetElapsed();
}

// ThreadFunction_System
//------------------------------------------------------------------------------
/*static*/ uint32_t TestSmallBlockAllocator::ThreadFunction_System( void * userData )
{
    ThreadInfo & info = *( static_cast<ThreadInfo *>( userData ) );
    info.m_TimeTaken = AllocateFromSystemAllocator( *info.m_AllocationSizes, info.m_RepeatCount );
    return 0;
}

// ThreadFunction_SmallBlock
//------------------------------------------------------------------------------
/*static*/ uint32_t TestSmallBlockAllocator::ThreadFunction_SmallBlock( void * userData )
{
    ThreadInfo & info = *( static_cast<ThreadInfo *>( userData ) );
    info.m_TimeTaken = AllocateFromSmallBlockAllocator( *info.m_AllocationSizes, info.m_RepeatCount );
    return 0;
}

//------------------------------------------------------------------------------
