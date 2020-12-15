// AString.cpp
//------------------------------------------------------------------------------

// Includes
//------------------------------------------------------------------------------
#include "AString.h"
#include "AStackString.h"
#include "Core/Math/Conversions.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Static
//------------------------------------------------------------------------------
/*static*/ const char * const AString::s_EmptyString( "" );
/*static*/ const AString AString::s_EmptyAString;

// CONSTRUCTOR
//------------------------------------------------------------------------------
AString::AString()
    : m_Contents( const_cast<char *>( s_EmptyString ) ) // cast to allow pointing to protected string
    , m_Length( 0 )
    , m_ReservedAndFlags( 0 )
{
}

// CONSTRUCTOR (uint32_t)
//------------------------------------------------------------------------------
AString::AString( uint32_t reserve )
{
    char * mem = const_cast<char *>( s_EmptyString ); // cast to allow pointing to protected string
    if ( reserve > 0 )
    {
        reserve = Math::RoundUp( reserve, (uint32_t)2 );
        mem = (char *)ALLOC( reserve + 1 );
        mem[ 0 ] = '\000';
    }
    m_Contents = mem;
    m_Length = 0;
    SetReserved( reserve, true );
}

// CONSTRUCTOR (const AString &)
//------------------------------------------------------------------------------
AString::AString( const AString & string )
{
    uint32_t len = string.GetLength();
    m_Length = len;
    uint32_t reserved = Math::RoundUp( len, (uint32_t)2 );
    m_Contents = (char *)ALLOC( reserved + 1 );
    SetReserved( reserved, true );
    Copy( string.Get(), m_Contents, len ); // handles terminator (NOTE: Using len to support embedded nuls)
}

// CONSTRUCTOR (AString &&)
//------------------------------------------------------------------------------
AString::AString( AString && string )
{
    // If source string memory can't be freed, it can't be moved
    if ( string.MemoryMustBeFreed() == false )
    {
        // Copy
        m_Contents = const_cast<char*>( s_EmptyString ); // cast to allow pointing to protected string
        m_Length = 0;
        m_ReservedAndFlags = 0;
        Assign( string );
    }
    else
    {
        // Move
        m_Contents = string.m_Contents;
        m_Length = string.m_Length;
        m_ReservedAndFlags = string.m_ReservedAndFlags;
    }

    // Clear other string
    string.m_Contents = const_cast<char*>( s_EmptyString );
    string.m_Length = 0;
    string.m_ReservedAndFlags = 0;
}

// CONSTRUCTOR (const char *)
//------------------------------------------------------------------------------
AString::AString( const char * string )
{
    ASSERT( string );
    uint32_t len = (uint32_t)StrLen( string );
    m_Length = len;
    uint32_t reserved = Math::RoundUp( len, (uint32_t)2 );
    m_Contents = (char *)ALLOC( reserved + 1 );
    SetReserved( reserved, true );
    Copy( string, m_Contents ); // copy handles terminator
}

// CONSTRUCTOR (const char *, const char *)
//------------------------------------------------------------------------------
AString::AString( const char * start, const char * end )
{
    ASSERT( start );
    ASSERT( end >= start );
    uint32_t len = uint32_t( end - start );
    m_Length = len;
    uint32_t reserved = Math::RoundUp( len, (uint32_t)2 );
    m_Contents = (char *)ALLOC( reserved + 1 );
    SetReserved( reserved, true );
    Copy( start, m_Contents, len ); // copy handles terminator
}

// DESTRUCTOR
//------------------------------------------------------------------------------
AString::~AString()
{
    // At this point, we should
    if ( MemoryMustBeFreed() )
    {
        // if we own the memory, we must:
        // a) NOT be pointing to the shared global string
        ASSERT( m_Contents != s_EmptyString );
        // b) NOT be pointing to an internal buffer
        // Depending on the memory alloctor, it could be valid to have an allocation
        // immediately after the string itself, so we can't have an assert for this
        FREE( m_Contents );
    }
    else
    {
        // if we don't own the memory, either:
        // a) We are an empty string, pointing to the special global empty string
        // OR:
        // b) We are a StackString, and we should point to our internal buffer
        ASSERT( ( m_Contents == s_EmptyString ) ||
                ( (void *)m_Contents == (void *)( (char *)this + sizeof( AString ) ) ) );
    }
}

// operator == (const char *)
//------------------------------------------------------------------------------
bool AString::operator == ( const char * other ) const
{
    const char * thisPos = m_Contents;
    const char * otherPos = other;

loop:
    char c1 = *thisPos;
    char c2 = *otherPos;
    if ( c1 != c2 )
    {
        return false;
    }
    if ( c1 == '\000' )
    {
        return true;
    }
    thisPos++;
    otherPos++;
    goto loop;
}

// operator == (const AString &)
//------------------------------------------------------------------------------
bool AString::operator == ( const AString & other ) const
{
    if ( other.GetLength() != GetLength() )
    {
        return false;
    }

    return ( *this == other.Get() );
}

