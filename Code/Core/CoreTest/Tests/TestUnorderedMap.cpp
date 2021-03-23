// TestUnorderedMap.cpp
//------------------------------------------------------------------------------

// Includes
//------------------------------------------------------------------------------
#include "TestFramework/UnitTest.h"

#include "Core/Containers/UnorderedMap.h"
#include "Core/Strings/AString.h"

// TestUnorderedMap
//------------------------------------------------------------------------------
class TestUnorderedMap : public UnitTest
{
private:
    DECLARE_TESTS

    void ConstructEmpty() const;
    void Destruct() const;
    void Insert() const;
    void Find() const;
};

// Register Tests
//------------------------------------------------------------------------------
REGISTER_TESTS_BEGIN( TestUnorderedMap )
    REGISTER_TEST( ConstructEmpty )
    REGISTER_TEST( Insert )
    REGISTER_TEST( Find )
    REGISTER_TEST( Destruct )
REGISTER_TESTS_END

// ConstructEmpty
//------------------------------------------------------------------------------
void TestUnorderedMap::ConstructEmpty() const
{
    UnorderedMap<AString, AString> map;
    TEST_ASSERT( map.IsEmpty() );
    TEST_ASSERT( map.GetSize() == 0 );
}

// Insert
//------------------------------------------------------------------------------
void TestUnorderedMap::Insert() const
{
    UnorderedMap<AString, AString> map;
    map.Insert( AString( "Hello" ), AString( "there" ) );
    TEST_ASSERT( map.IsEmpty() == false );
    TEST_ASSERT( map.GetSize() == 1 );
    map.Insert( AString( "Key" ), AString( "Value" ) );
    TEST_ASSERT( map.IsEmpty() == false );
    TEST_ASSERT( map.GetSize() == 2 );
}

// Find
//------------------------------------------------------------------------------
void TestUnorderedMap::Find() const
{
    // empty
    {
        UnorderedMap<AString, AString> map;
        TEST_ASSERT( map.Find( AString( "thing" ) ) == nullptr );
    }

    // not empty
    {
        UnorderedMap<AString, AString> map;
        map.Insert( AString( "Hello" ), AString( "there" ) );

        // found
        {
            auto* pair = map.Find( AString( "Hello" ) );
            TEST_ASSERT( pair );
            TEST_ASSERT( pair->m_Key == "Hello" );
            TEST_ASSERT( pair->m_Value == "there" );
        }

        // not found
        {
            auto* pair = map.Find( AString( "Thing" ) );
            TEST_ASSERT( pair == nullptr );
        }
    }
}

// Destruct
//------------------------------------------------------------------------------
void TestUnorderedMap::Destruct() const
{
    // empty
    {
        UnorderedMap<AString, AString> map;
        map.Destruct();
        TEST_ASSERT( map.IsEmpty() );
        TEST_ASSERT( map.GetSize() == 0 );
    }

    // not empty
    {
        UnorderedMap<AString, AString> map;
        map.Insert( AString( "Key" ), AString( "Value" ) );
        map.Destruct();
        TEST_ASSERT( map.IsEmpty() );
        TEST_ASSERT( map.GetSize() == 0 );
    }
}

//------------------------------------------------------------------------------
