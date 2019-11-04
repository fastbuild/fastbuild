// MultiBuffer
//------------------------------------------------------------------------------
#pragma once

// Includes
//------------------------------------------------------------------------------
// Core
#include "Core/Containers/Array.h"
#include "Core/Env/Types.h"

// Forward Declarations
//------------------------------------------------------------------------------
class AString;
class ConstMemoryStream;
class MemoryStream;

// MultiBuffer
//------------------------------------------------------------------------------
class MultiBuffer
{
public:
    MultiBuffer();
    explicit MultiBuffer( const void * data, size_t dataSize );
    ~MultiBuffer();

    bool CreateFromFiles( const Array< AString > & fileNames, size_t * outProblemFileIndex = nullptr );
    bool ExtractFile( size_t index, const AString& fileName ) const;

    const void *    GetData() const;
    uint64_t        GetDataSize() const;

    void *          Release( size_t & outSize );

private:
    enum : uint32_t { MAX_FILES = 4 };

    ConstMemoryStream * m_ReadStream;
    MemoryStream *      m_WriteStream;
};

//------------------------------------------------------------------------------
