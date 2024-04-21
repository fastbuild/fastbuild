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

    static bool     IsValidData( const void * data, size_t dataSize );
    static uint32_t GetUncompressedSize( const void * data, size_t dataSize );

    // compressionLevel:
    //   < 0 : use LZ4, with values directly mapping to "acceleration level"
    //  == 0 : disable compression
    //   > 0 : use LZ4HC, with values directly mapping to "compression level"
    bool Compress( const void * data, size_t dataSize, int32_t compressionLevel = -1 ); // -1 = default LZ4 compression level

    // Zstd
    bool CompressZstd( const void * data, size_t dataSize, int32_t compressionLevel = -1 ); // -1 = default Zstd compression level

    // Decompress (handled all formats including uncompressed)
    bool Decompress( const void * data );

    const void *    GetResult() const       { return m_Result; }
    size_t          GetResultSize() const   { return m_ResultSize; }

    inline void *   ReleaseResult()         { void * r = m_Result; m_Result = nullptr; m_ResultSize = 0; return r; }

private:
    enum CompressionType : uint32_t
    {
        eUncompressed   = 0,
        eLZ4            = 1,
        eZstd           = 2,
    };
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
