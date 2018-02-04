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
    void Test##function() const                                                                 \
    {                                                                                           \
        type data = initialValue;                                                               \
                                                                                                \
        Thread::ThreadHandle h = Thread::CreateThread( Test##function##ThreadEntryFunction,     \
                                                       #function,                               \
                                                       ( 64 * KILOBYTE ),                       \
                                                       static_cast< void * >( &data ) );        \
                                                                                                \
        volatile type & sharedVar = static_cast< volatile type & >( data );                     \
        ++sharedVar;                                                                            \
                                                                                                \
        for ( size_t i=0; i<1000000; ++i )                                                      \
        {                                                                                       \
            function( &sharedVar );                                                             \
        }                                                                                       \
                                                                                                \
        bool timedOut = false;                                                                  \
        Thread::WaitForThread( h, 1000, timedOut );                                             \
        TEST_ASSERT( timedOut == false );                                                       \
        Thread::CloseHandle( h );                                                               \
                                                                                                \
        type res = sharedVar;                                                                   \
        TEST_ASSERT( res == expectedResult );                                                   \
    }                                                                                           \
    static uint32_t Test##function##ThreadEntryFunction( void * userData )                      \
    {                                                                                           \
        type & data = *( static_cast< type * >( userData ) );                                   \
        volatile type & sharedVar = data;                                                       \
                                                                                                \
        while ( sharedVar == initialValue ) {}                                                  \
                                                                                                \
        for ( size_t i=0; i<1000000; ++i )                                                      \
        {                                                                                       \
            function( &sharedVar );                                                             \
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
    IMPLEMENT_TEST( uint32_t, AtomicIncU32, 0, 2000001 )
    IMPLEMENT_TEST( uint64_t, AtomicIncU64, 0, 2000001 )
    IMPLEMENT_TEST( int32_t, AtomicInc32, 0, 2000001 )
    IMPLEMENT_TEST( int64_t, AtomicInc64, 0, 2000001 )

    // Decrement
    IMPLEMENT_TEST( uint32_t, AtomicDecU32, 2000000, 1 )
    IMPLEMENT_TEST( uint64_t, AtomicDecU64, 2000000, 1 )
    IMPLEMENT_TEST( int32_t, AtomicDec32, 0, -1999999 )
    IMPLEMENT_TEST( int64_t, AtomicDec64, 0, -1999999 )

    // Add
    void Add32() const;
    void Add64() const;

    // Sub
    void Sub32() const;
    void Sub64() const;
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
    TEST_ASSERT( AtomicAdd32( &i32, 999 ) == 999 );
}

// Add64
//------------------------------------------------------------------------------
void TestAtomic::Add64() const
{
    // Ensure return result is post-add
    int64_t i64 = 0;
    TEST_ASSERT( AtomicAdd64( &i64, 999 ) == 999 );
}

// Sub32
//------------------------------------------------------------------------------
void TestAtomic::Sub32() const
{
    // Ensure return result is post-sub
    int32_t i32 = 999;
    TEST_ASSERT( AtomicSub32( &i32, 999 ) == 0 );
}

// Sub64
//------------------------------------------------------------------------------
void TestAtomic::Sub64() const
{
    // Ensure return result is post-sub
    int64_t i64 = 999;
    TEST_ASSERT( AtomicSub64( &i64, 999 ) == 0 );
}

//------------------------------------------------------------------------------
