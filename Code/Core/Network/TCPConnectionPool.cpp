// TCPConnectionPool
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
// Includes
//------------------------------------------------------------------------------
#include "TCPConnectionPool.h"

// Core
#include "Core/Env/ErrorFormat.h"
#include "Core/Mem/Mem.h"
#include "Core/Network/Network.h"
#include "Core/Process/Atomic.h"
#include "Core/Profile/Profile.h"
#include "Core/Strings/AStackString.h"
#include "Core/Strings/AString.h"
#include "Core/Time/Timer.h"

// System
#if defined( __WINDOWS__ )
    #include "Core/Env/WindowsHeader.h"
#elif defined( __APPLE__ ) || defined( __LINUX__ )
    #include <arpa/inet.h>
    #include <errno.h>
    #include <fcntl.h>
    #include <netdb.h>
    #include <netinet/in.h>
    #include <netinet/tcp.h>
    #include <string.h>
    #include <sys/ioctl.h>
    #include <sys/socket.h>
    #include <sys/uio.h>
    #include <unistd.h>
    #define INVALID_SOCKET ( -1 )
    #define SOCKET_ERROR -1
#else
    #error Unknown platform
#endif

// Defines
//------------------------------------------------------------------------------
// Enable for debugging
//#define TCPCONNECTION_DEBUG
#ifdef TCPCONNECTION_DEBUG
    #include "Core/Tracing/Tracing.h"
    #define TCPDEBUG( ... ) DEBUGSPAM( __VA_ARGS__ )
    #define LAST_NETWORK_ERROR_STR ERROR_STR( GetLastNetworkError() )
#else
    #define TCPDEBUG( ... ) (void)0
#endif

// TCPConnectionPoolProfileHelper
//------------------------------------------------------------------------------
#if defined( PROFILING_ENABLED )
    class TCPConnectionPoolProfileHelper
    {
    public:
        enum ThreadType
        {
            THREAD_LISTEN,
            THREAD_CONNECTION
        };

        TCPConnectionPoolProfileHelper( ThreadType threadType )
        {
            // Chose which bitmap to use
            uint64_t & bitmap = ( threadType == THREAD_LISTEN ) ? s_IdBitmapListen : s_IdBitmapConnection;

            // Find free bit
            uint32_t bit = 0;
            {
                MutexHolder mh( s_Mutex );
                for ( ; bit < 64; ++bit )
                {
                    // Is this bit clear?
                    if ( ( ( (uint64_t)1 << bit ) & bitmap ) == 0 )
                    {
                        // Set bit as we will use this Id
                        bitmap |= ( (uint64_t)1 << bit );
                        break;
                    }
                }
            }
            m_Bit = bit; // Store the bit for this thread
            m_ThreadType = threadType;

            // No free bits? (Last bit is never set)
            if ( bit == 63 )
            {
                return; // Can't set thread name
            }

            // Format and set
            AStackString<> threadName;
            threadName.Format( ( threadType == THREAD_LISTEN ) ? "Listen_%u" : "Connection_%u", bit );
            PROFILE_SET_THREAD_NAME( threadName.Get() );
        }
        ~TCPConnectionPoolProfileHelper()
        {
            // Clear bit if we reserved one
            if ( m_Bit < 63 )
            {
                // Chose which bitmap to use
                uint64_t& bitmap = ( m_ThreadType == THREAD_LISTEN ) ? s_IdBitmapListen : s_IdBitmapConnection;

                // Clear bit
                MutexHolder mh( s_Mutex );
                bitmap &= ~( (uint64_t)1 << m_Bit );
            }
        }

    protected:
        ThreadType          m_ThreadType;
        uint32_t            m_Bit;

        static Mutex        s_Mutex;
        static uint64_t     s_IdBitmapListen;
        static uint64_t     s_IdBitmapConnection;
    };
    /*static*/ Mutex    TCPConnectionPoolProfileHelper::s_Mutex;
    /*static*/ uint64_t TCPConnectionPoolProfileHelper::s_IdBitmapListen        = 0;
    /*static*/ uint64_t TCPConnectionPoolProfileHelper::s_IdBitmapConnection    = 0;

    #define TCP_CONNECTION_POOL_PROFILE_SET_THREAD_NAME( threadType )   \
        TCPConnectionPoolProfileHelper threadNameHelper( threadType )
#else
    #define TCP_CONNECTION_POOL_PROFILE_SET_THREAD_NAME( threadType ) (void)0
#endif

// CONSTRUCTOR - ConnectionInfo
//------------------------------------------------------------------------------
ConnectionInfo::ConnectionInfo( TCPConnectionPool * ownerPool )
    : m_Socket( INVALID_SOCKET )
    , m_RemoteAddress( 0 )
    , m_RemotePort( 0 )
    , m_ThreadQuitNotification( false )
    , m_TCPConnectionPool( ownerPool )
    , m_UserData( nullptr )
    #ifdef DEBUG
        , m_InUse( false )
    #endif
{
    ASSERT( ownerPool );
}

// CONSTRUCTOR
//------------------------------------------------------------------------------
TCPConnectionPool::TCPConnectionPool()
    : m_ListenConnection( nullptr )
    , m_Connections( 8, true )
    , m_ShuttingDown( false )
{
}

// DESTRUCTOR
//------------------------------------------------------------------------------
TCPConnectionPool::~TCPConnectionPool()
{
    // ShutdownAllConnections() must be called explicitly prior to destruction
    // as virtual callbacks in derived classes make it unsafe to do so here.
    // By enforcing explicit shutdown, even when not strictly needed, we can
    // ensure no unsafe cases exist (and can assert below)
    ASSERT( AtomicLoadRelaxed( &m_ShuttingDown ) && "ShutdownAllConnections not called" );
}

