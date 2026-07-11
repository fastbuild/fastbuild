// TestAString.cpp
//------------------------------------------------------------------------------

// Includes
//------------------------------------------------------------------------------
#include "TestFramework/TestGroup.h"

// Core
#include "Core/Containers/UniquePtr.h"
#include "Core/Strings/AStackString.h"
#include "Core/Strings/AString.h"

#include <memory.h>

//------------------------------------------------------------------------------
TEST_GROUP( TestAString, TestGroupTest )
{
public:
    // Helpers
    template <class SRC, class DST, uint32_t EXPECTED_ALLOCS, class SRC_CAST = SRC>
    void MoveConstructorHelper() const;
    template <class SRC, class DST, uint32_t EXPECTED_ALLOCS, class SRC_CAST = SRC>
    void MoveAssignmentHelper() const;
    template <bool REMOVE_QUOTES = false>
    void CheckTokenize( const char * originalString,
                        char splitChar,
                        const char * expectedToken1 = nullptr,
                        const char * expectedToken2 = nullptr,
                        const char * expectedToken3 = nullptr,
                        const char * expectedToken4 = nullptr ) const;
    template <bool REMOVE_QUOTES = false>
    void CheckTokenize( const char * originalString,
                        const char * expectedToken1 = nullptr,
                        const char * expectedToken2 = nullptr,
                        const char * expectedToken3 = nullptr,
                        const char * expectedToken4 = nullptr ) const;
};

//------------------------------------------------------------------------------
TEST_CASE( TestAString, AStringConstructors )
{
    {
        // AString with no arguments
        AString empty;
        TEST_ASSERT( empty.GetLength() == 0 );
        TEST_ASSERT( empty.GetReserved() == 0 );
        TEST_ASSERT( empty.IsEmpty() == true );
        TEST_ASSERT( empty.MemoryMustBeFreed() == false );
    }
    {
        // AString with reserve capacity argument
        AString empty( 16 );
        TEST_ASSERT( empty.GetLength() == 0 );
        TEST_ASSERT( empty.GetReserved() == 16 );
        TEST_ASSERT( empty.IsEmpty() == true );
        TEST_ASSERT( empty.MemoryMustBeFreed() == true );
    }
    {
        // AString from char *
        AString fromCharStar( "hello" );
        TEST_ASSERT( fromCharStar.GetLength() == 5 );
        TEST_ASSERT( fromCharStar.GetReserved() >= 5 );
        TEST_ASSERT( fromCharStar.IsEmpty() == false );
        TEST_ASSERT( fromCharStar.MemoryMustBeFreed() == true );

        // AString from AString
        AString fromAString( fromCharStar );
        TEST_ASSERT( fromAString.GetLength() == 5 );
        TEST_ASSERT( fromAString.GetReserved() >= 5 );
        TEST_ASSERT( fromAString.IsEmpty() == false );
        TEST_ASSERT( fromAString.MemoryMustBeFreed() == true );
    }
    {
        const char * hello = "hellohellohello";
        AString fromCharStarPair( hello, hello + 5 );
        TEST_ASSERT( fromCharStarPair.GetLength() == 5 );
        TEST_ASSERT( fromCharStarPair.GetReserved() >= 5 );
        TEST_ASSERT( fromCharStarPair.IsEmpty() == false );
        TEST_ASSERT( fromCharStarPair.MemoryMustBeFreed() == true );
    }
}

//------------------------------------------------------------------------------
TEST_CASE( TestAString, AStringAssignment )
{
    AString str;
    str = "test";
    TEST_ASSERT( str.GetLength() == 4 );
    TEST_ASSERT( str.GetReserved() >= 4 );
    TEST_ASSERT( str.IsEmpty() == false );
    TEST_ASSERT( str.MemoryMustBeFreed() == true );

    AString str2;
    str2 = str;
    TEST_ASSERT( str2.GetLength() == 4 );
    TEST_ASSERT( str2.GetReserved() >= 4 );
    TEST_ASSERT( str2.IsEmpty() == false );
    TEST_ASSERT( str2.MemoryMustBeFreed() == true );

    const char * testData = "hellozzzzzzzzz";
    AString str3;
    str3.Assign( testData, testData + 5 );
    TEST_ASSERT( str3.GetLength() == 5 );
    TEST_ASSERT( str3.GetReserved() >= 5 );
    TEST_ASSERT( str3.IsEmpty() == false );
    TEST_ASSERT( str3.MemoryMustBeFreed() == true );

    // assign empty
    {
        AString dst;
        dst.Assign( AString::GetEmpty() );
    }
    {
        AString dst;
        dst.Assign( AString::GetEmpty().Get(), AString::GetEmpty().Get() );
    }
}

