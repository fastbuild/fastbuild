// TCPConnectionPool
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
// Includes
//------------------------------------------------------------------------------
#include "Core/PrecompiledHeader.h"

#include "TCPConnectionPool.h"

// Core
#include "Core/Mem/Mem.h"
#include "Core/Network/Network.h"
#include "Core/Strings/AString.h"
#include "Core/Strings/AStackString.h"
#include "Core/Profile/Profile.h"
#include "Core/Time/Timer.h"

// System
#if defined( __WINDOWS__ )
    #include <winsock2.h>
    #include <windows.h>
#elif defined( __APPLE__ ) || defined( __LINUX__ )
    #include <string.h>
    #include <errno.h>
    #include <netdb.h>
    #include <arpa/inet.h>
    #include <sys/ioctl.h>
    #include <sys/socket.h>
    #include <netinet/in.h>
    #include <netinet/tcp.h>	
    #include <fcntl.h>
    #include <unistd.h>
	#define INVALID_SOCKET ( -1 )
    #define SOCKET_ERROR -1
#else
    #error Unknown platform
#endif

//------------------------------------------------------------------------------
// For Debugging
//------------------------------------------------------------------------------
//#define TCPCONNECTION_DEBUG
#ifdef TCPCONNECTION_DEBUG
    #include "Core/Tracing/Tracing.h"
	#define TCPDEBUG( ... ) DEBUGSPAM( __VA_ARGS__ )
#else
	#define TCPDEBUG( ... )
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
	m_ShuttingDown = true;
	ShutdownAllConnections();
}