// ShutdownAllConnections
//------------------------------------------------------------------------------
void TCPConnectionPool::ShutdownAllConnections()
{
    PROFILE_FUNCTION;

    AtomicStoreRelaxed( &m_ShuttingDown, true );

    m_ConnectionsMutex.Lock();

    // signal all remaining connections to close

    // listening connection
    if ( m_ListenConnection )
    {
        Disconnect( m_ListenConnection );
    }

    // wait for connections to be closed
    while ( m_ListenConnection ||
            !m_Connections.IsEmpty() )
    {
        // incoming connections
        for ( size_t i = 0; i < m_Connections.GetSize(); ++i )
        {
            const ConnectionInfo * const ci = m_Connections[ i ];
            Disconnect( ci );
        }

        m_ConnectionsMutex.Unlock();
        m_ShutdownSemaphore.Wait( 1 );
        m_ConnectionsMutex.Lock();
    }
    m_ConnectionsMutex.Unlock();
}

// GetAddressAsString
//------------------------------------------------------------------------------
/*static*/ void TCPConnectionPool::GetAddressAsString( uint32_t addr, AString & address )
{
    address.Format( "%u.%u.%u.%u", (unsigned int)( addr & 0x000000FF ) ,
                                   (unsigned int)( addr & 0x0000FF00 ) >> 8,
                                   (unsigned int)( addr & 0x00FF0000 ) >> 16,
                                   (unsigned int)( addr & 0xFF000000 ) >> 24 );
}

// Listen
//------------------------------------------------------------------------------
bool TCPConnectionPool::Listen( uint16_t port )
{
    // must not be listening already
    ASSERT( m_ListenConnection == nullptr );

    // create the socket
    TCPSocket sockfd = CreateSocket();
    if ( sockfd == INVALID_SOCKET )
    {
        return false;
    }

    // Configure socket
    AllowSocketReuse( sockfd );     // Allow socket re-use
    DisableSigPipe( sockfd );       // Prevent socket inheritence by child processes
    DisableNagle( sockfd );         // Disable Nagle's algorithm

    // set up the listen params
    struct sockaddr_in addrInfo;
    memset( &addrInfo, 0, sizeof( addrInfo ) );
    addrInfo.sin_family = AF_INET;
    addrInfo.sin_port = htons( port );
    addrInfo.sin_addr.s_addr = INADDR_ANY;

    // bind
    if ( bind( sockfd, (struct sockaddr *)&addrInfo, sizeof( addrInfo ) ) != 0 )
    {
        TCPDEBUG( "Bind failed. Error: %s\n", LAST_NETWORK_ERROR_STR );
        CloseSocket( sockfd );
        return false;
    }

    // listen
    TCPDEBUG( "Listen on port %i (%x)\n", port, (uint32_t)sockfd );
    if ( listen( sockfd, 0 ) == SOCKET_ERROR ) // no backlog
    {
        TCPDEBUG( "Listen FAILED %i (%x)\n", port, (uint32_t)sockfd );
        CloseSocket( sockfd );
        return false;
    }

    // spawn the handler thread
    uint32_t loopback = 127 & ( 1 << 24 ); // 127.0.0.1
    CreateListenThread( sockfd, loopback, port );

    // everything is ok - we are now listening, managing connections on the other thread
    return true;
}

// Connect
//------------------------------------------------------------------------------
const ConnectionInfo * TCPConnectionPool::Connect( const AString & host, uint16_t port, uint32_t timeout, void * userData )
{
    ASSERT( !host.IsEmpty() );

    // get IP
    uint32_t hostIP = Network::GetHostIPFromName( host, timeout );
    if ( hostIP == 0 )
    {
        TCPDEBUG( "Failed to get address for '%s'\n", host.Get() );
        return nullptr;
    }
    return Connect( hostIP, port, timeout, userData );
}