//------------------------------------------------------------------------------
TEST_CASE( TestAString, AStackStringConstructors )
{
    {
        // AStackString with no arguments
        AStackString empty;
        TEST_ASSERT( empty.GetLength() == 0 );
        TEST_ASSERT( empty.GetReserved() > 0 );
        TEST_ASSERT( empty.IsEmpty() == true );
        TEST_ASSERT( empty.MemoryMustBeFreed() == false );
    }
    {
        // AStackString from char *
        AStackString fromCharStar( "hello" );
        TEST_ASSERT( fromCharStar.GetLength() == 5 );
        TEST_ASSERT( fromCharStar.GetReserved() >= 5 );
        TEST_ASSERT( fromCharStar.IsEmpty() == false );
        TEST_ASSERT( fromCharStar.MemoryMustBeFreed() == false );

        // AStackString from AStackString
        AStackString fromAString( fromCharStar );
        TEST_ASSERT( fromAString.GetLength() == 5 );
        TEST_ASSERT( fromAString.GetReserved() >= 5 );
        TEST_ASSERT( fromAString.IsEmpty() == false );
        TEST_ASSERT( fromAString.MemoryMustBeFreed() == false );
    }
    {
        // AStackString from AString
        AString aString( "hello" );
        AStackString fromAString( aString );
        TEST_ASSERT( fromAString.GetLength() == 5 );
        TEST_ASSERT( fromAString.GetReserved() >= 5 );
        TEST_ASSERT( fromAString.IsEmpty() == false );
        TEST_ASSERT( fromAString.MemoryMustBeFreed() == false );
    }
    {
        const char * hello = "hellohellohello";
        AStackString fromCharStarPair( hello, hello + 5 );
        TEST_ASSERT( fromCharStarPair.GetLength() == 5 );
        TEST_ASSERT( fromCharStarPair.GetReserved() >= 5 );
        TEST_ASSERT( fromCharStarPair.IsEmpty() == false );
        TEST_ASSERT( fromCharStarPair.MemoryMustBeFreed() == false );
    }
}

//------------------------------------------------------------------------------
TEST_CASE( TestAString, AStackStringOverflow )
{
    {
        // constructor with string longer than buffer
        AStackString<8> string( "01234567890123456789" );
        TEST_ASSERT( string.GetLength() == 20 );
        TEST_ASSERT( string.GetLength() == AString::StrLen( string.Get() ) );
    }
    {
        // assigned of string longer than buffer
        AStackString<8> string;
        string = "01234567890123456789";
        TEST_ASSERT( string.GetLength() == 20 );
        TEST_ASSERT( string.GetLength() == AString::StrLen( string.Get() ) );
    }
    {
        // concatenation of string longer than buffer
        AStackString<8> string;
        string += "01234567890123456789";
        TEST_ASSERT( string.GetLength() == 20 );
        TEST_ASSERT( string.GetLength() == AString::StrLen( string.Get() ) );
    }
}

//------------------------------------------------------------------------------
TEST_CASE( TestAString, BigString )
{
    // create a massive string
    UniquePtr<char, FreeDeletor> mem( (char *)ALLOC( ( 10 * MEGABYTE ) + 1 ) );
    memset( mem.Get(), 'a', 10 * MEGABYTE );
    mem.Get()[ 10 * MEGABYTE ] = '\000';

    // create a stack string
    AStackString<2048> string;

    // now overflow massively so the buffer resizes
    // (string will now be 10MB, which is well over the 64K limit
    // previously imposed on strings)
    string += mem.Get();
    TEST_ASSERT( string.GetLength() == 10 * MEGABYTE );
    TEST_ASSERT( string.GetLength() == AString::StrLen( string.Get() ) );
}

//------------------------------------------------------------------------------
TEST_CASE( TestAString, CharacterHelpers )
{
    const AStackString lowerLetters( "abcdefghijklmnopqrstuvwxyz" );
    const AStackString upperLetters( "ABCDEFGHIJKLMNOPQRSTUZWXYZ" );
    const AStackString letters( "ABCDEFGHIJKLMNOPQRSTUZWXYZabcdefghijklmnopqrstuvwxyz" );
    const AStackString numbers( "0123456789" );
    const AStackString whitespace( " \r\n\t" );

    // IsWhitespace
    for ( const char c : whitespace )
    {
        TEST_ASSERT( AString::IsWhitespace( c ) );
    }
    for ( const char c : numbers )
    {
        TEST_ASSERT( AString::IsWhitespace( c ) == false );
    }
    for ( const char c : letters )
    {
        TEST_ASSERT( AString::IsWhitespace( c ) == false );
    }

    // IsUppercaseLetter
    for ( const char c : upperLetters )
    {
        TEST_ASSERT( AString::IsUppercaseLetter( c ) );
    }
    for ( const char c : lowerLetters )
    {
        TEST_ASSERT( AString::IsUppercaseLetter( c ) == false );
    }
    for ( const char c : whitespace )
    {
        TEST_ASSERT( AString::IsUppercaseLetter( c ) == false );
    }
    for ( const char c : numbers )
    {
        TEST_ASSERT( AString::IsUppercaseLetter( c ) == false );
    }

    // IsLowercaseLetter
    for ( const char c : lowerLetters )
    {
        TEST_ASSERT( AString::IsLowercaseLetter( c ) );
    }
    for ( const char c : upperLetters )
    {
        TEST_ASSERT( AString::IsLowercaseLetter( c ) == false );
    }
    for ( const char c : whitespace )
    {
        TEST_ASSERT( AString::IsLowercaseLetter( c ) == false );
    }
    for ( const char c : numbers )
    {
        TEST_ASSERT( AString::IsLowercaseLetter( c ) == false );
    }

    // IsLetter
    for ( const char c : letters )
    {
        TEST_ASSERT( AString::IsLetter( c ) );
    }
    for ( const char c : whitespace )
    {
        TEST_ASSERT( AString::IsLetter( c ) == false );
    }
    for ( const char c : numbers )
    {
        TEST_ASSERT( AString::IsLetter( c ) == false );
    }

    // IsNumber
    for ( const char c : numbers )
    {
        TEST_ASSERT( AString::IsNumber( c ) );
    }
    for ( const char c : letters )
    {
        TEST_ASSERT( AString::IsNumber( c ) == false );
    }
    for ( const char c : whitespace )
    {
        TEST_ASSERT( AString::IsNumber( c ) == false );
    }
}

