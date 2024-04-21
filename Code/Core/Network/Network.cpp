// Network
//------------------------------------------------------------------------------

// Includes
//------------------------------------------------------------------------------
#include "Network.h"

// Core
#include "Core/Containers/UniquePtr.h"
#include "Core/Network/NetworkStartupHelper.h"
#include "Core/Process/Atomic.h"
#include "Core/Process/Thread.h"
#include "Core/Profile/Profile.h"
#include "Core/Strings/AString.h"

// system
#if defined( __WINDOWS__ )
    #include "Core/Env/WindowsHeader.h"
    #include <iphlpapi.h>
#endif
#if defined( __LINUX__ ) || defined( __APPLE__ )
    #include <arpa/inet.h>
    #include <ifaddrs.h>
    #include <netdb.h>
    #include <string.h>
    #include <unistd.h>
#endif

// GetHostName
//------------------------------------------------------------------------------
/*static*/ void Network::GetHostName( AString & hostName )
{
    PROFILE_FUNCTION;

    const NetworkStartupHelper nsh; // ensure network is up if not already

    char buffer[ 64 ];
    if ( ::gethostname( buffer, 64 ) == 0 )
    {
        hostName = buffer;
        return;
    }

    ASSERT( false && "GetHostName should never fail" );
    hostName = "Unknown";
}

// GetDomainName
//------------------------------------------------------------------------------
/*static*/ void Network::GetDomainName( AString & domainName )
{
    PROFILE_FUNCTION;

    const NetworkStartupHelper nsh; // ensure network is up if not already

    #if defined( __WINDOWS__ )
        TCHAR buffer[ 256 ];
        DWORD bufferSize = sizeof( buffer );
        if ( GetComputerNameEx( ComputerNameDnsDomain, buffer, &bufferSize ) )
        {
            domainName = buffer;
            return;
        }
    #else
        char buffer[ 256 ];
        if ( ::getdomainname( buffer, 256 ) == 0 )
        {
            domainName = buffer;
            return;
        }
    #endif

    ASSERT( false && "GetDomainName should never fail" );
    domainName = "Unknown";
}

// GetIPv4Addresses
//------------------------------------------------------------------------------
/*static*/ void Network::GetIPv4Addresses( Array<AString> & outAddresses )
{
    PROFILE_FUNCTION;

    #if defined( __WINDOWS__ )
        // Find out how big the buffer needs to be (depends on number of adaptors)
        ULONG adapterInfoSize = 0;
        VERIFY( GetAdaptersInfo( nullptr, &adapterInfoSize ) == ERROR_BUFFER_OVERFLOW );

        // Allocate the buffer
        UniquePtr<IP_ADAPTER_INFO, FreeDeletor> buffer( static_cast<IP_ADAPTER_INFO *>( ALLOC( adapterInfoSize ) ) );

        // Fill the buffer
        VERIFY( GetAdaptersInfo( buffer.Get(), &adapterInfoSize ) == ERROR_SUCCESS );

        // Loop through all the adapters
        PIP_ADAPTER_INFO adapter = buffer.Get();
        while ( adapter )
        {
            // Add all IPv4 addresses associated with this adapter
            IP_ADDR_STRING * info = &adapter->IpAddressList;
            while ( info )
            {
                AString ipAddrString( info->IpAddress.String );

                // Filter out disconnected adapters
                if ( ipAddrString != "0.0.0.0" )
                {
                    outAddresses.EmplaceBack( Move( ipAddrString ) );
                }

                info = info->Next;
            }

            adapter = adapter->Next;
        }
    #else
        struct ifaddrs * addresses = nullptr;
        VERIFY( ::getifaddrs( &addresses ) == 0 );

        // Walk linked list
        struct ifaddrs * info = addresses;
        while ( info )
        {
            // Filter out disconnected adapters and non-IPv4 addresses
            if ( info->ifa_addr && ( info->ifa_addr->sa_family == AF_INET ) )
            {
                char host[ NI_MAXHOST ];
                VERIFY( getnameinfo( info->ifa_addr,
                                     sizeof(struct sockaddr_in),
                                     host,
                                     NI_MAXHOST,
                                     nullptr,
                                     0,
                                     NI_NUMERICHOST ) == 0 );
                outAddresses.EmplaceBack( host );
            }

            info = info->ifa_next;
        }

        freeifaddrs( addresses );
    #endif
}

