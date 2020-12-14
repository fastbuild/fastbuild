// NetworkStartupHelper
//------------------------------------------------------------------------------
#pragma once

// Includes
//------------------------------------------------------------------------------
#if defined( __WINDOWS__ )
    #include "Core/Env/WindowsHeader.h"
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
    ~NetworkStartupHelper() = default;

    // Set main flag to help abort network operations
    // NOTE: Flag must remain available once set (i.e. static var)
    static void SetMainShutdownFlag( volatile bool * shutdownFlag );
    static bool IsShuttingDown();

private:
    static bool s_Started;
    static Mutex s_Mutex;
    static volatile bool * s_MainShutdownFlag;
    #if defined( __WINDOWS__ )
        static WSADATA s_WSAData;
    #endif
};

//------------------------------------------------------------------------------
