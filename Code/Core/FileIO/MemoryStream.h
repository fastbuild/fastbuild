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

    // memory stream specific functions
    inline const void * GetData() const { return (void *)m_Begin; }
    inline void *       GetDataMutable() { return (void *)m_Begin; }
    inline size_t       GetSize() const { return (size_t)( m_End - m_Begin ); }
    void                Reset();
    void *              Release();

    uint64_t            WriteBuffer( IOStream & stream, uint64_t bytesToWrite );

    // raw read/write functions
    virtual uint64_t ReadBuffer( void * buffer, uint64_t bytesToRead ) override;
    virtual uint64_t WriteBuffer( const void * buffer, uint64_t bytesToWrite ) override;
    virtual void Flush() override;

    // size/position
    virtual uint64_t Tell() const override;
    virtual bool Seek( uint64_t pos ) const override;
    virtual uint64_t GetFileSize() const override;

private:
    NO_INLINE void GrowToAccomodate( uint64_t bytesToAccomodate );

    char *          m_Begin;
    char *          m_End;
    char *          m_MaxEnd;
    size_t          m_MinGrowth;
};

//------------------------------------------------------------------------------
