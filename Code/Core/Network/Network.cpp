// Network
//------------------------------------------------------------------------------

// Includes
//------------------------------------------------------------------------------
#include "Network.h"

// Core
#include "Core/Network/NetworkStartupHelper.h"
#include <Core/Process/Semaphore.h>
#include "Core/Process/Thread.h"
#include "Core/Profile/Profile.h"
#include "Core/Strings/AStackString.h"
#include "Core/Strings/AString.h"

// system
#if defined( __WINDOWS__ )
    #include "Core/Env/WindowsHeader.h"
#endif
#if defined( __LINUX__ ) || defined( __APPLE__ )
    #include <arpa/inet.h>
    #include <netdb.h>
    #include <string.h>
    #include <unistd.h>
#endif

const uint32_t Network::InvalidAddress = 0;
const in6_addr Network::InvalidAddress6 = {{{ 0 }}};

const uint32_t Network::LoopbackAddress = 0x0100007f;
const in6_addr Network::LoopbackAddress6 = {{{ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01 }}};

struct NameResolutionData
{
    AStackString<>  hostName;
    uint32_t        address;
    in6_addr        address6;
    Semaphore       completed;
    Semaphore       safeToFree;
    bool            resolve4and6;
    bool            success;
};

/*static*/ bool Network::IsValidAddress( const uint32_t ipAddress )
{
    return ipAddress != Network::InvalidAddress;
}
/*static*/ bool Network::IsValidAddress( const in6_addr & ipAddress )
{
    return memcmp( &Network::InvalidAddress6, &ipAddress, sizeof( in6_addr ) ) != 0;
}

/*static*/ bool Network::IsLoopbackAddress( const uint32_t ipAddress )
{
    return ipAddress == Network::LoopbackAddress;
}

/*static*/ bool Network::IsLoopbackAddress( const in6_addr & ipAddress )
{
    return memcmp( &Network::LoopbackAddress6, &ipAddress, sizeof( Network::LoopbackAddress6 ) ) == 0;
}