// ShutdownAllConnections
//------------------------------------------------------------------------------
void TCPConnectionPool::ShutdownAllConnections()
{
    PROFILE_FUNCTION

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
		for ( size_t i=0; i<m_Connections.GetSize(); ++i )
		{
			ConnectionInfo * ci = m_Connections[ i ];
			Disconnect( ci );
		}

		m_ConnectionsMutex.Unlock();
        Thread::Sleep( 1 );
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
    TCPSocket sockfd = socket( AF_INET, SOCK_STREAM, 0 );
    if ( sockfd == INVALID_SOCKET )
    {
        TCPDEBUG( "Create socket failed (Listen): %i\n", GetLastError() );
		return false;
    }

    // allow socket re-use
    static const int yes = 1;
    int ret = setsockopt( sockfd, SOL_SOCKET, SO_REUSEADDR, (const char *)&yes, sizeof( yes ) );
    if ( ret != 0 )
    {
        TCPDEBUG( "setsockopt failed: %i\n", GetLastError() );
        CloseSocket( sockfd );
        return false;
    }

	if ( !DisableNagle( sockfd ) )
	{
        return false; // DisableNagle will close socket
	}

    // set up the listen params
    struct sockaddr_in addrInfo;
    memset( &addrInfo, 0, sizeof( addrInfo ) );
    addrInfo.sin_family = AF_INET;
    addrInfo.sin_port = htons( port );
    addrInfo.sin_addr.s_addr = INADDR_ANY;

    // bind
    if ( bind( sockfd, (struct sockaddr *)&addrInfo, sizeof( addrInfo ) ) != 0 )
    {
        TCPDEBUG( "Bind failed: %i\n", GetLastError() );
        CloseSocket( sockfd );
        return false;
    }

    // listen
    TCPDEBUG( "Listen on port %i (%x)\n", port, sockfd );
    if ( listen( sockfd, 0 ) == SOCKET_ERROR ) // no backlog
	{
	    TCPDEBUG( "Listen FAILED %i (%x)\n", port, sockfd );
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
const ConnectionInfo * TCPConnectionPool::Connect( const AString & host, uint16_t port, uint32_t timeout )
{
	ASSERT( !host.IsEmpty() );

    // get IP
    uint32_t hostIP = Network::GetHostIPFromName( host, timeout );
	if( hostIP == 0 )
	{
		TCPDEBUG( "Failed to get address for '%s'\n" , host.Get() );
		return nullptr;
	}
	return Connect( hostIP, port, timeout );
}

// Connect
//------------------------------------------------------------------------------
const ConnectionInfo * TCPConnectionPool::Connect( uint32_t hostIP, uint16_t port, uint32_t timeout )
{
    // create a socket
    TCPSocket sockfd = socket( AF_INET, SOCK_STREAM, 0 );

    // outright failure?
    if ( sockfd == INVALID_SOCKET )
    {
        TCPDEBUG( "Create socket failed (Connect): %i\n", GetLastError() );
        return nullptr;
    }

	// set send/recv timeout
	#if defined( __APPLE__ )
		uint32_t bufferSize = ( 7 * 1024 * 1024 ); // larger values fail on OS X
	#else
		uint32_t bufferSize = ( 10 * 1024 * 1024 );
	#endif
    int ret = setsockopt( sockfd, SOL_SOCKET, SO_RCVBUF, (const char *)&bufferSize, sizeof( bufferSize ) );
    if ( ret != 0 )
    {
        TCPDEBUG( "setsockopt SO_RCVBUF failed: %i\n", GetLastError() );
	    CloseSocket( sockfd );
		return nullptr;
    }
    ret = setsockopt( sockfd, SOL_SOCKET, SO_SNDBUF, (const char *)&bufferSize, sizeof( bufferSize ) );
    if ( ret != 0 )
    {
        TCPDEBUG( "setsockopt SO_SNDBUF failed: %i\n", GetLastError() );
	    CloseSocket( sockfd );
		return nullptr;
    }

	if ( !DisableNagle( sockfd ) )
	{
        return nullptr; // DisableNagle will close socket
	}

    // we have a socket now
    //m_Socket = sockfd;

	// set non-blocking
	u_long nonBlocking = 1;
    #if defined( __WINDOWS__ )
        ioctlsocket( sockfd, FIONBIO, &nonBlocking );
    #elif defined( __APPLE__ ) || defined( __LINUX__ )
        VERIFY( ioctl( sockfd, FIONBIO, &nonBlocking ) >= 0 );
    #else
        #error Unknown platform
    #endif

    // setup destination address
    struct sockaddr_in destAddr;
    memset( &destAddr, 0, sizeof( destAddr ) );
    destAddr.sin_family = AF_INET;
    destAddr.sin_port = htons( port );
    destAddr.sin_addr.s_addr = hostIP;

    // initiate connection
    if ( connect(sockfd, (struct sockaddr *)&destAddr, sizeof( destAddr ) ) != 0 )
    {
		// we expect WSAEWOULDBLOCK
		if ( !WouldBlock() )
		{
			// connection initiation failed
			#ifdef TCPCONNECTION_DEBUG
				AStackString<> host;
				GetAddressAsString( hostIP, host );
				TCPDEBUG( "connect() failed: %i (host:%s port:%u)\n", GetLastError(), host.Get(), port );
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
		PRAGMA_DISABLE_PUSH_MSVC( 6319 ) // warning C6319: Use of the comma-operator in a tested expression...
		FD_SET( sockfd, &write );
		FD_SET( sockfd, &err );
		PRAGMA_DISABLE_POP_MSVC // 6319
 
		// check connection every 10ms
		timeval pollingTimeout;
		memset( &pollingTimeout, 0, sizeof( timeval ) );
		pollingTimeout.tv_usec = 10 * 1000;

		// check if the socket is ready
		int selRet = Select( sockfd+1, nullptr, &write, &err, &pollingTimeout );
		if ( selRet == SOCKET_ERROR )
		{
			// connection failed
			#ifdef TCPCONNECTION_DEBUG
				AStackString<> host;
				GetAddressAsString( hostIP, host );
				TCPDEBUG( "select() after connect() failed: %i (host:%s port:%u)\n", GetLastError(), host.Get(), port );
			#endif
			CloseSocket( sockfd );
			return nullptr;
		}

		// polling timeout hit?
		if ( selRet == 0 )
		{
			// are we shutting down?
			if ( m_ShuttingDown )
			{
				#ifdef TCPCONNECTION_DEBUG
					AStackString<> host;
					GetAddressAsString( hostIP, host );
					TCPDEBUG( "connect() aborted (Shutting Down) (host:%s port:%u)\n", host.Get(), port );
				#endif
				CloseSocket( sockfd );
				return nullptr;
			}

			// have we hit our real connection timeout?
			if ( connectionTimer.GetElapsedMS() >= timeout )
			{
				#ifdef TCPCONNECTION_DEBUG
					AStackString<> host;
					GetAddressAsString( hostIP, host );
					TCPDEBUG( "connect() time out %u hit (host:%s port:%u)\n", timeout, host.Get(), port );
				#endif
				CloseSocket( sockfd );
				return nullptr;
			}

			// keep waiting
			continue;
		}

		if( FD_ISSET( sockfd, &err ) ) 
		{	
			// connection failed
			#ifdef TCPCONNECTION_DEBUG
				AStackString<> host;
				GetAddressAsString( hostIP, host );
				TCPDEBUG( "select() after connect() error: %i (host:%s port:%u)\n", GetLastError(), host.Get(), port );
			#endif
			CloseSocket( sockfd );
			return nullptr;
		}

		if( FD_ISSET( sockfd, &write ) ) 
		{	
			break; // connection success!
		}

		ASSERT( false ); // should never get here
	}

	return CreateConnectionThread( sockfd, hostIP, port );
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
		ci->m_ThreadQuitNotification = true;
		return;
	}

	ConnectionInfo ** iter = m_Connections.Find( ci );
	if ( iter != nullptr )
	{
		ci->m_ThreadQuitNotification = true;
		return;
	}

	// connection is no longer valid.... we handle this gracefully
	// as the connection might be lost while trying to disconnect
	// on another thread
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
    PROFILE_FUNCTION

	ASSERT( connection );

	// closing connection, possibly from a previous failure
	if ( connection->m_ThreadQuitNotification || m_ShuttingDown )
	{
		return false;
	}

	Timer timer;

#ifdef DEBUG
	ASSERT( connection->m_InUse == false );
	connection->m_InUse = true;
#endif

	ASSERT( connection->m_Socket != INVALID_SOCKET );

	TCPDEBUG( "Send: %i (%x)\n", size, connection->m_Socket );

	bool sendOK = true;

    // send size of subsequent data
	uint32_t sizeData = (uint32_t)size;
	uint32_t bytesToSend = 4;
	while ( bytesToSend > 0 )
	{
		int sent = (int)send( connection->m_Socket, ( (const char *)&sizeData ) + 4 - bytesToSend, bytesToSend, 0 );
        if ( sent <= 0 )
        {
            if ( WouldBlock() )
            {
				if ( connection->m_ThreadQuitNotification || m_ShuttingDown )
				{
					sendOK = false;
					break;
				}

				if ( timer.GetElapsedMS() > timeoutMS )
				{
					Disconnect( connection );
					sendOK = false;
					break;
				}

                Thread::Sleep( 1 );
                continue;
            }
			// error
			TCPDEBUG( "send error A.  Send: %i (Error: %i) (%x)\n", sent, GetLastError(), connection->m_Socket );
			Disconnect( connection );
			sendOK = false;
			break;
		}
		bytesToSend -= sent;
	}

	// send actual data
	if ( sendOK )
	{
		// loop until we send all data
		size_t bytesRemaining = size;
		const char * dataAsChar = (const char *)data;
		while ( bytesRemaining > 0 )
		{
			int sent = (int)send( connection->m_Socket, dataAsChar, (uint32_t)bytesRemaining, 0 );
			if ( sent <= 0 )
			{
				if ( WouldBlock() )
				{
					if ( connection->m_ThreadQuitNotification || m_ShuttingDown )
					{
						sendOK = false;
						break;
					}

					if ( timer.GetElapsedMS() > timeoutMS )
					{
						Disconnect( connection );
						sendOK = false;
						break;
					}

					Thread::Sleep( 1 );
					continue;
				}
				// error
				TCPDEBUG( "send error B.  Send: %i (Error: %i) (%x)\n", sent, GetLastError(), connection->m_Socket );
				Disconnect( connection );
				sendOK = false;
				break;
			}
			bytesRemaining -= sent;
			dataAsChar += sent;
		}
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
	ConnectionInfo * const * end = m_Connections.End();
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
    PROFILE_FUNCTION

    // work out how many bytes there are
    uint32_t size( 0 );
	uint32_t bytesToRead = 4;
	while ( bytesToRead > 0 )
	{
	    int numBytes = (int)recv( ci->m_Socket, ( (char *)&size ) + 4 - bytesToRead, bytesToRead, 0 );
		if ( numBytes <= 0 )
		{
			if ( WouldBlock() )
			{
				if ( ci->m_ThreadQuitNotification || m_ShuttingDown )
				{
					return false;
				}

                Thread::Sleep( 1 );
				continue;
			}
			TCPDEBUG( "recv error A.  Read: %i (Error: %i) (%x)\n", numBytes, GetLastError(), ci->m_Socket );
			return false;
		}
		bytesToRead -= numBytes;
	}

    TCPDEBUG( "Handle read: %i (%x)\n", size, ci->m_Socket );

    // get output location
    void * buffer = AllocBuffer( size );
    ASSERT( buffer );
    
    // read data into the user supplied buffer
    uint32_t bytesRemaining = size;
    char * dest = (char *)buffer;
    while ( bytesRemaining > 0 )
    {
        int numBytes = (int)recv( ci->m_Socket, dest, bytesRemaining, 0 );
		if ( numBytes <= 0 )
		{
			if ( WouldBlock() )
			{
				if ( ci->m_ThreadQuitNotification || m_ShuttingDown )
				{
					FreeBuffer( buffer );
					return false;
				}

                Thread::Sleep( 1 );
				continue;
			}
			TCPDEBUG( "recv error B.  Read: %i (Error: %i) (%x)\n", numBytes, GetLastError(), ci->m_Socket );
			FreeBuffer( buffer );
			return false;
		}
        bytesRemaining -= numBytes;
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

// GetLastError
//------------------------------------------------------------------------------
int TCPConnectionPool::GetLastError() const
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
    PROFILE_SECTION( "Select" )
    return select( (int)socket, // NOTE: ignored by Windows
                    (fd_set *)a_ReadSocketSet,
                    (fd_set *)a_WriteSocketSet,
                    (fd_set *)a_ExceptionSocketSet,
                    a_TimeOut );
}

// Accept
//------------------------------------------------------------------------------
TCPSocket TCPConnectionPool::Accept( TCPSocket a_Socket,
									 struct sockaddr * a_Address,
									 int * a_AddressSize ) const
{
    #if defined( __WINDOWS__ )
        return accept( a_Socket, a_Address, a_AddressSize );
    #elif defined( __APPLE__ ) || defined( __LINUX__ )
        return accept( a_Socket, a_Address, (unsigned int *)a_AddressSize );
    #endif
}

// CreateThread
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
    ASSERT( h != INVALID_THREAD_HANDLE )
    Thread::CloseHandle( h ); // we don't need this anymore
}

// ThreadWrapperFunction
//------------------------------------------------------------------------------
/*static*/ uint32_t TCPConnectionPool::ListenThreadWrapperFunction( void * data )
{
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

	while ( ci->m_ThreadQuitNotification == false )
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
		PRAGMA_DISABLE_PUSH_MSVC( 6319 ) // warning C6319: Use of the comma-operator in a tested expression...
        FD_SET( (uint32_t)ci->m_Socket, &set );
		PRAGMA_DISABLE_POP_MSVC // 6319

        // peek
        int num = Select( ci->m_Socket+1, &set, NULL, NULL, &timeout );
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
			TCPDEBUG( "accept failed: %i\n", GetLastError() );
			break;
		}

		#ifdef TCPCONNECTION_DEBUG
			AStackString<32> addr;
			GetAddressAsString( remoteAddrInfo.sin_addr.s_addr, addr );
			TCPDEBUG( "Connection accepted from %s : %i (%x)\n",  addr.Get(), ntohs( remoteAddrInfo.sin_port ), newSocket );
		#endif

		// set non-blocking
		u_long nonBlocking = 1;
        #if defined( __WINDOWS__ )
            ioctlsocket( newSocket, FIONBIO, &nonBlocking );
        #elif defined( __APPLE__ ) || defined( __LINUX__ )
            ioctl( newSocket, FIONBIO, &nonBlocking );
        #else
            #error Unknown platform
        #endif
        
		// set send/recv timeout
		#if defined( __APPLE__ )
			uint32_t bufferSize = ( 7 * 1024 * 1024 ); // larger values fail on OS X
		#else
			uint32_t bufferSize = ( 10 * 1024 * 1024 );
		#endif
		int ret = setsockopt( newSocket, SOL_SOCKET, SO_RCVBUF, (const char *)&bufferSize, sizeof( bufferSize ) );
		if ( ret != 0 )
		{
			TCPDEBUG( "setsockopt SO_RCVBUF failed: %i\n", GetLastError() );
			break;
		}
		ret = setsockopt( newSocket, SOL_SOCKET, SO_SNDBUF, (const char *)&bufferSize, sizeof( bufferSize ) );
		if ( ret != 0 )
		{
			TCPDEBUG( "setsockopt SO_SNDBUF failed: %i\n", GetLastError() );
			break;
		}

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
	}

	FDELETE ci;

    // thread exit
    TCPDEBUG( "Listen thread exited\n" );
}

// CreateConnectionThread
//------------------------------------------------------------------------------
ConnectionInfo * TCPConnectionPool::CreateConnectionThread( TCPSocket socket, uint32_t host, uint16_t port )
{
	MutexHolder mh( m_ConnectionsMutex );

	ConnectionInfo * ci = FNEW( ConnectionInfo( this ) );
	ci->m_Socket = socket;
	ci->m_RemoteAddress = host;
	ci->m_RemotePort = port;
	ci->m_ThreadQuitNotification = false;

    #ifdef TCPCONNECTION_DEBUG
        AStackString<32> addr;
        GetAddressAsString( ci->m_RemoteAddress, addr );
        TCPDEBUG( "Connected to %s : %i (%x)\n", addr.Get(), port, socket );
    #endif

	// Spawn thread to handle socket
	Thread::ThreadHandle h = Thread::CreateThread( &ConnectionThreadWrapperFunction,
											"TCPConnection",
											( 32 * KILOBYTE ),
											ci ); // user data argument
    ASSERT( h != INVALID_THREAD_HANDLE )
    Thread::CloseHandle( h ); // we don't need this anymore

	m_Connections.Append( ci );

	return ci;
}

// ConnectionThreadWrapperFunction
//------------------------------------------------------------------------------
/*static*/ uint32_t TCPConnectionPool::ConnectionThreadWrapperFunction( void * data )
{
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
	while ( ci->m_ThreadQuitNotification == false )
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
		PRAGMA_DISABLE_PUSH_MSVC( 6319 ) // warning C6319: Use of the comma-operator in a tested expression...
        FD_SET( (uint32_t)ci->m_Socket, &readSet );
		PRAGMA_DISABLE_POP_MSVC // C6319

        int num = Select( ci->m_Socket+1, &readSet, NULL, NULL, &timeout );
        if ( num == 0 )
        {
            // timeout expired - loop again (checking quit notification)
            continue;
        }

		if ( ci->m_ThreadQuitNotification == true )
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
            ASSERT( false && "Unexpected" );
        }
    }

    OnDisconnected( ci ); // Do callback

    // close the socket
    CloseSocket( ci->m_Socket );
	ci->m_Socket = INVALID_SOCKET;
    //ci->m_Thread = INVALID_THREAD_HANDLE;

	{
		// try to remove from connection list
		// could validly be removed by another
		// thread already due to simultaneously
		// closing a connection while it is dropped
		MutexHolder mh( m_ConnectionsMutex );
		ConnectionInfo ** iter = m_Connections.Find( ci );
		ASSERT( iter );
		m_Connections.Erase( iter );
	}

	FDELETE ci;

    // thread exit
    TCPDEBUG( "connection thread exited\n" );
}

// DisableNagle
//------------------------------------------------------------------------------
bool TCPConnectionPool::DisableNagle( TCPSocket sockfd )
{
    // disable TCP nagle
    static const int disableNagle = 1;
    const int ret = setsockopt( sockfd, IPPROTO_TCP, TCP_NODELAY, (const char *)&disableNagle, sizeof( disableNagle ) );
    if ( ret != 0 )
    {
        TCPDEBUG( "setsockopt TCP_NODELAY failed: %i\n", GetLastError() );
        CloseSocket( sockfd );
        return false;
    }
	return true;
}

//------------------------------------------------------------------------------