// GetHostIPFromName
//------------------------------------------------------------------------------
/*static*/ uint32_t Network::GetHostIPFromName( const AString & hostName, uint32_t timeoutMS )
{
    PROFILE_FUNCTION;

    // Fast path for "localhost". Although we have a fast path for detecting ip4
    // format addresses, it can still take several ms to call
    if ( hostName == "127.0.0.1" )
    {
        return 0x0100007f;
    }

    // see if string it already in ip4 format
    PRAGMA_DISABLE_PUSH_MSVC( 4996 ) // Deprecated...
    PRAGMA_DISABLE_PUSH_CLANG_WINDOWS( "-Wdeprecated-declarations" ) // 'inet_addr' is deprecated: This function or variable may be unsafe...
    const uint32_t ip = inet_addr( hostName.Get() ); // TODO:C Consider using inet_pton()
    PRAGMA_DISABLE_POP_CLANG_WINDOWS // -Wdeprecated-declarations
    PRAGMA_DISABLE_POP_MSVC // 4996
    if ( ip != INADDR_NONE )
    {
        return ip;
    }

    // Perform name resolution on another thread

    // Data to communicate between threads
    NameResolutionData data;
    data.hostName = hostName;
    data.safeToFree = false; // will be marked by other thread

    // Create thread to perform resolution
    Thread thread;
    thread.Start( NameResolutionThreadFunc, "NameResolution", &data, ( 32 * KILOBYTE ) );

    // wait for name resolution with timeout
    bool timedOut( false );
    uint32_t returnCode( 0 );
    uint32_t remainingTimeMS( timeoutMS );
    const uint32_t sleepInterval( 100 ); // Check exit condition periodically - TODO:C would be better to use an event
    for ( ;; )
    {
        returnCode = thread.JoinWithTimeout( sleepInterval, timedOut ); // TODO:B Remove use of this unsafe API

        // Are we shutting down?
        if ( NetworkStartupHelper::IsShuttingDown() )
        {
            returnCode = 0; // ignore whatever we may have gotten back
            break;
        }

        // Manage timeout
        if ( timedOut )
        {
            if ( remainingTimeMS < sleepInterval )
            {
                break; // timeout hit
            }

            remainingTimeMS -= sleepInterval;
            continue; // keep waiting
        }

        break; // success!
    }
    if ( timedOut )
    {
        thread.Detach(); // TODO:B Remove use of this unsafe API
    }

    // handle race where timeout occurred before thread marked data as
    // safe to delete (this could happen if system was under load and timeout was very small)
    while ( !data.safeToFree )
    {
        Thread::Sleep( 1 );
    }

    if ( timedOut )
    {
        returnCode = 0; // timeout was hit
    }

    // return result of resolution (could also have failed)
    return returnCode;
}

// NameResolutionThreadFunc
//------------------------------------------------------------------------------
/*static*/ uint32_t Network::NameResolutionThreadFunc( void * userData )
{
    PROFILE_SET_THREAD_NAME( "DNSResolution" );

    uint32_t ip( 0 );
    {
        PROFILE_FUNCTION;

        const NetworkStartupHelper helper;

        AStackString<> hostName;

        {
            // take a copy of the HostName
            NameResolutionData * data = (NameResolutionData *)userData;
            hostName = data->hostName;

            // mark userData as safe to delete by calling thread
            MemoryBarrier();
            data->safeToFree = true;
        }

        // perform lookup
        {
            PROFILE_SECTION( "::getaddrinfo" );

            // We want IPv4
            struct addrinfo hints;
            memset( &hints, 0, sizeof(hints) );
            hints.ai_family = AF_INET;

            // Try to resolve
            struct addrinfo * result( nullptr );
            if ( ::getaddrinfo( hostName.Get(), nullptr, &hints, &result ) == 0 )
            {
                if ( result )
                {
                    PRAGMA_DISABLE_PUSH_CLANG_WINDOWS( "-Wcast-align" ) // cast from 'struct sockaddr *' to 'sockaddr_in *' increases required alignment from 2 to 4
                    const sockaddr_in * sockaddr_ipv4 = (sockaddr_in *)result->ai_addr;
                    PRAGMA_DISABLE_POP_CLANG_WINDOWS // -Wcast-align
                    ip = sockaddr_ipv4->sin_addr.s_addr;
                }
            }
            ::freeaddrinfo( result );
        }
    }
    return ip;
}

//------------------------------------------------------------------------------
