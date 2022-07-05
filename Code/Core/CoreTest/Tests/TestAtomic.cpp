// TestAtomic.cpp
//------------------------------------------------------------------------------

// Includes
//------------------------------------------------------------------------------
#include "TestFramework/TestGroup.h"

#include "Core/Process/Atomic.h"
#include "Core/Process/Thread.h"

// Macros
//------------------------------------------------------------------------------
template<typename T>
class AtomicTestHelper
{
public:
    static const size_t loopCount = 10000;
    static const size_t addValue = 3;
    static const size_t subValue = 7;

    AtomicTestHelper()
    {
        const T initialValue = 99;
        PRAGMA_DISABLE_PUSH_MSVC(4307) // integral constant overflow
        PRAGMA_DISABLE_PUSH_MSVC(4309) // truncation of constant value
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
        Thread::ThreadHandle h = Thread::CreateThread( ThreadWrapper,
                                                       "AtomicTestHelper",
                                                       ( 64 * KILOBYTE ),
                                                       this );

        // Do works locally that mirrors the thread
        DoWork();

        // Join thread
        bool timedOut = false;
        Thread::WaitForThread( h, 1000, timedOut );
        TEST_ASSERT( timedOut == false );
        Thread::CloseHandle( h );

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
    volatile T          m_Count;    // Direct access
    Atomic<T>           m_Count2;   // Via Atomic<> helper
};

// TestAtomic
//------------------------------------------------------------------------------
class TestAtomic : public TestGroup
{
private:
    DECLARE_TESTS

    // Basic types
    template<typename T>
    void DoAtomicTestsForType()
    {
        const AtomicTestHelper<T> helper;
    }

    // Boolean
    void Boolean() const;

    // Pointer
    void Pointer() const;
};

// Register Tests
//------------------------------------------------------------------------------
REGISTER_TESTS_BEGIN( TestAtomic )
    REGISTER_TEST( DoAtomicTestsForType<uint8_t> )
    REGISTER_TEST( DoAtomicTestsForType<uint16_t> )
    REGISTER_TEST( DoAtomicTestsForType<uint32_t> )
    REGISTER_TEST( DoAtomicTestsForType<uint64_t> )
    REGISTER_TEST( DoAtomicTestsForType<int8_t> )
    REGISTER_TEST( DoAtomicTestsForType<int16_t> )
    REGISTER_TEST( DoAtomicTestsForType<int32_t> )
    REGISTER_TEST( DoAtomicTestsForType<int64_t> )

    // Boolean
    REGISTER_TEST( Boolean )

    // Pointer
    REGISTER_TEST( Pointer )
REGISTER_TESTS_END

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

// Pointer
//------------------------------------------------------------------------------
void TestAtomic::Pointer() const
{
    // Direct member
    {
        const TestAtomic * volatile pointer;
        AtomicStoreRelease( &pointer, this );
        TEST_ASSERT( AtomicLoadAcquire( &pointer ) == this );
    }

    // Atomic
    {
        Atomic<const TestAtomic *> pointer( nullptr );
        pointer.Store( this );
        TEST_ASSERT( pointer.Load() == this );
    }
}

//------------------------------------------------------------------------------
