// ConstMemoryStream - read only access to data wrapped in an IOStream interface
//------------------------------------------------------------------------------
#pragma once

// Includes
//------------------------------------------------------------------------------
#include "IOStream.h"

// Forward Declarations
//------------------------------------------------------------------------------
class MemoryStream;

// ConstMemoryStream
//------------------------------------------------------------------------------
class ConstMemoryStream : public IOStream
{
public:
    ConstMemoryStream();
    explicit ConstMemoryStream( const void * data, size_t size );
    virtual ~ConstMemoryStream() override;

    // movable
    explicit ConstMemoryStream( MemoryStream && other );
    explicit ConstMemoryStream( ConstMemoryStream && other );
    void operator=( ConstMemoryStream && other );

    // non-copyable
    explicit ConstMemoryStream( const ConstMemoryStream & other ) = delete;
    void operator=( const ConstMemoryStream & other ) = delete;

    // memory stream specific functions
    const void * GetData() const { return m_Buffer; }
    size_t GetSize() const { return m_Size; }
    void Replace( const void * data, size_t size, bool ownsMemory );

    // raw read/write functions
    virtual uint64_t ReadBuffer( void * buffer, uint64_t bytesToRead ) override;
    virtual uint64_t WriteBuffer( const void * buffer, uint64_t bytesToWrite ) override;
    virtual void Flush() override;

    // size/position
    virtual uint64_t Tell() const override;
    virtual bool Seek( uint64_t pos ) const override;
    virtual uint64_t GetFileSize() const override;

private:
    const void * m_Buffer = nullptr;
    size_t m_Size = 0;
    mutable size_t m_CurrentPos = 0;
    bool m_OwnsMemory = false;
};

//------------------------------------------------------------------------------
