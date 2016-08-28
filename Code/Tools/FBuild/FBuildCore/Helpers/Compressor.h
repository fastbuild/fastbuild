// Compressor
//------------------------------------------------------------------------------
#pragma once

// Includes
//------------------------------------------------------------------------------
#include "Core/Env/Types.h"

// Compressor
//------------------------------------------------------------------------------
class Compressor
{
public:
    explicit Compressor();
    ~Compressor();

    bool IsValidData( const void * data, size_t dataSize ) const;

    bool Compress( const void * data, size_t dataSize );
    void Decompress( const void * data );

    const void *    GetResult() const       { return m_Result; }
    size_t          GetResultSize() const   { return m_ResultSize; }

    inline void *   ReleaseResult()         { void * r = m_Result; m_Result = nullptr; m_ResultSize = 0; return r; }

private:
    struct Header
    {
        uint32_t m_CompressionType;
        uint32_t m_UncompressedSize;
        uint32_t m_CompressedSize;
    };
    void * m_Result;
    size_t m_ResultSize;
};

//------------------------------------------------------------------------------
