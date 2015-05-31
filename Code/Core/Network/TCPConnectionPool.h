// TCPConnectionPool
//------------------------------------------------------------------------------
#pragma once
#ifndef CORE_NETWORK_TCPCONNECTIONPOOL_H
#define CORE_NETWORK_TCPCONNECTIONPOOL_H

// Includes
//------------------------------------------------------------------------------
#include "NetworkStartupHelper.h"

#include "Core/Env/Types.h"
#include "Core/Containers/Array.h"
#include "Core/Process/Mutex.h"
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
	ConnectionInfo( TCPConnectionPool * ownerPool );

	// users can store connection associate information
	void SetUserData( void * userData ) const { m_UserData = userData; }
	void * GetUserData() const { return m_UserData; }

	// access to info about this connection
	TCPConnectionPool & GetTCPConnectionPool() const { return *m_TCPConnectionPool; }
	inline uint32_t GetRemoteAddress() const { return m_RemoteAddress; }

private:
	friend class TCPConnectionPool;

	TCPSocket				m_Socket;
	uint32_t				m_RemoteAddress;
	uint16_t				m_RemotePort;
	volatile mutable bool	m_ThreadQuitNotification;
	TCPConnectionPool *		m_TCPConnectionPool; // back pointer to parent pool
	mutable void *			m_UserData;

#ifdef DEBUG
	mutable bool			m_InUse; // sanity check we aren't sending from multiple threads unsafely
#endif
};

// TCPConnectionPool
//------------------------------------------------------------------------------
class TCPConnectionPool
{
public:
    TCPConnectionPool();
    virtual ~TCPConnectionPool();

	// derived classes must call this from their destructor if they rely on virtual callbacks
	void ShutdownAllConnections();

    // manage connections
    bool Listen( uint16_t port );
	void StopListening();
    const ConnectionInfo * Connect( const AString & host, uint16_t port, uint32_t timeout = 2000 );
	const ConnectionInfo * Connect( uint32_t hostIP, uint16_t port, uint32_t timeout = 2000 );
    void Disconnect( const ConnectionInfo * ci );
	void SetShuttingDown() { m_ShuttingDown = true; }

	// query connection state
	size_t GetNumConnections() const;

    // transmit data
    bool Send( const ConnectionInfo * connection, const void * data, size_t size, uint32_t timeoutMS = 2000 );
    bool Broadcast( const void * data, size_t size );

	static void GetAddressAsString( uint32_t addr, AString & address );

protected:
    // network events - NOTE: these happen in another thread!
    virtual void OnReceive( const ConnectionInfo *, void * /*data*/, uint32_t /*size*/, bool & /*keepMemory*/ ) {}
    virtual void OnConnected( const ConnectionInfo * ) {}
    virtual void OnDisconnected( const ConnectionInfo * ) {}

	// derived class can provide custom memory allocation if desired
    virtual void * AllocBuffer( uint32_t size );
    virtual void FreeBuffer( void * data );

private:
    // helper functions
    bool        HandleRead( ConnectionInfo * ci );

    // platform specific abstraction
    int         GetLastError() const;
    bool        WouldBlock() const;
    int         CloseSocket( TCPSocket socket ) const;
    int			Select( TCPSocket maxSocketPlusOne,
                    	void * readSocketSet, // TODO: Using void * to avoid including header is ugly
	                void * writeSocketSet,
                    	void * exceptionSocketSet,
                        struct timeval * timeOut ) const;
    TCPSocket   Accept( TCPSocket socket,
                        struct sockaddr * address,
                        int * addressSize ) const;

    // thread management
    void                CreateListenThread( TCPSocket socket, uint32_t host, uint16_t port );
    static uint32_t     ListenThreadWrapperFunction( void * data );
    void                ListenThreadFunction( ConnectionInfo * ci );
	ConnectionInfo *	CreateConnectionThread( TCPSocket socket, uint32_t host, uint16_t port );
    static uint32_t     ConnectionThreadWrapperFunction( void * data );
	void                ConnectionThreadFunction( ConnectionInfo * ci );

    // internal helpers
    bool                DisableNagle( TCPSocket sockfd );

    // listen socket related info
	ConnectionInfo *			m_ListenConnection;

    // remote connection related info
	mutable Mutex				m_ConnectionsMutex;
	Array< ConnectionInfo * >	m_Connections;

	bool						m_ShuttingDown;

	// object to manage network subsystem lifetime
    NetworkStartupHelper m_EnsureNetworkStarted;
};

//------------------------------------------------------------------------------
#endif
