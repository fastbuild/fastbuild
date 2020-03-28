// TestAtomic.cpp
//------------------------------------------------------------------------------

// Includes
//------------------------------------------------------------------------------
#include "TestFramework/UnitTest.h"

#include "Core/Process/Atomic.h"
#include "Core/Process/Thread.h"

// Macros
//------------------------------------------------------------------------------
#define IMPLEMENT_TEST( type, function, initialValue, expectedResult )                          \
    struct Test##function##UserData                                                             \
    {                                                                                           \
        volatile type m_Count;                                                                  \
        volatile uint32_t m_BarrierCounter;                                                     \
    };                                                                                          \
    void Test##function() const                                                                 \
    {                                                                                           \
        Test##function##UserData data;                                                          \
        data.m_Count = initialValue;                                                            \
        data.m_BarrierCounter = 0;                                                              \
                                                                                                \
        Thread::ThreadHandle h = Thread::CreateThread( Test##function##ThreadEntryFunction,     \
                                                       #function,                               \
                                                       ( 64 * KILOBYTE ),                       \
                                                       static_cast< void * >( &data ) );        \
                                                                                                \
        AtomicIncU32( &data.m_BarrierCounter );                                                 \
        while ( AtomicLoadAcquire( &data.m_BarrierCounter ) != 2 ) {}                           \
                                                                                                \
        for ( size_t i = 0; i < 1000000; ++i )                                                  \
        {                                                                                       \
            function( &data.m_Count );                                                          \
        }                                                                                       \
                                                                                                \
        bool timedOut = false;                                                                  \
        Thread::WaitForThread( h, 1000, timedOut );                                             \
        TEST_ASSERT( timedOut == false );                                                       \
        Thread::CloseHandle( h );                                                               \
                                                                                                \
        type res = AtomicLoadRelaxed( &data.m_Count );                                          \
        TEST_ASSERT( res == expectedResult );                                                   \
    }                                                                                           \
    static uint32_t Test##function##ThreadEntryFunction( void * userData )                      \
    {                                                                                           \
        Test##function##UserData & data = *( static_cast< Test##function##UserData * >( userData ) ); \
                                                                                                \
        AtomicIncU32( &data.m_BarrierCounter );                                                 \
        while ( AtomicLoadAcquire( &data.m_BarrierCounter ) != 2 ) {}                           \
                                                                                                \
        for ( size_t i = 0; i < 1000000; ++i )                                                  \
        {                                                                                       \
            function( &data.m_Count );                                                          \
        }                                                                                       \
                                                                                                \
        return 0;                                                                               \
    }

// TestAtomic
//------------------------------------------------------------------------------
class TestAtomic : public UnitTest
{
private:
    DECLARE_TESTS

    // Increment
    IMPLEMENT_TEST( uint32_t, AtomicIncU32, 0, 2000000 )
    IMPLEMENT_TEST( uint64_t, AtomicIncU64, 0, 2000000 )
    IMPLEMENT_TEST( int32_t, AtomicInc32, 0, 2000000 )
    IMPLEMENT_TEST( int64_t, AtomicInc64, 0, 2000000 )

    // Decrement
    IMPLEMENT_TEST( uint32_t, AtomicDecU32, 2000000, 0 )
    IMPLEMENT_TEST( uint64_t, AtomicDecU64, 2000000, 0 )
    IMPLEMENT_TEST( int32_t, AtomicDec32, 0, -2000000 )
    IMPLEMENT_TEST( int64_t, AtomicDec64, 0, -2000000 )

    // Add
    void Add32() const;
    void AddU32() const;
    void Add64() const;
    void AddU64() const;

    // Sub
    void Sub32() const;
    void SubU32() const;
    void Sub64() const;
    void SubU64() const;
};

// Register Tests
//------------------------------------------------------------------------------
REGISTER_TESTS_BEGIN( TestAtomic )
    // Increment
    REGISTER_TEST( TestAtomicIncU32 )
    REGISTER_TEST( TestAtomicIncU64 )
    REGISTER_TEST( TestAtomicInc32 )
    REGISTER_TEST( TestAtomicInc64 )

    // Decrement
    REGISTER_TEST( TestAtomicDecU32 )
    REGISTER_TEST( TestAtomicDecU64 )
    REGISTER_TEST( TestAtomicDec32 )
    REGISTER_TEST( TestAtomicDec64 )

    // Add
    REGISTER_TEST( Add32 )
    REGISTER_TEST( Add64 )

    // Sub
    REGISTER_TEST( Sub32 )
    REGISTER_TEST( Sub64 )
REGISTER_TESTS_END

// Add32
//------------------------------------------------------------------------------
void TestAtomic::Add32() const
{
    // Ensure return result is post-add
    int32_t i32 = 0;
    TEST_ASSERT( AtomicAdd32( &i32, -999 ) == -999 );
}

// AddU32
//------------------------------------------------------------------------------
void TestAtomic::AddU32() const
{
    // Ensure return result is post-add
    uint32_t u32 = 0;
    TEST_ASSERT( AtomicAddU32( &u32, 999 ) == 999 );
}

// Add64
//------------------------------------------------------------------------------
void TestAtomic::Add64() const
{
    // Ensure return result is post-add
    int64_t i64 = 0;
    TEST_ASSERT( AtomicAdd64( &i64, -9876543210 ) == -9876543210 );
}

// AddU64
//------------------------------------------------------------------------------
void TestAtomic::AddU64() const
{
    // Ensure return result is post-add
    uint64_t u64 = 0;
    TEST_ASSERT( AtomicAddU64( &u64, 9876543210 ) == 9876543210 );
}

// Sub32
//------------------------------------------------------------------------------
void TestAtomic::Sub32() const
{
    // Ensure return result is post-sub
    int32_t i32 = 0;
    TEST_ASSERT( AtomicSub32( &i32, 999 ) == -999 );
}

// SubU32
//------------------------------------------------------------------------------
void TestAtomic::SubU32() const
{
    // Ensure return result is post-sub
    uint32_t u32 = 999;
    TEST_ASSERT( AtomicSubU32( &u32, 999 ) == 0 );
}

// Sub64
//------------------------------------------------------------------------------
void TestAtomic::Sub64() const
{
    // Ensure return result is post-sub
    int64_t i64 = 0;
    TEST_ASSERT( AtomicSub64( &i64, 9876543210 ) == -9876543210 );
}

// SubU64
//------------------------------------------------------------------------------
void TestAtomic::SubU64() const
{
    // Ensure return result is post-sub
    uint64_t u64 = 9876543210;
    TEST_ASSERT( AtomicSubU64( &u64, 9876543210 ) == 0 );
}

//------------------------------------------------------------------------------
