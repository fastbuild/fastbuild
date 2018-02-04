// TestAString.cpp
//------------------------------------------------------------------------------

// Includes
//------------------------------------------------------------------------------
#include "TestFramework/UnitTest.h"

#include "Core/Containers/AutoPtr.h"
#include "Core/Strings/AString.h"
#include "Core/Strings/AStackString.h"

#include <memory.h>

// TestAString
//------------------------------------------------------------------------------
class TestAString : public UnitTest
{
private:
    DECLARE_TESTS

    void AStringConstructors() const;
    void AStringAssignment() const;
    void AStackStringConstructors() const;
    void AStackStringOverflow() const;
    void BigString() const;
    void Compare() const;
    void Concatenation() const;
    void EmbeddedNuls() const;
    void EndsWithI() const;
    void Equals() const;
    void Find() const;
    void Format() const;
    void Tokenize() const;
    void PatternMatch() const;
    void PatternMatchI() const;
    void Replace() const;
    void Trim() const;
};

// Register Tests
//------------------------------------------------------------------------------
REGISTER_TESTS_BEGIN( TestAString )
    REGISTER_TEST( AStringConstructors )
    REGISTER_TEST( AStringAssignment )
    REGISTER_TEST( AStackStringConstructors )
    REGISTER_TEST( AStackStringOverflow )
    REGISTER_TEST( BigString )
    REGISTER_TEST( Compare )
    REGISTER_TEST( Concatenation )
    REGISTER_TEST( EmbeddedNuls )
    REGISTER_TEST( EndsWithI )
    REGISTER_TEST( Equals )
    REGISTER_TEST( Find )
    REGISTER_TEST( Format )
    REGISTER_TEST( Tokenize )
    REGISTER_TEST( PatternMatch )
    REGISTER_TEST( PatternMatchI )
    REGISTER_TEST( Replace )
    REGISTER_TEST( Trim )
REGISTER_TESTS_END

// AStringConstructors
//------------------------------------------------------------------------------
void TestAString::AStringConstructors() const
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

// AStringAssignment
//------------------------------------------------------------------------------
void TestAString::AStringAssignment() const
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

// AStackStringConstructors
//------------------------------------------------------------------------------
void TestAString::AStackStringConstructors() const
{
    {
        // AStackString<> with no arguments
        AStackString<> empty;
        TEST_ASSERT( empty.GetLength() == 0 );
        TEST_ASSERT( empty.GetReserved() > 0 );
        TEST_ASSERT( empty.IsEmpty() == true );
        TEST_ASSERT( empty.MemoryMustBeFreed() == false );
    }
    {
        // AStackString<> from char *
        AStackString<> fromCharStar( "hello" );
        TEST_ASSERT( fromCharStar.GetLength() == 5 );
        TEST_ASSERT( fromCharStar.GetReserved() >= 5 );
        TEST_ASSERT( fromCharStar.IsEmpty() == false );
        TEST_ASSERT( fromCharStar.MemoryMustBeFreed() == false );

        // AStackString<> from AStackString
        AStackString<> fromAString( fromCharStar );
        TEST_ASSERT( fromAString.GetLength() == 5 );
        TEST_ASSERT( fromAString.GetReserved() >= 5 );
        TEST_ASSERT( fromAString.IsEmpty() == false );
        TEST_ASSERT( fromAString.MemoryMustBeFreed() == false );
    }
    {
        // AStackString<> from AString
        AString astring( "hello" );
        AStackString<> fromAString( astring );
        TEST_ASSERT( fromAString.GetLength() == 5 );
        TEST_ASSERT( fromAString.GetReserved() >= 5 );
        TEST_ASSERT( fromAString.IsEmpty() == false );
        TEST_ASSERT( fromAString.MemoryMustBeFreed() == false );
    }
    {
        const char * hello = "hellohellohello";
        AStackString<> fromCharStarPair( hello, hello + 5 );
        TEST_ASSERT( fromCharStarPair.GetLength() == 5 );
        TEST_ASSERT( fromCharStarPair.GetReserved() >= 5 );
        TEST_ASSERT( fromCharStarPair.IsEmpty() == false );
        TEST_ASSERT( fromCharStarPair.MemoryMustBeFreed() == false );
    }
}

// AStackStringOverflow
//------------------------------------------------------------------------------
void TestAString::AStackStringOverflow() const
{
    {
        // constructor with string longer than buffer
        AStackString< 8 > string( "01234567890123456789" );
        TEST_ASSERT( string.GetLength() == 20 );
        TEST_ASSERT( string.GetLength() == AString::StrLen( string.Get() ) );
    }
    {
        // assigned of string longer than buffer
        AStackString< 8 > string;
        string = "01234567890123456789";
        TEST_ASSERT( string.GetLength() == 20 );
        TEST_ASSERT( string.GetLength() == AString::StrLen( string.Get() ) );
    }
    {
        // concetentation of string longer than buffer
        AStackString< 8 > string;
        string += "01234567890123456789";
        TEST_ASSERT( string.GetLength() == 20 );
        TEST_ASSERT( string.GetLength() == AString::StrLen( string.Get() ) );
    }
}

