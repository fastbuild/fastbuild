// MemoryStream
//------------------------------------------------------------------------------
#pragma once

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
    virtual ~MemoryStream() override;

    // movable
    explicit MemoryStream( MemoryStream && other );
    void operator=( MemoryStream && other );

    // non-copyable
    explicit MemoryStream( const MemoryStream & other ) = delete;
    void operator=( const MemoryStream & other ) = delete;

    // memory stream specific functions
    const void * GetData() const { return (void *)m_Begin; }
    void * GetDataMutable() { return (void *)m_Begin; }
    size_t GetSize() const { return (size_t)( m_End - m_Begin ); }
    void Reset();
    void * Release();
    void Replace( void * memory, size_t size );

    uint64_t WriteBuffer( IOStream & stream, uint64_t bytesToWrite );

    // raw read/write functions
    virtual uint64_t ReadBuffer( void * buffer, uint64_t bytesToRead ) override;
    virtual uint64_t WriteBuffer( const void * buffer, uint64_t bytesToWrite ) override;
    virtual void Flush() override;

    // size/position
    virtual uint64_t Tell() const override;
    virtual bool Seek( uint64_t pos ) const override;
    virtual uint64_t GetFileSize() const override;

private:
    NO_INLINE void GrowToAccommodate( uint64_t bytesToAccomodate );

    char * m_Begin = nullptr;
    char * m_End = nullptr;
    char * m_MaxEnd = nullptr;
    size_t m_MinGrowth = 4096;
};

//------------------------------------------------------------------------------
