// TestNetwork
//------------------------------------------------------------------------------

// Includes
//------------------------------------------------------------------------------
#include "TestFramework/TestGroup.h"

// Core
#include "Core/Containers/Array.h"
#include "Core/Network/Network.h"
#include "Core/Strings/AString.h"

// TestNetwork
//------------------------------------------------------------------------------
class TestNetwork : public TestGroup
{
private:
    DECLARE_TESTS

    void GetIPv4Addresses() const;
};

// Register Tests
//------------------------------------------------------------------------------
REGISTER_TESTS_BEGIN( TestNetwork )
    REGISTER_TEST( GetIPv4Addresses )
REGISTER_TESTS_END

// GetLocalIPv4Addresses
//------------------------------------------------------------------------------
void TestNetwork::GetIPv4Addresses() const
{
    StackArray<AString> addresses;
    Network::GetIPv4Addresses( addresses );

    // We expect any place we'd ever run this test has at least one valid IPv4 address
    TEST_ASSERT( addresses.IsEmpty() == false );
    for ( const AString & address : addresses )
    {
        uint32_t a, b, c, d;
        TEST_ASSERT( address.Scan( "%u.%u.%u.%u", &a, &b, &c, &d ) == 4 );
        TEST_ASSERT( a < 256 );
        TEST_ASSERT( b < 256 );
        TEST_ASSERT( c < 256 );
        TEST_ASSERT( d < 256 );
    }
}

//------------------------------------------------------------------------------
