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
    typedef uint32_t IpAddress;
    typedef in6_addr IpAddress6;

    static const IpAddress Loopback;
    static const IpAddress6 Loopback6;

    static void GetHostName( AString & hostName );
    static void GetDomainName( AString & domainName );

    static bool GetHostIPFromName( const AString & hostName, IpAddress & ipAddress, IpAddress6 & ipAddress6, const uint32_t timeoutMS = 1000 );

    static bool GetAddressFromString( const AString & address, IpAddress & ipAddress );
    static bool GetAddressFromString( const AString & address, IpAddress6 & ipAddress );

    static bool GetAddressAsString( const IpAddress ipAddress, AString & address );
    static bool GetAddressAsString( const IpAddress6 & ipAddress, AString & address );

    static bool NameResolution( const AString & hostName, IpAddress & ipAddress );
    static bool NameResolution( const AString & hostName, IpAddress6 & ipAddress );

private:
    static uint32_t NameResolutionThreadFunc( void * userData );
};

//------------------------------------------------------------------------------