//------------------------------------------------------------------------------
TEST_CASE( TestAString, Clear )
{
    {
        AString str( "String" );
        str.Clear();
        TEST_ASSERT( str.IsEmpty() );
        TEST_ASSERT( str.GetLength() == 0 );
    }
    {
        AStackString str( "String" );
        str.Clear();
        TEST_ASSERT( str.IsEmpty() );
        TEST_ASSERT( str.GetLength() == 0 );
    }
}

//------------------------------------------------------------------------------
TEST_CASE( TestAString, ClearAndFreeMemory )
{
    // AString
    {
        // Take note of memory state before
        TEST_MEMORY_SNAPSHOT( s1 );

        AString str( "String" );
        str.ClearAndFreeMemory();
        TEST_ASSERT( str.IsEmpty() );
        TEST_ASSERT( str.GetLength() == 0 );
        TEST_ASSERT( AString::StrLen( str.Get() ) == 0 );
        TEST_ASSERT( str.GetReserved() == 0 );

        // Check 1 alloc occurred but is no longer active
        TEST_EXPECT_ALLOCATION_EVENTS( s1, 1u )
        TEST_EXPECT_INCREASED_ACTIVE_ALLOCATIONS( s1, 0u )
    }
    // AString never used
    {
        AString str;
        str.ClearAndFreeMemory();

        TEST_ASSERT( str.IsEmpty() );
        TEST_ASSERT( str.GetLength() == 0 );
        TEST_ASSERT( AString::StrLen( str.Get() ) == 0 );
        TEST_ASSERT( str.GetReserved() == 0 );
    }
    // AStackString
    {
        // Take note of memory state before
        TEST_MEMORY_SNAPSHOT( s1 );

        AStackString str( "String" );
        str.ClearAndFreeMemory();
        TEST_ASSERT( str.IsEmpty() );
        TEST_ASSERT( str.GetLength() == 0 );
        TEST_ASSERT( AString::StrLen( str.Get() ) == 0 );
        TEST_ASSERT( str.GetReserved() > 0 ); // Stack reservation can be retained

        // Check no allocs occurred
        TEST_EXPECT_ALLOCATION_EVENTS( s1, 0u )
        TEST_EXPECT_INCREASED_ACTIVE_ALLOCATIONS( s1, 0u )
    }
    // AStackString never used
    {
        AStackString str;
        str.ClearAndFreeMemory();

        TEST_ASSERT( str.IsEmpty() );
        TEST_ASSERT( str.GetLength() == 0 );
        TEST_ASSERT( AString::StrLen( str.Get() ) == 0 );
        TEST_ASSERT( str.GetReserved() > 0 ); // Stack reservation can be retained
    }
    // AStackString overflows reservation
    {
        // Take note of memory state before
        TEST_MEMORY_SNAPSHOT( s1 );

        AStackString str;
        str.SetLength( 1024 );
        str.ClearAndFreeMemory();
        TEST_ASSERT( str.IsEmpty() );
        TEST_ASSERT( str.GetLength() == 0 );
        TEST_ASSERT( AString::StrLen( str.Get() ) == 0 );
        TEST_ASSERT( str.GetReserved() == 0 ); // Stack reservation can't be retained

        // Check 1 alloc occurred but is no longer active
        TEST_EXPECT_ALLOCATION_EVENTS( s1, 1u )
        TEST_EXPECT_INCREASED_ACTIVE_ALLOCATIONS( s1, 0u )
    }
}

//------------------------------------------------------------------------------
TEST_CASE( TestAString, Compare )
{
    AStackString str( "hello" );

    // Equals
    TEST_ASSERT( 0 == str.Compare( "hello" ) );
    TEST_ASSERT( 0 == str.Compare( AStackString( "hello" ) ) );
    TEST_ASSERT( 0 != str.Compare( "goodbye" ) );
    TEST_ASSERT( 0 != str.Compare( AStackString( "goodbye" ) ) );

    // EqualsI
    TEST_ASSERT( 0 == str.CompareI( "hEllO" ) );
    TEST_ASSERT( 0 == str.CompareI( AStackString( "hEllO" ) ) );
    TEST_ASSERT( 0 != str.CompareI( "goodbye" ) );
    TEST_ASSERT( 0 != str.CompareI( AStackString( "goodbye" ) ) );
}