// Compare
//------------------------------------------------------------------------------
int32_t AString::Compare( const AString & other ) const
{
    return strcmp( m_Contents, other.Get() );
}

// Compare
//------------------------------------------------------------------------------
int32_t AString::Compare( const char * other ) const
{
    return strcmp( m_Contents, other );
}

// CompareI
//------------------------------------------------------------------------------
int32_t AString::CompareI( const AString & other ) const
{
    #if defined( __WINDOWS__ )
        return _stricmp( m_Contents, other.Get() );
    #elif defined( __APPLE__ ) || defined( __LINUX__ )
        return strcasecmp( m_Contents, other.Get() );
    #else
        #error Unknown platform
    #endif
}

// CompareI
//------------------------------------------------------------------------------
int32_t AString::CompareI( const char * other ) const
{
    #if defined( __WINDOWS__ )
        return _stricmp( m_Contents, other );
    #elif defined( __APPLE__ ) || defined( __LINUX__ )
        return strcasecmp( m_Contents, other );
    #else
        #error Unknown platform
    #endif
}

// Format
//------------------------------------------------------------------------------
AString & AString::Format( MSVC_SAL_PRINTF const char * fmtString, ... )
{
    va_list args;
    va_start( args, fmtString );
    VFormat( fmtString, args );
    va_end( args );

    return *this;
}

// VFormat
//------------------------------------------------------------------------------
AString & AString::VFormat( const char * fmtString, va_list args )
{
    // try to work entirely on the stack
    const uint32_t STACK_BUFFER_SIZE( 8 * KILOBYTE );
    char stackBuffer[ STACK_BUFFER_SIZE ];
    char * buffer = stackBuffer;
    size_t bufferSize = STACK_BUFFER_SIZE;

    #if defined( __WINDOWS__ )
loop:
        // attempt the formatting
        int len = vsnprintf_s( buffer, bufferSize, _TRUNCATE, fmtString, args );

        // did it fail to fit?
        if ( len < 0 )
        {
            // free any old buffer allocations
            if ( buffer != stackBuffer )
            {
                FREE( buffer );
            }

            // double the buffer and try again
            bufferSize *= 2;
            buffer = (char *)ALLOC( bufferSize );
            goto loop;
        }
    #else
        va_list argsCopy;
        va_copy( argsCopy, args );
        int len = vsnprintf( nullptr, 0, fmtString, argsCopy );
        va_end( argsCopy );
        if ( len > ( (int)bufferSize - 1 ) )
        {
            bufferSize = len + 1;
            buffer = (char *)ALLOC( bufferSize );
        }
        VERIFY( vsnprintf( buffer, bufferSize, fmtString, args ) >= 0 );
    #endif

    // keep the final result
    Assign( buffer, buffer + len );

    // free memory if we allocated any
    if ( buffer != stackBuffer )
    {
        FREE( buffer );
    }

    return *this;
}

// Tokenize
//------------------------------------------------------------------------------
void AString::Tokenize( Array< AString > & tokens, char splitChar ) const
{
    Array< const char * > tokenStarts;
    Array< const char * > tokenEnds;

    const char * pos = Get();
    const char * end = GetEnd();
    bool lookingForStart = true;
    char quoteChar = 0;
    while ( pos < end )
    {
        if ( lookingForStart )
        {
            if ( *pos == splitChar )
            {
                ++pos;
                continue;
            }

            // found the start of a new token
            tokenStarts.Append( pos );
            lookingForStart = false;
        }

        // hit a quote?
        char c = *pos;
        if ( ( c == '"' ) || ( c == '\'' ) )
        {
            if ( quoteChar == 0 )
            {
                // opening quote
                quoteChar = c;
            }
            else if ( quoteChar == c )
            {
                // closing quote
                quoteChar = 0;
            }
            else
            {
                // quote of the 'other' type - consider as part of token
            }
        }
        else if ( c == splitChar )
        {
            if ( quoteChar == 0 )
            {
                tokenEnds.Append( pos );
                lookingForStart = true;
            }
            else
            {
                // space inside quoted token - consider as part of token
            }
        }
        else
        {
            // normal character part of token
        }
        ++pos;
    }
    ASSERT( ( tokenStarts.GetSize() == tokenEnds.GetSize() ) ||
            ( tokenStarts.GetSize() == ( tokenEnds.GetSize() + 1 ) ) );
    if ( tokenStarts.GetSize() > tokenEnds.GetSize() )
    {
        tokenEnds.Append( pos );
    }
    ASSERT( tokenStarts.GetSize() == tokenEnds.GetSize() );

    // pre-size output to avoid reallocations
    tokens.Clear();
    const size_t numTokens( tokenStarts.GetSize() );
    if ( tokens.GetCapacity() < numTokens )
    {
        tokens.SetCapacity( numTokens );
    }
    tokens.SetSize( numTokens );

    // copy tokens
    for ( size_t i = 0; i < numTokens; ++i )
    {
        tokens[ i ].Assign( tokenStarts[ i ], tokenEnds[ i ] );
    }
}

