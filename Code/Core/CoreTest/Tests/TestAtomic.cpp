// TestAtomic.cpp
//------------------------------------------------------------------------------

// Includes
//------------------------------------------------------------------------------
#include "TestFramework/TestGroup.h"

#include "Core/Process/Atomic.h"
#include "Core/Process/Thread.h"

//------------------------------------------------------------------------------
template <typename T>
class AtomicTestHelper
{
public:
    static const size_t loopCount = 10000;
    static const size_t addValue = 3;
    static const size_t subValue = 7;

    AtomicTestHelper()
    {
        const T initialValue = 99;
        PRAGMA_DISABLE_PUSH_MSVC( 4307 ) // integral constant overflow
        PRAGMA_DISABLE_PUSH_MSVC( 4309 ) // truncation of constant value
        const T expectedResult = static_cast<T>( initialValue + ( 2 * loopCount * ( addValue - subValue ) ) );
        PRAGMA_DISABLE_POP_MSVC
        PRAGMA_DISABLE_POP_MSVC

        // Initialize direct value
        AtomicStoreRelaxed( &m_Count, initialValue );
        AtomicStoreRelease( &m_Count, initialValue ); // Redundant to check template

        // Initialize Atomic<>
        m_Count2.Store( initialValue );

        // Check basic direct operations
        TEST_ASSERT( AtomicInc( &m_Count ) == ( initialValue + 1 ) ); // Returns new value
        TEST_ASSERT( AtomicDec( &m_Count ) == initialValue ); // Returns new value
        TEST_ASSERT( AtomicAdd( &m_Count, (T)13 ) == ( initialValue + (T)13 ) ); // Returns new value
        TEST_ASSERT( AtomicSub( &m_Count, (T)13 ) == initialValue ); // Returns new value

        // Check basic Atomic<> operations
        m_Count2.Increment();
        TEST_ASSERT( m_Count2.Load() == ( initialValue + 1 ) );
        m_Count2.Decrement();
        TEST_ASSERT( m_Count2.Load() == initialValue );
        m_Count2.Add( 13 );
        TEST_ASSERT( m_Count2.Load() == ( initialValue + 13 ) );
        m_Count2.Sub( 13 );
        TEST_ASSERT( m_Count2.Load() == initialValue );

        // Spawn thread
        Thread t;
        t.Start( ThreadWrapper, "AtomicTestHelper", this );

        // Do works locally that mirrors the thread
        DoWork();

        // Join thread
        t.Join();

        // Check expected results
        TEST_ASSERT( AtomicLoadRelaxed( &m_Count ) == expectedResult );
        TEST_ASSERT( AtomicLoadAcquire( &m_Count ) == expectedResult ); // Redundant to check template
        TEST_ASSERT( m_Count2.Load() == expectedResult );
    }

protected:
    void DoWork()
    {
        for ( size_t i = 0; i < loopCount; ++i )
        {
            // Direct
            AtomicInc( &m_Count );
            AtomicAdd( &m_Count, static_cast<T>( 3 ) );
            AtomicDec( &m_Count );
            AtomicSub( &m_Count, static_cast<T>( 7 ) );

            // Atomic<>
            m_Count2.Increment();
            m_Count2.Add( 3 );
            m_Count2.Decrement();
            m_Count2.Sub( 7 );
        }
    }

    static uint32_t ThreadWrapper( void * userData )
    {
        static_cast<AtomicTestHelper *>( userData )->DoWork();
        return 0;
    }

    // Test values to operator on
    volatile T m_Count; // Direct access
    Atomic<T> m_Count2; // Via Atomic<> helper
};

//------------------------------------------------------------------------------
TEST_GROUP( TestAtomic, TestGroupTest )
{
public:
};

//------------------------------------------------------------------------------
TEST_CASE( TestAtomic, AtomicOps_U8 )
{
    const AtomicTestHelper<uint8_t> helper;
}

//------------------------------------------------------------------------------
TEST_CASE( TestAtomic, AtomicOps_U16 )
{
    const AtomicTestHelper<uint16_t> helper;
}

//------------------------------------------------------------------------------
TEST_CASE( TestAtomic, AtomicOps_U32 )
{
    const AtomicTestHelper<uint32_t> helper;
}

//------------------------------------------------------------------------------
TEST_CASE( TestAtomic, AtomicOps_U64 )
{
    const AtomicTestHelper<uint64_t> helper;
}

//------------------------------------------------------------------------------
TEST_CASE( TestAtomic, AtomicOps_I8 )
{
    const AtomicTestHelper<int8_t> helper;
}

//------------------------------------------------------------------------------
TEST_CASE( TestAtomic, AtomicOps_I16 )
{
    const AtomicTestHelper<int16_t> helper;
}

//------------------------------------------------------------------------------
TEST_CASE( TestAtomic, AtomicOps_I32 )
{
    const AtomicTestHelper<int32_t> helper;
}

//------------------------------------------------------------------------------
TEST_CASE( TestAtomic, AtomicOps_I64 )
{
    const AtomicTestHelper<int64_t> helper;
}

//------------------------------------------------------------------------------
TEST_CASE( TestAtomic, Boolean )
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
TEST_CASE( TestAtomic, Pointer )
{
    // Direct member
    {
        const auto * volatile pointer = this;
        AtomicStoreRelease( &pointer, this );
        TEST_ASSERT( AtomicLoadAcquire( &pointer ) == this );
    }

    // Atomic
    {
        Atomic<const void *> pointer( nullptr );
        pointer.Store( this );
        TEST_ASSERT( pointer.Load() == this );
    }
}

//------------------------------------------------------------------------------
