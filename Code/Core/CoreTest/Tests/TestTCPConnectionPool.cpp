// TestTCPConnectionPool.cpp
//------------------------------------------------------------------------------

// Includes
//------------------------------------------------------------------------------
#include "TestFramework/UnitTest.h"

// Core
#include "Core/Containers/UniquePtr.h"
#include "Core/Network/TCPConnectionPool.h"
#include "Core/Process/Atomic.h"
#include "Core/Process/Semaphore.h"
#include "Core/Process/Thread.h"
#include "Core/Profile/Profile.h"
#include "Core/Strings/AStackString.h"
#include "Core/Time/Timer.h"
#include "Core/Tracing/Tracing.h"

#include <memory.h> // for memset

// Defines
//------------------------------------------------------------------------------
#define NUM_TEST_PASSES ( 16 )

// unique port for test in all configs so the tests can run in parallel
#ifdef WIN64
    #ifdef DEBUG
        #define TEST_PORT uint16_t( 21941 ) // arbitrarily chosen
    #else
        #define TEST_PORT uint16_t( 22941 ) // arbitrarily chosen
    #endif
#else
    #ifdef DEBUG
        #define TEST_PORT uint16_t( 23941 ) // arbitrarily chosen
    #else
        #define TEST_PORT uint16_t( 24941 ) // arbitrarily chosen
    #endif
#endif
// TestTestTCPConnectionPool
//------------------------------------------------------------------------------
class TestTestTCPConnectionPool : public UnitTest
{
private:
    DECLARE_TESTS

    void TestOneServerMultipleClients() const;
    void TestMultipleServersOneClient() const;
    void TestConnectionCount() const;
    void TestDataTransfer() const;

    void TestConnectionStuckDuringSend() const;
    static uint32_t TestConnectionStuckDuringSend_ThreadFunc( void * userData );

    void TestConnectionFailure() const;
};

// Helper Macros
//------------------------------------------------------------------------------
#define WAIT_UNTIL_WITH_TIMEOUT( cond )             \
    do {                                            \
        Timer t;                                    \
        t.Start();                                  \
        while ( ( cond ) == false )                 \
        {                                           \
            Thread::Sleep( 1 );                     \
            TEST_ASSERT( t.GetElapsed() < 30.0f );  \
        }                                           \
    } while( false )

// Register Tests
//------------------------------------------------------------------------------
REGISTER_TESTS_BEGIN( TestTestTCPConnectionPool )
    REGISTER_TEST( TestOneServerMultipleClients )
    REGISTER_TEST( TestMultipleServersOneClient )
    REGISTER_TEST( TestConnectionCount )
    REGISTER_TEST( TestDataTransfer )
    REGISTER_TEST( TestConnectionStuckDuringSend )
    REGISTER_TEST( TestConnectionFailure )
REGISTER_TESTS_END

// TestOneServerMultipleClients
//------------------------------------------------------------------------------
void TestTestTCPConnectionPool::TestOneServerMultipleClients() const
{
    const uint16_t testPort( TEST_PORT );

    for ( uint32_t i = 0; i < NUM_TEST_PASSES; ++i )
    {
        // listen like a server
        TCPConnectionPool server;
        TEST_ASSERT( server.Listen( testPort ) );

        // connect several clients
        const size_t numClients = 4;
        TCPConnectionPool clients[ numClients ];
        for ( size_t j = 0; j < numClients; ++j )
        {
            // All each client to retry in case of local resource exhaustion
            Timer t;
            while ( clients[ j ].Connect( AStackString<>( "127.0.0.1" ), testPort ) == nullptr )
            {
                TEST_ASSERTM( t.GetElapsed() < 5.0f, "Failed to connect. (Pass %u, client %u)", i, (uint32_t)j );
                Thread::Sleep( 50 );
            }

            clients[ j ].ShutdownAllConnections();
        }

        server.ShutdownAllConnections();
    }
}