// Connect
//------------------------------------------------------------------------------
const ConnectionInfo * TCPConnectionPool::Connect( uint32_t hostIP, uint16_t port, uint32_t timeout, void * userData )
{
    PROFILE_FUNCTION;

    // create a socket
    TCPSocket sockfd = CreateSocket();
    if ( sockfd == INVALID_SOCKET )
    {
        return nullptr; // outright failure?
    }

    // Configure socket
    DisableSigPipe( sockfd );       // Prevent socket inheritence by child processes
    DisableNagle( sockfd );         // Disable Nagle's algorithm
    SetLargeBufferSizes( sockfd );  // Set large send/recv buffer sizes
    SetNonBlocking( sockfd );       // Set non-blocking

    // setup destination address
    struct sockaddr_in destAddr;
    memset( &destAddr, 0, sizeof( destAddr ) );
    destAddr.sin_family = AF_INET;
    destAddr.sin_port = htons( port );
    destAddr.sin_addr.s_addr = hostIP;

    // initiate connection
    if ( connect( sockfd, (struct sockaddr *)&destAddr, sizeof( destAddr ) ) != 0 )
    {
        // we expect WSAEWOULDBLOCK
        if ( !WouldBlock() )
        {
            // connection initiation failed
            #ifdef TCPCONNECTION_DEBUG
                AStackString<> host;
                GetAddressAsString( hostIP, host );
                TCPDEBUG( "connect() failed. Error: %s (Host: %s, Port: %u)\n", LAST_NETWORK_ERROR_STR, host.Get(), port );
            #endif
            CloseSocket( sockfd );
            return nullptr;
        }
    }

    Timer connectionTimer;

    // wait for connection
    for ( ;; )
    {
        fd_set write, err;
        FD_ZERO( &write );
        FD_ZERO( &err );
        PRAGMA_DISABLE_PUSH_MSVC( 4548 ) // warning C4548: expression before comma has no effect; expected expression with side-effect
        PRAGMA_DISABLE_PUSH_MSVC( 6319 ) // warning C6319: Use of the comma-operator in a tested expression...
        PRAGMA_DISABLE_PUSH_CLANG_WINDOWS( "-Wcomma" ) // possible misuse of comma operator here [-Wcomma]
        PRAGMA_DISABLE_PUSH_CLANG_WINDOWS( "-Wunused-value" ) // expression result unused [-Wunused-value]
        FD_SET( sockfd, &write );
        FD_SET( sockfd, &err );
        PRAGMA_DISABLE_POP_CLANG_WINDOWS // -Wunused-value
        PRAGMA_DISABLE_POP_CLANG_WINDOWS // -Wcomma
        PRAGMA_DISABLE_POP_MSVC // 6319
        PRAGMA_DISABLE_POP_MSVC // 4548

        // check connection every 10ms
        timeval pollingTimeout;
        memset( &pollingTimeout, 0, sizeof( timeval ) );
        pollingTimeout.tv_usec = 10 * 1000;

        // check if the socket is ready
        int selRet = Select( sockfd + 1, nullptr, &write, &err, &pollingTimeout );
        if ( selRet == SOCKET_ERROR )
        {
            // connection failed
            #ifdef TCPCONNECTION_DEBUG
                AStackString<> host;
                GetAddressAsString( hostIP, host );
                TCPDEBUG( "select() after connect() failed. Error: %s (Host: %s, Port: %u)\n", LAST_NETWORK_ERROR_STR, host.Get(), port );
            #endif
            CloseSocket( sockfd );
            return nullptr;
        }

        // polling timeout hit?
        if ( selRet == 0 )
        {
            // are we shutting down?
            if ( AtomicLoadRelaxed( &m_ShuttingDown ) )
            {
                #ifdef TCPCONNECTION_DEBUG
                    AStackString<> host;
                    GetAddressAsString( hostIP, host );
                    TCPDEBUG( "connect() aborted (Shutting Down) (Host: %s, Port: %u)\n", host.Get(), port );
                #endif
                CloseSocket( sockfd );
                return nullptr;
            }

            // have we hit our real connection timeout?
            if ( connectionTimer.GetElapsedMS() >= (float)timeout )
            {
                #ifdef TCPCONNECTION_DEBUG
                    AStackString<> host;
                    GetAddressAsString( hostIP, host );
                    TCPDEBUG( "connect() time out %u hit (Host: %s, Port: %u)\n", timeout, host.Get(), port );
                #endif
                CloseSocket( sockfd );
                return nullptr;
            }

            // keep waiting
            continue;
        }

        if ( FD_ISSET( sockfd, &err ) )
        {
            // connection failed
            #ifdef TCPCONNECTION_DEBUG
                AStackString<> host;
                GetAddressAsString( hostIP, host );
                const int lastNetworkError = GetLastNetworkError(); // NOTE: Get error before call to getsockopt

                int32_t error = 0;
                socklen_t size = sizeof(error);
                if ( getsockopt( sockfd, SOL_SOCKET, SO_ERROR, (char*)&error, &size ) != SOCKET_ERROR )
                {
                    TCPDEBUG( "select() after connect() failed. Error: %s (Host:%s, Port: %u), select returned: %i, SO_ERROR %i\n", ERROR_STR( lastNetworkError ), host.Get(), port, selRet, error );
                }
                else
                {
                    TCPDEBUG( "select() after connect() failed. Error: %s (Host:%s, Port: %u), select returned: %i\n", ERROR_STR( lastNetworkError ), host.Get(), port, selRet );
                }
            #endif
            CloseSocket( sockfd );
            return nullptr;
        }

        if ( FD_ISSET( sockfd, &write ) )
        {
            #if defined( __APPLE__ ) || defined( __LINUX__ )
                // On Linux a write flag set by select() doesn't mean that
                // connect() succeeded, it only means that it is completed.
                // To get the result we need to query SO_ERROR value via getsockopt().
                int32_t error = 0;
                socklen_t size = sizeof(error);
                if ( getsockopt( sockfd, SOL_SOCKET, SO_ERROR, (char*)&error, &size ) == SOCKET_ERROR )
                {
                    #ifdef TCPCONNECTION_DEBUG
                        AStackString<> host;
                        GetAddressAsString( hostIP, host );
                        TCPDEBUG( "getsockopt() failed. Error: %s (Host: %s, Port: %u)\n", LAST_NETWORK_ERROR_STR, host.Get(), port );
                    #endif
                    CloseSocket( sockfd );
                    return nullptr;
                }
                if ( error != 0 )
                {
                    #ifdef TCPCONNECTION_DEBUG
                        AStackString<> host;
                        GetAddressAsString( hostIP, host );
                        TCPDEBUG( "connect() failed, SO_ERROR: %s (Host: %s, Port: %u)\n", ERROR_STR( error ), host.Get(), port );
                    #endif
                    CloseSocket( sockfd );
                    return nullptr;
                }
            #endif
            break; // connection success!
        }

        ASSERT( false ); // should never get here
    }

    return CreateConnectionThread( sockfd, hostIP, port, userData );
}

// Disconnect
//------------------------------------------------------------------------------
void TCPConnectionPool::Disconnect( const ConnectionInfo * ci )
{
    ASSERT( ci );

    //
    // The ConnectionInfo is only valid while we still have a
    // pointer to it in our list of connections (or as the special
    // listener connection)
    //

    // ensure the connection thread isn't busy destroying itself
    MutexHolder mh( m_ConnectionsMutex );

    if ( ci == m_ListenConnection )
    {
        AtomicStoreRelease( &ci->m_ThreadQuitNotification, true );
        return;
    }

    const ConnectionInfo * const * iter = m_Connections.Find( ci );
    if ( iter != nullptr )
    {
        AtomicStoreRelease( &ci->m_ThreadQuitNotification, true );
        return;
    }

    // connection is no longer valid.... we handle this gracefully
    // as the connection might be lost while trying to disconnect
    // on another thread
}

