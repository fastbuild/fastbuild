// TestNetwork.cpp
//------------------------------------------------------------------------------

// Includes
//------------------------------------------------------------------------------
#include "TestFramework/UnitTest.h"

#include "Core/Env/Env.h"
#include "Core/Network/Network.h"
#include "Core/Network/NetworkStartupHelper.h"
#include "Core/Strings/AStackString.h"

// system
#if defined( __WINDOWS__ )
    #include "Core/Env/WindowsHeader.h"
#endif
#if defined( __LINUX__ ) || defined( __APPLE__ )
    #include <arpa/inet.h>
    #include <netdb.h>
    #include <string.h>
    #include <unistd.h>
#endif

class TestNetwork : public UnitTest
{
private:
    DECLARE_TESTS

    void TestGetHostName() const;
    void TestGetDomainName() const;

    void TestIsValid() const;
    void TestIsLoopback() const;
    void TestNameResolution() const;
    void TestGetAddressFromString() const;
    void TestGetAddressAsString() const;

    void TestGetHostIPFromName() const;

    void TestIsValid_ipv6() const;
    void TestIsLoopback_ipv6() const;
    void TestNameResolution_ipv6() const;
    void TestGetAddressFromString_ipv6() const;
    void TestGetAddressAsString_ipv6() const;

};

// Register Tests
//------------------------------------------------------------------------------
REGISTER_TESTS_BEGIN( TestNetwork )

    REGISTER_TEST( TestGetHostName )
    REGISTER_TEST( TestGetDomainName )

    REGISTER_TEST( TestIsValid )
    REGISTER_TEST( TestIsLoopback )
    REGISTER_TEST( TestNameResolution )
    REGISTER_TEST( TestGetAddressFromString )
    REGISTER_TEST( TestGetAddressAsString )

    // Travis CI does not support ipv6
    // https://docs.travis-ci.com/user/reference/overview/#virtualisation-environment-vs-operating-system
    AStackString<> travisEnv;
    if ( !Env::GetEnvVariable( "TRAVIS", travisEnv ) || travisEnv != "true" )
    {
        REGISTER_TEST( TestGetHostIPFromName )

        REGISTER_TEST( TestIsValid_ipv6 )
        REGISTER_TEST( TestIsLoopback_ipv6 )
        REGISTER_TEST( TestNameResolution_ipv6 )
        REGISTER_TEST( TestGetAddressFromString_ipv6 )
        REGISTER_TEST( TestGetAddressAsString_ipv6 )
    }

REGISTER_TESTS_END

// TestGetHostName
//------------------------------------------------------------------------------
void TestNetwork::TestGetHostName() const
{
    AStackString<> hostName;
    Network::GetHostName( hostName );
    TEST_ASSERT( !hostName.Equals( "Unknown" ) );
}

// TestGetDomainName
//------------------------------------------------------------------------------
void TestNetwork::TestGetDomainName() const
{
    AStackString<> domainName;
    Network::GetDomainName( domainName );
    TEST_ASSERT( !domainName.Equals( "Unknown" ) );
}

// TestIsValid
//------------------------------------------------------------------------------
void TestNetwork::TestIsValid() const
{
    {
        const uint32_t& addr = Network::LoopbackAddress;
        TEST_ASSERT(Network::IsValidAddress(addr));
    }
    {
        const uint32_t & addr = Network::InvalidAddress;
        TEST_ASSERT( !Network::IsValidAddress( addr ) );
    }
    {
        const uint32_t & addr = 0x00ff00ff;
        TEST_ASSERT( Network::IsValidAddress( addr ) );
    }
}

// TestIsLoopback
//------------------------------------------------------------------------------
void TestNetwork::TestIsLoopback() const
{
    {
        const uint32_t & addr = Network::LoopbackAddress;
        TEST_ASSERT( Network::IsLoopbackAddress( addr ) );
    }
    {
        const uint32_t & addr = Network::InvalidAddress;
        TEST_ASSERT( !Network::IsLoopbackAddress( addr ) );
    }
    {
        const uint32_t & addr = 0x00ff00ff;
        TEST_ASSERT( !Network::IsLoopbackAddress( addr ) );
    }
}

