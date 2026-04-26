// TestUnorderedMap.cpp
//------------------------------------------------------------------------------

// Includes
//------------------------------------------------------------------------------
#include "TestFramework/TestGroup.h"

#include "Core/Containers/UnorderedMap.h"
#include "Core/Strings/AString.h"

//------------------------------------------------------------------------------
TEST_GROUP( TestUnorderedMap, TestGroupTest )
{
public:
};

//------------------------------------------------------------------------------
TEST_CASE( TestUnorderedMap, ConstructEmpty )
{
    UnorderedMap<AString, AString> map;
    TEST_ASSERT( map.IsEmpty() );
    TEST_ASSERT( map.GetSize() == 0 );
}

//------------------------------------------------------------------------------
TEST_CASE( TestUnorderedMap, Insert )
{
    UnorderedMap<AString, AString> map;
    map.Insert( AString( "Hello" ), AString( "there" ) );
    TEST_ASSERT( map.IsEmpty() == false );
    TEST_ASSERT( map.GetSize() == 1 );
    map.Insert( AString( "Key" ), AString( "Value" ) );
    TEST_ASSERT( map.IsEmpty() == false );
    TEST_ASSERT( map.GetSize() == 2 );
}

//------------------------------------------------------------------------------
TEST_CASE( TestUnorderedMap, Find )
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
            const auto * pair = map.Find( AString( "Hello" ) );
            TEST_ASSERT( pair );
            TEST_ASSERT( pair->m_Key == "Hello" );
            TEST_ASSERT( pair->m_Value == "there" );
        }

        // not found
        {
            const auto * pair = map.Find( AString( "Thing" ) );
            TEST_ASSERT( pair == nullptr );
        }
    }
}

//------------------------------------------------------------------------------
TEST_CASE( TestUnorderedMap, Destruct )
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