// BigString
//------------------------------------------------------------------------------
void TestAString::BigString() const
{
    // create a massive string
    AutoPtr< char > mem( (char *)ALLOC( ( 10 * MEGABYTE ) + 1 ) );
    memset( mem.Get(), 'a', 10 * MEGABYTE );
    mem.Get()[ 10 * MEGABYTE ] = '\000';

    // create a stack string
    AStackString< 2048 > string;

    // now overflow massively so the buffer resizes
    // (string will now be 10MB, which is well over the 64K limit
    // previously imposed on strings)
    string += mem.Get();
    TEST_ASSERT( string.GetLength() == 10 * MEGABYTE );
    TEST_ASSERT( string.GetLength() == AString::StrLen( string.Get() ) );
}

// Compare
//------------------------------------------------------------------------------
void TestAString::Compare() const
{
    AStackString<> str( "hello" );

    // Equals
    TEST_ASSERT( 0 == str.Compare( "hello" ) );
    TEST_ASSERT( 0 == str.Compare( AStackString<>( "hello" ) ) );
    TEST_ASSERT( 0 != str.Compare( "goodbye" ) );
    TEST_ASSERT( 0 != str.Compare( AStackString<>( "goodbye" ) ) );

    // EqualsI
    TEST_ASSERT( 0 == str.CompareI( "hEllO" ) );
    TEST_ASSERT( 0 == str.CompareI( AStackString<>( "hEllO" ) ) );
    TEST_ASSERT( 0 != str.CompareI( "goodbye" ) );
    TEST_ASSERT( 0 != str.CompareI( AStackString<>( "goodbye" ) ) );
}

// Concatenation
//------------------------------------------------------------------------------
void TestAString::Concatenation() const
{
    // Ensure empty strings are correctly handled
    // Because an empty AString is a special case pointer to a global empty
    // buffer, it's read-only
    {
        AString a, b;
        a += b;
    }
    {
        AString a;
        const char * b = "";
        a += b;
    }
    {
        AString a, b;
        a.Append( b );
    }
    {
        AString a;
        const char * b = "";
        a.Append( b, 0 );
    }
}

// EmbeddedNuls
//------------------------------------------------------------------------------
void TestAString::EmbeddedNuls() const
{
    // Create a string with an embedded nul and check various behaviours
    AStackString<> string( "0123456789" );
    const uint32_t originalStringLen = string.GetLength();
    string[ 5 ] = 0; // insert null terminator

    // Copy construction
    {
        AString copy( string );
        TEST_ASSERT( copy.GetLength() == originalStringLen );
        TEST_ASSERT( memcmp( "01234" "\0" "6789", copy.Get(), originalStringLen ) == 0 );
    }

    // Assignment (operator =)
    {
        AString copy;
        copy = string;
        TEST_ASSERT( copy.GetLength() == originalStringLen );
        TEST_ASSERT( memcmp( "01234" "\0" "6789", copy.Get(), originalStringLen ) == 0 );
    }

    // Assignment (Assign)
    {
        AString copy;
        copy.Assign( string );
        TEST_ASSERT( copy.GetLength() == originalStringLen );
        TEST_ASSERT( memcmp( "01234" "\0" "6789", copy.Get(), originalStringLen ) == 0 );
    }

    // Assignment (Assign with iterators)
    {
        AString copy;
        copy.Assign( string.Get(), string.GetEnd() );
        TEST_ASSERT( copy.GetLength() == originalStringLen );
        TEST_ASSERT( memcmp( "01234" "\0" "6789", copy.Get(), originalStringLen ) == 0 );
    }

    // Append (operator +=)
    {
        // Append to empty
        AString copy;
        copy += string;
        TEST_ASSERT( copy.GetLength() == originalStringLen );
        TEST_ASSERT( AString::StrNCmp( string.Get(), copy.Get(), originalStringLen ) == 0 );
        TEST_ASSERT( memcmp( "01234" "\0" "6789", copy.Get(), originalStringLen ) == 0 );

        // Append to existing
        AString copy2( string );
        copy2 += string;
        TEST_ASSERT( copy2.GetLength() == ( originalStringLen * 2 ) );
        TEST_ASSERT( memcmp( "01234" "\0" "678901234" "\0" "6789", copy2.Get(), ( originalStringLen * 2 ) ) == 0 );
    }

    // Append (Append)
    {
        // Append to empty
        AString copy;
        copy.Append( string );
        TEST_ASSERT( copy.GetLength() == originalStringLen );
        TEST_ASSERT( memcmp( "01234" "\0" "6789", copy.Get(), originalStringLen ) == 0 );

        // Append to existing
        AString copy2( string );
        copy2.Append( string );
        TEST_ASSERT( copy2.GetLength() == ( originalStringLen * 2 ) );
        TEST_ASSERT( memcmp( "01234" "\0" "678901234" "\0" "6789", copy2.Get(), ( originalStringLen * 2 ) ) == 0 );
    }
}

