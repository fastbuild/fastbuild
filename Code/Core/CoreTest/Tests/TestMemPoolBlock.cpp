// TestMemPoolBlock.cpp
//------------------------------------------------------------------------------

// Includes
//------------------------------------------------------------------------------
#include "TestFramework/UnitTest.h"

#include "Core/Mem/Mem.h"
#include "Core/Mem/MemPoolBlock.h"
#include "Core/Time/Timer.h"
#include "Core/Tracing/Tracing.h"

// System
#include <stdlib.h>

// TestMemPoolBlock
//------------------------------------------------------------------------------
class TestMemPoolBlock : public UnitTest
{
private:
    DECLARE_TESTS

    void TestUnused() const;
    void TestAllocs() const;
    void TestAllocsMultiplePages() const;
    void TestSpeed();
};

// Register Tests
//------------------------------------------------------------------------------
REGISTER_TESTS_BEGIN( TestMemPoolBlock )
    REGISTER_TEST( TestUnused )
    REGISTER_TEST( TestAllocs )
    REGISTER_TEST( TestAllocsMultiplePages )
    REGISTER_TEST( TestSpeed )
REGISTER_TESTS_END

// TestUnused
//------------------------------------------------------------------------------
void TestMemPoolBlock::TestUnused() const
{
    // Create a MemPoolBlock but don't do anything with it
    MemPoolBlock block( 32, 4 );
}

// TestAllocs
//------------------------------------------------------------------------------
void TestMemPoolBlock::TestAllocs() const
{
    const size_t blockSize( 32 );
    const size_t blockAlignment( 4 );
    MemPoolBlock block( blockSize, blockAlignment );

    // allocate every size upto the block size
    Array< void * > allocs( blockSize + 1, false );
    for ( size_t i = 0; i <= blockSize; ++i )
    {
        void * mem = block.Alloc();
        TEST_ASSERT( mem );
        TEST_ASSERT( ( (size_t)mem % blockAlignment ) == 0 );
        allocs.Append( mem );
    }

    // free them
    for ( size_t i = 0; i < allocs.GetSize(); ++i )
    {
        block.Free( allocs[ i ] );
    }
}

// TestAllocsMultiplePages
//------------------------------------------------------------------------------
void TestMemPoolBlock::TestAllocsMultiplePages() const
{
    const size_t blockSize( 32 * 1024 );
    const size_t blockAlignment( 4 );
    MemPoolBlock block( blockSize, blockAlignment );

    void * a = block.Alloc(); // 32 KiB
    void * b = block.Alloc(); // 32 KiB
    void * c = block.Alloc(); // 32 KiB, new page
    block.Free( a );
    block.Free( b );
    block.Free( c );
}

// TestSpeed
//------------------------------------------------------------------------------
void TestMemPoolBlock::TestSpeed()
{
    #if defined( DEBUG )
        const uint32_t numAllocs( 100 * 1000 );
    #else
        const uint32_t numAllocs( 1000 * 1000 );
    #endif
    const uint32_t allocSize( 24 );

    float time1( 0.0f );
    float time2( 0.0f );
    float time3( 0.0f );

    // System Allocator
    {
        Array< void * > allocs( numAllocs, false );
        Timer t1;
        {
            for ( uint32_t i = 0; i < numAllocs; ++i )
            {
                uint32_t * const mem = (uint32_t *)malloc( allocSize );
                allocs.Append( mem );
            }
            for ( uint32_t i = 0; i < numAllocs; ++i )
            {
                void * mem = allocs[ i ];
                free( mem );
            }
        }
        time1 = t1.GetElapsed();
    }

    // Alloc
    {
        Array< void * > allocs( numAllocs, false );
        Timer t2;
        {
            for ( uint32_t i = 0; i < numAllocs; ++i )
            {
                uint32_t * mem = (uint32_t *)ALLOC( allocSize );
                allocs.Append( mem );
            }
            for ( uint32_t i = 0; i < numAllocs; ++i )
            {
                void * mem = allocs[ i ];
                FREE( mem );
            }
        }
        time2 = t2.GetElapsed();
    }

    // MemPoolBlock
    {
        Array< void * > allocs( numAllocs, false );
        Timer t3;
        {
            MemPoolBlock block( allocSize, 4 );
            for ( uint32_t i = 0; i < numAllocs; ++i )
            {
                uint32_t * mem = (uint32_t *)block.Alloc();
                allocs.Append( mem );
            }
            for ( uint32_t i = 0; i < numAllocs; ++i )
            {
                void * mem = allocs[ i ];
                block.Free( mem );
            }
        }
        time3 = t3.GetElapsed();
    }

    // output
    OUTPUT( "malloc       : %2.3fs - %u allocs @ %u allocs/sec\n", (double)time1, numAllocs, (uint32_t)( (float)numAllocs / time1 ) );
    OUTPUT( "Alloc        : %2.3fs - %u allocs @ %u allocs/sec\n", (double)time2, numAllocs, (uint32_t)( (float)numAllocs / time2 ) );
    OUTPUT( "MemPoolBlock : %2.3fs - %u allocs @ %u allocs/sec\n", (double)time3, numAllocs, (uint32_t)( (float)numAllocs / time3 ) );
}

//------------------------------------------------------------------------------