// Assign (const char *)
//------------------------------------------------------------------------------
void AString::Assign( const char * string )
{
    Assign( string, string + StrLen( string ) );
}

// Assign (const char *, const char *)
//------------------------------------------------------------------------------
void AString::Assign( const char * start, const char * end )
{
    ASSERT( start );
    ASSERT( end >= start );
    uint32_t len = uint32_t( end - start );
    if ( len > GetReserved() )
    {
        GrowNoCopy( len );
    }
    else if ( m_Contents == s_EmptyString )
    {
        // if we are the special empty string, and we
        // didn't resize then the passed in string is empty too
        return;
    }
    Copy( start, m_Contents, len ); // handles terminator
    m_Length = len;
}

// Assign (const AString &)
//------------------------------------------------------------------------------
void AString::Assign( const AString & string )
{
    uint32_t len = string.GetLength();
    if ( len > GetReserved() )
    {
        GrowNoCopy( len );
    }
    else if ( m_Contents == s_EmptyString )
    {
        // if we are the special empty string, and we
        // didn't resize then the passed in string is empty too
        return;
    }
    Copy( string.Get(), m_Contents, len ); // handles terminator (NOTE: Using len to support embedded nuls)
    m_Length = len;
}

// Assign (AString &&)
//------------------------------------------------------------------------------
void AString::Assign( AString && string )
{
    // If memory can't be freed, it can't be moved
    if ( string.MemoryMustBeFreed() == false )
    {
        // Fallback to regular assignment
        Assign( string );
    }
    else
    {
        if ( MemoryMustBeFreed() )
        {
            FREE( m_Contents );
        }
        m_Contents = string.m_Contents;
        m_Length = string.m_Length;
        m_ReservedAndFlags = string.m_ReservedAndFlags;
    }

    // Clear other string
    string.m_Contents = const_cast<char*>( s_EmptyString );
    string.m_Length = 0;
    string.m_ReservedAndFlags = 0;
}

// Clear
//------------------------------------------------------------------------------
void AString::Clear()
{
    // handle the special case empty string with no mem usage
    if ( m_Contents == s_EmptyString )
    {
        return;
    }

    // truncate, but don't free the memory
    m_Contents[ 0 ] = '\000';
    m_Length = 0;
}

// SetReserved
//------------------------------------------------------------------------------
void AString::SetReserved( size_t capacity )
{
    // shrinking content?
    if ( capacity < GetLength() )
    {
        SetLength( (uint32_t)capacity ); // truncate to new capacity
    }

    // allocate memory of new capacity and copy existing string
    Grow( (uint32_t)capacity );
}

// SetLength
//------------------------------------------------------------------------------
void AString::SetLength( uint32_t len )
{
    if ( len > GetReserved() )
    {
        Grow( len );
    }

    // Gracefully handle SetLength( 0 ) on already empty string pointing to the
    // global storage.
    if ( m_Contents != s_EmptyString )
    {
        m_Contents[ len ] = '\000';
    }
    m_Length = len;

    // NOTE: it's up to the user to ensure everything upto the null is
    // valid
}

// operator += (char)
//------------------------------------------------------------------------------
AString & AString::operator += ( char c )
{
    // need more space?
    if ( m_Length >= GetReserved() )
    {
        Grow( m_Length + 1 );
    }
    m_Contents[ m_Length++ ] = c;
    m_Contents[ m_Length ] = '\000';

    return *this;
}

// operator += (const char *)
//------------------------------------------------------------------------------
AString & AString::operator += ( const char * string )
{
    uint32_t suffixLen = (uint32_t)StrLen( string );
    if ( suffixLen )
    {
        uint32_t newLen = m_Length + suffixLen;
        if ( newLen > GetReserved() )
        {
            Grow( newLen );
        }

        Copy( string, m_Contents + m_Length ); // handles terminator
        m_Length += suffixLen;
    }
    return *this;
}

// operator += ( const AString & )
//------------------------------------------------------------------------------
AString & AString::operator += ( const AString & string )
{
    uint32_t suffixLen = string.GetLength();
    if ( suffixLen )
    {
        uint32_t newLen = m_Length + suffixLen;
        if ( newLen > GetReserved() )
        {
            Grow( newLen );
        }

        Copy( string.Get(), m_Contents + m_Length, suffixLen ); // handles terminator (NOTE: Using suffixLen to support embedded nuls)
        m_Length += suffixLen;
    }
    return *this;
}

// Append
//------------------------------------------------------------------------------
AString & AString::Append( const char * string, size_t len )
{
    if ( len )
    {
        uint32_t newLen = m_Length + (uint32_t)len;
        if ( newLen > GetReserved() )
        {
            Grow( newLen );
        }

        Copy( string, m_Contents + m_Length, len ); // handles terminator
        m_Length = newLen;
    }

    return *this;
}

