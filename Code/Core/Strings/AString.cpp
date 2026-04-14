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
    : m_ReferenceCount( nullptr )
    , m_Contents( const_cast<char *>( s_EmptyString ) ) // cast to allow pointing to protected string
    , m_Length( 0 )
    , m_Reserved( 0 )
{
}

// CONSTRUCTOR (uint32_t)
//------------------------------------------------------------------------------
AString::AString( uint32_t reserve )
{
    if ( reserve == 0 )
    {
        UnsafeAssignSharingWith( s_EmptyAString );
    }
    else
    {
        UnsafeInitAsEmpty( reserve );
    }
}

// CONSTRUCTOR (const AString &)
//------------------------------------------------------------------------------
AString::AString( const AString & string )
{
    if ( string.m_ReferenceCount || string.m_Contents == s_EmptyString )
    {
        m_ReferenceCount = string.m_Contents != s_EmptyString ? string.m_ReferenceCount : nullptr;
        m_Contents = string.m_Contents;
        m_Length = string.m_Length;
        m_Reserved = string.m_Reserved;

        if ( m_ReferenceCount )
        {
            m_ReferenceCount->Increment();
        }
    }
    else
    {
        // Source string doesn't support reference counting, so copy
        UnsafeInitAsEmpty( string.GetReserved() );
        Assign( string );
    }

    ASSERT( ( StrNCmp( m_Contents, "x64", 4 ) != 0 ) || ( m_Length == 3 ) );
}

// CONSTRUCTOR (AString &&)
//------------------------------------------------------------------------------
AString::AString( AString && string )
{
    if ( string.IsUsingSharedMemory() )
    {
        UnsafeAssignSharingWith( string );
        string.UnsafeAssignSharingWith( s_EmptyAString );
    }
    else
    {
        // Source string memory doesn't support reference counting, so copy
        UnsafeInitAsEmpty( string.GetLength() );
        Assign( string );

        // Clear other string
        string.Assign( s_EmptyAString );
    }

    ASSERT( ( StrNCmp( m_Contents, "x64", 4 ) != 0 ) || ( m_Length == 3 ) );
}

// CONSTRUCTOR (const char *)
//------------------------------------------------------------------------------
AString::AString( const char * string )
{
    ASSERT( string );
    const uint32_t len = (uint32_t)StrLen( string );
    UnsafeInitAsEmpty( len );
    Assign( string, string + len );

    ASSERT( ( StrNCmp( m_Contents, "x64", 4 ) != 0 ) || ( m_Length == 3 ) );
}

// CONSTRUCTOR (const char *, const char *)
//------------------------------------------------------------------------------
AString::AString( const char * start, const char * end )
{
    ASSERT( start );
    ASSERT( end >= start );
    const uint32_t len = uint32_t( end - start );
    UnsafeInitAsEmpty( len );
    Assign( start, end );

    ASSERT( ( StrNCmp( m_Contents, "x64", 4 ) != 0 ) || ( m_Length == 3 ) );
}

// DESTRUCTOR
//------------------------------------------------------------------------------
AString::~AString()
{
    UnsafeReleaseSharedMemory();
}

