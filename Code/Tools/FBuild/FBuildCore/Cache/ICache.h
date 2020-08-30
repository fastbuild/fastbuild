// Cache - Cache interface
//------------------------------------------------------------------------------
#pragma once

// Includes
//------------------------------------------------------------------------------
#include <Core/Env/Types.h>

// Forward Declarations
//------------------------------------------------------------------------------
class AString;

// Cache
//------------------------------------------------------------------------------
class ICache
{
public:
    inline virtual ~ICache() = default;

    // Interface that cache implementations must provide
    virtual bool Init( const AString & cachePath,
                       const AString & cachePathMountPoint,
                       bool cacheRead,
                       bool cacheWrite,
                       bool cacheVerbose,
                       const AString & pluginDLLConfig ) = 0;
    virtual void Shutdown() = 0;
    virtual bool Publish( const AString & cacheId, const void * data, size_t dataSize ) = 0;
    virtual bool Retrieve( const AString & cacheId, void * & data, size_t & dataSize ) = 0;
    virtual void FreeMemory( void * data, size_t dataSize ) = 0;
    virtual bool OutputInfo( bool showProgress ) = 0;
    virtual bool Trim( bool showProgress, uint32_t sizeMiB ) = 0;

    // Helper functions
    static void GetCacheId( const uint64_t preprocessedSourceKey,
                            const uint32_t commandLineKey,
                            const uint64_t toolChainKey,
                            const uint64_t pchKey,
                            AString & outCacheId );
};

//------------------------------------------------------------------------------