// AppendFormat
//------------------------------------------------------------------------------
AString & AString::AppendFormat( MSVC_SAL_PRINTF const char * fmtString, ... )
{
    AStackString< 1024 > buffer;
    va_list args;
    va_start( args, fmtString );
    buffer.VFormat( fmtString, args );
    va_end( args );

    Append( buffer );

    return *this;
}

// Replace ( char, char )
//------------------------------------------------------------------------------
uint32_t AString::Replace( char from, char to, uint32_t maxReplaces )
{
    uint32_t replaceCount = 0;
    char * pos = m_Contents;
    const char * end = m_Contents + m_Length;
    while ( pos < end )
    {
        if ( *pos == from )
        {
            *pos = to;
            replaceCount++;
            if ( replaceCount == maxReplaces )
            {
                break;
            }
        }
        pos++;
    }
    return replaceCount;
}

// ToLower
//------------------------------------------------------------------------------
void AString::ToLower()
{
    char * pos = m_Contents;
    const char * const end = m_Contents + m_Length;
    while ( pos < end )
    {
        char c = *pos;
        if ( ( c >= 'A' ) && ( c <= 'Z' ) )
        {
            c = 'a' + ( c - 'A' );
            *pos = c;
        }
        pos++;
    }
}

// ToUpper
//------------------------------------------------------------------------------
void AString::ToUpper()
{
    char * pos = m_Contents;
    const char * const end = m_Contents + m_Length;
    while ( pos < end )
    {
        char c = *pos;
        if ( ( c >= 'a' ) && ( c <= 'z' ) )
        {
            c = 'A' + ( c - 'a' );
            *pos = c;
        }
        pos++;
    }
}

// Trim
//------------------------------------------------------------------------------
void AString::Trim( uint32_t startCharsToTrim, uint32_t endCharsToTrim )
{
    // Ensure trim operation is valid
    ASSERT( GetLength() >= ( startCharsToTrim + endCharsToTrim ) );

    if ( startCharsToTrim == 0 )
    {
        if ( endCharsToTrim == 0 )
        {
            return; // nothing to do
        }

        // Trim by adjusting length
        SetLength( GetLength() - endCharsToTrim );
        return;
    }

    // Shuffle string up
    Assign( Get() + startCharsToTrim, GetEnd() - endCharsToTrim );
}

// TrimStart
//------------------------------------------------------------------------------
void AString::TrimStart( char charToTrimFromStart )
{
    uint32_t nbrCharsToRemoveFromStart = 0;
    const char * pos = m_Contents;
    const char * end = m_Contents + m_Length;
    for ( ; pos < end && *pos == charToTrimFromStart; ++pos, ++nbrCharsToRemoveFromStart ) 
    {
    }

    Trim( nbrCharsToRemoveFromStart, 0 );
}

// TrimEnd
//------------------------------------------------------------------------------
void AString::TrimEnd( char charToTrimFromEnd )
{
    uint32_t nbrCharsToRemoveFromEnd = 0;
    const char * pos = m_Contents + m_Length - 1;
    const char * end = m_Contents;
    for ( ; pos >= end && *pos == charToTrimFromEnd; --pos, ++nbrCharsToRemoveFromEnd ) 
    {
    }

    Trim( 0, nbrCharsToRemoveFromEnd );
}

// Replace ( char *, char * )
//------------------------------------------------------------------------------
uint32_t AString::Replace( const char * from, const char * to, uint32_t maxReplaces )
{
    const size_t fromLength = StrLen( from );
    if ( fromLength == 0 )
    {
        // string to replace can't be empty, otherwise replace operation doesn't make sense
        return 0;
    }

    AStackString< 2 * KILOBYTE > temp;
    uint32_t replaceCount = 0;

    // loop until the last possible position for a potential match
    const char * pos = m_Contents;
    const char * end = m_Contents + m_Length;
    while ( pos <= ( end - fromLength ) )
    {
        if ( StrNCmp( pos, from, fromLength ) == 0 )
        {
            temp += to;
            pos += fromLength;
            maxReplaces--;
            replaceCount++;
            if ( maxReplaces == 0 )
            {
                break;
            }
        }
        else
        {
            temp += *pos;
            pos++;
        }
    }

    // did we actually replace anything?
    if ( replaceCount > 0 )
    {
        // copy remaining unmatchable characters to the temp
        while ( pos < end )
        {
            temp += *pos;
            pos++;
        }

        // keep the result
        Assign( temp );
    }
    return replaceCount;
}