// TestMultipleServersOneClient
//------------------------------------------------------------------------------
void TestTestTCPConnectionPool::TestMultipleServersOneClient() const
{
    const uint16_t testPort( TEST_PORT );

    for ( uint32_t i = 0; i < NUM_TEST_PASSES; ++i )
    {
        // multiple servers
        TCPConnectionPool serverA;
        TEST_ASSERT( serverA.Listen( testPort ) );
        TCPConnectionPool serverB;
        TEST_ASSERT( serverB.Listen( testPort + 1 ) );
        TCPConnectionPool serverC;
        TEST_ASSERT( serverC.Listen( testPort + 2 ) );
        TCPConnectionPool serverD;
        TEST_ASSERT( serverD.Listen( testPort + 3 ) );

        // connect client to multiple servers
        TCPConnectionPool clientA;
        for ( size_t j = 0; j < 4; ++j )
        {
            // All each connection to be retried in case of local resource exhaustion
            Timer t;
            const uint16_t port = (uint16_t)( testPort + j );
            while ( clientA.Connect( AStackString<>( "127.0.0.1" ), port ) == nullptr )
            {
                TEST_ASSERTM( t.GetElapsed() < 5.0f, "Failed to connect. (Pass %u, client %u)", i, (uint32_t)j );
                Thread::Sleep( 50 );
            }
        }

        clientA.ShutdownAllConnections();
        serverA.ShutdownAllConnections();
        serverB.ShutdownAllConnections();
        serverC.ShutdownAllConnections();
        serverD.ShutdownAllConnections();
    }
}

// TestConnectionCount
//------------------------------------------------------------------------------
void TestTestTCPConnectionPool::TestConnectionCount() const
{
    const uint16_t testPort( TEST_PORT );

    for ( uint32_t i = 0; i < NUM_TEST_PASSES; ++i )
    {
        // multiple servers
        TCPConnectionPool serverA;
        TEST_ASSERT( serverA.Listen( testPort ) );
        TEST_ASSERT( serverA.GetNumConnections() == 0 );
        TCPConnectionPool serverB;
        TEST_ASSERT( serverB.Listen( testPort + 1 ) );
        TEST_ASSERT( serverB.GetNumConnections() == 0 );

        // connect client to multiple servers
        {
            TCPConnectionPool clientA;
            for ( size_t j = 0; j < 2; ++j )
            {
                // All each connection to be retried in case of local resource exhaustion
                Timer t;
                const uint16_t port = (uint16_t)( testPort + j );
                while ( clientA.Connect( AStackString<>( "127.0.0.1" ), port ) == nullptr )
                {
                    TEST_ASSERTM( t.GetElapsed() < 5.0f, "Failed to connect. (Pass %u, client %u)", i, (uint32_t)j );
                    Thread::Sleep( 50 );
                }
            }

            WAIT_UNTIL_WITH_TIMEOUT( serverA.GetNumConnections() == 1 );
            WAIT_UNTIL_WITH_TIMEOUT( serverB.GetNumConnections() == 1 );
            WAIT_UNTIL_WITH_TIMEOUT( clientA.GetNumConnections() == 2 );
            clientA.ShutdownAllConnections();
        }
        WAIT_UNTIL_WITH_TIMEOUT( serverA.GetNumConnections() == 0 );
        WAIT_UNTIL_WITH_TIMEOUT( serverB.GetNumConnections() == 0 );
        serverA.ShutdownAllConnections();
        serverB.ShutdownAllConnections();
    }
}

