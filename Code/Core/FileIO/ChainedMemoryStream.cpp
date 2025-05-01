// ChainedMemoryStream.cpp
//------------------------------------------------------------------------------

// Includes
//------------------------------------------------------------------------------
#include "ChainedMemoryStream.h"

#include <memory.h> // for memcpy

// Defines
//------------------------------------------------------------------------------

// CONSTRUCTOR
//------------------------------------------------------------------------------
ChainedMemoryStream::ChainedMemoryStream( size_t pageSize )
    : m_PageSize( static_cast<uint32_t>( pageSize ) )
{
}

// DESTRUCTOR
//------------------------------------------------------------------------------
ChainedMemoryStream::~ChainedMemoryStream()
{
    for ( char * page : m_Pages )
    {
        FDELETE [] page;
    }
}

//------------------------------------------------------------------------------
char * ChainedMemoryStream::GetPage( uint32_t index, uint32_t & outSize )
{
    // All pages are always full, except the last one, which can be partially
    // filled
    const bool lastPage = ( index == ( m_Pages.GetSize() - 1 ) );
    outSize = lastPage ? m_PosInPage : m_PageSize;
    return m_Pages[ index ];
}

// Read
//------------------------------------------------------------------------------
uint64_t ChainedMemoryStream::ReadBuffer( void * /*buffer*/, uint64_t /*bytesToRead*/ )
{
    ASSERT( false ); // Not implemented - implement if required
    return 0;
}

// Write
//------------------------------------------------------------------------------
/*virtual*/ uint64_t ChainedMemoryStream::WriteBuffer( const void * buffer, uint64_t bytesToWrite )
{
    // Write data into one or more pages
    const uint64_t bytesWritten = bytesToWrite; // We'll always succeed writing all bytes
    const char * src = static_cast<const char *>( buffer );
    while ( bytesToWrite > 0 )
    {
        const bool noPages = m_Pages.IsEmpty();
        uint32_t bytesAvailableInPage = ( noPages ? 0u : ( m_PageSize - m_PosInPage ) );

        // Current page full?
        if ( bytesAvailableInPage == 0 )
        {
            // Get a new page
            m_Pages.EmplaceBack( FNEW_ARRAY( char[ m_PageSize ] ) );
            m_CurrentPageIndex = static_cast<uint32_t>( m_Pages.GetSize() - 1u );
            m_PosInPage = 0;
            bytesAvailableInPage = m_PageSize;

            // Continue and use newly allocated page
        }

        // Write as much as will fit in the current page
        ASSERT( bytesAvailableInPage > 0 );
        const uint64_t bytesToWriteToPage = Math::Min<uint64_t>( bytesAvailableInPage , bytesToWrite );
        char * dst = m_Pages[ m_CurrentPageIndex ] + m_PosInPage;
        memcpy( dst, src, bytesToWriteToPage );
        src += bytesToWriteToPage;
        m_PosInPage += static_cast<uint32_t>( bytesToWriteToPage );
        ASSERT( m_PosInPage <= m_PageSize );
        bytesToWrite -= bytesToWriteToPage;

        // Continue writing remaining data (if any)
    }

    return bytesWritten;
}

// Flush
//------------------------------------------------------------------------------
/*virtual*/ void ChainedMemoryStream::Flush()
{
    // nothing to do
}

// Tell
//------------------------------------------------------------------------------
/*virtual*/ uint64_t ChainedMemoryStream::Tell() const
{
    return GetFileSize(); // Position is always at the end
}

// Seek
//------------------------------------------------------------------------------
bool ChainedMemoryStream::Seek( uint64_t /*pos*/ ) const
{
    ASSERT( false ); // Not implemented - implement if required
    return true;
}

// GetFileSize
//------------------------------------------------------------------------------
/*virtual*/ uint64_t ChainedMemoryStream::GetFileSize() const
{
    const uint32_t numPages = GetNumPages();
    uint64_t totalSize = 0;
    if ( numPages > 1 )
    {
        // Pages other than the last are full
        totalSize += ( ( numPages - 1 ) * m_PageSize );
    }

    // Add usage of last page (which may also be zero)
    totalSize += m_PosInPage;

    return totalSize;
}

//------------------------------------------------------------------------------