//------------------------------------------------------------------------------
TEST_CASE( TestAString, Concatenation )
{
    // Ensure empty strings are correctly handled
    // Because an empty AString is a special case pointer to a global empty
    // buffer, it's read-only
    {
        AString a, b;
        a += b;
        TEST_ASSERT( a.IsEmpty() );
    }
    {
        AString a;
        const char * b = "";
        a += b;
        TEST_ASSERT( a.IsEmpty() );
    }
    {
        AString a, b;
        a.Append( b );
        TEST_ASSERT( a.IsEmpty() );
    }
    {
        AString a;
        const char * b = "";
        a.Append( b, static_cast<size_t>( 0 ) );
        TEST_ASSERT( a.IsEmpty() );
    }
    {
        AString a;
        const char * b = "";
        a.Append( b, b );
        TEST_ASSERT( a.IsEmpty() );
    }

    // Non-empty strings
    {
        AStackString a;
        const char * b = "hello";
        a.Append( b, AString::StrLen( b ) );
        TEST_ASSERT( a == "hello" );
    }
    {
        AStackString a;
        const char * b = "hello";
        a.Append( b, b + AString::StrLen( b ) );
        TEST_ASSERT( a == "hello" );
    }
    {
        AStackString a;
        a.Append( AStackString( "hello" ) );
        TEST_ASSERT( a == "hello" );
    }

    // Arrays
    {
        AStackString a;
        StackArray<AString> strings;
        strings.EmplaceBack( "One" );
        strings.EmplaceBack( "Two" );
        strings.EmplaceBack( "Three" );
        a.AppendList( strings );
        TEST_ASSERT( a == "One,Two,Three" );
    }
}

//------------------------------------------------------------------------------
TEST_CASE( TestAString, EmbeddedNuls )
{
    // Create a string with an embedded nul and check various behaviours
    AStackString string( "0123456789" );
    const uint32_t originalStringLen = string.GetLength();
    string[ 5 ] = 0; // insert null terminator

    // Copy construction
    {
        AString copy( string );
        TEST_ASSERT( copy.GetLength() == originalStringLen );
        TEST_ASSERT( memcmp( "01234"
                             "\0"
                             "6789",
                             copy.Get(),
                             originalStringLen ) == 0 );
    }

    // Assignment (operator =)
    {
        AString copy;
        copy = string;
        TEST_ASSERT( copy.GetLength() == originalStringLen );
        TEST_ASSERT( memcmp( "01234"
                             "\0"
                             "6789",
                             copy.Get(),
                             originalStringLen ) == 0 );
    }

    // Assignment (Assign)
    {
        AString copy;
        copy.Assign( string );
        TEST_ASSERT( copy.GetLength() == originalStringLen );
        TEST_ASSERT( memcmp( "01234"
                             "\0"
                             "6789",
                             copy.Get(),
                             originalStringLen ) == 0 );
    }

    // Assignment (Assign with iterators)
    {
        AString copy;
        copy.Assign( string.Get(), string.GetEnd() );
        TEST_ASSERT( copy.GetLength() == originalStringLen );
        TEST_ASSERT( memcmp( "01234"
                             "\0"
                             "6789",
                             copy.Get(),
                             originalStringLen ) == 0 );
    }

    // Append (operator +=)
    {
        // Append to empty
        AString copy;
        copy += string;
        TEST_ASSERT( copy.GetLength() == originalStringLen );
        TEST_ASSERT( AString::StrNCmp( string.Get(), copy.Get(), originalStringLen ) == 0 );
        TEST_ASSERT( memcmp( "01234"
                             "\0"
                             "6789",
                             copy.Get(),
                             originalStringLen ) == 0 );

        // Append to existing
        AString copy2( string );
        copy2 += string;
        TEST_ASSERT( copy2.GetLength() == ( originalStringLen * 2 ) );
        TEST_ASSERT( memcmp( "01234"
                             "\0"
                             "678901234"
                             "\0"
                             "6789",
                             copy2.Get(),
                             ( originalStringLen * 2 ) ) == 0 );
    }

    // Append (Append)
    {
        // Append to empty
        AString copy;
        copy.Append( string );
        TEST_ASSERT( copy.GetLength() == originalStringLen );
        TEST_ASSERT( memcmp( "01234"
                             "\0"
                             "6789",
                             copy.Get(),
                             originalStringLen ) == 0 );

        // Append to existing
        AString copy2( string );
        copy2.Append( string );
        TEST_ASSERT( copy2.GetLength() == ( originalStringLen * 2 ) );
        TEST_ASSERT( memcmp( "01234"
                             "\0"
                             "678901234"
                             "\0"
                             "6789",
                             copy2.Get(),
                             ( originalStringLen * 2 ) ) == 0 );
    }
}

//------------------------------------------------------------------------------
TEST_CASE( TestAString, EndsWithI )
{
    // empty string is handled
    {
        AString empty;
        TEST_ASSERT( empty.EndsWithI( AString( "hello" ) ) == false );
        TEST_ASSERT( empty.EndsWithI( "hello" ) == false );
    }

    // empty compare string is handled
    {
        AString empty;
        TEST_ASSERT( empty.EndsWithI( AString::GetEmpty() ) == true );
        TEST_ASSERT( empty.EndsWithI( "" ) == true );
    }

    // compare with longer string is handled
    {
        AString shortString( "short" );
        TEST_ASSERT( shortString.EndsWithI( AString( "Does not end with this" ) ) == false );
        TEST_ASSERT( shortString.EndsWithI( "Does not end with this" ) == false );
    }

    // compare with shorter string is handled
    {
        AString str( "this is a string ending with Chicken" );
        TEST_ASSERT( str.EndsWithI( "Chicken" ) == true );
        TEST_ASSERT( str.EndsWithI( "chicken" ) == true );
        TEST_ASSERT( str.EndsWithI( "ICKEN" ) == true );
        TEST_ASSERT( str.EndsWithI( "Chicken " ) == false );
        TEST_ASSERT( str.EndsWithI( "Turkey" ) == false );
    }
}

