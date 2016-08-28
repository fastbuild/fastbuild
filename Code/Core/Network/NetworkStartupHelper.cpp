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
#if defined( __WINDOWS__ )
    /*static*/ WSADATA NetworkStartupHelper::s_WSAData;
#endif
/*static*/ Mutex NetworkStartupHelper::s_Mutex;
/*static*/ volatile uint32_t NetworkStartupHelper::s_RefCount( 0 );

// CONSTRUCTOR
//------------------------------------------------------------------------------
NetworkStartupHelper::NetworkStartupHelper()
    : m_Stopped( false )
{
    MutexHolder mh( s_Mutex );

    s_RefCount++;
    if ( s_RefCount > 1 )
    {
        return; // already previously started
    }

    // start up
    #if defined( __WINDOWS__ )
        VERIFY( WSAStartup( MAKEWORD( 2, 2 ), &s_WSAData ) == 0 );
    #endif
}

// Stop
//------------------------------------------------------------------------------
void NetworkStartupHelper::Stop()
{
    // Already manually stopped?
    if ( m_Stopped == true )
    {
        return;
    }
    m_Stopped = true;

    MutexHolder mh( s_Mutex );

    ASSERT( s_RefCount > 0 );
    s_RefCount--;
    if ( s_RefCount > 0 )
    {
        return; // still in use
    }

    // clean up
    #if defined( __WINDOWS__ )
        WSACleanup();
    #endif
}

//------------------------------------------------------------------------------