// Find
//------------------------------------------------------------------------------
const char * AString::Find( char c, const char * startPos, const char * endPos ) const
{
    // if startPos is provided, validate it
    // (deliberately allow startPos to point one past end of string)
    ASSERT( ( startPos == nullptr ) || ( startPos >= m_Contents ) );
    ASSERT( ( startPos == nullptr ) || ( startPos <= m_Contents + GetLength() ) );

    const char * pos = startPos ? startPos : m_Contents;
    const char * end = endPos ? endPos : m_Contents + m_Length;

    ASSERT( end >= pos );
    ASSERT( end <= m_Contents + GetLength() );

    while ( pos < end )
    {
        if ( *pos == c )
        {
            return pos;
        }
        pos++;
    }
    return nullptr;
}

// Find
//------------------------------------------------------------------------------
const char * AString::Find( const char * subString, const char * startPos, const char * endPos ) const
{
    // if startPos is provided, validate it
    // (deliberately allow startPos to point one past end of string)
    ASSERT( ( startPos == nullptr ) || ( startPos >= m_Contents ) );
    ASSERT( ( startPos == nullptr ) || ( startPos <= m_Contents + GetLength() ) );

    const size_t subStrLen = StrLen( subString );

    const char * pos = startPos ? startPos : m_Contents;
    const char * end = endPos ? endPos : m_Contents + m_Length;
    ASSERT( end >= pos );
    end -= subStrLen;

    ASSERT( end <= m_Contents + GetLength() );

    while ( pos <= end )
    {
        if ( StrNCmp( pos, subString, subStrLen ) == 0 )
        {
            return pos;
        }
        pos++;
    }
    return nullptr;
}

// Find
//------------------------------------------------------------------------------
const char * AString::Find( const AString & subString, const char * startPos, const char * endPos ) const
{
    // if startPos is provided, validate it
    // (deliberately allow startPos to point one past end of string)
    ASSERT( ( startPos == nullptr ) || ( startPos >= m_Contents ) );
    ASSERT( ( startPos == nullptr ) || ( startPos <= m_Contents + GetLength() ) );

    const size_t subStrLen = subString.GetLength();

    const char * pos = startPos ? startPos : m_Contents;
    const char * end = endPos ? endPos : m_Contents + m_Length;
    ASSERT( end >= pos );
    end -= subStrLen;

    ASSERT( end <= m_Contents + GetLength() );

    while ( pos <= end )
    {
        if ( StrNCmp( pos, subString.Get(), subStrLen ) == 0 )
        {
            return pos;
        }
        ++pos;
    }
    return nullptr;
}

// FindI
//------------------------------------------------------------------------------
const char * AString::FindI( char c, const char * startPos, const char * endPos ) const
{
    // if startPos is provided, validate it
    // (deliberately allow startPos to point one past end of string)
    ASSERT( ( startPos == nullptr ) || ( startPos >= m_Contents ) );
    ASSERT( ( startPos == nullptr ) || ( startPos <= m_Contents + GetLength() ) );

    const char * pos = startPos ? startPos : m_Contents;
    const char * end = endPos ? endPos : m_Contents + m_Length;

    ASSERT( end >= pos );
    ASSERT( end <= m_Contents + GetLength() );

    char a1 = c;
    if ( ( a1 >= 'A' ) && ( a1 <= 'Z' ) )
    {
        a1 = 'a' + ( a1 - 'A' );
    }

    while ( pos < end )
    {
        char b1 = *pos;
        if ( ( b1 >= 'A' ) && ( b1 <= 'Z' ) )
        {
            b1 = 'a' + ( b1 - 'A' );
        }
        if ( a1 == b1 )
        {
            return pos;
        }
        ++pos;
    }
    return nullptr;
}

// FindI
//------------------------------------------------------------------------------
const char * AString::FindI( const char * subString, const char * startPos, const char * endPos ) const
{
    // if startPos is provided, validate it
    // (deliberately allow startPos to point one past end of string)
    ASSERT( ( startPos == nullptr ) || ( startPos >= m_Contents ) );
    ASSERT( ( startPos == nullptr ) || ( startPos <= m_Contents + GetLength() ) );

    const size_t subStrLen = StrLen( subString );

    const char * pos = startPos ? startPos : m_Contents;
    const char * end = endPos ? endPos : m_Contents + m_Length;
    ASSERT( end >= pos );
    end -= subStrLen;

    ASSERT( end <= m_Contents + GetLength() );

    while ( pos <= end )
    {
        if ( StrNCmpI( pos, subString, subStrLen ) == 0 )
        {
            return pos;
        }
        pos++;
    }
    return nullptr;
}

// FindI
//------------------------------------------------------------------------------
const char * AString::FindI( const AString & subString, const char * startPos, const char * endPos ) const
{
    // if startPos is provided, validate it
    // (deliberately allow startPos to point one past end of string)
    ASSERT( ( startPos == nullptr ) || ( startPos >= m_Contents ) );
    ASSERT( ( startPos == nullptr ) || ( startPos <= m_Contents + GetLength() ) );

    const size_t subStrLen = subString.GetLength();

    const char * pos = startPos ? startPos : m_Contents;
    const char * end = endPos ? endPos : m_Contents + m_Length;
    ASSERT( end >= pos );
    end -= subStrLen;

    ASSERT( end <= m_Contents + GetLength() );

    while ( pos <= end )
    {
        if ( StrNCmpI( pos, subString.Get(), subStrLen ) == 0 )
        {
            return pos;
        }
        pos++;
    }
    return nullptr;
}