// TestDataTransfer
//------------------------------------------------------------------------------
void TestTestTCPConnectionPool::TestDataTransfer() const
{
    // a special server which will assert that it receives some expected data
    class TestServer : public TCPConnectionPool
    {
    public:
        virtual ~TestServer() override { ShutdownAllConnections(); }
        virtual void OnReceive( const ConnectionInfo *, void * data, uint32_t size, bool & ) override
        {
            TEST_ASSERT( size == m_DataSize );
            TEST_ASSERT( memcmp( data, m_ExpectedData, size ) == 0 );
            AtomicAddU64( &m_ReceivedBytes, size );
            m_DataReceviedSemaphore.Signal();
        }
        volatile uint64_t m_ReceivedBytes = 0;
        size_t m_DataSize = 0;
        const char * m_ExpectedData;
        Semaphore m_DataReceviedSemaphore;
    };

    const uint16_t testPort( TEST_PORT );

    // a big piece of data, initialized to some known pattern
    const size_t maxSendSize( 1024 * 1024 * 10 );
    UniquePtr< char > data( (char *)ALLOC( maxSendSize ) );
    for ( size_t i = 0; i < maxSendSize; ++i )
    {
        data.Get()[ i ] = (char)i;
    }

    TestServer server;
    server.m_ExpectedData = data.Get(); // Allow OnReceive to compare data to expected
    TEST_ASSERT( server.Listen( testPort ) );

    // client
    TCPConnectionPool client;
    const ConnectionInfo * ci = client.Connect( AStackString<>( "127.0.0.1" ), testPort );
    TEST_ASSERT( ci );

    size_t sendSize = 31;
    while ( sendSize <= maxSendSize )
    {
        AtomicStoreRelaxed( &server.m_ReceivedBytes, 0 );
        server.m_DataSize = sendSize;

        Timer timer;

        size_t totalSent = 0;
        while ( ( totalSent < maxSendSize ) && ( timer.GetElapsed() < 0.1f ) )
        {
            // client sends some know data to the server
            TEST_ASSERT( client.Send( ci, data.Get(), sendSize ) );
            totalSent += sendSize;
        }

        while ( static_cast< size_t >( AtomicLoadRelaxed( &server.m_ReceivedBytes ) ) < totalSent )
        {
            server.m_DataReceviedSemaphore.Wait();
        }

        const float speedMBs = ( float( totalSent ) / timer.GetElapsed() ) / float( 1024 * 1024 );
        OUTPUT( "Speed: %2.1f MiB/s, SendSize: %u\n", (double)speedMBs, (uint32_t)sendSize );

        sendSize = ( sendSize * 2 ) + 33; // +33 to avoid powers of 2
    }

    client.ShutdownAllConnections();
}

// TestConnectionStuckDuringSend
//------------------------------------------------------------------------------
void TestTestTCPConnectionPool::TestConnectionStuckDuringSend() const
{
    // create a slow server
    class SlowServer : public TCPConnectionPool
    {
    public:
        virtual ~SlowServer() override { ShutdownAllConnections(); }
        virtual void OnReceive( const ConnectionInfo *, void *, uint32_t, bool & ) override
        {
            Thread::Sleep( 200 );
        }
    };
    SlowServer slowServer;
    const uint16_t testPort( TEST_PORT );
    TEST_ASSERT( slowServer.Listen( testPort ) );

    // connect to slow server
    TCPConnectionPool client;
    const ConnectionInfo * ci = client.Connect( AStackString<>( "127.0.0.1" ), testPort );
    TEST_ASSERT( ci );

    // start a thread to flood the slow server
    Thread::ThreadHandle h = Thread::CreateThread( TestConnectionStuckDuringSend_ThreadFunc,
                                                   "Sender",
                                                   ( 64 * KILOBYTE ),
                                                   (void *)ci );

    // let thread send enough data to become blocked in Send
    Thread::Sleep( 100 );

    // flag for shutdown
    client.SetShuttingDown();

    // wait for client thread to exit, with timeout
    bool timedOut( false );
    Thread::WaitForThread( h, 1000, timedOut );
    Thread::CloseHandle( h );

    // if timeout was hit, things were stuck
    TEST_ASSERT( timedOut == false );

    client.ShutdownAllConnections();
}
/*static*/ uint32_t TestTestTCPConnectionPool::TestConnectionStuckDuringSend_ThreadFunc( void * userData )
{
    PROFILE_SET_THREAD_NAME( "ConnectionStuckSend" );

    const ConnectionInfo * ci = (const ConnectionInfo *)userData;
    TCPConnectionPool & client = ci->GetTCPConnectionPool();
    // send lots of data to slow server
    UniquePtr< char > mem( (char *)ALLOC( 10 * MEGABYTE ) );
    memset( mem.Get(), 0, 10 * MEGABYTE );
    for ( size_t i = 0; i < 1000; ++i )
    {
        if ( !client.Send( ci, mem.Get(), 10 * MEGABYTE ) )
        {
            break;
        }
    }
    return 0;
}

// TestConnectionFailure
//------------------------------------------------------------------------------
void TestTestTCPConnectionPool::TestConnectionFailure() const
{
    const uint16_t testPort( TEST_PORT );
    const uint32_t timeoutMS( 100 );

    TCPConnectionPool client;

    // Check that TCPConnectionPool doesn't create ConnectionInfo when
    // connection fails after wait for it via select().
    // To do that we try to connect to our chosen test port without listening on it.
    TEST_ASSERT( client.Connect( AStackString<>( "127.0.0.1" ), testPort, timeoutMS ) == nullptr );

    client.ShutdownAllConnections();
}

//------------------------------------------------------------------------------
