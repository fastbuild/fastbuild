// MemoryStream
//------------------------------------------------------------------------------
#pragma once
#ifndef CORE_FILEIO_MEMORYSTREAM_H
#define CORE_FILEIO_MEMORYSTREAM_H

// Includes
//------------------------------------------------------------------------------
#include "IOStream.h"

// MemoryStream
//------------------------------------------------------------------------------
class MemoryStream : public IOStream
{
public:
	explicit MemoryStream();
	explicit MemoryStream( size_t initialBufferSize, size_t minGrowth = 4096 );
	~MemoryStream();

	// memory stream specific functions
	inline const void * GetData() const { return (void *)m_Begin; }
	inline void *		GetDataMutable() { return (void *)m_Begin; }
	inline size_t		GetSize() const { return ( m_End - m_Begin ); } 

	// raw read/write functions
	virtual uint64_t ReadBuffer( void * buffer, uint64_t bytesToRead );
	virtual uint64_t WriteBuffer( const void * buffer, uint64_t bytesToWrite );
	virtual void Flush();

	// size/position
	virtual uint64_t Tell() const;
	virtual bool Seek( uint64_t pos ) const;
	virtual uint64_t GetFileSize() const;

private:
	NO_INLINE void GrowToAccomodate( uint64_t bytesToAccomodate );

	char *			m_Begin;
	char *			m_End;
	char *			m_MaxEnd;
	size_t			m_MinGrowth;
};

//------------------------------------------------------------------------------
#endif // CORE_FILEIO_MEMORYSTREAM_H
