// TestMutex.cpp
//------------------------------------------------------------------------------

// Includes
//------------------------------------------------------------------------------
#include "TestFramework/UnitTest.h"

#include "Core/Process/Mutex.h"
#include "Core/Process/Thread.h"

// TestMutex
//------------------------------------------------------------------------------
class TestMutex : public UnitTest
{
private:
    DECLARE_TESTS

    void TestConstruct() const;
    void TestLockUnlock() const;
    void TestMultiLockUnlock() const;

    void TestExclusivity() const;
    struct TestExclusivityUserData
    {
        Mutex m_Mutex;
        volatile uint32_t m_Count;
    };
    static uint32_t TestExclusivityThreadEntryFunction( void * userData );
};

// Register Tests
//------------------------------------------------------------------------------
REGISTER_TESTS_BEGIN( TestMutex )
    REGISTER_TEST( TestConstruct )
    REGISTER_TEST( TestLockUnlock )
    REGISTER_TEST( TestMultiLockUnlock )
    REGISTER_TEST( TestExclusivity )
REGISTER_TESTS_END

// TestConstruct
//------------------------------------------------------------------------------
void TestMutex::TestConstruct() const
{
    Mutex m;
}

// TestLockUnlock
//------------------------------------------------------------------------------
void TestMutex::TestLockUnlock() const
{
    Mutex m;
    m.Lock();
    m.Unlock();
}

// TestMutex
//------------------------------------------------------------------------------
void TestMutex::TestMultiLockUnlock() const
{
    Mutex m;
    m.Lock();
    m.Lock();
    m.Unlock();
    m.Unlock();
}

// TestExclusivity
//------------------------------------------------------------------------------
void TestMutex::TestExclusivity() const
{
    TestExclusivityUserData data;
    data.m_Count = 0;

    Thread::ThreadHandle h = Thread::CreateThread( TestExclusivityThreadEntryFunction,
                                                   "TestExclusivity",
                                                   ( 64 * KILOBYTE ),
                                                   static_cast< void * >( &data ) );

    // signal thread
    volatile uint32_t & sharedVar = data.m_Count;
    ++sharedVar;

    // increment
    for ( size_t i=0; i<1000000; ++i )
    {
        MutexHolder mh( data.m_Mutex );
        ++sharedVar;
    }

    // wait for other thread to complete
    bool timedOut = false;
    Thread::WaitForThread( h, 1000, timedOut );
    TEST_ASSERT( timedOut == false );
    Thread::CloseHandle( h );

    // ensure total is correct
    TEST_ASSERT( sharedVar == 2000001 );
}

// TestExclusivityThreadEntryFunction
//------------------------------------------------------------------------------
/*static*/ uint32_t TestMutex::TestExclusivityThreadEntryFunction( void * userData )
{
    TestExclusivityUserData & data = *( static_cast< TestExclusivityUserData * >( userData ) );
    volatile uint32_t & sharedVar = data.m_Count;

    // wait for start notification
    while ( sharedVar == 0 ) {}

    // increment
    for ( size_t i=0; i<1000000; ++i )
    {
        MutexHolder mh( data.m_Mutex );
        ++sharedVar;
    }

    return 0;
}

//------------------------------------------------------------------------------
