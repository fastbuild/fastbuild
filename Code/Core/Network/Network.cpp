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

// system
#if defined( __WINDOWS__ )
    #include <Winsock2.h>
#endif

// Static
//------------------------------------------------------------------------------
/*static*/ Mutex Network::s_GetHostByNameMutex;

// GetHostName
//------------------------------------------------------------------------------
/*static*/ void Network::GetHostName( AString & hostName )
{
    #if defined( __WINDOWS__ )
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
    #elif defined( __APPLE__ )
        ASSERT( false ); // TODO:MAC Implement GetHostName
    #endif
}

// GetHostIPFromName
//------------------------------------------------------------------------------
/*static*/ uint32_t Network::GetHostIPFromName( const AString & hostName, uint32_t timeoutMS )
{
    #if defined( __WINDOWS__ )
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
    #elif defined( __APPLE__ )
        ASSERT( false ); // TODO:MAC Implement GetHostIPFromName
        return 0;
    #elif defined( __LINUX__ )
        ASSERT( false ); // TODO:LINUX Implement GetHostIPFromName
        return 0;
    #endif
}

// NameResolutionThreadFunc
//------------------------------------------------------------------------------
#if defined( __WINDOWS__ )
    /*static*/ uint32_t Network::NameResolutionThreadFunc( void * userData )
    {
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
            uint32_t ip = *( ( unsigned int * )host->h_addr );
            return ip;
        }
        return 0; // lookup failed
    }
#endif // __WINDOWS__

//------------------------------------------------------------------------------