// operator == (const char *)
//------------------------------------------------------------------------------
bool AString::operator==( const char * other ) const
{
    const char * thisPos = m_Contents;
    const char * otherPos = other;

loop:
    const char c1 = *thisPos;
    const char c2 = *otherPos;
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
bool AString::operator==( const AString & other ) const
{
    if ( ( other.m_ReferenceCount == m_ReferenceCount ) &&
         ( other.m_Contents == m_Contents ) &&
         ( other.m_Length == m_Length ) &&
         ( other.m_Reserved == m_Reserved ) )
    {
        return true;
    }

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
    const int len = vsnprintf_s( buffer, bufferSize, _TRUNCATE, fmtString, args );

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
    PRAGMA_DISABLE_PUSH_CLANG( "-Wformat-nonliteral" )
    int len = vsnprintf( nullptr, 0, fmtString, argsCopy );
    PRAGMA_DISABLE_POP_CLANG
    va_end( argsCopy );
    if ( len > ( (int)bufferSize - 1 ) )
    {
        bufferSize = static_cast<size_t>( len ) + 1;
        buffer = (char *)ALLOC( bufferSize );
    }
    PRAGMA_DISABLE_PUSH_CLANG( "-Wformat-nonliteral" )
    VERIFY( vsnprintf( buffer, bufferSize, fmtString, args ) >= 0 );
    PRAGMA_DISABLE_POP_CLANG
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

// Scan
//------------------------------------------------------------------------------
int32_t AString::Scan( MSVC_SAL_SCANF const char * fmtString, ... ) const
{
    va_list args;
    va_start( args, fmtString );
    const int32_t result = vsscanf( m_Contents, fmtString, args );
    va_end( args );
    return result;
}

// ScanS
//------------------------------------------------------------------------------
/*static*/ int32_t AString::ScanS( const char * buffer, MSVC_SAL_SCANF const char * fmtString, ... )
{
    va_list args;
    va_start( args, fmtString );
    const int32_t result = vsscanf( buffer, fmtString, args );
    va_end( args );
    return result;
}

// Tokenize
//------------------------------------------------------------------------------
void AString::Tokenize( Array<TokenRange> & outTokenRanges,
                        char splitChar ) const
{
    const char * pos = Get();
    const char * end = GetEnd();
    char quoteChar = 0;
    TokenRange * current = nullptr;
    while ( pos < end )
    {
        if ( current == nullptr )
        {
            if ( *pos == splitChar )
            {
                ++pos;
                continue;
            }

            // found the start of a new token
            current = &outTokenRanges.EmplaceBack();
            current->m_StartIndex = static_cast<uint32_t>( pos - Get() );
        }

        // hit a quote?
        const char c = *pos;
        if ( c == '"' )
        {
            if ( quoteChar == 0 )
            {
                // opening quote
                quoteChar = c;
            }
            else
            {
                // closing quote
                ASSERT( quoteChar == c );
                quoteChar = 0;
            }
        }
        else if ( c == '\\' ) // Escape char
        {
            // skip over escaped quotes
            if ( ( pos + 1 ) < end )
            {
                const char nextChar = pos[ 1 ];
                if ( nextChar == '"' )
                {
                    ++pos;
                }
            }
        }
        else if ( c == splitChar )
        {
            if ( quoteChar == 0 )
            {
                current->m_EndIndex = static_cast<uint32_t>( pos - Get() );
                current = nullptr;
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

    // Terminate last token if needed
    if ( current != nullptr )
    {
        ASSERT( current->m_EndIndex == 0 );
        current->m_EndIndex = static_cast<uint32_t>( pos - Get() );
    }
}

// Tokenize
//------------------------------------------------------------------------------
void AString::Tokenize( Array<AString> & tokens, char splitChar ) const
{
    // Get the bounds of the tokens
    StackArray<TokenRange, 128> tokenRanges;
    Tokenize( tokenRanges, splitChar );

    // pre-size output to avoid reallocations
    tokens.Clear();
    tokens.SetCapacity( tokenRanges.GetSize() );

    // copy tokens
    for ( const TokenRange & tokenRange : tokenRanges )
    {
        tokens.EmplaceBack( ( Get() + tokenRange.m_StartIndex ),
                            ( Get() + tokenRange.m_EndIndex ) );
    }
}

// RemoveQuotes
//------------------------------------------------------------------------------
/*static*/ void AString::RemoveQuotes( Array<AString> & inoutTokens )
{
    for ( AString & token : inoutTokens )
    {
        token.RemoveQuotes();
    }
}

// RemoveQuotes
//------------------------------------------------------------------------------
void AString::RemoveQuotes()
{
    EnsureOnlyOwner();

    // Remove quotes in-place
    char * src = Get();
    char * dst = src;
    const char * const end = GetEnd();
    char quoteChar = 0;
    while ( src < end )
    {
        const char c = *src;
        if ( c == '"' )
        {
            if ( quoteChar == 0 )
            {
                // opening quote - remove from output
                quoteChar = c;
                ++src;
                continue;
            }
            else
            {
                // closing quote
                ASSERT( quoteChar == c );
                quoteChar = 0;
                ++src;
                continue; // Remove quote from token
            }
        }
        else if ( c == '\\' ) // Escape char
        {
            // collapse escaped quotes
            if ( ( src + 1 ) < end )
            {
                const char nextChar = src[ 1 ];
                if ( nextChar == '"' )
                {
                    // Replace escaped char with quote
                    src += 2;
                    *dst = nextChar;
                    ++dst;
                    continue;
                }
            }
        }

        *dst = *src;
        ++dst;
        ++src;
    }
    SetLength( static_cast<uint32_t>( dst - Get() ) );
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
    const uint32_t len = uint32_t( end - start );
	if ( !IsOnlyOwner() )
	{
		ClearAndFreeMemory();
	}
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

    ASSERT( ( StrNCmp( m_Contents, "x64", 4 ) != 0 ) || ( m_Length == 3 ) );
}

// Assign (const AString &)
//------------------------------------------------------------------------------
void AString::Assign( const AString & string )
{
    if ( IsUsingSharedMemory() && string.IsUsingSharedMemory() )
    {
        // Other string supports shared memory, so we don't need to perform a deep copy.
        if ( (void *)m_Contents == (void *)string.Get() )
        {
            // Already pointing to the same shared memory. Nothing to do.
            ASSERT( ( m_ReferenceCount == string.m_ReferenceCount ) && ( m_Length == string.m_Length ) && ( m_Reserved == string.m_Reserved ) );
            return;
        }

        // Copy the pointers and increment the reference count.
        UnsafeReleaseSharedMemory();
        UnsafeAssignSharingWith( string );
        if ( m_ReferenceCount )
        {
            m_ReferenceCount->Increment();
        }
        return;
    }

    const uint32_t len = string.GetLength();
    if ( len > GetReserved() )
    {
        GrowNoCopy( len );
    }
    if ( m_Contents == s_EmptyString )
    {
        // if we are the special empty string, and we
        // didn't resize then the passed in string is empty too
        return;
    }
    EnsureOnlyOwner(); // should do nothing if grew
    Copy( string.Get(), m_Contents, len ); // handles terminator (NOTE: Using len to support embedded nuls)
    m_Length = len;
    ASSERT( ( StrNCmp( m_Contents, "x64", 4 ) != 0 ) || ( m_Length == 3 ) );

}

// Assign (AString &&)
//------------------------------------------------------------------------------
void AString::Assign( AString && string )
{
    if ( &string == this )
    {
        return; // self-assignmet is a no-op
    }

    if ( IsUsingSharedMemory() && string.IsUsingSharedMemory() )
    {
        // Other string supports shared memory, so we don't need to perform a deep copy.
        if ( (void *)m_Contents == (void *)const_cast<const AString &>(string).Get() )
        {
            // Already pointing to the same shared memory. Just clear the other.
            ASSERT( ( m_ReferenceCount == string.m_ReferenceCount ) && ( m_Length == string.m_Length ) && ( m_Reserved == string.m_Reserved ) );
            string.Clear();
            return;
        }

        UnsafeReleaseSharedMemory();
        UnsafeAssignSharingWith( string );
        string.UnsafeAssignSharingWith( s_EmptyAString );
        ASSERT( ( m_ReferenceCount != nullptr ) || ( m_ReferenceCount->Load() != 0 ) );
        return;
    }

    // Fallback to regular copy assignment
    const uint32_t len = string.GetLength();
    if ( len > GetReserved() )
    {
        GrowNoCopy( len );
    }
    if ( m_Contents == s_EmptyString )
    {
        // if we are the special empty string, and we
        // didn't resize then the passed in string is empty too
        return;
    }
    EnsureOnlyOwner(); // should do nothing if grew
    Copy( string.Get(), m_Contents, len ); // handles terminator (NOTE: Using len to support embedded nuls)
    m_Length = len;

    // Clear other string
    string.Clear();
    ASSERT( ( StrNCmp( m_Contents, "x64", 4 ) != 0 ) || ( m_Length == 3 ) );
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
    if ( !IsOnlyOwner() )
    {
        UnsafeReleaseSharedMemory();
        UnsafeAssignSharingWith( s_EmptyAString );
        return;
    }

    // truncate, but don't free the memory
    m_Contents[ 0 ] = '\000';
    m_Length = 0;
}

// ClearAndFreeMemory
//------------------------------------------------------------------------------
void AString::ClearAndFreeMemory()
{
    // handle the special case empty string with no mem usage
    if ( m_Contents == s_EmptyString )
    {
        return;
    }
    if ( IsUsingSharedMemory() )
    {
        UnsafeReleaseSharedMemory();
        UnsafeAssignSharingWith( s_EmptyAString );
        return;
    }

    // Pointing to unfreeable memory so just reset state
    m_Contents[ 0 ] = '\000';
    m_Length = 0;
}

// SetReserved
//------------------------------------------------------------------------------
void AString::SetReserved( size_t capacity )
{
    // shrinking content?
    // TODO:C This code path seems suspect and hitting this is likely an error
    // in the calling code
    if ( capacity < GetLength() )
    {
        SetLength( (uint32_t)capacity ); // truncate to new capacity
        return;
    }

    // Ignore requests for capacity lower that already available
    if ( capacity <= GetReserved() )
    {
        return;
    }

    // allocate memory of new capacity and copy existing string
    Grow( (uint32_t)capacity );
}

// SetLength
//------------------------------------------------------------------------------
void AString::SetLength( uint32_t len )
{
    if ( len == m_Length )
    {
        return;
    }
    if ( len > GetReserved() )
    {
        Grow( len );
    }
    // Gracefully handle SetLength( 0 ) on already empty string pointing to the
    // global storage.
    if ( m_Contents == s_EmptyString )
    {
        return;
    }

    EnsureOnlyOwner();
    m_Contents[ len ] = '\000';
    m_Length = len;

    // NOTE: it's up to the user to ensure everything upto the null is
    // valid
}

// operator += (char)
//------------------------------------------------------------------------------
AString & AString::operator+=( char c )
{
    // need more space?
    if ( m_Length >= GetReserved() )
    {
        Grow( m_Length + 1 );
    }

    EnsureOnlyOwner();
    m_Contents[ m_Length++ ] = c;
    m_Contents[ m_Length ] = '\000';
    ASSERT( ( StrNCmp( m_Contents, "x64", 4 ) != 0 ) || ( m_Length == 3 ) );

    return *this;
}

// operator += (const char *)
//------------------------------------------------------------------------------
AString & AString::operator+=( const char * string )
{
    const uint32_t suffixLen = (uint32_t)StrLen( string );
    if ( suffixLen )
    {
        const uint32_t newLen = m_Length + suffixLen;
        if ( newLen > GetReserved() )
        {
            Grow( newLen );
        }

        EnsureOnlyOwner();
        Copy( string, m_Contents + m_Length ); // handles terminator
        m_Length += suffixLen;
    }
    ASSERT( ( StrNCmp( m_Contents, "x64", 4 ) != 0 ) || ( m_Length == 3 ) );

    return *this;
}

// operator += ( const AString & )
//------------------------------------------------------------------------------
AString & AString::operator+=( const AString & string )
{
    if ( IsEmpty() )
    {
        Assign( string );
        return *this;
    }
    const uint32_t suffixLen = string.GetLength();
    if ( suffixLen )
    {
        const uint32_t newLen = m_Length + suffixLen;
        if ( newLen > GetReserved() )
        {
            Grow( newLen );
        }

        EnsureOnlyOwner();
        Copy( string.Get(), m_Contents + m_Length, suffixLen ); // handles terminator (NOTE: Using suffixLen to support embedded nuls)
        m_Length += suffixLen;
    }
    ASSERT( ( StrNCmp( m_Contents, "x64", 4 ) != 0 ) || ( m_Length == 3 ) );

    return *this;
}

// Append
//------------------------------------------------------------------------------
AString & AString::Append( const char * string, size_t len )
{
    if ( len )
    {
        const uint32_t newLen = m_Length + (uint32_t)len;
        if ( newLen > GetReserved() )
        {
            Grow( newLen );
        }

        EnsureOnlyOwner();
        Copy( string, m_Contents + m_Length, len ); // handles terminator
        m_Length = newLen;
    }
    ASSERT( ( StrNCmp( m_Contents, "x64", 4 ) != 0 ) || ( m_Length == 3 ) );

    return *this;
}

// AppendFormat
//------------------------------------------------------------------------------
AString & AString::AppendFormat( MSVC_SAL_PRINTF const char * fmtString, ... )
{
    AStackString<1024> buffer;
    va_list args;
    va_start( args, fmtString );
    buffer.VFormat( fmtString, args );
    va_end( args );

    Append( buffer );
    ASSERT( ( StrNCmp( m_Contents, "x64", 4 ) != 0 ) || ( m_Length == 3 ) );

    return *this;
}

// AppendList
//------------------------------------------------------------------------------
AString & AString::AppendList( const Array<AString> & list, char separator )
{
    // Ignore empty lists explicitly to simplify logic below
    if ( list.IsEmpty() )
    {
        return *this;
    }

    // Determine how many separators will be needed
    size_t numSeparatorsRemaining = ( list.GetSize() - 1 );

    size_t sumStringLengths = 0;
    for ( const AString & string : list )
    {
        sumStringLengths += string.GetLength();
    }

    const uint32_t newLength = GetLength() + (uint32_t)numSeparatorsRemaining + (uint32_t)sumStringLengths;
    if ( newLength > GetReserved() )
    {
        Grow( newLength );
    }

    // Append items with separators
    for ( const AString & item : list )
    {
        Append( item );
        if ( numSeparatorsRemaining > 0 )
        {
            Append( separator );
            --numSeparatorsRemaining;
        }
    }
    ASSERT( ( StrNCmp( m_Contents, "x64", 4 ) != 0 ) || ( m_Length == 3 ) );

    return *this;
}

// Replace ( char, char )
//------------------------------------------------------------------------------
uint32_t AString::Replace( char from, char to, uint32_t maxReplaces )
{
    if ( from == to )
    {
        return 0;
    }

    EnsureOnlyOwner();
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
    ASSERT( ( StrNCmp( m_Contents, "x64", 4 ) != 0 ) || ( m_Length == 3 ) );
    return replaceCount;
}

// ToLower
//------------------------------------------------------------------------------
void AString::ToLower()
{
    EnsureOnlyOwner();
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
    EnsureOnlyOwner();
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
    const size_t toLength = StrLen( to );
    if ( fromLength == 0 || StrNCmp( from, to, Math::Max( fromLength, toLength ) ) == 0 )
    {
        // string to replace can't be empty, otherwise replace operation doesn't make sense
        return 0;
    }

    AStackString<2 * KILOBYTE> temp;
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
    const uint32_t len = m_Length;
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
    const size_t otherLen = StrLen( string );
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
    const uint32_t otherLen = string.GetLength();
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
    const size_t otherLen = StrLen( string );
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
    const uint32_t otherLen = string.GetLength();
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
        do
        {
            pat++;
        } while ( *pat == '*' );
    }

test_match:
    int i;
    for ( i = 0; pat[ i ] && ( pat[ i ] != '*' ); i++ )
    {
        const char a = str[ i ];
        const char b = pat[ i ];
        if ( a != b )
        {
            if ( !str[ i ] )
            {
                return false;
            }
            if ( ( pat[ i ] == '?' ) && ( str[ i ] != '.' ) )
            {
                continue;
            }
            if ( !star )
            {
                return false;
            }
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
    if ( !str[ i ] )
    {
        return true;
    }
    if ( i && pat[ i - 1 ] == '*' )
    {
        return true;
    }
    if ( !star )
    {
        return false;
    }
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
        do
        {
            pat++;
        } while ( *pat == '*' );
    }

test_match:
    int i;
    for ( i = 0; pat[ i ] && ( pat[ i ] != '*' ); i++ )
    {
        char a = str[ i ];
        a = ( ( a >= 'A' ) && ( a <= 'Z' ) ) ? 'a' + ( a - 'A' ) : a;
        char b = pat[ i ];
        b = ( ( b >= 'A' ) && ( b <= 'Z' ) ) ? 'a' + ( b - 'A' ) : b;
        if ( a != b )
        {
            if ( !str[ i ] )
            {
                return false;
            }
            if ( ( pat[ i ] == '?' ) && ( str[ i ] != '.' ) )
            {
                continue;
            }
            if ( !star )
            {
                return false;
            }
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
    if ( !str[ i ] )
    {
        return true;
    }
    if ( i && pat[ i - 1 ] == '*' )
    {
        return true;
    }
    if ( !star )
    {
        return false;
    }
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
    dst[ len ] = '\000';
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
    Atomic<uint32_t> * const oldReferenceCount = m_ReferenceCount;
    const char * const oldContents = m_Contents;

    UnsafeAllocateSharedMemory( newLength );

    // transfer existing string data
    Copy( oldContents, m_Contents, m_Length ); // copy handles terminator

    AString::ReleaseSharedMemory( this, oldReferenceCount, oldContents );
}

// GrowNoCopy
//------------------------------------------------------------------------------
void AString::GrowNoCopy( uint32_t newLength )
{
    UnsafeReleaseSharedMemory();
    UnsafeAllocateSharedMemory( newLength );
}

// UnsafeAllocateSharedMemory
//------------------------------------------------------------------------------
void AString::UnsafeAllocateSharedMemory( uint32_t reserve )
{
    AString::AllocateSharedMemory( m_ReferenceCount, m_Contents, reserve );
    m_Reserved = reserve;
}

// UnsafeReleaseSharedMemory
//------------------------------------------------------------------------------
void AString::UnsafeReleaseSharedMemory()
{
    // Unsafe because it leaves the string in an unsafe state.
    // Safe to call from any state, but caller must restore a valid state ASAP.
    AString::ReleaseSharedMemory( this, m_ReferenceCount, m_Contents );
}

// AllocateSharedMemory
//------------------------------------------------------------------------------
/*static*/ void AString::AllocateSharedMemory( Atomic<uint32_t> * & referenceCount, char * & contents, uint32_t reserve )
{
    void * const newMem = ALLOC( sizeof(Atomic<uint32_t>) + reserve + 1 ); // also allocate for \0 terminator
    referenceCount = (Atomic<uint32_t> *)newMem;
    contents = (char *)newMem + sizeof( Atomic<uint32_t> );
    INPLACE_NEW( referenceCount ) Atomic<uint32_t>( 1 );
}

// ReleaseSharedMemory
//------------------------------------------------------------------------------
/*static*/ void AString::ReleaseSharedMemory( const AString* string, Atomic<uint32_t> * referenceCount, const char * contents )
{
    (void) contents;
    (void) string;
    if ( referenceCount )
    {
        // if we own the memory, we must:
        // a) NOT be pointing to the shared global string
        ASSERT( contents != s_EmptyString );
        // b) NOT be pointing to different places in memory
        ASSERT( (void *)contents == (void *)( (char *)referenceCount + sizeof( Atomic<uint32_t> ) ) );
        // c) NOT be pointing to an internal buffer
        // Depending on the memory allocator, it could be valid to have an allocation
        // immediately after the string itself, so we can't have an assert for this

        // Memory is on the heap, but may be shared with others.
        const uint32_t remainingReferenceCount = referenceCount->Decrement();
        if ( remainingReferenceCount == 0 )
        {
            // We were the last remaining string owning this memory, so it must be freed
            referenceCount->~Atomic<uint32_t>();
            FREE( (void *)referenceCount );
        }
    }
    else
    {
        // if we don't own the memory, either:
        // a) We are an empty string, pointing to the special global empty string
        // OR:
        // b) We are a StackString, and we should point to our internal buffer
        ASSERT( ( contents == s_EmptyString ) ||
                ( (void *)contents == (void *)( (char *)string + sizeof( AString ) ) ) );
    }
}

// UnsafeInitAsEmpty
//------------------------------------------------------------------------------
void AString::UnsafeInitAsEmpty( uint32_t reserve )
{
    UnsafeAllocateSharedMemory( reserve );
    m_Contents[ 0 ] = '\000';
    m_Length = 0;
}

// UnsafeAssignSharingWith
//------------------------------------------------------------------------------
void AString::UnsafeAssignSharingWith( const AString & string )
{
    m_ReferenceCount = string.m_Contents != s_EmptyString ? const_cast<Atomic<uint32_t> *>( string.m_ReferenceCount ) : nullptr; // special value for the empty string
    m_Contents = const_cast<char *>( string.m_Contents );
    m_Length = string.m_Length;
    m_Reserved = string.m_Reserved;
    ASSERT( ( m_ReferenceCount != nullptr ) || ( m_Contents == s_EmptyString ) ); // consistency check: shouldn't be using this on self-managed memory
}

// EnsureOnlyOwner
//------------------------------------------------------------------------------
void AString::EnsureOnlyOwner()
{
    if ( ( !IsOnlyOwner() ) && ( m_Reserved > 0 ) )
    {
        ASSERT( m_Contents != AString::s_EmptyString ); // prevent calling this on s_EmptyAString

        Grow( m_Reserved ); // reuse Grow's solid logic
    }
}

// ToMutableContentPtr
//------------------------------------------------------------------------------
char * AString::ToMutableContentPtr( const char * pos )
{
    if ( pos == nullptr )
    {
        return nullptr;
    }
    ASSERT( ( pos >= m_Contents ) );
    ASSERT( ( pos <= m_Contents + GetLength() ) );
    const size_t offset = ( pos - m_Contents );
    EnsureOnlyOwner();
    return m_Contents + offset;
}

//------------------------------------------------------------------------------
