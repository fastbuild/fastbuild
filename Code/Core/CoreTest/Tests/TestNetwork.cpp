// TestNetwork
//------------------------------------------------------------------------------

// Includes
//------------------------------------------------------------------------------
#include "TestFramework/TestGroup.h"

// Core
#include "Core/Containers/Array.h"
#include "Core/Network/Network.h"
#include "Core/Strings/AString.h"

//------------------------------------------------------------------------------
TEST_GROUP( TestNetwork, TestGroupTest )
{
public:
};

//------------------------------------------------------------------------------
TEST_CASE( TestNetwork, GetIPv4Addresses )
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
