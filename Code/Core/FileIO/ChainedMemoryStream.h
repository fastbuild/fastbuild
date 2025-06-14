// ChainedMemoryStream
//------------------------------------------------------------------------------
#pragma once

// Includes
//------------------------------------------------------------------------------
#include "IOStream.h"

// Core
#include "Core/Containers/Array.h"

// ChainedMemoryStream
//------------------------------------------------------------------------------
class ChainedMemoryStream : public IOStream
{
public:
    explicit ChainedMemoryStream( size_t pageSize );
    virtual ~ChainedMemoryStream() override;

    // Copying not allowed
    ChainedMemoryStream & operator=( ChainedMemoryStream & other ) = delete;

    // ChainedMemoryStream specific functions
    uint32_t GetNumPages() const { return static_cast<uint32_t>( m_Pages.GetSize() ); }
    char * GetPage( uint32_t index, uint32_t & outSize );

    // IOStream interface
    virtual uint64_t ReadBuffer( void * buffer, uint64_t bytesToRead ) override;
    virtual uint64_t WriteBuffer( const void * buffer, uint64_t bytesToWrite ) override;
    virtual void Flush() override;
    virtual uint64_t Tell() const override;
    virtual bool Seek( uint64_t pos ) const override;
    virtual uint64_t GetFileSize() const override;

private:
    const uint32_t m_PageSize;
    uint32_t m_CurrentPageIndex = 0;
    uint32_t m_PosInPage = 0;
    Array<char *> m_Pages;
};

//------------------------------------------------------------------------------