// GetHostName
//------------------------------------------------------------------------------
/*static*/ void Network::GetHostName( AString & hostName )
{
    PROFILE_FUNCTION

    NetworkStartupHelper nsh; // ensure network is up if not already

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
    PROFILE_FUNCTION

    NetworkStartupHelper nsh; // ensure network is up if not already

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

// GetHostIPFromName
//------------------------------------------------------------------------------
/*static*/ bool Network::GetHostIPFromName( const AString & hostName, uint32_t & ipAddress, in6_addr & ipAddress6, const uint32_t timeoutMS, const bool resolve4and6)
{
    PROFILE_FUNCTION

    ipAddress = Network::InvalidAddress;
    ipAddress6 = Network::InvalidAddress6;

    // Fast path for "localhost". Although we have a fast path for detecting ip4 or ip6
    // format addresses, it can still take several ms to call
    if ( hostName == "localhost" )
    {
        ipAddress = Network::LoopbackAddress;
        ipAddress6 = Network::LoopbackAddress6;
        return true;
    }
    else if ( hostName == "127.0.0.1" ) // Fast path for ipv4 localhost only
    {
        ipAddress = Network::LoopbackAddress;
        return true;
    }
    else if ( hostName == "::1" ) // Fast path for ipv6 localhost only
    {
        ipAddress6 = Network::LoopbackAddress6;
        return true;
    }

    // see if string is already in ipv4 format
    if ( GetAddressFromString( hostName, ipAddress ) )
    {
        return true;
    }

    // see if string is already in ipv6 format
    if ( GetAddressFromString( hostName, ipAddress6 ) )
    {
        return true;
    }

    // Perform name resolution on another thread

    // Data to communicate between threads, will be deleted by background thread
    NameResolutionData * data = FNEW( NameResolutionData() );
    data->success = false;
    data->resolve4and6 = resolve4and6;
    data->address = Network::InvalidAddress;
    data->address6 = Network::InvalidAddress6;
    data->hostName = hostName;

    // Create thread to perform resolution
    Thread::ThreadHandle handle = Thread::CreateThread( NameResolutionThreadFunc,
                                                        "NameResolution",
                                                        ( 32 * KILOBYTE ),
                                                        data );

    // wait for name resolution with timeout
    bool timedOut( false );
    const uint32_t sleepInterval( 100 ); // Check exit condition periodically - TODO:C would be better to use an event
    uint32_t remainingTimeMS( Math::Max( timeoutMS, sleepInterval ) );
    for ( ;; )
    {
        timedOut = !data->completed.Wait( sleepInterval );

        // Are we shutting down?
        if ( NetworkStartupHelper::IsShuttingDown() )
        {
            timedOut = true; // ignore whatever we may have gotten back
            break;
        }

        // Manage timeout
        if ( timedOut )
        {
            if ( timeoutMS == 0 )
            {
                continue; // Never timeout
            }
            if ( remainingTimeMS < sleepInterval )
            {
                break; // timeout hit
            }

            remainingTimeMS -= sleepInterval;
            continue; // keep waiting
        }

        break; // success!
    }
    Thread::DetachThread( handle ); // Always detach

    bool success = !timedOut && data->success;
    // Save results if not timed out
    if ( success )
    {
        ipAddress = data->address;
        ipAddress6 = data->address6;
    }

    // Signal that the background thread is safe to free memory and exit
    data->safeToFree.Signal();
    data = nullptr;

    // Wait for thread to terminate if not timed out to free memory (prevents false positive unit-test memory leak)
    if ( !timedOut )
    {
        Thread::WaitForThread( handle );
    }
    Thread::CloseHandle( handle );

    // return if the name was resolved successfully or not
    return success;
}

// GetAddressAsInt
//------------------------------------------------------------------------------
/*static*/ bool Network::GetAddressFromString( const AString & address, uint32_t & ipAddress )
{
    PROFILE_FUNCTION

    return ::inet_pton( AF_INET, address.Get(), &ipAddress ) == 1;
}

// GetAddressAsInt
//------------------------------------------------------------------------------
/*static*/ bool Network::GetAddressFromString( const AString & address, in6_addr & ipAddress )
{
    PROFILE_FUNCTION

    return ::inet_pton( AF_INET6, address.Get(), &ipAddress ) == 1;
}

// GetAddressAsString
//------------------------------------------------------------------------------
/*static*/ bool Network::GetAddressAsString( const uint32_t ipAddress, AString & address )
{
    PROFILE_FUNCTION

    char buffer[INET_ADDRSTRLEN];
    if ( ::inet_ntop( AF_INET, &ipAddress, buffer, sizeof( buffer ) ) )
    {
        address.Assign( buffer );
        return true;
    }
    return false;
}

// GetAddressAsString
//------------------------------------------------------------------------------
/*static*/ bool Network::GetAddressAsString( const in6_addr & ipAddress, AString & address )
{
    PROFILE_FUNCTION

    char buffer[INET6_ADDRSTRLEN];
    if ( ::inet_ntop( AF_INET6, &ipAddress, buffer, sizeof( buffer ) ) )
    {
        address.Assign( buffer );
        return true;
    }
    return false;
}

// NameResolution
//------------------------------------------------------------------------------
/*static*/ bool Network::NameResolution( const AString & hostName, uint32_t & ipAddress )
{
    PROFILE_FUNCTION

    // We want IPv4
    bool success( false );
    struct addrinfo hints;
    memset( &hints, 0, sizeof( hints ) );
    hints.ai_family = AF_INET;

    // Try to resolve
    struct addrinfo * result( nullptr );
    if ( ::getaddrinfo( hostName.Get(), nullptr, &hints, &result ) == 0 )
    {
        if ( result )
        {
            const sockaddr_in * sockaddr_ipv4 = (sockaddr_in *)result->ai_addr;
            ipAddress = sockaddr_ipv4->sin_addr.s_addr;
            success = true;
        }
    }
    ::freeaddrinfo( result );
    return success;
}

// NameResolution
//------------------------------------------------------------------------------
/*static*/ bool Network::NameResolution( const AString & hostName, in6_addr & ipAddress )
{
    PROFILE_FUNCTION

    // We want IPv6
    bool success( false );
    struct addrinfo hints;
    memset( &hints, 0, sizeof( hints ) );
    hints.ai_family = AF_INET6;

    // Try to resolve
    struct addrinfo * result( nullptr );
    if ( ::getaddrinfo( hostName.Get(), nullptr, &hints, &result ) == 0 )
    {
        if ( result )
        {
            const sockaddr_in6 * sockaddr_ipv6 = (sockaddr_in6 *)result->ai_addr;
            ipAddress = sockaddr_ipv6->sin6_addr;
            success = true;
        }
    }
    ::freeaddrinfo( result );
    return success;
}


// NameResolutionThreadFunc
//------------------------------------------------------------------------------
/*static*/ uint32_t Network::NameResolutionThreadFunc( void * userData )
{
    PROFILE_SET_THREAD_NAME( "DNSResolution" )

    bool success( false );
    {
        PROFILE_FUNCTION

        NetworkStartupHelper helper;  // ensure network is up if not already

        // This thread is responsible for freeing userData
        NameResolutionData * data = (NameResolutionData *)userData;

        // perform ipv4 lookup first
        success = Network::NameResolution( data->hostName, data->address );

        // perform ipv6 lookup only if ipv4 fails to return a result, unless requested
        if ( !success || data->resolve4and6 )
        {
            success = Network::NameResolution( data->hostName, data->address6 ) || success;
        }

        data->success = success;

        // Mark background work as completed
        data->completed.Signal();

        // Wait until main thread is finished with shared memory
        data->safeToFree.Wait();

        // Free userData
        FDELETE( data );
    }
    return (uint32_t)( success ? 0 : 1 );
}

//------------------------------------------------------------------------------


