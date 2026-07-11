// TestUniquePtr.cpp
//------------------------------------------------------------------------------

// Includes
//------------------------------------------------------------------------------
#include "TestFramework/TestGroup.h"

// Core
#include "Core/Containers/Array.h"
#include "Core/Containers/UniquePtr.h"
#include "Core/Strings/AString.h"

//------------------------------------------------------------------------------
TEST_GROUP( TestUniquePtr, TestGroupTest )
{
public:
};

//------------------------------------------------------------------------------
TEST_CASE( TestUniquePtr, Empty )
{
    UniquePtr<uint32_t> u;
    TEST_ASSERT( u.Get() == nullptr );
}

//------------------------------------------------------------------------------
TEST_CASE( TestUniquePtr, FreeMemory )
{
    UniquePtr<AString> u( FNEW( AString( "str" ) ) );
    TEST_ASSERT( u.Get() != nullptr );
    TEST_ASSERT( u->Equals( "str" ) );
}

//------------------------------------------------------------------------------
TEST_CASE( TestUniquePtr, Destroy )
{
    UniquePtr<AString> u( FNEW( AString( "str" ) ) );
    u.Destroy(); // Manually free
    TEST_ASSERT( u.Get() == nullptr );
}

//------------------------------------------------------------------------------
TEST_CASE( TestUniquePtr, ReleaseOwnership )
{
    UniquePtr<AString> u( FNEW( AString( "str" ) ) );
    AString * str = u.ReleaseOwnership();
    TEST_ASSERT( u.Get() == nullptr );
    FDELETE( str );
}

//------------------------------------------------------------------------------
TEST_CASE( TestUniquePtr, Replace )
{
    UniquePtr<AString> u( FNEW( AString( "str1" ) ) );
    u.Replace( FNEW( AString( "str2" ) ) );
    TEST_ASSERT( u.Get() != nullptr );
    TEST_ASSERT( u->Equals( "str2" ) );
}

//------------------------------------------------------------------------------
TEST_CASE( TestUniquePtr, ArrayOf )
{
    Array<UniquePtr<AString>> a;
    a.EmplaceBack( FNEW( AString( "str1" ) ) );

    // add multiple items so array resizes/moves
    a.EmplaceBack( FNEW( AString( "str2" ) ) );
    a.EmplaceBack( FNEW( AString( "str3" ) ) );
    a.EmplaceBack( FNEW( AString( "str4" ) ) );
    a.EmplaceBack( FNEW( AString( "str5" ) ) );

    // Ensure pointers have been property retained
    ASSERT( a[ 0 ]->Equals( "str1" ) );
    ASSERT( a[ 1 ]->Equals( "str2" ) );
    ASSERT( a[ 2 ]->Equals( "str3" ) );
    ASSERT( a[ 3 ]->Equals( "str4" ) );
    ASSERT( a[ 4 ]->Equals( "str5" ) );
}

//------------------------------------------------------------------------------
