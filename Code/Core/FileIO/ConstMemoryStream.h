// ConstMemoryStream - read only access to data wrapped in an IOStream interface
//------------------------------------------------------------------------------
#pragma once

// Includes
//------------------------------------------------------------------------------
#include "IOStream.h"

// ConstMemoryStream
//------------------------------------------------------------------------------
class ConstMemoryStream : public IOStream
{
public:
    explicit ConstMemoryStream( const void * data, size_t size );
    ~ConstMemoryStream();

    // memory stream specific functions
    inline const void * GetData() const { return m_Buffer; }
    inline size_t       GetSize() const { return m_Size; }
    void                Replace( const void * data, size_t size );

    // raw read/write functions
    virtual uint64_t ReadBuffer( void * buffer, uint64_t bytesToRead );
    virtual uint64_t WriteBuffer( const void * buffer, uint64_t bytesToWrite );
    virtual void Flush();

    // size/position
    virtual uint64_t Tell() const;
    virtual bool Seek( uint64_t pos ) const;
    virtual uint64_t GetFileSize() const;

private:
    const void * m_Buffer;
    size_t m_Size;
    mutable size_t m_CurrentPos;
};

//------------------------------------------------------------------------------
