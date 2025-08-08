// MemoryStream.cpp
//------------------------------------------------------------------------------

// Includes
//------------------------------------------------------------------------------
#include "MemoryStream.h"

#include <memory.h> // for memcpy

// Defines
//------------------------------------------------------------------------------

// CONSTRUCTOR
//------------------------------------------------------------------------------
MemoryStream::MemoryStream() = default;

// CONSTRUCTOR
//------------------------------------------------------------------------------
MemoryStream::MemoryStream( size_t initialBufferSize, size_t minGrowthFactor )
    : m_Begin( (char *)ALLOC( initialBufferSize ) )
    , m_End( m_Begin )
    , m_MaxEnd( m_Begin + initialBufferSize )
    , m_MinGrowth( minGrowthFactor )
{
}

// DESTRUCTOR
//------------------------------------------------------------------------------
MemoryStream::~MemoryStream()
{
    FREE( m_Begin );
}

// CONSTRUCTOR
//------------------------------------------------------------------------------
MemoryStream::MemoryStream( MemoryStream && other )
    : m_Begin( other.m_Begin )
    , m_End( other.m_End )
    , m_MaxEnd( other.m_MaxEnd )
    , m_MinGrowth( other.m_MinGrowth )
{
    other.m_Begin = nullptr;
    other.m_End = nullptr;
    other.m_MaxEnd = nullptr;
}

// oeprator=
//------------------------------------------------------------------------------
void MemoryStream::operator=( MemoryStream && other )
{
    FREE( m_Begin );
    m_Begin = other.m_Begin;
    m_End = other.m_End;
    m_MaxEnd = other.m_MaxEnd;
    m_MinGrowth = other.m_MinGrowth;
    other.m_Begin = nullptr;
    other.m_End = nullptr;
    other.m_MaxEnd = nullptr;
}

// Reset
//------------------------------------------------------------------------------
void MemoryStream::Reset()
{
    m_End = m_Begin;
}

// Release
//------------------------------------------------------------------------------
void * MemoryStream::Release()
{
    void * mem = m_Begin;
    m_Begin = nullptr;
    m_End = nullptr;
    m_MaxEnd = nullptr;
    return mem;
}

// Replace
//------------------------------------------------------------------------------
void MemoryStream::Replace( void * memory, size_t size )
{
    // Free existing memory if we have any
    FREE( m_Begin );

    // Own new memory
    m_Begin = static_cast<char *>( memory );
    m_End = m_Begin + size;
    m_MaxEnd = m_End;
}

// WriteBuffer
//------------------------------------------------------------------------------
uint64_t MemoryStream::WriteBuffer( IOStream & stream, uint64_t bytesToWrite )
{
    if ( ( m_End + bytesToWrite ) > m_MaxEnd )
    {
        GrowToAccommodate( bytesToWrite );
    }

    // Read directly into end of buffer
    const uint64_t bytesRead = stream.ReadBuffer( m_End, bytesToWrite );
    m_End += bytesRead;
    return bytesRead;
}

// Read
//------------------------------------------------------------------------------
uint64_t MemoryStream::ReadBuffer( void * buffer, uint64_t bytesToRead )
{
    (void)buffer;
    (void)bytesToRead;
    ASSERT( false ); // Not implemented - implement if required
    return 0;
}

// Write
//------------------------------------------------------------------------------
uint64_t MemoryStream::WriteBuffer( const void * buffer, uint64_t bytesToWrite )
{
    if ( ( m_End + bytesToWrite ) > m_MaxEnd )
    {
        GrowToAccommodate( bytesToWrite );
    }

    memcpy( m_End, buffer, (size_t)bytesToWrite );
    m_End += bytesToWrite;

    return bytesToWrite;
}

// Flush
//------------------------------------------------------------------------------
void MemoryStream::Flush()
{
    // nothing to do
}

// Tell
//------------------------------------------------------------------------------
uint64_t MemoryStream::Tell() const
{
    return GetSize(); // Write position is always at end
}

// Seek
//------------------------------------------------------------------------------
bool MemoryStream::Seek( uint64_t pos ) const
{
    (void)pos;
    ASSERT( false ); // Not implemented - implement if required
    return true;
}

// GetFileSize
//------------------------------------------------------------------------------
uint64_t MemoryStream::GetFileSize() const
{
    return GetSize();
}

// GrowToAccommodate
//------------------------------------------------------------------------------
void MemoryStream::GrowToAccommodate( uint64_t bytesToAccommodate )
{
    // grow by at least MinGrowth
    size_t newCapacity = (size_t)( m_MaxEnd - m_Begin ) + Math::Max<size_t>( (size_t)bytesToAccommodate, m_MinGrowth );
    char * oldBegin = m_Begin;
    char * oldEnd = m_End;
    m_Begin = (char *)ALLOC( newCapacity );
    m_End = m_Begin + ( oldEnd - oldBegin );
    m_MaxEnd = m_Begin + newCapacity;
    memcpy( m_Begin, oldBegin, (size_t)( oldEnd - oldBegin ) );
    FREE( oldBegin );
}

//------------------------------------------------------------------------------
