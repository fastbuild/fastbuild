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
#endif
#if defined( __LINUX__ ) || defined( __APPLE__ )
    #include <arpa/inet.h>
    #include <netdb.h>
    #include <unistd.h>
#endif

// Static
//------------------------------------------------------------------------------
/*static*/ Mutex Network::s_GetHostByNameMutex;

// GetHostName
//------------------------------------------------------------------------------
/*static*/ void Network::GetHostName( AString & hostName )
{
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
    int returnCode = Thread::WaitForThread( handle, timeoutMS, timedOut );
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
    uint32_t ip( 0 );
    {
        PROFILE_FUNCTION

        AStackString<> hostName;

        {
            // take a copy of the HostName
            NameResolutionData * data = (NameResolutionData *)userData;
            hostName = data->hostName;

            // mark userData as safe to delete by calling thread
            MemoryBarrier();
            data->safeToFree = true;
        }

        NetworkStartupHelper nsh; // ensure network is up if not already

        // perform lookup

        // ::gethostbyname (and the ret value) is not thread safe, so make it so
        MutexHolder mh( s_GetHostByNameMutex );

        hostent * host = ::gethostbyname( hostName.Get() );
        if ( host )
        {
            ip = *( ( unsigned int * )host->h_addr );
        }
    }
    return ip;
}

//------------------------------------------------------------------------------
