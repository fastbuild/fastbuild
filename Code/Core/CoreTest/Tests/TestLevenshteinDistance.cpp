// TestLevenshteinDistance.cpp
//------------------------------------------------------------------------------

// Includes
//------------------------------------------------------------------------------
#include "TestFramework/UnitTest.h"

#include "Core/Strings/AStackString.h"
#include "Core/Strings/AString.h"
#include "Core/Strings/LevenshteinDistance.h"

// TestLevenshteinDistance
//------------------------------------------------------------------------------
class TestLevenshteinDistance : public UnitTest
{
private:
    DECLARE_TESTS

    void Distance() const;
    void DistanceI() const;
};

// Register Tests
//------------------------------------------------------------------------------
REGISTER_TESTS_BEGIN( TestLevenshteinDistance )
    REGISTER_TEST( Distance )
    REGISTER_TEST( DistanceI )
REGISTER_TESTS_END

// Distance
//------------------------------------------------------------------------------
void TestLevenshteinDistance::Distance() const
{
    {
        TEST_ASSERT( 0 == LevenshteinDistance::Distance( nullptr, nullptr ) );
        TEST_ASSERT( 4 == LevenshteinDistance::Distance( nullptr, "test" ) );
        TEST_ASSERT( 4 == LevenshteinDistance::Distance( "test", nullptr ) );
        TEST_ASSERT( 0 == LevenshteinDistance::Distance( "test", "test" ) );
        TEST_ASSERT( 4 == LevenshteinDistance::Distance( "test", "TEST" ) );
        TEST_ASSERT( 5 == LevenshteinDistance::Distance( "test", "TOAST" ) );
        TEST_ASSERT( 2 == LevenshteinDistance::Distance( "TEST", "TOAST" ) );
        TEST_ASSERT( 7 == LevenshteinDistance::Distance( "test", "TEST123" ) );
        TEST_ASSERT( 3 == LevenshteinDistance::Distance( "TEST", "TEST123" ) );
        TEST_ASSERT( 5 == LevenshteinDistance::Distance( "TOAST", "TEST123" ) );
        TEST_ASSERT( 7 == LevenshteinDistance::Distance( "123TEST", "test" ) );
        TEST_ASSERT( 3 == LevenshteinDistance::Distance( "123TEST", "TEST" ) );
        TEST_ASSERT( 5 == LevenshteinDistance::Distance( "123TEST", "TOAST" ) );
        TEST_ASSERT( 6 == LevenshteinDistance::Distance( "123TEST", "TEST123" ) );
        TEST_ASSERT( 6 == LevenshteinDistance::Distance( "TEST123", "123TEST" ) );
    }
    {
        AString nil;
        AStackString< 16 > test( "test" );
        AStackString< 16 > TEST( "TEST" );
        AStackString< 16 > TOAST( "TOAST" );
        AStackString< 16 > TEST123( "TEST123" );
        AStackString< 16 > _123TEST( "123TEST" );
        TEST_ASSERT( 0 == LevenshteinDistance::Distance( nil, nil ) );
        TEST_ASSERT( 4 == LevenshteinDistance::Distance( nil, test ) );
        TEST_ASSERT( 4 == LevenshteinDistance::Distance( test, nil ) );
        TEST_ASSERT( 0 == LevenshteinDistance::Distance( test, test ) );
        TEST_ASSERT( 4 == LevenshteinDistance::Distance( test, TEST ) );
        TEST_ASSERT( 5 == LevenshteinDistance::Distance( test, TOAST ) );
        TEST_ASSERT( 2 == LevenshteinDistance::Distance( TEST, TOAST ) );
        TEST_ASSERT( 7 == LevenshteinDistance::Distance( test, TEST123 ) );
        TEST_ASSERT( 3 == LevenshteinDistance::Distance( TEST, TEST123 ) );
        TEST_ASSERT( 5 == LevenshteinDistance::Distance( TOAST, TEST123 ) );
        TEST_ASSERT( 7 == LevenshteinDistance::Distance( _123TEST, test ) );
        TEST_ASSERT( 3 == LevenshteinDistance::Distance( _123TEST, TEST ) );
        TEST_ASSERT( 5 == LevenshteinDistance::Distance( _123TEST, TOAST ) );
        TEST_ASSERT( 6 == LevenshteinDistance::Distance( _123TEST, TEST123 ) );
        TEST_ASSERT( 6 == LevenshteinDistance::Distance( TEST123, _123TEST ) );
    }
}

