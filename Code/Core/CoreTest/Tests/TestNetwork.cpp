// TestNetwork.cpp
//------------------------------------------------------------------------------

// Includes
//------------------------------------------------------------------------------
#include "TestFramework/UnitTest.h"

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

    void TestNameResolution() const;
    void TestGetAddressFromString() const;
    void TestGetAddressAsString() const;

    void TestNameResolution_ipv6() const;
    void TestGetAddressFromString_ipv6() const;
    void TestGetAddressAsString_ipv6() const;
};

// Register Tests
//------------------------------------------------------------------------------
REGISTER_TESTS_BEGIN( TestNetwork )
    REGISTER_TEST( TestGetHostName )
    REGISTER_TEST( TestNameResolution )
    REGISTER_TEST( TestGetAddressFromString )
    REGISTER_TEST( TestGetAddressAsString )
    REGISTER_TEST( TestNameResolution_ipv6 )
    REGISTER_TEST( TestGetAddressFromString_ipv6 )
    REGISTER_TEST( TestGetAddressAsString_ipv6 )
REGISTER_TESTS_END

// TestGetHostName
//------------------------------------------------------------------------------
void TestNetwork::TestGetHostName() const
{
    AStackString<> hostName;
    Network::GetHostName( hostName );
    TEST_ASSERT( !hostName.Equals( "Unknown" ) );
}

// TestNameResolution
//------------------------------------------------------------------------------
void TestNetwork::TestNameResolution() const
{
    NetworkStartupHelper nsh; // ensure network is up if not already

    const AStackString<> hostName( "localhost" );
    const Network::IpAddress expected = Network::Loopback;
    Network::IpAddress addr;
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
        const Network::IpAddress expected = Network::Loopback;
        Network::IpAddress addr;
        bool ret = Network::GetAddressFromString( address, addr );
        TEST_ASSERT( ret );
        TEST_ASSERTM( addr == expected, "Expected: 0x%08x\nGot     : 0x%08x", expected, addr );
    }

    {
        const AStackString<> address( "255.0.255.0" );
        const Network::IpAddress expected = 0x00ff00ff;
        Network::IpAddress addr;
        bool ret = Network::GetAddressFromString( address, addr );
        TEST_ASSERT( ret );
        TEST_ASSERTM( addr == expected, "Expected: 0x%08x\nGot     : 0x%08x", expected, addr );
    }

    {
        const AStackString<> address;
        Network::IpAddress addr;
        bool ret = Network::GetAddressFromString( address, addr );
        TEST_ASSERT( !ret );
    }
}

// TestGetAddressAsString
//------------------------------------------------------------------------------
void TestNetwork::TestGetAddressAsString() const
{
    {
        const Network::IpAddress addr = Network::Loopback;
        const AStackString<> expected( "127.0.0.1" );
        AStackString<> address;
        bool ret = Network::GetAddressAsString( addr, address );
        TEST_ASSERT( ret );
        TEST_ASSERTM( address.Equals( expected ), "Expected: %s\nGot     : %s", expected.Get(), address.Get() );
    }

    {
        const Network::IpAddress addr = 0x00ff00ff;
        const AStackString<> expected( "255.0.255.0");
        AStackString<> address;
        bool ret = Network::GetAddressAsString( addr, address );
        TEST_ASSERT( ret );
        TEST_ASSERTM( address.Equals( expected ), "Expected: %s\nGot     : %s", expected.Get(), address.Get() );
    }
}

// TestNameResolution_ipv6
//------------------------------------------------------------------------------
void TestNetwork::TestNameResolution_ipv6() const
{
    NetworkStartupHelper nsh; // ensure network is up if not already

    const AStackString<> address( "localhost" );
    const Network::IpAddress6 & expected = Network::Loopback6;
    Network::IpAddress6 addr;
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
        const Network::IpAddress6 & expected = Network::Loopback6;
        Network::IpAddress6 addr;
        bool ret = Network::GetAddressFromString( address, addr );
        TEST_ASSERT( ret );
        TEST_ASSERT( memcmp( &addr, &expected, sizeof( addr ) ) == 0 );
    }

    {
        const AStackString<> address( "ff00:ff00:ff00:ff00:ff00:ff00:ff00:ff00" );
        const Network::IpAddress6 & expected = { 0xFF, 0x00, 0xFF, 0x00, 0xFF, 0x00, 0xFF, 0x00, 0xFF, 0x00, 0xFF, 0x00, 0xFF, 0x00, 0xFF, 0x00 };
        Network::IpAddress6 addr;
        bool ret = Network::GetAddressFromString( address, addr );
        TEST_ASSERT( ret );
        TEST_ASSERT( memcmp( &addr, &expected, sizeof( addr ) ) == 0 );
    }

    {
        const AStackString<> address;
        Network::IpAddress6 addr;
        bool ret = Network::GetAddressFromString( address, addr );
        TEST_ASSERT( !ret );
    }
}

// TestGetAddressAsString_ipv6
//------------------------------------------------------------------------------
void TestNetwork::TestGetAddressAsString_ipv6() const
{
    {
        const Network::IpAddress6 & addr = Network::Loopback6;
        const AStackString<> expected( "::1" );
        AStackString<> address;
        bool ret = Network::GetAddressAsString( addr, address );
        TEST_ASSERT( ret );
        TEST_ASSERTM( address.Equals( expected ), "Expected: %s\nGot     : %s", expected.Get(), address.Get() );
    }

    {
        const Network::IpAddress6 & addr = { 0xFF, 0x00, 0xFF, 0x00, 0xFF, 0x00, 0xFF, 0x00, 0xFF, 0x00, 0xFF, 0x00, 0xFF, 0x00, 0xFF, 0x00 };
        const AStackString<> expected( "ff00:ff00:ff00:ff00:ff00:ff00:ff00:ff00" );
        AStackString<> address;
        bool ret = Network::GetAddressAsString( addr, address );
        TEST_ASSERT( ret );
        TEST_ASSERTM( address.Equals( expected ), "Expected: %s\nGot     : %s", expected.Get(), address.Get() );
    }
}