// SetShuttingDown
//------------------------------------------------------------------------------
void TCPConnectionPool::SetShuttingDown()
{
    AtomicStoreRelaxed( &m_ShuttingDown, true );
}

// GetNumConnections
//------------------------------------------------------------------------------
size_t TCPConnectionPool::GetNumConnections() const
{
    MutexHolder mh( m_ConnectionsMutex );
    return m_Connections.GetSize();
}

// Send
//------------------------------------------------------------------------------
bool TCPConnectionPool::Send( const ConnectionInfo * connection, const void * data, size_t size, uint32_t timeoutMS )
{
    SendBuffer buffers[ 2 ]; // size + data

    // size
    uint32_t sizeData = (uint32_t)size;
    buffers[ 0 ].size = sizeof( sizeData );
    buffers[ 0 ].data = &sizeData;

    // data
    buffers[ 1 ].size = (uint32_t)size;
    buffers[ 1 ].data = data;

    return SendInternal( connection, buffers, 2, timeoutMS );
}

//------------------------------------------------------------------------------
bool TCPConnectionPool::Send( const ConnectionInfo * connection, const void * data, size_t size, const void * payloadData, size_t payloadSize, uint32_t timeoutMS )
{
    SendBuffer buffers[ 4 ]; // size + data + payloadSize + payloadData

    // size
    uint32_t sizeData = (uint32_t)size;
    buffers[ 0 ].size = sizeof( sizeData );
    buffers[ 0 ].data = &sizeData;

    // data
    buffers[ 1 ].size = (uint32_t)size;
    buffers[ 1 ].data = data;

    // payloadSize
    uint32_t payloadSizeData = (uint32_t)payloadSize;
    buffers[ 2 ].size = sizeof( payloadSizeData );
    buffers[ 2 ].data = &payloadSizeData;

    // payloadData
    buffers[ 3 ].size = (uint32_t)payloadSize;
    buffers[ 3 ].data = payloadData;

    return SendInternal( connection, buffers, 4, timeoutMS );
}

// SendInternal
//------------------------------------------------------------------------------
bool TCPConnectionPool::SendInternal( const ConnectionInfo * connection, const TCPConnectionPool::SendBuffer * buffers, uint32_t numBuffers, uint32_t timeoutMS )
{
    PROFILE_FUNCTION;

    ASSERT( connection );

    // closing connection, possibly from a previous failure
    if ( AtomicLoadAcquire( &connection->m_ThreadQuitNotification ) || AtomicLoadRelaxed( &m_ShuttingDown ) )
    {
        return false;
    }

    ASSERT( numBuffers <= 4 ); // Worst case = size + data + payloadSize + payload
    #if defined( __WINDOWS__ )
        WSABUF sendBuffers[ 4 ];
    #else
        struct iovec sendBuffers[ 4 ];
    #endif

    // Calculate total to send
    uint32_t totalBytes( 0 );
    for ( uint32_t i = 0; i < numBuffers; ++i )
    {
        totalBytes += buffers[ i ].size;
    }

    Timer timer;

#ifdef DEBUG
    ASSERT( connection->m_InUse == false );
    connection->m_InUse = true;
#endif

    ASSERT( connection->m_Socket != INVALID_SOCKET );

    TCPDEBUG( "Send: %i (%x)\n", totalBytes, (uint32_t)( connection->m_Socket ) );

    bool sendOK = true;

    // Repeat until all bytes sent
    uint32_t bytesSent = 0;
    while ( bytesSent < totalBytes )
    {
        // Fill buffers for any unsent data
        uint32_t numSendBuffers( 0 );
        uint32_t offset( 0 );
        for ( uint32_t i = 0; i < numBuffers; ++i )
        {
            const uint32_t overlap = bytesSent > offset ? ( bytesSent - offset ) : 0;
            if ( overlap < buffers[ i ].size )
            {
                // add remaining data for this buffer
                const uint32_t remainder = ( buffers[ i ].size - overlap );
                #if defined( __WINDOWS__ )
                    sendBuffers[ numSendBuffers ].len = remainder;
                    sendBuffers[ numSendBuffers ].buf = const_cast< CHAR * >( (const char *)buffers[ i ].data + buffers[ i ].size - remainder );
                #else
                    sendBuffers[ numSendBuffers ].iov_len = remainder;
                    sendBuffers[ numSendBuffers ].iov_base = const_cast< char * >( (const char *)buffers[ i ].data + buffers[ i ].size - remainder );
                #endif
                ++numSendBuffers;
            }
            offset += buffers[ i ].size;
        }
        ASSERT( offset == totalBytes ); // sanity check
        ASSERT( numSendBuffers > 0 ); // shouldn't be in loop if there was no data to send!

        // Try send
        #if defined( __WINDOWS__ )
            uint32_t sent( 0 );
            int result = WSASend( connection->m_Socket, sendBuffers, numSendBuffers, (LPDWORD)&sent, 0, nullptr, nullptr );
            if ( result == SOCKET_ERROR )
        #else
            ssize_t sent = writev( connection->m_Socket, sendBuffers, numSendBuffers );
            if ( sent <= 0 )
        #endif
        {
            if ( WouldBlock() )
            {
                if ( AtomicLoadAcquire( &connection->m_ThreadQuitNotification ) || AtomicLoadRelaxed( &m_ShuttingDown ) )
                {
                    sendOK = false;
                    break;
                }

                if ( timer.GetElapsedMS() > (float)timeoutMS )
                {
                    Disconnect( connection );
                    sendOK = false;
                    break;
                }

                Thread::Sleep( 1 );
                continue;
            }
            // error
            TCPDEBUG( "send() failed (A). Error: %s (Sent: %u, Socket: %x)\n", LAST_NETWORK_ERROR_STR, sent, (uint32_t)( connection->m_Socket ) );
            Disconnect( connection );
            sendOK = false;
            break;
        }
        bytesSent += sent;
    }

    #ifdef DEBUG
        connection->m_InUse = false;
    #endif
    return sendOK;
}