// FindLast
//------------------------------------------------------------------------------
const char * AString::FindLast( char c, const char * startPos, const char * endPos ) const
{
    // if startPos is provided, validate it
    // (deliberately allow startPos to point one past end of string)
    ASSERT( ( startPos == nullptr ) || ( startPos >= m_Contents ) );
    ASSERT( ( startPos == nullptr ) || ( startPos <= m_Contents + GetLength() ) );

    const char * pos = startPos ? startPos : ( m_Contents + m_Length - 1 );
    const char * end = endPos ? endPos : m_Contents;
    while ( pos >= end )
    {
        if ( *pos == c )
        {
            return pos;
        }
        pos--;
    }
    return nullptr;
}

// FindLast
//------------------------------------------------------------------------------
const char * AString::FindLast( const char * subString, const char * startPos, const char * endPos ) const
{
    // if startPos is provided, validate it
    // (deliberately allow startPos to point one past end of string)
    ASSERT( ( startPos == nullptr ) || ( startPos >= m_Contents ) );
    ASSERT( ( startPos == nullptr ) || ( startPos <= m_Contents + GetLength() ) );

    const size_t subStrLen = StrLen( subString );

    const char * pos = startPos ? startPos : ( m_Contents + m_Length - subStrLen );
    const char * end = endPos ? endPos : m_Contents;
    ASSERT( ( end <= pos ) && ( end >= m_Contents ) );

    while ( pos >= end )
    {
        if ( StrNCmp( pos, subString, subStrLen ) == 0 )
        {
            return pos;
        }
        pos--;
    }
    return nullptr;
}

// FindLast
//------------------------------------------------------------------------------
const char * AString::FindLast( const AString & subString, const char * startPos, const char * endPos ) const
{
    // if startPos is provided, validate it
    // (deliberately allow startPos to point one past end of string)
    ASSERT( ( startPos == nullptr ) || ( startPos >= m_Contents ) );
    ASSERT( ( startPos == nullptr ) || ( startPos <= m_Contents + GetLength() ) );

    const size_t subStrLen = subString.GetLength();

    const char * pos = startPos ? startPos : ( m_Contents + m_Length - subStrLen );
    const char * end = endPos ? endPos : m_Contents;
    ASSERT( ( end <= pos ) && ( end >= m_Contents ) );

    while ( pos >= end )
    {
        if ( StrNCmp( pos, subString.Get(), subStrLen ) == 0 )
        {
            return pos;
        }
        pos--;
    }
    return nullptr;
}

// FindLastI
//------------------------------------------------------------------------------
const char * AString::FindLastI( char c, const char * startPos, const char * endPos ) const
{
    // if startPos is provided, validate it
    // (deliberately allow startPos to point one past end of string)
    ASSERT( ( startPos == nullptr ) || ( startPos >= m_Contents ) );
    ASSERT( ( startPos == nullptr ) || ( startPos <= m_Contents + GetLength() ) );

    const char * pos = startPos ? startPos : ( m_Contents + m_Length - 1 );
    const char * end = endPos ? endPos : m_Contents;

    char a1 = c;
    if ( ( a1 >= 'A' ) && ( a1 <= 'Z' ) )
    {
        a1 = 'a' + ( a1 - 'A' );
    }

    while ( pos >= end )
    {
        char b1 = *pos;
        if ( ( b1 >= 'A' ) && ( b1 <= 'Z' ) )
        {
            b1 = 'a' + ( b1 - 'A' );
        }
        if ( a1 == b1 )
        {
            return pos;
        }
        pos--;
    }
    return nullptr;
}

// FindLastI
//------------------------------------------------------------------------------
const char * AString::FindLastI( const char * subString, const char * startPos, const char * endPos ) const
{
    // if startPos is provided, validate it
    // (deliberately allow startPos to point one past end of string)
    ASSERT( ( startPos == nullptr ) || ( startPos >= m_Contents ) );
    ASSERT( ( startPos == nullptr ) || ( startPos <= m_Contents + GetLength() ) );

    const size_t subStrLen = StrLen( subString );

    const char * pos = startPos ? startPos : ( m_Contents + m_Length - subStrLen );
    const char * end = endPos ? endPos : m_Contents;
    ASSERT( ( end <= pos ) && ( end >= m_Contents ) );

    while ( pos >= end )
    {
        if ( StrNCmpI( pos, subString, subStrLen ) == 0 )
        {
            return pos;
        }
        pos--;
    }
    return nullptr;
}

