// WindowsHeader
//------------------------------------------------------------------------------
//
// Windows.h and WinSock2.h mustbe included in a specific order to avoid
// compile errors. This is problematic when headers include other headers
// that include Windows.h, or when using Unity.
//
// To remedy this, we always include Windows.h via this wrapper
//
//------------------------------------------------------------------------------
#pragma once

#if !defined( __WINDOWS__ )
    #error Should only be used on __WINDOWS__
#endif

// Includes
//------------------------------------------------------------------------------
#pragma warning(push, 0)  
#include <WinSock2.h> // WinSock2.h must be first
#include <WS2tcpip.h>
#include <Windows.h>
#pragma warning(pop)

//------------------------------------------------------------------------------