//------------------------------------------------------------------------------
TEST_CASE( TestAString, Equals )
{
    AStackString str( "hello" );

    // Equals
    TEST_ASSERT( true == str.Equals( "hello" ) );
    TEST_ASSERT( true == str.Equals( AStackString( "hello" ) ) );
    TEST_ASSERT( false == str.Equals( "goodbye" ) );
    TEST_ASSERT( false == str.Equals( AStackString( "goodbye" ) ) );

    // EqualsI
    TEST_ASSERT( true == str.EqualsI( "hEllO" ) );
    TEST_ASSERT( true == str.EqualsI( AStackString( "hEllO" ) ) );
    TEST_ASSERT( false == str.EqualsI( "goodbye" ) );
    TEST_ASSERT( false == str.EqualsI( AStackString( "goodbye" ) ) );
}

//------------------------------------------------------------------------------
TEST_CASE( TestAString, Find )
{
    {
        AStackString str( "the quick brown fox jumps over the lazy dog" );

        TEST_ASSERT( str.Find( 't' ) == str.Get() );
        TEST_ASSERT( str.Find( 't', str.Get() + 1 ) == str.Get() + 31 );
        TEST_ASSERT( str.Find( 't', str.Get() + 1, str.Get() + 31 ) == nullptr );

        TEST_ASSERT( str.Find( "the" ) == str.Get() );
        TEST_ASSERT( str.Find( "the", str.Get() + 1 ) == str.Get() + 31 );
        TEST_ASSERT( str.Find( "the", str.Get() + 1, str.Get() + 31 ) == nullptr );

        TEST_ASSERT( str.Find( AStackString( "the" ) ) == str.Get() );
        TEST_ASSERT( str.Find( AStackString( "the" ), str.Get() + 1 ) == str.Get() + 31 );
        TEST_ASSERT( str.Find( AStackString( "the" ), str.Get() + 1, str.Get() + 31 ) == nullptr );
    }

    // BUG: Returning contents past end of string
    {
        AString tmp( "12345678--X---" );
        tmp.SetLength( 8 );
        TEST_ASSERT( tmp.Find( 'X' ) == nullptr );                // This was OK
        TEST_ASSERT( tmp.Find( 'X', tmp.Get() + 8 ) == nullptr ); // This was returning junk data past end of string
    }
}

//------------------------------------------------------------------------------
TEST_CASE( TestAString, FindI )
{
    AStackString str( "ThE qUiCk BrOwN fOx JuMpS oVeR tHe LaZy DoG" );

    TEST_ASSERT( str.FindI( 't' ) == str.Get() );
    TEST_ASSERT( str.FindI( 't', str.Get() + 1 ) == str.Get() + 31 );
    TEST_ASSERT( str.FindI( 't', str.Get() + 1, str.Get() + 31 ) == nullptr );

    TEST_ASSERT( str.FindI( "the" ) == str.Get() );
    TEST_ASSERT( str.FindI( "the", str.Get() + 1 ) == str.Get() + 31 );
    TEST_ASSERT( str.FindI( "the", str.Get() + 1, str.Get() + 31 ) == nullptr );

    TEST_ASSERT( str.FindI( AStackString( "the" ) ) == str.Get() );
    TEST_ASSERT( str.FindI( AStackString( "the" ), str.Get() + 1 ) == str.Get() + 31 );
    TEST_ASSERT( str.FindI( AStackString( "the" ), str.Get() + 1, str.Get() + 31 ) == nullptr );
}

//------------------------------------------------------------------------------
TEST_CASE( TestAString, FindLast )
{
    {
        AStackString str( "the quick brown fox jumps over the lazy dog" );

        TEST_ASSERT( str.FindLast( 't' ) == str.Get() + 31 );
        TEST_ASSERT( str.FindLast( 't', str.Get() + 30 ) == str.Get() );
        TEST_ASSERT( str.FindLast( 't', str.Get() + 30, str.Get() + 1 ) == nullptr );

        TEST_ASSERT( str.FindLast( "the" ) == str.Get() + 31 );
        TEST_ASSERT( str.FindLast( "the", str.Get() + 30 ) == str.Get() );
        TEST_ASSERT( str.FindLast( "the", str.Get() + 30, str.Get() + 1 ) == nullptr );

        TEST_ASSERT( str.FindLast( AStackString( "the" ) ) == str.Get() + 31 );
        TEST_ASSERT( str.FindLast( AStackString( "the" ), str.Get() + 30 ) == str.Get() );
        TEST_ASSERT( str.FindLast( AStackString( "the" ), str.Get() + 30, str.Get() + 1 ) == nullptr );
    }

    // BUG: Returning contents past end of string
    {
        AString tmp( "12345678--X---" );
        tmp.SetLength( 8 );
        TEST_ASSERT( tmp.Find( 'X' ) == nullptr );                // This was OK
        TEST_ASSERT( tmp.Find( 'X', tmp.Get() + 8 ) == nullptr ); // This was returning junk data past end of string
    }
}

