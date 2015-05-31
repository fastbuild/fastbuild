// Compressor
//------------------------------------------------------------------------------

// Includes
//------------------------------------------------------------------------------
#include "Tools/FBuild/FBuildCore/PrecompiledHeader.h"

#include "Compressor.h"

#include "Core/Containers/AutoPtr.h"
#include "Core/Env/Assert.h"
#include "Core/Env/Types.h"
#include "Core/Math/Conversions.h"
#include "Core/Mem/Mem.h"
#include "Core/Profile/Profile.h"

#include "lz4.h"

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
bool Compressor::Compress( const void * data, size_t dataSize )
{
    PROFILE_FUNCTION

	ASSERT( data );
	ASSERT( ( (size_t)data % 4 ) == 0 ); // input must be 4 byte aligned
	ASSERT( m_Result == nullptr );

	// allocate worst case output size for LZ4
	const int worstCaseSize = LZ4_compressBound( (int)dataSize );
	AutoPtr< char > output( (char *)ALLOC( worstCaseSize ) );

	// do compression
	const int compressedSize = LZ4_compress( (const char*)data, output.Get(), (int)dataSize);

	// did the compression yield any benefit?
	const bool compressed = ( compressedSize < (int)dataSize );

	if ( compressed )
	{
		// trim memory usage to compressed size
		m_Result = ALLOC( compressedSize + sizeof( Header ) );
		memcpy( (char *)m_Result + sizeof( Header ), output.Get(), compressedSize );
		m_ResultSize = compressedSize + sizeof( Header );
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
	header->m_CompressionType = compressed ? 1 : 0;		// compression type
	header->m_UncompressedSize = (uint32_t)dataSize;	// input size
	header->m_CompressedSize = compressed ? compressedSize : (uint32_t)dataSize;	// output size

	return compressed;
}

// Decompress
//------------------------------------------------------------------------------
void Compressor::Decompress( const void * data )
{
    PROFILE_FUNCTION

	ASSERT( data );
	ASSERT( ( (size_t)data % 4 ) == 0 ); // output must be 4 byte aligned
	ASSERT( m_Result == nullptr );

	const Header * header = (const Header *)data;

	// handle uncompressed case
	if ( header->m_CompressionType == 0 )
	{
		m_Result = ALLOC( header->m_UncompressedSize );
		memcpy( m_Result, (char *)data + sizeof( Header ), header->m_UncompressedSize );
		m_ResultSize = header->m_UncompressedSize;
		return;
	}
	ASSERT( header->m_CompressionType == 1 );

	// uncompressed size
	const uint32_t uncompressedSize = header->m_UncompressedSize;
	m_Result = ALLOC( uncompressedSize );
	m_ResultSize = uncompressedSize;

	// skip over header to LZ4 data
	const char * compressedData = ( (const char *)data + sizeof( Header ) );

	// decompress
	const int compressedSize = LZ4_decompress_fast( compressedData, (char *)m_Result, (int)uncompressedSize);
	ASSERT( compressedSize == (int)header->m_CompressedSize ); (void)compressedSize;
}

//------------------------------------------------------------------------------