// Broadcast
//------------------------------------------------------------------------------
bool TCPConnectionPool::Broadcast( const void * data, size_t size )
{
    MutexHolder mh( m_ConnectionsMutex );

    bool result = true;

    ConnectionInfo ** it = m_Connections.Begin();
    const ConnectionInfo * const * end = m_Connections.End();
    while ( it < end )
    {
        result &= Send( *it, data, size );
        it++;
    }
    return result;
}

// AllocBuffer
//------------------------------------------------------------------------------
/*virtual*/ void * TCPConnectionPool::AllocBuffer( uint32_t size )
{
    return ALLOC( size );
}

// FreeBuffer
//------------------------------------------------------------------------------
/*virtual*/ void TCPConnectionPool::FreeBuffer( void * data )
{
    FREE( data );
}

// HandleRead
//------------------------------------------------------------------------------
bool TCPConnectionPool::HandleRead( ConnectionInfo * ci )
{
    PROFILE_FUNCTION;

    // work out how many bytes there are
    uint32_t size( 0 );
    uint32_t bytesToRead = 4;
    while ( bytesToRead > 0 )
    {
        int numBytes = (int)recv( ci->m_Socket, ( (char *)&size ) + 4 - bytesToRead, (int32_t)bytesToRead, 0 );
        if ( numBytes <= 0 )
        {
            if ( WouldBlock() )
            {
                if ( AtomicLoadAcquire( &ci->m_ThreadQuitNotification ) || AtomicLoadRelaxed( &m_ShuttingDown ) )
                {
                    return false;
                }

                Thread::Sleep( 1 );
                continue;
            }
            TCPDEBUG( "recv() failed (A). Error: %s (Read: %i, Socket: %x)\n", LAST_NETWORK_ERROR_STR, numBytes, (uint32_t)( ci->m_Socket ) );
            return false;
        }
        bytesToRead -= (uint32_t)numBytes;
    }

    TCPDEBUG( "Handle read: %i (%x)\n", size, (uint32_t)( ci->m_Socket ) );

    // get output location
    void * buffer = AllocBuffer( size );
    ASSERT( buffer );

    // read data into the user supplied buffer
    uint32_t bytesRemaining = size;
    char * dest = (char *)buffer;
    while ( bytesRemaining > 0 )
    {
        int numBytes = (int)recv( ci->m_Socket, dest, (int32_t)bytesRemaining, 0 );
        if ( numBytes <= 0 )
        {
            if ( WouldBlock() )
            {
                if ( AtomicLoadAcquire( &ci->m_ThreadQuitNotification ) || AtomicLoadRelaxed( &m_ShuttingDown ) )
                {
                    FreeBuffer( buffer );
                    return false;
                }

                Thread::Sleep( 1 );
                continue;
            }
            TCPDEBUG( "recv() failed (B). Error: %s (Read: %i, Socket: %x)\n", LAST_NETWORK_ERROR_STR, numBytes, (uint32_t)( ci->m_Socket ) );
            FreeBuffer( buffer );
            return false;
        }
        bytesRemaining -= (uint32_t)numBytes;
        dest += numBytes;
    }

    // tell user the data is in their buffer
    bool keepMemory = false;
    OnReceive( ci, buffer, size, keepMemory );
    if ( !keepMemory )
    {
        FreeBuffer( buffer );
    }

    return true;
}

// GetLastNetworkError
//------------------------------------------------------------------------------
int TCPConnectionPool::GetLastNetworkError() const
{
    #if defined( __WINDOWS__ )
        return WSAGetLastError();
    #elif defined( __APPLE__ ) || defined( __LINUX__ )
        return errno;
    #else
        #error Unknown platform
    #endif
}

// WouldBlock
//------------------------------------------------------------------------------
bool TCPConnectionPool::WouldBlock() const
{
    #if defined( __WINDOWS__ )
        return ( WSAGetLastError() == WSAEWOULDBLOCK );
    #elif defined( __APPLE__ ) || defined( __LINUX__ )
        return ( ( errno == EAGAIN ) || ( errno == EWOULDBLOCK ) || ( errno == EINPROGRESS ) );
    #else
        #error Unknown platform
    #endif
}

// CloseSocket
//------------------------------------------------------------------------------
int TCPConnectionPool::CloseSocket( TCPSocket a_Socket ) const
{
    #if defined( __WINDOWS__ )
        return closesocket( a_Socket );
    #elif defined( __APPLE__ ) || defined( __LINUX__ )
        return close( a_Socket );
    #else
        #error Unknown platform
    #endif
}

// Select
//------------------------------------------------------------------------------
int TCPConnectionPool::Select( TCPSocket socket,
                               void * a_ReadSocketSet, // TODO: Using void * to avoid including header is ugly
                               void * a_WriteSocketSet,
                               void * a_ExceptionSocketSet,
                               timeval * a_TimeOut ) const
{
    PROFILE_SECTION( "Select" );
    return select( (int)socket, // NOTE: ignored by Windows
                   (fd_set *)a_ReadSocketSet,
                   (fd_set *)a_WriteSocketSet,
                   (fd_set *)a_ExceptionSocketSet,
                   a_TimeOut );
}