//------------------------------------------------------------------------------
TEST_CASE( TestAString, FindLastI )
{
    AStackString str( "ThE qUiCk BrOwN fOx JuMpS oVeR tHe LaZy DoG" );

    TEST_ASSERT( str.FindLastI( 't' ) == str.Get() + 31 );
    TEST_ASSERT( str.FindLastI( 't', str.Get() + 30 ) == str.Get() );
    TEST_ASSERT( str.FindLastI( 't', str.Get() + 30, str.Get() + 1 ) == nullptr );

    TEST_ASSERT( str.FindLastI( "the" ) == str.Get() + 31 );
    TEST_ASSERT( str.FindLastI( "the", str.Get() + 30 ) == str.Get() );
    TEST_ASSERT( str.FindLastI( "the", str.Get() + 30, str.Get() + 1 ) == nullptr );

    TEST_ASSERT( str.FindLastI( AStackString( "the" ) ) == str.Get() + 31 );
    TEST_ASSERT( str.FindLastI( AStackString( "the" ), str.Get() + 30 ) == str.Get() );
    TEST_ASSERT( str.FindLastI( AStackString( "the" ), str.Get() + 30, str.Get() + 1 ) == nullptr );
}

//------------------------------------------------------------------------------
TEST_CASE( TestAString, Format )
{
    // Create a really long input string
    AStackString longInput;
    const size_t longStringLen( 1024 * 1024 );
    for ( size_t i = 0; i < longStringLen; ++i )
    {
        longInput += 'A';
    }

    // Make sure we correctly handle formatting large strings
    AStackString buffer;
    buffer.Format( "%s", longInput.Get() );
    TEST_ASSERT( buffer.GetLength() == longStringLen );
    TEST_ASSERT( AString::StrLen( buffer.Get() ) == longStringLen );
}

//------------------------------------------------------------------------------
TEST_CASE( TestAString, Tokenize )
{
    // empty
    CheckTokenize( "" );

    // one token
    CheckTokenize( "one_token", "one_token" );

    // multiple tokens
    CheckTokenize( "this is four tokens", "this", "is", "four", "tokens" );

    // whitespace
    CheckTokenize( "     token", "token" );
    CheckTokenize( "token      ", "token" );
    CheckTokenize( "   lots  of      spaces   ", "lots", "of", "spaces" );

    // double quotes
    CheckTokenize<false>( R"(this is "only three tokens")", "this", "is", R"("only three tokens")" );
    CheckTokenize<true>( R"(this is "only three tokens")", "this", "is", "only three tokens" );

    // quotes inside string
    CheckTokenize<false>( R"(this is -DARG="a b")", "this", "is", R"(-DARG="a b")" );
    CheckTokenize<true>( R"(this is -DARG="a b")", "this", "is", R"(-DARG=a b)" );

    // escaped quotes
    CheckTokenize<false>( R"(-D=\")", R"(-D=\")" );
    CheckTokenize<true>( R"(-D=\")", R"(-D=")" );
    CheckTokenize<false>( R"(-D=\" -D2)", R"(-D=\")", R"(-D2)" );
    CheckTokenize<true>( R"(-D=\" -D2)", R"(-D=")", R"(-D2)" );
    CheckTokenize<false>( R"("-D=   \"   ")", R"("-D=   \"   ")" );
    CheckTokenize<true>( R"("-D=   \"   ")", R"(-D=   "   )" );
    CheckTokenize<false>( R"("-D=\" string \"  ")", R"("-D=\" string \"  ")" );
    CheckTokenize<true>( R"("-D=\" string \"  ")", R"(-D=" string "  )" );
    CheckTokenize<false>( R"(\")", R"(\")" );
    CheckTokenize<true>( R"(\")", R"(")" );

    // malformed - ensure unterminated quotes are correctly handled
    CheckTokenize<false>( R"(-X=")", R"(-X=")" );
    CheckTokenize<true>( R"(-X=")", R"(-X=)" );

    // alternate split char
    CheckTokenize( "c:\\path\\path;d:\\path;e:\\", ';', "c:\\path\\path", "d:\\path", "e:\\" );
}

//------------------------------------------------------------------------------
TEST_CASE( TestAString, PatternMatch )
{
#define CHECK_MATCH( pat, str, match )              \
    do                                                  \
    {                                                   \
        AStackString string( str );                     \
        TEST_ASSERT( string.Matches( pat ) == match );  \
    } while ( false )

    CHECK_MATCH( "*.cpp",   "File.cpp", true );
    CHECK_MATCH( "*",       "File.cpp", true );
    CHECK_MATCH( "File*.*", "File.cpp", true );
    CHECK_MATCH( "*.c*",    "File.cpp", true );
    CHECK_MATCH( "File.cpp","File.cpp", true );

    CHECK_MATCH( "*.cpp",   "File.CPP", false );
    CHECK_MATCH( "File*.*", "FILE.cpp", false );
    CHECK_MATCH( "*.c*",    "File.CPP", false );
    CHECK_MATCH( "File.cpp","file.cpp", false );

    CHECK_MATCH( "*.cpp",   "File.c",           false );
    CHECK_MATCH( "*.cpp",   "File.cpp~",        false );
    CHECK_MATCH( "*.cpp",   "File.cpp.notcpp",  false );
    CHECK_MATCH( "*.cpp",   "",                 false );

    CHECK_MATCH( "*.cpp",   "File.c",           false );
    CHECK_MATCH( "*.cpp",   "File.cpp~",        false );
    CHECK_MATCH( "*.cpp",   "File.cpp.notcpp",  false );
    CHECK_MATCH( "*.cpp",   "",                 false );

#undef CHECK_MATCH
}

