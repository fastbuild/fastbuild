// FileStream
//------------------------------------------------------------------------------
#pragma once

//------------------------------------------------------------------------------
#include "IOStream.h"

// FileStream
//------------------------------------------------------------------------------
class FileStream : public IOStream
{
public:
    explicit FileStream();
    virtual ~FileStream() override;

    enum FileMode
    {
        READ_ONLY                     = 0x1,
        WRITE_ONLY                    = 0x2,
        TEMP                          = 0x4,
        NO_RETRY_ON_SHARING_VIOLATION = 0x80,
    };

    bool Open( const char * fileName, uint32_t mode = FileStream::READ_ONLY );
    void Close();

    bool IsOpen() const;

    // raw read/write functions
    virtual uint64_t ReadBuffer( void * buffer, uint64_t bytesToRead ) override;
    virtual uint64_t WriteBuffer( const void * buffer, uint64_t bytesToWrite ) override;
    virtual void Flush() override;

    // size/position
    virtual uint64_t Tell() const override;
    virtual bool Seek( uint64_t pos ) const override;
    virtual uint64_t GetFileSize() const override;

    // file specific
    #if defined( __WINDOWS__ )
        // Set on already open file via handle (Windows only)
        bool SetLastWriteTime( uint64_t lastWriteTime );
    #endif

private:
    #if defined( __WINDOWS__ )
        void * m_Handle;
    #else
        int32_t m_Handle;
    #endif
};

//------------------------------------------------------------------------------
