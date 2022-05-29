// TestAtomic.cpp
//------------------------------------------------------------------------------

// Includes
//------------------------------------------------------------------------------
#include "TestFramework/UnitTest.h"

#include "Core/Process/Atomic.h"
#include "Core/Process/Thread.h"

// Macros
//------------------------------------------------------------------------------
#define IMPLEMENT_TEST( type, function, function2, initialValue, expectedResult )               \
    class Test##function##_##type##UserData                                                     \
    {                                                                                           \
    public:                                                                                     \
        volatile type m_Count = initialValue;                                                   \
        Atomic<type> m_Count2{ initialValue };                                                  \
        volatile uint32_t m_BarrierCounter = 0;                                                 \
    };                                                                                          \
    void Test##function##_##type() const                                                        \
    {                                                                                           \
        Test##function##_##type##UserData data;                                                 \
                                                                                                \
        Thread::ThreadHandle h = Thread::CreateThread( Test##function##_##type##ThreadEntryFunction, \
                                                       #function,                               \
                                                       ( 64 * KILOBYTE ),                       \
                                                       static_cast< void * >( &data ) );        \
                                                                                                \
        AtomicInc( &data.m_BarrierCounter );                                                    \
        while ( AtomicLoadAcquire( &data.m_BarrierCounter ) != 2 ) {}                           \
                                                                                                \
        for ( size_t i = 0; i < 1000000; ++i )                                                  \
        {                                                                                       \
            function( &data.m_Count );                                                          \
        }                                                                                       \
                                                                                                \
        AtomicInc( &data.m_BarrierCounter );                                                    \
        while ( AtomicLoadAcquire( &data.m_BarrierCounter ) != 4 ) {}                           \
                                                                                                \
        for ( size_t i = 0; i < 1000000; ++i )                                                  \
        {                                                                                       \
            data.m_Count2.function2();                                                          \
        }                                                                                       \
                                                                                                \
        bool timedOut = false;                                                                  \
        Thread::WaitForThread( h, 1000, timedOut );                                             \
        TEST_ASSERT( timedOut == false );                                                       \
        Thread::CloseHandle( h );                                                               \
                                                                                                \
        type res = AtomicLoadRelaxed( &data.m_Count );                                          \
        TEST_ASSERT( res == expectedResult );                                                   \
                                                                                                \
        type res2 = data.m_Count2.Load();                                                       \
        TEST_ASSERT( res2 == expectedResult );                                                  \
    }                                                                                           \
    static uint32_t Test##function##_##type##ThreadEntryFunction( void * userData )             \
    {                                                                                           \
        Test##function##_##type##UserData & data = *( static_cast< Test##function##_##type##UserData * >( userData ) ); \
                                                                                                \
        AtomicInc( &data.m_BarrierCounter );                                                    \
        while ( AtomicLoadAcquire( &data.m_BarrierCounter ) != 2 ) {}                           \
                                                                                                \
        for ( size_t i = 0; i < 1000000; ++i )                                                  \
        {                                                                                       \
            function( &data.m_Count );                                                          \
        }                                                                                       \
                                                                                                \
        AtomicInc( &data.m_BarrierCounter );                                                    \
        while ( AtomicLoadAcquire( &data.m_BarrierCounter ) != 4 ) {}                           \
                                                                                                \
        for ( size_t i = 0; i < 1000000; ++i )                                                  \
        {                                                                                       \
            data.m_Count2.function2();                                                          \
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
    IMPLEMENT_TEST( uint32_t, AtomicInc, Increment, 0, 2000000 )
    IMPLEMENT_TEST( uint64_t, AtomicInc, Increment, 0, 2000000 )
    IMPLEMENT_TEST( int32_t, AtomicInc, Increment, 0, 2000000 )
    IMPLEMENT_TEST( int64_t, AtomicInc, Increment, 0, 2000000 )

    // Decrement
    IMPLEMENT_TEST( uint32_t, AtomicDec, Decrement, 2000000, 0 )
    IMPLEMENT_TEST( uint64_t, AtomicDec, Decrement, 2000000, 0 )
    IMPLEMENT_TEST( int32_t, AtomicDec, Decrement, 0, -2000000 )
    IMPLEMENT_TEST( int64_t, AtomicDec, Decrement, 0, -2000000 )

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

    // Boolean
    void Boolean() const;
};

// Register Tests
//------------------------------------------------------------------------------
REGISTER_TESTS_BEGIN( TestAtomic )
    // Increment
    REGISTER_TEST( TestAtomicInc_uint32_t )
    REGISTER_TEST( TestAtomicInc_uint64_t )
    REGISTER_TEST( TestAtomicInc_int32_t )
    REGISTER_TEST( TestAtomicInc_int64_t )

    // Decrement
    REGISTER_TEST( TestAtomicDec_uint32_t )
    REGISTER_TEST( TestAtomicDec_uint64_t )
    REGISTER_TEST( TestAtomicDec_int32_t )
    REGISTER_TEST( TestAtomicDec_int64_t )

    // Add
    REGISTER_TEST( Add32 )
    REGISTER_TEST( Add64 )
    REGISTER_TEST( AddU32 )
    REGISTER_TEST( AddU64 )

    // Sub
    REGISTER_TEST( Sub32 )
    REGISTER_TEST( Sub64 )
    REGISTER_TEST( SubU32 )
    REGISTER_TEST( SubU64 )

    // Boolean
    REGISTER_TEST( Boolean )
REGISTER_TESTS_END

// Add32
//------------------------------------------------------------------------------
void TestAtomic::Add32() const
{
    // Ensure return result is post-add
    int32_t i32 = 0;
    TEST_ASSERT( AtomicAdd( &i32, -999 ) == -999 );
}

// AddU32
//------------------------------------------------------------------------------
void TestAtomic::AddU32() const
{
    // Ensure return result is post-add
    uint32_t u32 = 0;
    TEST_ASSERT( AtomicAdd( &u32, 999 ) == 999 );
}

// Add64
//------------------------------------------------------------------------------
void TestAtomic::Add64() const
{
    // Ensure return result is post-add
    int64_t i64 = 0;
    TEST_ASSERT( AtomicAdd( &i64, -9876543210 ) == -9876543210 );
}

// AddU64
//------------------------------------------------------------------------------
void TestAtomic::AddU64() const
{
    // Ensure return result is post-add
    uint64_t u64 = 0;
    TEST_ASSERT( AtomicAdd( &u64, 9876543210 ) == 9876543210 );
}

// Sub32
//------------------------------------------------------------------------------
void TestAtomic::Sub32() const
{
    // Ensure return result is post-sub
    int32_t i32 = 0;
    TEST_ASSERT( AtomicSub( &i32, 999 ) == -999 );
}

// SubU32
//------------------------------------------------------------------------------
void TestAtomic::SubU32() const
{
    // Ensure return result is post-sub
    uint32_t u32 = 999;
    TEST_ASSERT( AtomicSub( &u32, 999 ) == 0 );
}

// Sub64
//------------------------------------------------------------------------------
void TestAtomic::Sub64() const
{
    // Ensure return result is post-sub
    int64_t i64 = 0;
    TEST_ASSERT( AtomicSub( &i64, 9876543210 ) == -9876543210 );
}

// SubU64
//------------------------------------------------------------------------------
void TestAtomic::SubU64() const
{
    // Ensure return result is post-sub
    uint64_t u64 = 9876543210;
    TEST_ASSERT( AtomicSub( &u64, 9876543210 ) == 0 );
}

// Boolean
//------------------------------------------------------------------------------
void TestAtomic::Boolean() const
{
    // Direct member
    {
        volatile bool b = false;
        AtomicStoreRelease( &b, true );
        TEST_ASSERT( AtomicLoadAcquire( &b ) == true );
    }

    // Atomic
    {
        Atomic<bool> b( false );
        b.Store( true );
        TEST_ASSERT( b.Load() == true );
    }
}

//------------------------------------------------------------------------------