//------------------------------------------------------------------------------
TEST_CASE( TestAString, PatternMatchI )
{
#define CHECK_MATCH( pat, str, match )              \
    do                                                  \
    {                                                   \
        AStackString string( str );                     \
        TEST_ASSERT( string.MatchesI( pat ) == match ); \
    } while ( false )

    CHECK_MATCH( "*.cpp",   "File.cpp", true );
    CHECK_MATCH( "*",       "File.cpp", true );
    CHECK_MATCH( "File*.*", "File.cpp", true );
    CHECK_MATCH( "*.c*",    "File.cpp", true );
    CHECK_MATCH( "File.cpp","File.cpp", true );

    CHECK_MATCH( "*.cpp",   "File.CPP", true );
    CHECK_MATCH( "File*.*", "FILE.cpp", true );
    CHECK_MATCH( "*.c*",    "File.CPP", true );
    CHECK_MATCH( "File.cpp","file.cpp", true );

    CHECK_MATCH( "*.cpp",   "File.c",           false );
    CHECK_MATCH( "*.cpp",   "File.cpp~",        false );
    CHECK_MATCH( "*.cpp",   "File.cpp.notcpp",  false );
    CHECK_MATCH( "*.cpp",   "",                 false );

#undef CHECK_MATCH
}

//------------------------------------------------------------------------------
TEST_CASE( TestAString, Replace )
{
    // Replace empty - make sure this is correctly handled
    AStackString test( "Test" );
    test.Replace( "", "" );
}

//------------------------------------------------------------------------------
TEST_CASE( TestAString, Trim )
{
    {
        // No trim
        AStackString empty;
        empty.Trim( 0, 0 );
    }

    {
        // Left trim
        AStackString test( "zzHello" );
        test.Trim( 2, 0 );
        TEST_ASSERT( test.GetLength() == 5 );
        TEST_ASSERT( test == "Hello" );
    }

    {
        // Right trim
        AStackString test( "Hellozz" );
        test.Trim( 0, 2 );
        TEST_ASSERT( test.GetLength() == 5 );
        TEST_ASSERT( test == "Hello" );
    }

    {
        // Trim left and right
        AStackString test( "zzHellozz" );
        test.Trim( 2, 2 );
        TEST_ASSERT( test.GetLength() == 5 );
        TEST_ASSERT( test == "Hello" );
    }
}

//------------------------------------------------------------------------------
TEST_CASE( TestAString, TrimStart )
{
    {
        // No trim (empty)
        AStackString empty;
        empty.TrimStart( 'x' );
    }

    {
        // No trim (doesn't start with)
        AStackString test( "String" );
        test.TrimStart( 'x' );
        TEST_ASSERT( test.GetLength() == 6 );
    }

    {
        // No trim (doesn't start with)
        AStackString test( "Stringxx" );
        test.TrimStart( 'x' );
        TEST_ASSERT( test.GetLength() == 8 );
    }

    {
        // Trim
        AStackString test( "xxString" );
        test.TrimStart( 'x' );
        TEST_ASSERT( test.GetLength() == 6 );
        TEST_ASSERT( test == "String" );
    }

    {
        // Trim (entire string)
        AStackString test( "xxxx" );
        test.TrimStart( 'x' );
        TEST_ASSERT( test.IsEmpty() );
    }
}

//------------------------------------------------------------------------------
TEST_CASE( TestAString, TrimEnd )
{
    {
        // No trim (empty)
        AStackString empty;
        empty.TrimEnd( 'x' );
    }

    {
        // No trim (doesn't end with)
        AStackString test( "String" );
        test.TrimEnd( 'x' );
        TEST_ASSERT( test.GetLength() == 6 );
    }

    {
        // No trim (doesn't end with)
        AStackString test( "xxString" );
        test.TrimEnd( 'x' );
        TEST_ASSERT( test.GetLength() == 8 );
    }

    {
        // Trim
        AStackString test( "Stringxx" );
        test.TrimEnd( 'x' );
        TEST_ASSERT( test.GetLength() == 6 );
        TEST_ASSERT( test == "String" );
    }

    {
        // Trim (entire string)
        AStackString test( "xxxx" );
        test.TrimEnd( 'x' );
        TEST_ASSERT( test.IsEmpty() );
    }
}

// MoveConstructorHelper
//------------------------------------------------------------------------------
template <class SRC, class DST, uint32_t EXPECTED_ALLOCS, class SRC_CAST>
void TestAString::MoveConstructorHelper() const
{
    // Create the source string
    SRC stringA( "string" );

    // Take note of memory state before
    TEST_MEMORY_SNAPSHOT( s1 );

    // Move construct destination. SRC_CAST allows us to check AString/AStackString
    // behave the same
    DST stringB( Move( (SRC_CAST &)( stringA ) ) );

    // Check expected amount of allocs occurred
    TEST_EXPECT_ALLOCATION_EVENTS( s1, EXPECTED_ALLOCS )

    // Source string should be empty
    PRAGMA_DISABLE_PUSH_MSVC( 26800 ) // Use of a moved from object here is deliberate
    TEST_ASSERT( stringA.IsEmpty() );
    PRAGMA_DISABLE_POP_MSVC
}