// Accept
//------------------------------------------------------------------------------
TCPSocket TCPConnectionPool::Accept( TCPSocket socket,
                                     struct sockaddr * address,
                                     int * addressSize ) const
{
    #if defined( __WINDOWS__ )
        // On Windows, the newSocket inherits WSA_FLAG_NO_HANDLE_INHERIT from socket
        TCPSocket newSocket = accept( socket, address, addressSize );

        // TODO: Re-enable
        //DWORD flags;
        //ASSERT( GetHandleInformation( (HANDLE)newSocket, &flags ) );
        //ASSERT( ( flags & HANDLE_FLAG_INHERIT ) == 0 );
        //(void)flags;
    #elif defined( __LINUX__ )
        // On Linux we can create the socket with inheritance disables (SOCK_CLOEXEC)
        TCPSocket newSocket = accept4( socket, address, (unsigned int *)addressSize, SOCK_CLOEXEC );
    #elif defined( __APPLE__ )
        // On OS X, we must explicitly set FD_CLOEXEC after creating the socket
        TCPSocket newSocket = accept( socket, address, (unsigned int *)addressSize );
    #endif

    if ( newSocket == INVALID_SOCKET )
    {
        TCPDEBUG( "accept() failed. Error: %s\n", LAST_NETWORK_ERROR_STR );
        return newSocket;
    }

    #if defined( __APPLE__ )
        // OS X does not support atomic setting of CLOEXEC: see notes in CreateSocket
        VERIFY( fcntl( newSocket, F_SETFD, FD_CLOEXEC ) == 0 );
    #endif

    return newSocket;
}

// CreateSocket
//------------------------------------------------------------------------------
TCPSocket TCPConnectionPool::CreateSocket() const
{
    #if defined( __LINUX__ )
        // On Linux we can create the socket with inheritance disabled (SOCK_CLOEXEC)
        TCPSocket newSocket = socket( AF_INET, SOCK_STREAM | SOCK_CLOEXEC, 0 );
    #elif defined( __WINDOWS__ )
        TCPSocket newSocket = socket( AF_INET, SOCK_STREAM, 0 );

        // TODO: Re-enable
        // On Windows we can create the socket with inheritance disabled (WSA_FLAG_NO_HANDLE_INHERIT)
        //TCPSocket newSocket = WSASocketW( AF_INET, SOCK_STREAM, IPPROTO_TCP, nullptr, 0, WSA_FLAG_NO_HANDLE_INHERIT );
    #else
        // On OS X, we must explicitly set FD_CLOEXEC after creating the socket
        TCPSocket newSocket = socket( AF_INET, SOCK_STREAM, 0 );
    #endif

    // Failure?
    if ( newSocket == INVALID_SOCKET )
    {
        TCPDEBUG( "Create socket failed (Connect). Error: %s\n", LAST_NETWORK_ERROR_STR );
        return newSocket;
    }

    #if defined( __APPLE__ )
        // OS X does not support atomic setting of CLOEXEC, which exposes
        // us to race conditions if another thread is spawning a process.
        // The best we can do is reduce the likelyhood of problems by immediately
        // setting the flag after creation.
        // In practice, the listen socket is the most problematic one to be
        // inherited (as it prevents re-use), but thankfully starting listening
        // while spawning a process is not something we generally do.
        VERIFY( fcntl( newSocket, F_SETFD, FD_CLOEXEC ) == 0 );
    #endif

    return newSocket;
}

// CreateListenThread
//------------------------------------------------------------------------------
void TCPConnectionPool::CreateListenThread( TCPSocket socket, uint32_t host, uint16_t port )
{
    MutexHolder mh( m_ConnectionsMutex );

    m_ListenConnection = FNEW( ConnectionInfo( this ) );
    m_ListenConnection->m_Socket = socket;
    m_ListenConnection->m_RemoteAddress = host;
    m_ListenConnection->m_RemotePort = port;
    m_ListenConnection->m_ThreadQuitNotification = false;

    // Spawn thread to handle socket
    Thread::ThreadHandle h = Thread::CreateThread( &ListenThreadWrapperFunction,
                                         "TCPListen",
                                         ( 32 * KILOBYTE ),
                                         m_ListenConnection ); // user data argument
    ASSERT( h != INVALID_THREAD_HANDLE );
    Thread::DetachThread( h );
    Thread::CloseHandle( h ); // we don't need this anymore
}

// ThreadWrapperFunction
//------------------------------------------------------------------------------
/*static*/ uint32_t TCPConnectionPool::ListenThreadWrapperFunction( void * data )
{
    TCP_CONNECTION_POOL_PROFILE_SET_THREAD_NAME( TCPConnectionPoolProfileHelper::THREAD_LISTEN );
    PROFILE_FUNCTION;

    ConnectionInfo * ci = (ConnectionInfo *)data;
    ci->m_TCPConnectionPool->ListenThreadFunction( ci );
    return 0;
}

