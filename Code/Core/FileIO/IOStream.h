// IOStream - interface for serialization
//------------------------------------------------------------------------------
#pragma once

// Includes
//------------------------------------------------------------------------------
#include "Core/Containers/Array.h"
#include "Core/Env/Types.h"

// Forward Declarations
//------------------------------------------------------------------------------
class AString;

// IStream
//------------------------------------------------------------------------------
class IOStream
{
public:
    explicit inline IOStream() = default;
    inline virtual ~IOStream() = default;

    // interface that must be implemented
    virtual uint64_t ReadBuffer( void * buffer, uint64_t bytesToRead ) = 0;
    virtual uint64_t WriteBuffer( const void * buffer, uint64_t bytesToWrite ) = 0;
    virtual void Flush() = 0;

    // size/position
    virtual uint64_t Tell() const = 0;
    virtual bool Seek( uint64_t pos ) const = 0;
    virtual uint64_t GetFileSize() const = 0;

    // helper read wrappers
    inline uint64_t Read( void * b, size_t s ) { return ReadBuffer( b, s ); }
    inline bool Read( bool & b )        { return ( ReadBuffer( &b, sizeof( b ) ) == sizeof( b ) ); }
    inline bool Read( int8_t & i )      { return ( ReadBuffer( &i, sizeof( i ) ) == sizeof( i ) ); }
    inline bool Read( int16_t & i )     { return ( ReadBuffer( &i, sizeof( i ) ) == sizeof( i ) ); }
    inline bool Read( int32_t & i )     { return ( ReadBuffer( &i, sizeof( i ) ) == sizeof( i ) ); }
    inline bool Read( int64_t & i )     { return ( ReadBuffer( &i, sizeof( i ) ) == sizeof( i ) ); }
    inline bool Read( uint8_t & u )     { return ( ReadBuffer( &u, sizeof( u ) ) == sizeof( u ) ); }
    inline bool Read( uint16_t & u )    { return ( ReadBuffer( &u, sizeof( u ) ) == sizeof( u ) ); }
    inline bool Read( uint32_t & u )    { return ( ReadBuffer( &u, sizeof( u ) ) == sizeof( u ) ); }
    inline bool Read( uint64_t & u )    { return ( ReadBuffer( &u, sizeof( u ) ) == sizeof( u ) ); }
    bool Read( AString & string );
    template< class T > inline bool Read( Array< T > & a );

    // helper write wrappers
    inline uint64_t Write( const void * b, size_t s ) { return WriteBuffer( b, s ); }
    inline bool Write( const bool & b )     { return ( WriteBuffer( &b, sizeof( b ) ) == sizeof( b ) ); }
    inline bool Write( const int8_t & i )   { return ( WriteBuffer( &i, sizeof( i ) ) == sizeof( i ) ); }
    inline bool Write( const int16_t & i )  { return ( WriteBuffer( &i, sizeof( i ) ) == sizeof( i ) ); }
    inline bool Write( const int32_t & i )  { return ( WriteBuffer( &i, sizeof( i ) ) == sizeof( i ) ); }
    inline bool Write( const int64_t & i )  { return ( WriteBuffer( &i, sizeof( i ) ) == sizeof( i ) ); }
    inline bool Write( const uint8_t & u )  { return ( WriteBuffer( &u, sizeof( u ) ) == sizeof( u ) ); }
    inline bool Write( const uint16_t & u ) { return ( WriteBuffer( &u, sizeof( u ) ) == sizeof( u ) ); }
    inline bool Write( const uint32_t & u ) { return ( WriteBuffer( &u, sizeof( u ) ) == sizeof( u ) ); }
    inline bool Write( const uint64_t & u ) { return ( WriteBuffer( &u, sizeof( u ) ) == sizeof( u ) ); }
    bool Write( const AString & string );
    template< class T > inline bool Write( const Array< T > & a );

    // manage memory-mapped aligned data
    void AlignRead( size_t alignment );
    void AlignWrite( size_t alignment );
};

// Read ( Array< T > )
//------------------------------------------------------------------------------
template< class T >
bool IOStream::Read( Array< T > & a )
{
    uint32_t num = 0;
    if ( Read( num ) == false ) { return false; }
    a.SetSize( num );
    for ( uint32_t i = 0; i < num; ++i )
    {
        if ( Read( a[ i ] ) == false ) { return false; }
    }
    return true;
}

// Write ( Array< T > )
//------------------------------------------------------------------------------
template< class T >
bool IOStream::Write( const Array< T > & a )
{
    const uint32_t num = (uint32_t)a.GetSize();
    if ( Write( num ) == false ) { return false; }
    for ( uint32_t i = 0; i < num; ++i )
    {
        if ( Write( a[ i ] ) == false ) { return false; }
    }
    return true;
}

//------------------------------------------------------------------------------
