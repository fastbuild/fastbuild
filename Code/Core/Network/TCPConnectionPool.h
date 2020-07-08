// TCPConnectionPool
//------------------------------------------------------------------------------
#pragma once

// Includes
//------------------------------------------------------------------------------
#include "NetworkStartupHelper.h"

#include "Core/Containers/Array.h"
#include "Core/Env/Types.h"
#include "Core/Network/Network.h"
#include "Core/Process/Mutex.h"
#include "Core/Process/Semaphore.h"
#include "Core/Process/Thread.h"
#include "Core/Strings/AString.h"

// Forward Declarations
//------------------------------------------------------------------------------
class TCPConnectionPool;

#if defined( __WINDOWS__ )
    typedef uintptr_t TCPSocket;
#elif defined( __APPLE__ ) || defined( __LINUX__ )
    typedef int TCPSocket;
#endif

// ConnectionInfo - one connection in the pool
//------------------------------------------------------------------------------
class ConnectionInfo
{
public:
    explicit ConnectionInfo( TCPConnectionPool * ownerPool );

    // users can store connection associate information
    void SetUserData( void * userData ) const { m_UserData = userData; }
    void * GetUserData() const { return m_UserData; }

    // access to info about this connection
    TCPConnectionPool & GetTCPConnectionPool() const { return *m_TCPConnectionPool; }
    void GetRemoteAddressString( AString & remoteAddress ) const;

private:
    friend class TCPConnectionPool;

    TCPSocket                      m_Socket;
    Network::IpAddress             m_RemoteAddress;
    AutoPtr< Network::IpAddress6 > m_RemoteAddress6;
    uint16_t                       m_RemotePort;
    volatile mutable bool          m_ThreadQuitNotification;
    TCPConnectionPool *            m_TCPConnectionPool; // back pointer to parent pool
    mutable void *                 m_UserData;

#ifdef DEBUG
    mutable bool                   m_InUse; // sanity check we aren't sending from multiple threads unsafely
#endif
};

// TCPConnectionPool
//------------------------------------------------------------------------------
class TCPConnectionPool
{
public:
    TCPConnectionPool();
    virtual ~TCPConnectionPool();

    // Must be called explicitly before destruction
    void ShutdownAllConnections();

    // manage connections
    bool Listen( uint16_t port );
    void StopListening();
    const ConnectionInfo * Connect( const AString & host, const uint16_t port, const uint32_t timeout = 2000, void * userData = nullptr );
    const ConnectionInfo * Connect( const Network::IpAddress hostIP, const uint16_t port, const uint32_t timeout = 2000, void * userData = nullptr );
    const ConnectionInfo * Connect( const Network::IpAddress6 & hostIP, const uint16_t port, const uint32_t timeout = 2000, void * userData = nullptr );
    void Disconnect( const ConnectionInfo * ci );
    void SetShuttingDown();

    // query connection state
    size_t GetNumConnections() const;

    // transmit data
    bool Send( const ConnectionInfo * connection, const void * data, size_t size, uint32_t timeoutMS = 30000 );
    bool Send( const ConnectionInfo * connection, const void * data, size_t size, const void * payloadData, size_t payloadSize, uint32_t timeoutMS = 30000 );
    bool Broadcast( const void * data, size_t size );

protected:
    // network events - NOTE: these happen in another thread! (but never at the same time)
    virtual void OnReceive( const ConnectionInfo *, void * /*data*/, uint32_t /*size*/, bool & /*keepMemory*/ ) {}
    virtual void OnConnected( const ConnectionInfo * ) {}
    virtual void OnDisconnected( const ConnectionInfo * ) {}

    // derived class can provide custom memory allocation if desired
    virtual void * AllocBuffer( uint32_t size );
    virtual void FreeBuffer( void * data );

private:
    // helper functions
    bool        WaitForConnection( TCPSocket sockfd, const AString & host, const uint16_t port, const uint32_t timeout );
    bool        HandleRead( ConnectionInfo * ci );

    // platform specific abstraction
    int         GetLastNetworkError() const;
    bool        WouldBlock() const;
    int         CloseSocket( TCPSocket socket ) const;
    int         Select( TCPSocket maxSocketPlusOne,
                        void * readSocketSet, // TODO: Using void * to avoid including header is ugly
                        void * writeSocketSet,
                        void * exceptionSocketSet,
                        struct timeval * timeOut ) const;
    TCPSocket   Accept( TCPSocket socket,
                        struct sockaddr * address,
                        int * addressSize ) const;
    TCPSocket   CreateSocket( int af = AF_INET ) const;

    struct SendBuffer
    {
        uint32_t        size;
        const void *    data;
    };
    bool        SendInternal( const ConnectionInfo * connection, const SendBuffer * buffers, uint32_t numBuffers, uint32_t timeoutMS );

    // thread management
    void                CreateListenThread( TCPSocket socket, uint32_t host, uint16_t port );
    static uint32_t     ListenThreadWrapperFunction( void * data );
    void                ListenThreadFunction( ConnectionInfo * ci );
    ConnectionInfo *    CreateConnectionThread( TCPSocket socket, Network::IpAddress host, uint16_t port, void * userData = nullptr );
    ConnectionInfo *    CreateConnectionThread( TCPSocket socket, const Network::IpAddress6 & host, uint16_t port, void * userData = nullptr );
    static uint32_t     ConnectionThreadWrapperFunction( void * data );
    void                ConnectionThreadFunction( ConnectionInfo * ci );

    // internal helpers
    void                AllowSocketReuse( TCPSocket socket ) const;
    void                DisableNagle( TCPSocket socket ) const;
    void                DisableSigPipe( TCPSocket socket ) const;
    void                DisableV6Only( TCPSocket socket ) const;
    void                SetLargeBufferSizes( TCPSocket socket ) const;
    void                SetNonBlocking( TCPSocket socket ) const;

    // listen socket related info
    ConnectionInfo *            m_ListenConnection;

    // remote connection related info
    mutable Mutex               m_ConnectionsMutex;
    Array< ConnectionInfo * >   m_Connections;

    bool                        m_ShuttingDown;
    Semaphore                   m_ShutdownSemaphore;

    // object to manage network subsystem lifetime
protected:
    NetworkStartupHelper m_EnsureNetworkStarted;
};

//------------------------------------------------------------------------------