// ListenThreadFunction
//------------------------------------------------------------------------------
void TCPConnectionPool::ListenThreadFunction( ConnectionInfo * ci )
{
    ASSERT( ci->m_Socket != INVALID_SOCKET );

    struct sockaddr_in remoteAddrInfo;
    int remoteAddrInfoSize = sizeof( remoteAddrInfo );

    while ( AtomicLoadAcquire( &ci->m_ThreadQuitNotification ) == false )
    {
        // timout for select() operations
        // (modified by select, so we must recreate it)
        struct timeval timeout;
        timeout.tv_sec = 0;
        timeout.tv_usec = 100000; // 100 ms

        // create a socket set (with just our listen socket in it)
        // (modified by the select() function, so we must recreate it)
        fd_set set;
        FD_ZERO( &set );
        PRAGMA_DISABLE_PUSH_MSVC( 4548 ) // warning C4548: expression before comma has no effect; expected expression with side-effect
        PRAGMA_DISABLE_PUSH_MSVC( 6319 ) // warning C6319: Use of the comma-operator in a tested expression...
        PRAGMA_DISABLE_PUSH_CLANG_WINDOWS( "-Wcomma" ) // possible misuse of comma operator here [-Wcomma]
        PRAGMA_DISABLE_PUSH_CLANG_WINDOWS( "-Wunused-value" ) // expression result unused [-Wunused-value]
        FD_SET( (uint32_t)ci->m_Socket, &set );
        PRAGMA_DISABLE_POP_CLANG_WINDOWS // -Wunused-value
        PRAGMA_DISABLE_POP_CLANG_WINDOWS // -Wcomma
        PRAGMA_DISABLE_POP_MSVC // 6319
        PRAGMA_DISABLE_POP_MSVC // 4548

        // peek
        int num = Select( ci->m_Socket + 1, &set, nullptr, nullptr, &timeout );
        if ( num == 0 )
        {
            // timeout expired - loop again (checking quit notification)
            continue;
        }

        // new connection

        // get a socket for the new connection
        TCPSocket newSocket = Accept( ci->m_Socket, (struct sockaddr *)&remoteAddrInfo, &remoteAddrInfoSize );

        // handle errors or socket shutdown
        if ( newSocket == INVALID_SOCKET )
        {
            TCPDEBUG( "accept() failed. Error: %s\n", LAST_NETWORK_ERROR_STR );
            break;
        }

        #ifdef TCPCONNECTION_DEBUG
            AStackString<32> addr;
            GetAddressAsString( remoteAddrInfo.sin_addr.s_addr, addr );
            TCPDEBUG( "Connection accepted from %s : %i (%x)\n", addr.Get(), ntohs( remoteAddrInfo.sin_port ), (uint32_t)newSocket );
        #endif

        // Configure socket
        DisableSigPipe( newSocket );        // Prevent socket inheritence by child processes
        DisableNagle( newSocket );          // Disable Nagle's algorithm
        SetLargeBufferSizes( newSocket );   // Set send/recv buffer sizes
        SetNonBlocking( newSocket );        // Set non-blocking

        // keep the new connected socket
        CreateConnectionThread( newSocket,
                                remoteAddrInfo.sin_addr.s_addr,
                                ntohs( remoteAddrInfo.sin_port ) );

        continue; // keep listening for more connections
    }

    // close the socket
    CloseSocket( ci->m_Socket );
    ci->m_Socket = INVALID_SOCKET;

    {
        // clear connection (might already be null
        // if simultaneously closed on another thread
        // but we'll hapily set it null redundantly
        MutexHolder mh( m_ConnectionsMutex );
        ASSERT( m_ListenConnection == ci );
        m_ListenConnection = nullptr;
        FDELETE ci;
        m_ShutdownSemaphore.Signal(); // Wake main thread which may be waiting on shutdown
    }

    // thread exit
    TCPDEBUG( "Listen thread exited\n" );
}

// CreateConnectionThread
//------------------------------------------------------------------------------
ConnectionInfo * TCPConnectionPool::CreateConnectionThread( TCPSocket socket, uint32_t host, uint16_t port, void * userData )
{
    MutexHolder mh( m_ConnectionsMutex );

    ConnectionInfo * ci = FNEW( ConnectionInfo( this ) );
    ci->m_Socket = socket;
    ci->m_RemoteAddress = host;
    ci->m_RemotePort = port;
    ci->m_ThreadQuitNotification = false;
    ci->m_UserData = userData;

    #ifdef TCPCONNECTION_DEBUG
        AStackString<32> addr;
        GetAddressAsString( ci->m_RemoteAddress, addr );
        TCPDEBUG( "Connected to %s : %i (%x)\n", addr.Get(), port, (uint32_t)socket );
    #endif

    // Spawn thread to handle socket
    Thread::ThreadHandle h = Thread::CreateThread( &ConnectionThreadWrapperFunction,
                                                   "TCPConnection",
                                                   ( 64 * KILOBYTE ),
                                                   ci ); // user data argument
    ASSERT( h != INVALID_THREAD_HANDLE );
    Thread::DetachThread( h );
    Thread::CloseHandle( h ); // we don't need this anymore

    m_Connections.Append( ci );

    return ci;
}

// ConnectionThreadWrapperFunction
//------------------------------------------------------------------------------
/*static*/ uint32_t TCPConnectionPool::ConnectionThreadWrapperFunction( void * data )
{
    TCP_CONNECTION_POOL_PROFILE_SET_THREAD_NAME( TCPConnectionPoolProfileHelper::THREAD_CONNECTION );
    PROFILE_FUNCTION;

    ConnectionInfo * ci = (ConnectionInfo *)data;
    ci->m_TCPConnectionPool->ConnectionThreadFunction( ci );
    return 0;
}

