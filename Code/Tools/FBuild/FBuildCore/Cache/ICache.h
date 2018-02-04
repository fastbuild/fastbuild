// Cache - Cache interface
//------------------------------------------------------------------------------
#pragma once

// Forward Declarations
//------------------------------------------------------------------------------
class AString;

// Cache
//------------------------------------------------------------------------------
class ICache
{
public:
    inline virtual ~ICache() = default;

    virtual bool Init( const AString & cachePath ) = 0;
    virtual void Shutdown() = 0;
    virtual bool Publish( const AString & cacheId, const void * data, size_t dataSize ) = 0;
    virtual bool Retrieve( const AString & cacheId, void * & data, size_t & dataSize ) = 0;
    virtual void FreeMemory( void * data, size_t dataSize ) = 0;
    virtual bool OutputInfo( bool showProgress ) = 0;
    virtual bool Trim( bool showProgress, uint32_t sizeMiB ) = 0;
};

//------------------------------------------------------------------------------