// FindLastI
//------------------------------------------------------------------------------
const char * AString::FindLastI( const AString & subString, const char * startPos, const char * endPos ) const
{
    // if startPos is provided, validate it
    // (deliberately allow startPos to point one past end of string)
    ASSERT( ( startPos == nullptr ) || ( startPos >= m_Contents ) );
    ASSERT( ( startPos == nullptr ) || ( startPos <= m_Contents + GetLength() ) );

    const size_t subStrLen = subString.GetLength();

    const char * pos = startPos ? startPos : ( m_Contents + m_Length - subStrLen );
    const char * end = endPos ? endPos : m_Contents;
    ASSERT( ( end <= pos ) && ( end >= m_Contents ) );

    while ( pos >= end )
    {
        if ( StrNCmpI( pos, subString.Get(), subStrLen ) == 0 )
        {
            return pos;
        }
        pos--;
    }
    return nullptr;
}

// EndsWith
//------------------------------------------------------------------------------
bool AString::EndsWith( char c ) const
{
    uint32_t len = m_Length;
    if ( len == 0 )
    {
        return false;
    }
    return ( m_Contents[ len - 1 ] == c );
}

// EndsWith
//------------------------------------------------------------------------------
bool AString::EndsWith( const char * string ) const
{
    const size_t stringLen = StrLen( string );
    const char * possiblePos = m_Contents + m_Length - stringLen;
    if ( possiblePos < m_Contents )
    {
        return false; // string to search is longer than this string
    }
    return ( StrNCmp( possiblePos, string, stringLen ) == 0 );
}

// EndsWith
//------------------------------------------------------------------------------
bool AString::EndsWith( const AString & other ) const
{
    const size_t otherLen = other.GetLength();
    if ( otherLen > GetLength() )
    {
        return false;
    }
    return ( StrNCmp( GetEnd() - otherLen, other.Get(), otherLen ) == 0 );
}

// EndsWithI
//------------------------------------------------------------------------------
bool AString::EndsWithI( const char * other ) const
{
    const size_t otherLen = StrLen( other );
    if ( otherLen > GetLength() )
    {
        return false;
    }
    return ( StrNCmpI( GetEnd() - otherLen, other, otherLen ) == 0 );
}

// EnsWithI
//------------------------------------------------------------------------------
bool AString::EndsWithI( const AString & other ) const
{
    const size_t otherLen = other.GetLength();
    if ( otherLen > GetLength() )
    {
        return false;
    }
    return ( StrNCmpI( GetEnd() - otherLen, other.Get(), otherLen ) == 0 );
}

// BeginsWith
//------------------------------------------------------------------------------
bool AString::BeginsWith( char c ) const
{
    if ( IsEmpty() )
    {
        return false;
    }
    return ( m_Contents[ 0 ] == c );
}

// BeginsWith
//------------------------------------------------------------------------------
bool AString::BeginsWith( const char * string ) const
{
    size_t otherLen = StrLen( string );
    if ( otherLen > GetLength() )
    {
        return false;
    }
    return ( StrNCmp( m_Contents, string, otherLen ) == 0 );
}

// BeginsWith
//------------------------------------------------------------------------------
bool AString::BeginsWith( const AString & string ) const
{
    uint32_t otherLen = string.GetLength();
    if ( otherLen > GetLength() )
    {
        return false;
    }
    return ( StrNCmp( m_Contents, string.Get(), otherLen ) == 0 );
}

// BeginsWithI
//------------------------------------------------------------------------------
bool AString::BeginsWithI( const char * string ) const
{
    size_t otherLen = StrLen( string );
    if ( otherLen > GetLength() )
    {
        return false;
    }
    return ( StrNCmpI( m_Contents, string, otherLen ) == 0 );
}

// BeginsWithI
//------------------------------------------------------------------------------
bool AString::BeginsWithI( const AString & string ) const
{
    uint32_t otherLen = string.GetLength();
    if ( otherLen > GetLength() )
    {
        return false;
    }
    return ( StrNCmpI( m_Contents, string.Get(), otherLen ) == 0 );
}

// Match
// NOTE: This code is based on that in the C/C++ Users Journal (Mike Cornelison)
//------------------------------------------------------------------------------
/*static*/ bool AString::Match( const char * pat, const char * str )
{
    bool star;

new_segment:
    star = false;
    if ( *pat == '*' )
    {
        star = true;
        do { pat++; } while ( *pat == '*' );
    }

test_match:
    int i;
    for ( i = 0; pat[ i ] && ( pat[ i ] != '*' ); i++ )
    {
        char a = str[ i ];
        char b = pat[ i ];
        if ( a != b )
        {
            if ( !str[ i ] ) return false;
            if ( ( pat[ i ] == '?' ) && ( str[i] != '.' ) ) continue;
            if ( !star ) return false;
            str++;
            goto test_match;
        }
    }
    if ( pat[ i ] == '*' )
    {
        str += i;
        pat += i;
        goto new_segment;
    }
    if ( !str[ i ] ) return true;
    if ( i && pat[ i - 1 ] == '*' ) return true;
    if ( !star ) return false;
    str++;
    goto test_match;
}