//------------------------------------------------------------------------------
TEST_CASE( TestAString, MoveConstructor )
{
    //                    Src, Dest, Allocs, SrcCast
    //----------------------------------------------------------------------
    // Moves from heap can be performed
    MoveConstructorHelper<AString, AString, 0>();
    MoveConstructorHelper<AString, AStackString<>, 0>();

    // Moves from stack to stack are copies, but avoid memory allocation
    MoveConstructorHelper<AStackString<>, AStackString<>, 0>();
    MoveConstructorHelper<AStackString<>, AStackString<>, 0, AString>(); // Src as AString, behave the same

    // Moves from stack to AString need re-allocation and copy
    MoveConstructorHelper<AStackString<>, AString, 1>();
    MoveConstructorHelper<AStackString<>, AString, 1, AString>(); // Src as AString, behave the same
}

// MoveAssignmentHelper
//------------------------------------------------------------------------------
template <class SRC, class DST, uint32_t EXPECTED_ALLOCS, class SRC_CAST>
void TestAString::MoveAssignmentHelper() const
{
    // Empty destination
    {
        // Create the source string
        SRC stringA( "string" );

        // Create the destination
        DST stringB;

        // Take note of memory state before
        TEST_MEMORY_SNAPSHOT( s1 );

        // Move assign. SRC_CAST allows us to check AString/AStackString behave the same
        stringB = Move( (SRC_CAST &)( stringA ) );

        // Check expected amount of allocs occurred
        TEST_EXPECT_ALLOCATION_EVENTS( s1, EXPECTED_ALLOCS )

        // Source string should be empty
        PRAGMA_DISABLE_PUSH_MSVC( 26800 ) // Use of a moved from object here is deliberate
        TEST_ASSERT( stringA.IsEmpty() );
        PRAGMA_DISABLE_POP_MSVC
    }

    // Non-empty destination (check move doesn't leak destination string memory)
    {
        // Take note of memory state before
        TEST_MEMORY_SNAPSHOT( s1 );

        {
            // Create the source string
            SRC stringA( "string" );

            // Create the destination
            DST stringB;
            stringB.SetLength( 512 ); // Allocate some memory, even for AStackString

            // Move assign. SRC_CAST allows us to check AString/AStackString behave the same
            stringB = Move( (SRC_CAST &)( stringA ) );

            // Source string should be empty
            PRAGMA_DISABLE_PUSH_MSVC( 26800 ) // Use of a moved from object here is deliberate
            TEST_ASSERT( stringA.IsEmpty() );
            PRAGMA_DISABLE_POP_MSVC
        }

        // Check should be no more active allocs in total, even if some allocs occurred
        TEST_EXPECT_INCREASED_ACTIVE_ALLOCATIONS( s1, 0u )
    }
}

//------------------------------------------------------------------------------
TEST_CASE( TestAString, MoveAssignment )
{
    //                   Src             Dest            Allocs SrcCast
    //------------------------------------------------------------------
    // Moves from heap can be performed
    MoveAssignmentHelper<AString, AString, 0>();
    MoveAssignmentHelper<AString, AStackString<>, 0>();

    // Moves from stack to stack are copies, but avoid memory allocation
    MoveAssignmentHelper<AStackString<>, AStackString<>, 0>();
    MoveAssignmentHelper<AStackString<>, AStackString<>, 0, AString>(); // Src as AString, behave the same

    // Moves from stack to AString need re-allocation and copy
    MoveAssignmentHelper<AStackString<>, AString, 1>();
    MoveAssignmentHelper<AStackString<>, AString, 1, AString>(); // Src as AString, behave the same
}

// CheckTokenize
//------------------------------------------------------------------------------
template <bool REMOVE_QUOTES>
void TestAString::CheckTokenize( const char * originalString,
                                 char splitChar,
                                 const char * expectedToken1,
                                 const char * expectedToken2,
                                 const char * expectedToken3,
                                 const char * expectedToken4 ) const
{
    // Tokenize
    StackArray<AString> tokens;
    AStackString( originalString ).Tokenize( tokens, splitChar );
    if constexpr ( REMOVE_QUOTES )
    {
        AString::RemoveQuotes( tokens );
    }

    // Check expected count
    const size_t numExpected = ( expectedToken1 ? 1u : 0u ) +
                               ( expectedToken2 ? 1u : 0u ) +
                               ( expectedToken3 ? 1u : 0u ) +
                               ( expectedToken4 ? 1u : 0u );
    TEST_ASSERT( tokens.GetSize() == numExpected );

    // Check tokens contain expected values
    TEST_ASSERT( ( expectedToken1 == nullptr ) || ( tokens[ 0 ] == expectedToken1 ) );
    TEST_ASSERT( ( expectedToken2 == nullptr ) || ( tokens[ 1 ] == expectedToken2 ) );
    TEST_ASSERT( ( expectedToken3 == nullptr ) || ( tokens[ 2 ] == expectedToken3 ) );
    TEST_ASSERT( ( expectedToken4 == nullptr ) || ( tokens[ 3 ] == expectedToken4 ) );
}

// CheckTokenize
//------------------------------------------------------------------------------
template <bool REMOVE_QUOTES>
void TestAString::CheckTokenize( const char * originalString,
                                 const char * expectedToken1,
                                 const char * expectedToken2,
                                 const char * expectedToken3,
                                 const char * expectedToken4 ) const
{
    CheckTokenize<REMOVE_QUOTES>( originalString,
                                  ' ',
                                  expectedToken1,
                                  expectedToken2,
                                  expectedToken3,
                                  expectedToken4 );
}

//------------------------------------------------------------------------------