// EndsWithI
//------------------------------------------------------------------------------
void TestAString::EndsWithI() const
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

// Equals
//------------------------------------------------------------------------------
void TestAString::Equals() const
{
    AStackString<> str( "hello" );

    // Equals
    TEST_ASSERT( true == str.Equals( "hello" ) );
    TEST_ASSERT( true == str.Equals( AStackString<>( "hello" ) ) );
    TEST_ASSERT( false == str.Equals( "goodbye" ) );
    TEST_ASSERT( false == str.Equals( AStackString<>( "goodbye" ) ) );

    // EqualsI
    TEST_ASSERT( true == str.EqualsI( "hEllO" ) );
    TEST_ASSERT( true == str.EqualsI( AStackString<>( "hEllO" ) ) );
    TEST_ASSERT( false == str.EqualsI( "goodbye" ) );
    TEST_ASSERT( false == str.EqualsI( AStackString<>( "goodbye" ) ) );
}

// Find
//------------------------------------------------------------------------------
void TestAString::Find() const
{
    // BUG: Returning contents past end of string
    {
        AString tmp( "12345678--X---" );
        tmp.SetLength(8);
        TEST_ASSERT(tmp.Find('X') == nullptr ); // This was OK
        TEST_ASSERT(tmp.Find('X', tmp.Get() + 8) == nullptr); // This was returning junk data past end of string
    }
}

// Format
//------------------------------------------------------------------------------
void TestAString::Format() const
{
    // Create a really long input string
    AStackString<> longInput;
    const size_t longStringLen( 1024 * 1024 );
    for ( size_t i=0; i<longStringLen; ++i )
    {
        longInput += 'A';
    }

    // Make sure we correctly handle formatting large strings
    AStackString<> buffer;
    buffer.Format( "%s", longInput.Get() );
    TEST_ASSERT( buffer.GetLength() == longStringLen );
    TEST_ASSERT( AString::StrLen( buffer.Get() ) == longStringLen );
}

// Tokenize
//------------------------------------------------------------------------------
void TestAString::Tokenize() const
{
    // empty
    {
        Array< AString > tokens;
        AString::GetEmpty().Tokenize( tokens );
        TEST_ASSERT( tokens.GetSize() == 0 );
    }

    // one token
    {
        Array< AString > tokens;
        AStackString<> input( "one_token" );
        input.Tokenize( tokens );
        TEST_ASSERT( tokens.GetSize() == 1 );
        TEST_ASSERT( tokens[ 0 ] == "one_token" );
    }

    // multiple tokens
    {
        Array< AString > tokens;
        AStackString<> input( "this is four tokens" );
        input.Tokenize( tokens );
        TEST_ASSERT( tokens.GetSize() == 4 );
        TEST_ASSERT( tokens[ 0 ] == "this" );
        TEST_ASSERT( tokens[ 1 ] == "is" );
        TEST_ASSERT( tokens[ 2 ] == "four" );
        TEST_ASSERT( tokens[ 3 ] == "tokens" );
    }

    // leading whitespace
    {
        Array< AString > tokens;
        AStackString<> input( "     token" );
        input.Tokenize( tokens );
        TEST_ASSERT( tokens.GetSize() == 1 );
        TEST_ASSERT( tokens[ 0 ] == "token" );
    }

    // trailing whitespace
    {
        Array< AString > tokens;
        AStackString<> input( "token      " );
        input.Tokenize( tokens );
        TEST_ASSERT( tokens.GetSize() == 1 );
        TEST_ASSERT( tokens[ 0 ] == "token" );
    }

    // multiple whitespaces
    {
        Array< AString > tokens;
        AStackString<> input( "   lots  of      spaces   " );
        input.Tokenize( tokens );
        TEST_ASSERT( tokens.GetSize() == 3 );
        TEST_ASSERT( tokens[ 0 ] == "lots" );
        TEST_ASSERT( tokens[ 1 ] == "of" );
        TEST_ASSERT( tokens[ 2 ] == "spaces" );
    }

    // single quotes
    {
        Array< AString > tokens;
        AStackString<> input( "this is 'only three tokens'" );
        input.Tokenize( tokens );
        TEST_ASSERT( tokens.GetSize() == 3 );
        TEST_ASSERT( tokens[ 0 ] == "this" );
        TEST_ASSERT( tokens[ 1 ] == "is" );
        TEST_ASSERT( tokens[ 2 ] == "'only three tokens'" );
    }

    // double quotes
    {
        Array< AString > tokens;
        AStackString<> input( "this is \"only three tokens\"" );
        input.Tokenize( tokens );
        TEST_ASSERT( tokens.GetSize() == 3 );
        TEST_ASSERT( tokens[ 0 ] == "this" );
        TEST_ASSERT( tokens[ 1 ] == "is" );
        TEST_ASSERT( tokens[ 2 ] == "\"only three tokens\"" );
    }

    // mixed quotes
    {
        Array< AString > tokens;
        AStackString<> input( "'on token with an \" in it'" );
        input.Tokenize( tokens );
        TEST_ASSERT( tokens.GetSize() == 1 );
        TEST_ASSERT( tokens[ 0 ] == "'on token with an \" in it'" );
    }

    // alternate split char
    {
        Array< AString > tokens;
        AStackString<> input( "c:\\path\\path;d:\\path;e:\\" );
        input.Tokenize( tokens, ';' );
        TEST_ASSERT( tokens.GetSize() == 3 );
        TEST_ASSERT( tokens[ 0 ] == "c:\\path\\path" );
        TEST_ASSERT( tokens[ 1 ] == "d:\\path" );
        TEST_ASSERT( tokens[ 2 ] == "e:\\" );
    }
}