// MatchI
// NOTE: This code is based on that in the C/C++ Users Journal (Mike Cornelison)
//------------------------------------------------------------------------------
/*static*/ bool AString::MatchI( const char * pat, const char * str )
{
    bool star;

new_segment:
    star = false;
    if ( *pat == '*' )
    {
        star = true;
        do { pat++; } while ( *pat == '*' );
    }

test_match:
    int i;
    for ( i = 0; pat[ i ] && ( pat[ i ] != '*' ); i++ )
    {
        char a = str[ i ]; a = ( ( a >= 'A' ) && ( a <= 'Z' ) ) ? 'a' + ( a - 'A' ) : a;
        char b = pat[ i ]; b = ( ( b >= 'A' ) && ( b <= 'Z' ) ) ? 'a' + ( b - 'A' ) : b;
        if ( a != b )
        {
            if ( !str[ i ] ) return false;
            if ( ( pat[ i ] == '?' ) && ( str[i] != '.' ) ) continue;
            if ( !star ) return false;
            str++;
            goto test_match;
        }
    }
    if ( pat[ i ] == '*' )
    {
        str += i;
        pat += i;
        goto new_segment;
    }
    if ( !str[ i ] ) return true;
    if ( i && pat[ i - 1 ] == '*' ) return true;
    if ( !star ) return false;
    str++;
    goto test_match;
}

// Copy
//------------------------------------------------------------------------------
/*static*/ void AString::Copy( const char * src, char * dst )
{
    for ( ;; )
    {
        const char c = *src;
        *dst = c; // Includes the null terminator
        if ( c == 0 )
        {
            break;
        }
        ++dst;
        ++src;
    }
}

// Copy
//------------------------------------------------------------------------------
/*static*/ void AString::Copy( const char * src, char * dst, size_t len )
{
    memmove( dst, src, len );
    dst[len] = '\000';
}

// StrLen
//------------------------------------------------------------------------------
/*static*/ size_t AString::StrLen( const char * string )
{
    const char * pos = string;
    while ( *pos != '\000' )
    {
        pos++;
    }
    return (size_t)( pos - string );
}

// StrNCmp
//------------------------------------------------------------------------------
/*static*/ int32_t AString::StrNCmp( const char * a, const char * b, size_t num )
{
    while ( num > 0 )
    {
        if ( *a == *b )
        {
            if ( *a == '\000' )
            {
                return 0; // both strings ended and equal
            }

            // keep going
            a++;
            b++;
            num--;
            continue;
        }

        // different, so return relationship
        return ( *a > *b ) ? 1 : -1;
    }
    return 0; // strings identical upto 'num' chars
}

// StrNCmpI
//------------------------------------------------------------------------------
/*static*/ int32_t AString::StrNCmpI( const char * a, const char * b, size_t num )
{
    while ( num > 0 )
    {
        char a1 = *a;
        if ( ( a1 >= 'A' ) && ( a1 <= 'Z' ) )
        {
            a1 = 'a' + ( a1 - 'A' );
        }
        char b1 = *b;
        if ( ( b1 >= 'A' ) && ( b1 <= 'Z' ) )
        {
            b1 = 'a' + ( b1 - 'A' );
        }
        if ( a1 == b1 )
        {
            if ( a1 == '\000' )
            {
                return 0; // both strings ended and equal
            }

            // keep going
            a++;
            b++;
            num--;
            continue;
        }

        // different, so return relationship
        return ( a1 - b1 );
    }
    return 0; // strings identical upto 'num' chars
}

// Grow
//------------------------------------------------------------------------------
void AString::Grow( uint32_t newLength )
{
    // allocate space, rounded up to multiple of 2
    const uint32_t amortizedReserve = ( GetReserved() * 2 );
    const uint32_t reserve = Math::RoundUp( Math::Max( amortizedReserve, newLength ),(uint32_t)2 );
    char * newMem = (char *)ALLOC( reserve + 1 ); // also allocate for \0 terminator

    // transfer existing string data
    Copy( m_Contents, newMem, m_Length ); // copy handles terminator

    if ( MemoryMustBeFreed() )
    {
        FREE( m_Contents );
    }

    m_Contents = newMem;
    SetReserved( reserve, true );
}

// GrowNoCopy
//------------------------------------------------------------------------------
void AString::GrowNoCopy( uint32_t newLength )
{
    if ( MemoryMustBeFreed() )
    {
        FREE( m_Contents );
    }

    // allocate space, rounded up to multiple of 2
    uint32_t reserve = Math::RoundUp( newLength, (uint32_t)2 );
    m_Contents = (char *)ALLOC( reserve + 1 ); // also allocate for \0 terminator
    SetReserved( reserve, true );
}

//------------------------------------------------------------------------------
