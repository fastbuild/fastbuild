// Compressor
//------------------------------------------------------------------------------

// Includes
//------------------------------------------------------------------------------
#include "Compressor.h"

// FBuildCore
#include "Tools/FBuild/FBuildCore/FBuild.h"

// Core
#include "Core/Containers/UniquePtr.h"
#include "Core/Env/Assert.h"
#include "Core/Env/Types.h"
#include "Core/Math/Conversions.h"
#include "Core/Mem/Mem.h"
#include "Core/Profile/Profile.h"

// External
#include "lz4.h"
#include "lz4hc.h"

#include <memory.h>

//------------------------------------------------------------------------------
Compressor::Compressor()
    : m_Result( nullptr )
    , m_ResultSize( 0 )
{
}

// DESTRUCTOR
//------------------------------------------------------------------------------
Compressor::~Compressor()
{
    FREE( m_Result );
}

// IsValidData
//------------------------------------------------------------------------------
bool Compressor::IsValidData( const void * data, size_t dataSize ) const
{
    const Header * header = (const Header *)data;
    if ( header->m_CompressionType > 1 )
    {
        return false;
    }
    if ( ( header->m_CompressedSize + sizeof( Header ) ) != dataSize )
    {
        return false;
    }
    if ( header->m_CompressedSize > header->m_UncompressedSize )
    {
        return false;
    }
    return true;
}

// Compress
//------------------------------------------------------------------------------
bool Compressor::Compress( const void * data, size_t dataSize, int32_t compressionLevel )
{
    PROFILE_FUNCTION;

    ASSERT( data );
    ASSERT( m_Result == nullptr );

    // allocate worst case output size for LZ4
    const int worstCaseSize = LZ4_compressBound( (int)dataSize );
    UniquePtr< char > output( (char *)ALLOC( (size_t)worstCaseSize ) );

    int32_t compressedSize;

    // do compression
    if ( compressionLevel > 0 )
    {
        // Higher compression, using LZ4HC
        compressedSize = LZ4_compress_HC( (const char*)data, output.Get(), (int)dataSize, worstCaseSize, compressionLevel );
    }
    else if ( compressionLevel < 0 )
    {
        // Lower compression, using regular LZ4
        const int32_t acceleration = ( 0 - compressionLevel );
        compressedSize = LZ4_compress_fast( (const char*)data, output.Get(), (int)dataSize, worstCaseSize, acceleration );
    }
    else
    {
        // Disable compression
        compressedSize = (int32_t)dataSize; // Act as if compression achieved nothing
    }

    // did the compression yield any benefit?
    const bool compressed = ( compressedSize < (int)dataSize );

    if ( compressed )
    {
        // trim memory usage to compressed size
        m_Result = ALLOC( (uint32_t)compressedSize + sizeof( Header ) );
        memcpy( (char *)m_Result + sizeof( Header ), output.Get(), (size_t)compressedSize );
        m_ResultSize = (uint32_t)compressedSize + sizeof( Header );
    }
    else
    {
        // compression failed, so just copy the old data
        m_Result = ALLOC( dataSize + sizeof( Header ) );
        memcpy( (char *)m_Result + sizeof( Header ), data, dataSize );
        m_ResultSize = dataSize + sizeof( Header );
    }

    // fill out header
    Header * header = (Header*)m_Result;
    header->m_CompressionType = compressed ? 1u : 0u;   // compression type
    header->m_UncompressedSize = (uint32_t)dataSize;    // input size
    header->m_CompressedSize = compressed ? (uint32_t)compressedSize : (uint32_t)dataSize;    // output size

    return compressed;
}

// Decompress
//------------------------------------------------------------------------------
bool Compressor::Decompress( const void * data )
{
    PROFILE_FUNCTION;

    ASSERT( data );
    ASSERT( m_Result == nullptr );

    const Header * header = (const Header *)data;

    // handle uncompressed case
    if ( header->m_CompressionType == 0 )
    {
        m_Result = ALLOC( header->m_UncompressedSize );
        memcpy( m_Result, (char *)data + sizeof( Header ), header->m_UncompressedSize );
        m_ResultSize = header->m_UncompressedSize;
        return true;
    }
    ASSERT( header->m_CompressionType == 1 );

    // uncompressed size
    const uint32_t uncompressedSize = header->m_UncompressedSize;
    m_Result = ALLOC( uncompressedSize );
    m_ResultSize = uncompressedSize;

    // skip over header to LZ4 data
    const char * compressedData = ( (const char *)data + sizeof( Header ) );

    // decompress
    const int bytesDecompressed = LZ4_decompress_safe( compressedData, (char *)m_Result, (int)header->m_CompressedSize, (int)uncompressedSize);
    if ( bytesDecompressed == (int)uncompressedSize )
    {
        return true;
    }

    // Data is corrupt
    FREE( m_Result );
    m_Result = nullptr;
    m_ResultSize = 0;
    return false;
}

//------------------------------------------------------------------------------
