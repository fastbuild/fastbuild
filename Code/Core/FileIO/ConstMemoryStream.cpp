// ConstMemoryStream.cpp
//------------------------------------------------------------------------------

// Includes
//------------------------------------------------------------------------------
#include "Core/PrecompiledHeader.h"

#include "ConstMemoryStream.h"

// Core
#include "Core/Env/Types.h"

#include <memory.h> // for memcpy

// Defines
//------------------------------------------------------------------------------

// CONSTRUCTOR
//------------------------------------------------------------------------------
ConstMemoryStream::ConstMemoryStream( const void * data, size_t size )
    : m_Buffer( data )
    , m_Size( size )
    , m_CurrentPos( 0 )
{
}

// DESTRUCTOR
//------------------------------------------------------------------------------
ConstMemoryStream::~ConstMemoryStream() = default;

// Replace
//------------------------------------------------------------------------------
void ConstMemoryStream::Replace( const void * data, size_t size )
{
    m_Buffer = data;
    m_Size = size;
    m_CurrentPos = 0;
}

// Read
//------------------------------------------------------------------------------
uint64_t ConstMemoryStream::ReadBuffer( void * buffer, uint64_t bytesToRead )
{
    size_t availBytes = ( m_Size - m_CurrentPos );
    size_t actualBytesToRead = Math::Min( availBytes, (size_t)bytesToRead );
    memcpy( buffer, (void *)( (size_t)m_Buffer + m_CurrentPos ), actualBytesToRead );
    m_CurrentPos += actualBytesToRead;
    return actualBytesToRead;
}

// Write
//------------------------------------------------------------------------------
uint64_t ConstMemoryStream::WriteBuffer( const void *, uint64_t )
{
    ASSERT( false ); // not permitted
    return 0;
}

// Flush
//------------------------------------------------------------------------------
void ConstMemoryStream::Flush()
{
    ASSERT( false ); // not permitted
}

// Tell
//------------------------------------------------------------------------------
uint64_t ConstMemoryStream::Tell() const
{
    return m_CurrentPos;
}

// Seek
//------------------------------------------------------------------------------
bool ConstMemoryStream::Seek( uint64_t pos ) const
{
    if ( (size_t)pos > m_Size )
    {
        return false;
    }
    m_CurrentPos = (size_t)pos;
    return true;
}

// GetFileSize
//------------------------------------------------------------------------------
uint64_t ConstMemoryStream::GetFileSize() const
{
    return m_Size;
}

//------------------------------------------------------------------------------