// PatternMatch
//------------------------------------------------------------------------------
void TestAString::PatternMatch() const
{
    #define CHECK_MATCH( pat, str, match )              \
    {                                                   \
        AStackString<> string( str );                   \
        TEST_ASSERT( string.Matches( pat ) == match );  \
    }

    CHECK_MATCH( "*.cpp",   "File.cpp", true )
    CHECK_MATCH( "*",       "File.cpp", true )
    CHECK_MATCH( "File*.*", "File.cpp", true )
    CHECK_MATCH( "*.c*",    "File.cpp", true )
    CHECK_MATCH( "File.cpp","File.cpp", true )

    CHECK_MATCH( "*.cpp",   "File.CPP", false )
    CHECK_MATCH( "File*.*", "FILE.cpp", false )
    CHECK_MATCH( "*.c*",    "File.CPP", false )
    CHECK_MATCH( "File.cpp","file.cpp", false )

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

// PatternMatchI
//------------------------------------------------------------------------------
void TestAString::PatternMatchI() const
{
    #define CHECK_MATCH( pat, str, match )              \
    {                                                   \
        AStackString<> string( str );                   \
        TEST_ASSERT( string.MatchesI( pat ) == match ); \
    }

    CHECK_MATCH( "*.cpp",   "File.cpp", true )
    CHECK_MATCH( "*",       "File.cpp", true )
    CHECK_MATCH( "File*.*", "File.cpp", true )
    CHECK_MATCH( "*.c*",    "File.cpp", true )
    CHECK_MATCH( "File.cpp","File.cpp", true )

    CHECK_MATCH( "*.cpp",   "File.CPP", true )
    CHECK_MATCH( "File*.*", "FILE.cpp", true )
    CHECK_MATCH( "*.c*",    "File.CPP", true )
    CHECK_MATCH( "File.cpp","file.cpp", true )

    CHECK_MATCH( "*.cpp",   "File.c",           false );
    CHECK_MATCH( "*.cpp",   "File.cpp~",        false );
    CHECK_MATCH( "*.cpp",   "File.cpp.notcpp",  false );
    CHECK_MATCH( "*.cpp",   "",                 false );

    #undef CHECK_MATCH
}


//------------------------------------------------------------------------------
void TestAString::Replace() const
{
    // Replace empty - make sure this is correctly handled
    AStackString<> test("Test");
    test.Replace("", "");
}

// Trim
//------------------------------------------------------------------------------
void TestAString::Trim() const
{
    {
        // No trim
        AStackString<> empty;
        empty.Trim( 0, 0 );
    }

    {
        // Left trim
        AStackString<> test( "zzHello" );
        test.Trim( 2, 0 );
        TEST_ASSERT( test.GetLength() == 5 );
        TEST_ASSERT( test  == "Hello" );
    }

    {
        // Right trim
        AStackString<> test( "Hellozz" );
        test.Trim( 0, 2 );
        TEST_ASSERT( test.GetLength() == 5 );
        TEST_ASSERT( test  == "Hello" );
    }

    {
        // Trim left and right
        AStackString<> test( "zzHellozz" );
        test.Trim( 2, 2 );
        TEST_ASSERT( test.GetLength() == 5 );
        TEST_ASSERT( test  == "Hello" );
    }
}

//------------------------------------------------------------------------------
