// IOStream - interface for serialization
//------------------------------------------------------------------------------

// Includes
//------------------------------------------------------------------------------
#include "IOStream.h"
#include "Core/Math/Conversions.h"
#include "Core/Strings/AString.h"

// Read (AString)
//------------------------------------------------------------------------------
bool IOStream::Read( AString & string )
{
    uint32_t len;
    if ( Read( len ) )
    {
        string.SetLength( len );
        return ( Read( string.Get(), len ) == len );
    }
    return false;
}

// Write (AString)
//------------------------------------------------------------------------------
bool IOStream::Write( const AString & string )
{
    uint32_t len = string.GetLength();
    bool ok = Write( len );
    ok &= ( Write( string.Get(), len ) == len );
    return ok;
}

// AlignRead
//------------------------------------------------------------------------------
void IOStream::AlignRead( size_t alignment )
{
    const uint64_t tell = Tell();
    const uint64_t toSkip = Math::RoundUp( tell, (uint64_t)alignment ) - tell;
    for ( uint64_t i = 0; i < toSkip; ++i )
    {
        uint8_t tmp;
        Read( tmp );
    }
    ASSERT( ( Tell() % alignment ) == 0 );
}

// AlignWrite
//------------------------------------------------------------------------------
void IOStream::AlignWrite( size_t alignment )
{
    const uint64_t tell = Tell();
    const uint64_t toPad = Math::RoundUp( tell, (uint64_t)alignment ) - tell;
    for ( uint64_t i = 0; i < toPad; ++i )
    {
        uint8_t padChar( 0 );
        Write( padChar );
    }
    ASSERT( ( Tell() % alignment ) == 0 );
}

// AlignWrite
//------------------------------------------------------------------------------
bool IOStream::ReadLines( Array< AString > & lines )
{
    const uint64_t size = GetFileSize();

    const int bufferSize = 2048;
    char buffer[2048];

    uint64_t start = Tell();
    uint64_t i = start;
    for ( ; i<size ; ++i )
    {
        int8_t ch;
        Read( ch );

        uint64_t stop = start;

        if ( ch  == '\n' || ch == '\r' )
        {
            stop = i;

            if ( ch == '\r' )
            {
                Read( ch );
                if ( ch != '\n' )
                {
                    Seek( i );
                }
                else
                {
                    i++;
                }
            }
        }

        ASSERT( start<=stop );

        if ( start < stop )
        {
            const int b = int( stop - start );
            lines.Append( AString( &buffer[0], &buffer[b] ) );
            start = i+1;
        }
        else
        {
            const int b = int( i - start );
            if ( b >= bufferSize )
            {
                ASSERT( false );
                return false;
            }

            buffer[b] = char(ch);
        }
    }

    if (start < i)
    {
        const int b = int( i - start );
        lines.Append( AString( &buffer[0], &buffer[b] ) );
    }

    return true;
}

//------------------------------------------------------------------------------
