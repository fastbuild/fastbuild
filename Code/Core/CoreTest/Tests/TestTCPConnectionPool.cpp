// TestTCPConnectionPool.cpp
//------------------------------------------------------------------------------

// Includes
//------------------------------------------------------------------------------
#include "TestFramework/UnitTest.h"

#include "Core/Containers/AutoPtr.h"
#include "Core/Network/TCPConnectionPool.h"
#include "Core/Process/Thread.h"
#include "Core/Strings/AStackString.h"
#include "Core/Time/Timer.h"

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
};

// Helper Macros
//------------------------------------------------------------------------------
#define WAIT_UNTIL_WITH_TIMEOUT( cond )             \
    {                                               \
        Timer t;                                    \
        t.Start();                                  \
        while ( ( cond ) == false )                 \
        {                                           \
            Thread::Sleep( 1 );                     \
            TEST_ASSERT( t.GetElapsed() < 5.0f );   \
        }                                           \
    }

// Register Tests
//------------------------------------------------------------------------------
REGISTER_TESTS_BEGIN( TestTestTCPConnectionPool )
    REGISTER_TEST( TestOneServerMultipleClients )
    REGISTER_TEST( TestMultipleServersOneClient )
    REGISTER_TEST( TestConnectionCount )
    REGISTER_TEST( TestDataTransfer )
    REGISTER_TEST( TestConnectionStuckDuringSend )
REGISTER_TESTS_END

// TestOneServerMultipleClients
//------------------------------------------------------------------------------
void TestTestTCPConnectionPool::TestOneServerMultipleClients() const
{
    const uint16_t testPort( TEST_PORT );

    for ( uint32_t i=0; i<NUM_TEST_PASSES; ++i )
    {
        // listen like a server
        TCPConnectionPool server;
        TEST_ASSERT( server.Listen( testPort ) );

        // connect like a client
        TCPConnectionPool clientA;
        TEST_ASSERT( clientA.Connect( AStackString<>( "127.0.0.1" ), testPort ) );

        // connect like a client
        TCPConnectionPool clientB;
        TEST_ASSERT( clientB.Connect( AStackString<>( "127.0.0.1" ), testPort ) );

        // connect like a client
        TCPConnectionPool clientC;
        TEST_ASSERT( clientC.Connect( AStackString<>( "127.0.0.1" ), testPort ) );

        // connect like a client
        TCPConnectionPool clientD;
        TEST_ASSERT( clientD.Connect( AStackString<>( "127.0.0.1" ), testPort ) );
    }
}

// TestMultipleServersOneClient
//------------------------------------------------------------------------------
void TestTestTCPConnectionPool::TestMultipleServersOneClient() const
{
    const uint16_t testPort( TEST_PORT );

    for ( uint32_t i=0; i<NUM_TEST_PASSES; ++i )
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
        TEST_ASSERT( clientA.Connect( AStackString<>( "127.0.0.1" ), testPort ) );
        TEST_ASSERT( clientA.Connect( AStackString<>( "127.0.0.1" ), testPort + 1 ) );
        TEST_ASSERT( clientA.Connect( AStackString<>( "127.0.0.1" ), testPort + 2 ) );
        TEST_ASSERT( clientA.Connect( AStackString<>( "127.0.0.1" ), testPort + 3 ) );
    }
}

// TestConnectionCount
//------------------------------------------------------------------------------
void TestTestTCPConnectionPool::TestConnectionCount() const
{
    const uint16_t testPort( TEST_PORT );

    for ( uint32_t i=0; i<NUM_TEST_PASSES; ++i )
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
            TEST_ASSERT( clientA.Connect( AStackString<>( "127.0.0.1" ), testPort ) );
            TEST_ASSERT( clientA.Connect( AStackString<>( "127.0.0.1" ), testPort + 1 ) );

            WAIT_UNTIL_WITH_TIMEOUT( serverA.GetNumConnections() == 1 );
            WAIT_UNTIL_WITH_TIMEOUT( serverB.GetNumConnections() == 1 );
            WAIT_UNTIL_WITH_TIMEOUT( clientA.GetNumConnections() == 2 );
        }
        WAIT_UNTIL_WITH_TIMEOUT( serverA.GetNumConnections() == 0 );
        WAIT_UNTIL_WITH_TIMEOUT( serverB.GetNumConnections() == 0 );
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
        virtual void OnReceive( const ConnectionInfo *, void * data, uint32_t size, bool & )
        {
            TEST_ASSERT( m_DataSize == size );
            for ( size_t i=0; i< size; ++i )
            {
                TEST_ASSERT( ((char *)data)[ i ] == (char)i );
            }
            m_ReceivedOK = true;
        }
        bool m_ReceivedOK;
        size_t m_DataSize;
    };

    const uint16_t testPort( TEST_PORT );

    // a big piece of data, initialized to some known pattern
    #define maxDataSize size_t( 1024 * 1024 * 10 )
    AutoPtr< char > data( (char *)ALLOC( maxDataSize ) );
    for ( size_t i=0; i< maxDataSize; ++i )
    {
        data.Get()[ i ] = (char)i;
    }

    TestServer server;
    TEST_ASSERT( server.Listen( testPort ) );

    // client
    TCPConnectionPool client;
    const ConnectionInfo * ci = client.Connect( AStackString<>( "127.0.0.1" ), testPort );
    TEST_ASSERT( ci );

    size_t testSize = 1;
    while ( testSize <= maxDataSize )
    {
        for ( size_t repeat = 0; repeat < 8; ++repeat )
        {
            server.m_ReceivedOK = false;
            server.m_DataSize = testSize;

            // client sends some know data to the server
            client.Send( ci, data.Get(), testSize );

            while( !server.m_ReceivedOK )
            {
                Thread::Sleep( 1 );
            }
        }

        testSize = ( testSize * 2 ) + 33; // +33 to avoid powers of 2
    }
}

// TestConnectionStuckDuringSend
//------------------------------------------------------------------------------
void TestTestTCPConnectionPool::TestConnectionStuckDuringSend() const
{
    // create a slow server
    class SlowServer : public TCPConnectionPool
    {
    public:
        virtual void OnReceive( const ConnectionInfo *, void *, uint32_t, bool & )
        {
            Thread::Sleep( 1000 );
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
                                                    (void*)ci );

    // let thread send enough data to become blocked in Send
    Thread::Sleep( 1000 );

    // flag for shutdown
    client.SetShuttingDown();

    // wait for client thread to exit, with timeout
    bool timedOut( false );
    Thread::WaitForThread( h, 1000, timedOut );
    Thread::CloseHandle( h );

    // if timeout was hit, things were stuck
    TEST_ASSERT( timedOut == false );
}
/*static*/ uint32_t TestTestTCPConnectionPool::TestConnectionStuckDuringSend_ThreadFunc( void * userData )
{
    const ConnectionInfo * ci = (const ConnectionInfo *)userData;
    TCPConnectionPool & client = ci->GetTCPConnectionPool();
    // send lots of data to slow server
    AutoPtr< char > mem( FNEW( char[ 10 * MEGABYTE ] ) );
    for ( size_t i=0; i<1000; ++i )
    {
        if ( !client.Send( ci, mem.Get(), 10 * MEGABYTE ) )
        {
            break;
        }
    }
    return 0;
}

//------------------------------------------------------------------------------
