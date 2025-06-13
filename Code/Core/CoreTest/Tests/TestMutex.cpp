// TestMutex.cpp
//------------------------------------------------------------------------------

// Includes
//------------------------------------------------------------------------------
#include "TestFramework/TestGroup.h"

#include "Core/Mem/Mem.h"
#include "Core/Process/Atomic.h"
#include "Core/Process/Mutex.h"
#include "Core/Process/Semaphore.h"
#include "Core/Process/Thread.h"

// TestMutex
//------------------------------------------------------------------------------
class TestMutex : public TestGroup
{
private:
    DECLARE_TESTS

    void TestConstruct() const;
    void TestLockUnlock() const;
    void TestMultiLockUnlock() const;
    void TryLock() const;
    void TryLockMultiple() const;
    void TryLockMixed() const;
    void TryLockFail() const;
    static uint32_t TestLockFailThreadEntryFunction( void * data );

    void TestExclusivity() const;
    struct TestExclusivityUserData
    {
        Mutex m_Mutex;
        uint32_t m_Count;
        volatile uint32_t m_BarrierCounter;
    };
    static uint32_t TestExclusivityThreadEntryFunction( void * userData );

#if defined( __WINDOWS__ )
    struct PaddingStruct
    {
        Mutex m_Mutex1;
        uint8_t m_Padding;
        Mutex m_Mutex2;
    };
    void TestAlignment() const;
#endif
};

// Register Tests
//------------------------------------------------------------------------------
REGISTER_TESTS_BEGIN( TestMutex )
    REGISTER_TEST( TestConstruct )
    REGISTER_TEST( TestLockUnlock )
    REGISTER_TEST( TestMultiLockUnlock )
    REGISTER_TEST( TryLock )
    REGISTER_TEST( TryLockMultiple )
    REGISTER_TEST( TryLockMixed )
    REGISTER_TEST( TryLockFail )
    REGISTER_TEST( TestExclusivity )
#if defined( __WINDOWS__ )
    REGISTER_TEST( TestAlignment )
#endif
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

// TryLock
//------------------------------------------------------------------------------
void TestMutex::TryLock() const
{
    // Ensure lock can be acquired
    Mutex m;
    TEST_ASSERT( m.TryLock() );
    m.Unlock();
}

// TryLockMultiple
//------------------------------------------------------------------------------
void TestMutex::TryLockMultiple() const
{
    // Ensure lock can be acquired multiple times
    Mutex m;
    {
        // Manual locks
        TEST_ASSERT( m.TryLock() );
        TEST_ASSERT( m.TryLock() );
        m.Unlock();
        m.Unlock();
    }
    {
        // RAII lock
        MutexHolder mh( m );
        MutexHolder m2( m );
    }
    {
        // RAII lock (Try)
        TryMutexHolder mh( m );
        TEST_ASSERT( mh.IsLocked() );
        TryMutexHolder mh2( m );
        TEST_ASSERT( mh2.IsLocked() );
    }
}

// TryLockMixed
//------------------------------------------------------------------------------
void TestMutex::TryLockMixed() const
{
    Mutex m;
    // Lock then TryLock
    {
        m.Lock();
        TEST_ASSERT( m.TryLock() );
        m.Unlock();
        m.Unlock();
    }
    // TryLock then Lock
    {
        TEST_ASSERT( m.TryLock() );
        m.Lock();
        m.Unlock();
        m.Unlock();
    }
}

// TryLockFail
//------------------------------------------------------------------------------
void TestMutex::TryLockFail() const
{
    Mutex m;
    MutexHolder mh( m ); // Hold lock so thread cannot acquire it

    Thread t;
    t.Start( TestLockFailThreadEntryFunction, "TryLockFail", &m );
    t.Join();
}

// TestLockFailThreadEntryFunction
//------------------------------------------------------------------------------
/*static*/ uint32_t TestMutex::TestLockFailThreadEntryFunction( void * data )
{
    // main thread should hold lock
    Mutex * m = static_cast<Mutex *>( data );
    TEST_ASSERT( m->TryLock() == false );
    return 0;
}

// TestExclusivity
//------------------------------------------------------------------------------
void TestMutex::TestExclusivity() const
{
    TestExclusivityUserData data;
    data.m_Count = 0;
    data.m_BarrierCounter = 0;

    Thread t;
    t.Start( TestExclusivityThreadEntryFunction, "TestExclusivity", &data );

    // arrive at barrier and wait
    AtomicInc( &data.m_BarrierCounter );
    while ( AtomicLoadAcquire( &data.m_BarrierCounter ) != 2 )
    {
    }

    // increment
    for ( size_t i = 0; i < 1000000; ++i )
    {
        MutexHolder mh( data.m_Mutex );
        ++data.m_Count;
    }

    // wait for other thread to complete
    t.Join();

    // ensure total is correct
    TEST_ASSERT( data.m_Count == 2000000 );
}

// TestExclusivityThreadEntryFunction
//------------------------------------------------------------------------------
/*static*/ uint32_t TestMutex::TestExclusivityThreadEntryFunction( void * userData )
{
    TestExclusivityUserData & data = *( static_cast<TestExclusivityUserData *>( userData ) );

    // arrive at barrier and wait
    AtomicInc( &data.m_BarrierCounter );
    while ( AtomicLoadAcquire( &data.m_BarrierCounter ) != 2 )
    {
    }

    // increment
    for ( size_t i = 0; i < 1000000; ++i )
    {
        MutexHolder mh( data.m_Mutex );
        ++data.m_Count;
    }

    return 0;
}

// TestAlignment
//------------------------------------------------------------------------------
#if defined( __WINDOWS__ )
void TestMutex::TestAlignment() const
{
    // Check that alignment on stack and heap is correct
    PaddingStruct ps1;
    PaddingStruct * ps2 = FNEW( PaddingStruct );

    TEST_ASSERT( ( (size_t)&ps1.m_Mutex1 % sizeof( void * ) ) == 0 );
    TEST_ASSERT( ( (size_t)&ps1.m_Mutex2 % sizeof( void * ) ) == 0 );

    TEST_ASSERT( ( (size_t)&ps2->m_Mutex1 % sizeof( void * ) ) == 0 );
    TEST_ASSERT( ( (size_t)&ps2->m_Mutex2 % sizeof( void * ) ) == 0 );

    delete ps2;
}
#endif

//------------------------------------------------------------------------------
