// NetworkStartupHelper
//------------------------------------------------------------------------------

// Includes
//------------------------------------------------------------------------------
#include "NetworkStartupHelper.h"

#include "Core/Env/Assert.h"
#include "Core/Process/Atomic.h"

//------------------------------------------------------------------------------
// Static Data
//------------------------------------------------------------------------------
/*static*/ bool NetworkStartupHelper::s_Started = false;
/*static*/ Mutex NetworkStartupHelper::s_Mutex;
/*static*/ volatile bool * NetworkStartupHelper::s_MainShutdownFlag = nullptr;
#if defined( __WINDOWS__ )
    /*static*/ WSADATA NetworkStartupHelper::s_WSAData;
#elif defined( __LINUX__ ) || defined( __OSX__ )
    #include <signal.h>
#endif

// CONSTRUCTOR
//------------------------------------------------------------------------------
NetworkStartupHelper::NetworkStartupHelper()
{
    MutexHolder mh( s_Mutex );
    if ( s_Started )
    {
        return;
    }

    // start up
    #if defined( __WINDOWS__ )
        VERIFY( WSAStartup( MAKEWORD( 2, 2 ), &s_WSAData ) == 0 );
    #endif

    #if defined( __LINUX__ ) || defined( __OSX__ )
        // Disable SIGPIPE signals - we want to handle errors in the calling code
        // On OS X, this doesn't actually work, so we must also disable per socket
        signal( SIGPIPE, SIG_IGN );
    #endif

    s_Started = true;
}

// SetMainShutdownFlag
//------------------------------------------------------------------------------
/*static*/ void NetworkStartupHelper::SetMainShutdownFlag( volatile bool * shutdownFlag )
{
    MutexHolder mh( s_Mutex );
    s_MainShutdownFlag = shutdownFlag;
}

// IsShuttingDown
//------------------------------------------------------------------------------
/*static*/ bool NetworkStartupHelper::IsShuttingDown()
{
    MutexHolder mh( s_Mutex );
    return ( s_MainShutdownFlag ) ? AtomicLoadRelaxed( s_MainShutdownFlag ) : false;
}

//------------------------------------------------------------------------------