// TestNameResolution
//------------------------------------------------------------------------------
void TestNetwork::TestNameResolution() const
{
    NetworkStartupHelper nsh; // ensure network is up if not already

    const AStackString<> hostName( "localhost" );
    const uint32_t & expected = Network::LoopbackAddress;
    uint32_t addr;
    bool ret = Network::NameResolution( hostName, addr );
    TEST_ASSERT( ret );
    TEST_ASSERTM( addr == expected, "Expected: 0x%08x\nGot     : 0x%08x", expected, addr );
}

// TestGetAddressFromString
//------------------------------------------------------------------------------
void TestNetwork::TestGetAddressFromString() const
{
    {
        const AStackString<> address( "127.0.0.1" );
        const uint32_t & expected = Network::LoopbackAddress;
        uint32_t addr;
        bool ret = Network::GetAddressFromString( address, addr );
        TEST_ASSERT( ret );
        TEST_ASSERTM( addr == expected, "Expected: 0x%08x\nGot     : 0x%08x", expected, addr );
    }

    {
        const AStackString<> address( "255.0.255.0" );
        const uint32_t & expected = 0x00ff00ff;
        uint32_t addr;
        bool ret = Network::GetAddressFromString( address, addr );
        TEST_ASSERT( ret );
        TEST_ASSERTM( addr == expected, "Expected: 0x%08x\nGot     : 0x%08x", expected, addr );
    }

    {
        const AStackString<> address;
        uint32_t addr;
        bool ret = Network::GetAddressFromString( address, addr );
        TEST_ASSERT( !ret );
    }
}

// TestGetAddressAsString
//------------------------------------------------------------------------------
void TestNetwork::TestGetAddressAsString() const
{
    {
        const uint32_t & addr = Network::LoopbackAddress;
        const AStackString<> expected( "127.0.0.1" );
        AStackString<> address;
        bool ret = Network::GetAddressAsString( addr, address );
        TEST_ASSERT( ret );
        TEST_ASSERTM( address.Equals( expected ), "Expected: %s\nGot     : %s", expected.Get(), address.Get() );
    }

    {
        const uint32_t & addr = 0x00ff00ff;
        const AStackString<> expected( "255.0.255.0");
        AStackString<> address;
        bool ret = Network::GetAddressAsString( addr, address );
        TEST_ASSERT( ret );
        TEST_ASSERTM( address.Equals( expected ), "Expected: %s\nGot     : %s", expected.Get(), address.Get() );
    }
}

// TestGetHostIPFromName
//------------------------------------------------------------------------------
void TestNetwork::TestGetHostIPFromName() const
{
    NetworkStartupHelper nsh; // ensure network is up if not already

    AStackString<> hostName;
    Network::GetHostName( hostName );

    uint32_t addr;
    in6_addr addr6;
    bool ret = Network::GetHostIPFromName( hostName, addr, addr6, 0, true ); // no timeout
    TEST_ASSERT( ret );
    TEST_ASSERT( Network::IsValidAddress( addr ) || Network::IsValidAddress( addr6 ) );
}

// TestIsValid_ipv6
//------------------------------------------------------------------------------
void TestNetwork::TestIsValid_ipv6() const
{
    {
        const in6_addr & addr = Network::LoopbackAddress6;
        TEST_ASSERT( Network::IsValidAddress( addr ) );
    }
    {
        const in6_addr & addr = Network::InvalidAddress6;
        TEST_ASSERT( !Network::IsValidAddress( addr ) );
    }
    {
        const in6_addr & addr = {{{ 0xFF, 0x00, 0xFF, 0x00, 0xFF, 0x00, 0xFF, 0x00, 0xFF, 0x00, 0xFF, 0x00, 0xFF, 0x00, 0xFF, 0x00 }}};
        TEST_ASSERT( Network::IsValidAddress( addr ) );
    }
}