// DistanceI
//------------------------------------------------------------------------------
void TestLevenshteinDistance::DistanceI() const
{
    {
        TEST_ASSERT( 0 == LevenshteinDistance::DistanceI( nullptr, nullptr ) );
        TEST_ASSERT( 4 == LevenshteinDistance::DistanceI( nullptr, "test" ) );
        TEST_ASSERT( 4 == LevenshteinDistance::DistanceI( "test", nullptr ) );
        TEST_ASSERT( 0 == LevenshteinDistance::DistanceI( "test", "test" ) );
        TEST_ASSERT( 0 == LevenshteinDistance::DistanceI( "test", "TEST" ) );
        TEST_ASSERT( 2 == LevenshteinDistance::DistanceI( "test", "TOAST" ) );
        TEST_ASSERT( 2 == LevenshteinDistance::DistanceI( "TEST", "TOAST" ) );
        TEST_ASSERT( 3 == LevenshteinDistance::DistanceI( "test", "TEST123" ) );
        TEST_ASSERT( 3 == LevenshteinDistance::DistanceI( "TEST", "TEST123" ) );
        TEST_ASSERT( 5 == LevenshteinDistance::DistanceI( "TOAST", "TEST123" ) );
        TEST_ASSERT( 3 == LevenshteinDistance::DistanceI( "123TEST", "test" ) );
        TEST_ASSERT( 3 == LevenshteinDistance::DistanceI( "123TEST", "TEST" ) );
        TEST_ASSERT( 5 == LevenshteinDistance::DistanceI( "123TEST", "TOAST" ) );
        TEST_ASSERT( 6 == LevenshteinDistance::DistanceI( "123TEST", "TEST123" ) );
        TEST_ASSERT( 6 == LevenshteinDistance::DistanceI( "TEST123", "123TEST" ) );
    }
    {
        AString nil;
        AStackString< 16 > test( "test" );
        AStackString< 16 > TEST( "TEST" );
        AStackString< 16 > TOAST( "TOAST" );
        AStackString< 16 > TEST123( "TEST123" );
        AStackString< 16 > _123TEST( "123TEST" );
        TEST_ASSERT( 0 == LevenshteinDistance::DistanceI( nil, nil ) );
        TEST_ASSERT( 4 == LevenshteinDistance::DistanceI( nil, test ) );
        TEST_ASSERT( 4 == LevenshteinDistance::DistanceI( test, nil ) );
        TEST_ASSERT( 0 == LevenshteinDistance::DistanceI( test, test ) );
        TEST_ASSERT( 0 == LevenshteinDistance::DistanceI( test, TEST ) );
        TEST_ASSERT( 2 == LevenshteinDistance::DistanceI( test, TOAST ) );
        TEST_ASSERT( 2 == LevenshteinDistance::DistanceI( TEST, TOAST ) );
        TEST_ASSERT( 3 == LevenshteinDistance::DistanceI( test, TEST123 ) );
        TEST_ASSERT( 3 == LevenshteinDistance::DistanceI( TEST, TEST123 ) );
        TEST_ASSERT( 5 == LevenshteinDistance::DistanceI( TOAST, TEST123 ) );
        TEST_ASSERT( 3 == LevenshteinDistance::DistanceI( _123TEST, test ) );
        TEST_ASSERT( 3 == LevenshteinDistance::DistanceI( _123TEST, TEST ) );
        TEST_ASSERT( 5 == LevenshteinDistance::DistanceI( _123TEST, TOAST ) );
        TEST_ASSERT( 6 == LevenshteinDistance::DistanceI( _123TEST, TEST123 ) );
        TEST_ASSERT( 6 == LevenshteinDistance::DistanceI( TEST123, _123TEST ) );
    }
}

//------------------------------------------------------------------------------
