// NetworkStartupHelper
//------------------------------------------------------------------------------

// Includes
//------------------------------------------------------------------------------
#include "Core/PrecompiledHeader.h"

#include "NetworkStartupHelper.h"

#include "Core/Env/Assert.h"

//------------------------------------------------------------------------------
// Static Data
//------------------------------------------------------------------------------
/*static*/ bool NetworkStartupHelper::s_Started = false;
/*static*/ Mutex NetworkStartupHelper::s_Mutex;
/*static*/ volatile bool * NetworkStartupHelper::s_MasterShutdownFlag = nullptr;
#if defined( __WINDOWS__ )
    /*static*/ WSADATA NetworkStartupHelper::s_WSAData;
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

    s_Started = true;
}

// SetMasterShutdownFlag
//------------------------------------------------------------------------------
/*static*/ void NetworkStartupHelper::SetMasterShutdownFlag( volatile bool * shutdownFlag )
{
    MutexHolder mh( s_Mutex );
    s_MasterShutdownFlag = shutdownFlag;
}

// IsShuttingDown
//------------------------------------------------------------------------------
/*static*/ bool NetworkStartupHelper::IsShuttingDown()
{
    MutexHolder mh( s_Mutex );
    return ( s_MasterShutdownFlag ) ? ( *s_MasterShutdownFlag ) : false;
}

//------------------------------------------------------------------------------
