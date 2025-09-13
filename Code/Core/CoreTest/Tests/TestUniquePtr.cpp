// TestUniquePtr.cpp
//------------------------------------------------------------------------------

// Includes
//------------------------------------------------------------------------------
#include "TestFramework/TestGroup.h"

// Core
#include "Core/Containers/Array.h"
#include "Core/Containers/UniquePtr.h"
#include "Core/Strings/AString.h"

// TestUniquePtr
//------------------------------------------------------------------------------
class TestUniquePtr : public TestGroup
{
private:
    DECLARE_TESTS

    void Empty() const;
    void FreeMemory() const;
    void Destroy() const;
    void ReleaseOwnership() const;
    void Replace() const;
    void ArrayOf() const;
};

// Register Tests
//------------------------------------------------------------------------------
REGISTER_TESTS_BEGIN( TestUniquePtr )
    REGISTER_TEST( Empty )
    REGISTER_TEST( FreeMemory )
    REGISTER_TEST( Destroy )
    REGISTER_TEST( ReleaseOwnership )
    REGISTER_TEST( Replace )
    REGISTER_TEST( ArrayOf )
REGISTER_TESTS_END

// Empty
//------------------------------------------------------------------------------
void TestUniquePtr::Empty() const
{
    UniquePtr<uint32_t> u;
    TEST_ASSERT( u.Get() == nullptr );
}

// FreeMemory
//------------------------------------------------------------------------------
void TestUniquePtr::FreeMemory() const
{
    UniquePtr<AString> u( FNEW( AString( "str" ) ) );
    TEST_ASSERT( u.Get() != nullptr );
    TEST_ASSERT( u->Equals( "str" ) );
}

// Destroy
//------------------------------------------------------------------------------
void TestUniquePtr::Destroy() const
{
    UniquePtr<AString> u( FNEW( AString( "str" ) ) );
    u.Destroy(); // Manually free
    TEST_ASSERT( u.Get() == nullptr );
}

// ReleaseOwnership
//------------------------------------------------------------------------------
void TestUniquePtr::ReleaseOwnership() const
{
    UniquePtr<AString> u( FNEW( AString( "str" ) ) );
    AString * str = u.ReleaseOwnership();
    TEST_ASSERT( u.Get() == nullptr );
    FDELETE( str );
}

// Replace
//------------------------------------------------------------------------------
void TestUniquePtr::Replace() const
{
    UniquePtr<AString> u( FNEW( AString( "str1" ) ) );
    u.Replace( FNEW( AString( "str2" ) ) );
    TEST_ASSERT( u.Get() != nullptr );
    TEST_ASSERT( u->Equals( "str2" ) );
}

// Array
//------------------------------------------------------------------------------
void TestUniquePtr::ArrayOf() const
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