// ConnectionThreadFunction
//------------------------------------------------------------------------------
void TCPConnectionPool::ConnectionThreadFunction( ConnectionInfo * ci )
{
    ASSERT( ci );
    ASSERT( ci->m_Socket != INVALID_SOCKET );

    OnConnected( ci ); // Do callback

    // process socket events
    while ( AtomicLoadAcquire( &ci->m_ThreadQuitNotification ) == false )
    {
        // timout for select() operations
        // (modified by select, so we must recreate it)
        struct timeval timeout;
        timeout.tv_sec = 0;
        timeout.tv_usec = 10000; // 10 ms

        // create a socket set (with just our socket in it)
        // (modified by the select() function, so we must recreate it)
        fd_set readSet;
        FD_ZERO( &readSet );
        PRAGMA_DISABLE_PUSH_MSVC( 4548 ) // warning C4548: expression before comma has no effect; expected expression with side-effect
        PRAGMA_DISABLE_PUSH_MSVC( 6319 ) // warning C6319: Use of the comma-operator in a tested expression...
        PRAGMA_DISABLE_PUSH_CLANG_WINDOWS( "-Wcomma" ) // possible misuse of comma operator here [-Wcomma]
        PRAGMA_DISABLE_PUSH_CLANG_WINDOWS( "-Wunused-value" ) // expression result unused [-Wunused-value]
        FD_SET( (uint32_t)ci->m_Socket, &readSet );
        PRAGMA_DISABLE_POP_CLANG_WINDOWS // -Wunused-value
        PRAGMA_DISABLE_POP_CLANG_WINDOWS // -Wcomma
        PRAGMA_DISABLE_POP_MSVC // C6319
        PRAGMA_DISABLE_POP_MSVC // 4548

        int num = Select( ci->m_Socket + 1, &readSet, nullptr, nullptr, &timeout );
        if ( num == 0 )
        {
            // timeout expired - loop again (checking quit notification)
            continue;
        }

        if ( AtomicLoadAcquire( &ci->m_ThreadQuitNotification ) )
        {
            break; // don't bother reading any pending data if shutting down
        }

        // Something happened, work out what it is
        if ( FD_ISSET( ci->m_Socket, &readSet ) )
        {
            if ( HandleRead( ci ) == false )
            {
                break;
            }
        }
        else
        {
            PRAGMA_DISABLE_PUSH_CLANG_WINDOWS( "-Wunreachable-code" ) // TOBO:B : Investigate - code will never be executed [-Wunreachable-code]
            ASSERT( false && "Unexpected" );
            PRAGMA_DISABLE_POP_CLANG_WINDOWS // -Wunreachable-code
        }
    }

    OnDisconnected( ci ); // Do callback

    // close the socket
    CloseSocket( ci->m_Socket );
    ci->m_Socket = INVALID_SOCKET;

    {
        // try to remove from connection list
        // could validly be removed by another
        // thread already due to simultaneously
        // closing a connection while it is dropped
        MutexHolder mh( m_ConnectionsMutex );
        ConnectionInfo ** iter = m_Connections.Find( ci );
        ASSERT( iter );
        m_Connections.Erase( iter );
        FDELETE ci;
        if ( AtomicLoadRelaxed( &m_ShuttingDown ) )
        {
            m_ShutdownSemaphore.Signal(); // Wake main thread which will be waiting on shutdown
        }
    }

    // thread exit
    TCPDEBUG( "connection thread exited\n" );
}

// AllowSocketReuse
//------------------------------------------------------------------------------
void TCPConnectionPool::AllowSocketReuse( TCPSocket socket ) const
{
    static const int yes = 1;
    int ret = setsockopt( socket, SOL_SOCKET, SO_REUSEADDR, (const char *)&yes, sizeof( yes ) );
    if ( ret != 0 )
    {
        TCPDEBUG( "setsockopt(SO_REUSEADDR) failed. Error: %s\n", LAST_NETWORK_ERROR_STR );
    }
    #if defined( __APPLE__ )
        // OS X changed the behavior or ADDR vs PORT, so we set both
        ret = setsockopt( socket, SOL_SOCKET, SO_REUSEPORT, (const char *)&yes, sizeof( yes ) );
        if ( ret != 0 )
        {
            TCPDEBUG( "setsockopt(SO_REUSEADDR) failed. Error: %s\n", LAST_NETWORK_ERROR_STR );
        }
    #endif
}

// DisableNagle
//------------------------------------------------------------------------------
void TCPConnectionPool::DisableNagle( TCPSocket socket ) const
{
    // disable TCP nagle
    static const int disableNagle = 1;
    const int ret = setsockopt( socket, IPPROTO_TCP, TCP_NODELAY, (const char *)&disableNagle, sizeof( disableNagle ) );
    if ( ret != 0 )
    {
        TCPDEBUG( "setsockopt(TCP_NODELAY) failed. Error: %s\n", LAST_NETWORK_ERROR_STR );
    }
}

// DisableSigPipe
//------------------------------------------------------------------------------
void TCPConnectionPool::DisableSigPipe( TCPSocket socket ) const
{
    // We handle socket errors at the point of interacting with the socket
    // We don't want the default behaviour of sig pipe errors in any arbitrary
    // kernel function we might call

    #if defined( __WINDOWS__ )
        // Nothing to do on Windows
        (void)socket;
    #elif defined( __LINUX__ )
        // We disable SIGPIPE system wide in NetworkStartupHelper
        (void)socket;
    #elif defined( __APPLE__ )
        // Must be done on every socket on OSX
        int nosigpipe = 1;
        VERIFY( setsockopt( socket, SOL_SOCKET, SO_NOSIGPIPE, (void *)&nosigpipe, sizeof(int) ) == 0 );
    #endif
}

// SetLargeBufferSizes
//------------------------------------------------------------------------------
void TCPConnectionPool::SetLargeBufferSizes( TCPSocket socket ) const
{
    #if defined( __APPLE__ )
        const uint32_t bufferSize = ( 5 * 1024 * 1024 ); // larger values fail on OS X
    #else
        const uint32_t bufferSize = ( 10 * 1024 * 1024 );
    #endif

    // Receive Buffer
    {
        int ret = setsockopt( socket, SOL_SOCKET, SO_RCVBUF, (const char *)&bufferSize, sizeof( bufferSize ) );
        if ( ret != 0 )
        {
            TCPDEBUG( "setsockopt(SO_RCVBUF) failed. Error: %s\n", LAST_NETWORK_ERROR_STR );
        }
    }

    // Send Buffer
    {
        int ret = setsockopt( socket, SOL_SOCKET, SO_SNDBUF, (const char *)&bufferSize, sizeof( bufferSize ) );
        if ( ret != 0 )
        {
            TCPDEBUG( "setsockopt(SO_SNDBUF) failed. Error: %s\n", LAST_NETWORK_ERROR_STR );
        }
    }
}

// SetNonBlocking
//------------------------------------------------------------------------------
void TCPConnectionPool::SetNonBlocking( TCPSocket socket ) const
{
    u_long nonBlocking = 1;
    #if defined( __WINDOWS__ )
        VERIFY( ioctlsocket( socket, (long)FIONBIO, &nonBlocking ) == 0 );
    #else
        VERIFY( ioctl( socket, FIONBIO, &nonBlocking ) == 0 );
    #endif
}

//------------------------------------------------------------------------------
