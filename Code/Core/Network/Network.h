// Network
//------------------------------------------------------------------------------
#pragma once

// Includes
//------------------------------------------------------------------------------
#include "Core/Containers/AutoPtr.h"

// Forward Declarations
//------------------------------------------------------------------------------
class AString;
struct in6_addr;

// Network
//------------------------------------------------------------------------------
class Network
{
public:
    static const uint32_t Loopback;
    static const in6_addr Loopback6;

    static void GetHostName( AString & hostName );
    static void GetDomainName( AString & domainName );

    static bool GetHostIPFromName( const AString & hostName, uint32_t & ipAddress, in6_addr & ipAddress6, const uint32_t timeoutMS = 1000 );

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
