// Network
//------------------------------------------------------------------------------
#pragma once

// Includes
//------------------------------------------------------------------------------
#include "Core/Env/Types.h"

// Forward Declarations
//------------------------------------------------------------------------------
class AString;
struct in6_addr;

// Network
//------------------------------------------------------------------------------
class Network
{
public:
    static const uint32_t InvalidAddress;
    static const in6_addr InvalidAddress6;

    static const uint32_t LoopbackAddress;
    static const in6_addr LoopbackAddress6;

    static bool IsValidAddress( const uint32_t ipAddress );
    static bool IsValidAddress( const in6_addr & ipAddress );

    static bool IsLoopbackAddress( const uint32_t ipAddress );
    static bool IsLoopbackAddress( const in6_addr & ipAddress );

    static void GetHostName( AString & hostName );
    static void GetDomainName( AString & domainName );

    static bool GetHostIPFromName( const AString & hostName, uint32_t & ipAddress, in6_addr & ipAddress6, const uint32_t timeoutMS = 1000, const bool resolve4and6 = false );

    static bool GetAddressFromString( const AString & address, uint32_t & ipAddress );
    static bool GetAddressFromString( const AString & address, in6_addr & ipAddress );

    static bool GetAddressAsString( const uint32_t ipAddress, AString & address );
    static bool GetAddressAsString( const in6_addr & ipAddress, AString & address );

    static bool NameResolution( const AString & hostName, uint32_t & ipAddress );
    static bool NameResolution( const AString & hostName, in6_addr & ipAddress );

private:
    static uint32_t NameResolutionThreadFunc( void * userData );
};

//------------------------------------------------------------------------------