// TestIsLoopback_ipv6
//------------------------------------------------------------------------------
void TestNetwork::TestIsLoopback_ipv6() const
{
    {
        const in6_addr & addr = Network::LoopbackAddress6;
        TEST_ASSERT( Network::IsLoopbackAddress( addr ) );
    }
    {
        const in6_addr & addr = Network::InvalidAddress6;
        TEST_ASSERT( !Network::IsLoopbackAddress( addr ) );
    }
    {
        const in6_addr & addr = {{{ 0xFF, 0x00, 0xFF, 0x00, 0xFF, 0x00, 0xFF, 0x00, 0xFF, 0x00, 0xFF, 0x00, 0xFF, 0x00, 0xFF, 0x00 }}};
        TEST_ASSERT( !Network::IsLoopbackAddress( addr ) );
    }
}

// TestNameResolution_ipv6
//------------------------------------------------------------------------------
void TestNetwork::TestNameResolution_ipv6() const
{
    NetworkStartupHelper nsh; // ensure network is up if not already

    const AStackString<> address( "localhost" );
    const in6_addr & expected = Network::LoopbackAddress6;
    in6_addr addr;
    bool ret = Network::NameResolution( address, addr );
    TEST_ASSERT( ret );
    TEST_ASSERT( memcmp( &addr, &expected, sizeof( addr ) ) == 0 );
}

// TestGetAddressFromString_ipv6
//------------------------------------------------------------------------------
void TestNetwork::TestGetAddressFromString_ipv6() const
{
    {
        const AStackString<> address( "::1" );
        const in6_addr & expected = Network::LoopbackAddress6;
        in6_addr addr;
        bool ret = Network::GetAddressFromString( address, addr );
        TEST_ASSERT( ret );
        TEST_ASSERT( memcmp( &addr, &expected, sizeof( addr ) ) == 0 );
    }

    {
        const AStackString<> address( "ff00:ff00:ff00:ff00:ff00:ff00:ff00:ff00" );
        const in6_addr & expected = {{{ 0xFF, 0x00, 0xFF, 0x00, 0xFF, 0x00, 0xFF, 0x00, 0xFF, 0x00, 0xFF, 0x00, 0xFF, 0x00, 0xFF, 0x00 }}};
        in6_addr addr;
        bool ret = Network::GetAddressFromString( address, addr );
        TEST_ASSERT( ret );
        TEST_ASSERT( memcmp( &addr, &expected, sizeof( addr ) ) == 0 );
    }

    {
        const AStackString<> address;
        in6_addr addr;
        bool ret = Network::GetAddressFromString( address, addr );
        TEST_ASSERT( !ret );
    }
}

// TestGetAddressAsString_ipv6
//------------------------------------------------------------------------------
void TestNetwork::TestGetAddressAsString_ipv6() const
{
    {
        const in6_addr & addr = Network::LoopbackAddress6;
        const AStackString<> expected( "::1" );
        AStackString<> address;
        bool ret = Network::GetAddressAsString( addr, address );
        TEST_ASSERT( ret );
        TEST_ASSERTM( address.Equals( expected ), "Expected: %s\nGot     : %s", expected.Get(), address.Get() );
    }

    {
        const in6_addr & addr = {{{ 0xFF, 0x00, 0xFF, 0x00, 0xFF, 0x00, 0xFF, 0x00, 0xFF, 0x00, 0xFF, 0x00, 0xFF, 0x00, 0xFF, 0x00 }}};
        const AStackString<> expected( "ff00:ff00:ff00:ff00:ff00:ff00:ff00:ff00" );
        AStackString<> address;
        bool ret = Network::GetAddressAsString( addr, address );
        TEST_ASSERT( ret );
        TEST_ASSERTM( address.Equals( expected ), "Expected: %s\nGot     : %s", expected.Get(), address.Get() );
    }
}
