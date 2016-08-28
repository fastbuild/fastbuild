// NetworkStartupHelper
//------------------------------------------------------------------------------
#pragma once

// Includes
//------------------------------------------------------------------------------
#if defined( __WINDOWS__ )
    #include <winsock2.h>
#endif

#include "Core/Env/Types.h"
#include "Core/Process/Mutex.h"

// NetworkStartup
//------------------------------------------------------------------------------
class NetworkStartupHelper
{
public:
    // ensure the network is up around the scope of this object
    NetworkStartupHelper();
    ~NetworkStartupHelper() { Stop(); }

    void Stop();

    static inline bool IsStarted() { return (s_RefCount > 0); }

private:
    bool m_Stopped;
#if defined( __WINDOWS__ )
    static WSADATA s_WSAData;
#endif
    static Mutex s_Mutex;
    static volatile uint32_t s_RefCount;
};

//------------------------------------------------------------------------------
