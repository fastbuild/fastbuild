// Network
//------------------------------------------------------------------------------

// Includes
//------------------------------------------------------------------------------
#include "Core/PrecompiledHeader.h"

#include "Network.h"

// Core
#include "Core/Strings/AString.h"
#include "Core/Network/NetworkStartupHelper.h"
#include "Core/Process/Thread.h"
#include "Core/Profile/Profile.h"

// system
#if defined( __WINDOWS__ )
    #include <Winsock2.h>
    #include <ws2tcpip.h>
#endif
#if defined( __LINUX__ ) || defined( __APPLE__ )
    #include <arpa/inet.h>
    #include <netdb.h>
    #include <string.h>
    #include <unistd.h>
#endif

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
    }
    else
    {
        ASSERT( false && "This should never fail" );
        hostName = "Unknown";
    }
}

// GetHostIPFromName
//------------------------------------------------------------------------------
/*static*/ uint32_t Network::GetHostIPFromName( const AString & hostName, uint32_t timeoutMS )
{
    PROFILE_FUNCTION

    // see if string it already in ip4 format
    uint32_t ip = inet_addr( hostName.Get() );
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
    Thread::ThreadHandle handle = Thread::CreateThread( NameResolutionThreadFunc,
                                                        "NameResolution",
                                                        ( 32 * KILOBYTE ),
                                                        &data );

    // wait for name resolution with timeout
    bool timedOut( false );
    int returnCode( 0 );
    uint32_t remainingTimeMS( timeoutMS );
    const uint32_t sleepInterval( 100 ); // Check exit condition periodically - TODO:C would be better to use an event
    for (;;)
    {
        returnCode = Thread::WaitForThread( handle, sleepInterval, timedOut );

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
        Thread::DetachThread( handle );
    }
    Thread::CloseHandle( handle );

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
    PROFILE_SET_THREAD_NAME( "DNSResolution" )

    uint32_t ip( 0 );
    {
        PROFILE_FUNCTION

        NetworkStartupHelper helper;

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
            PROFILE_SECTION( "::getaddrinfo" )

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
                    const sockaddr_in * sockaddr_ipv4 = (sockaddr_in *)result->ai_addr;
                    ip = sockaddr_ipv4->sin_addr.s_addr;
                }
            }
            ::freeaddrinfo( result );
        }
    }
    return ip;
}

//------------------------------------------------------------------------------
