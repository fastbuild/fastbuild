// Cache - Default cache implementation
//------------------------------------------------------------------------------
#pragma once

// Includes
//------------------------------------------------------------------------------
#include "ICache.h"
#include "Core/FileIO/FileIO.h"
#include "Core/Strings/AString.h"

// Cache
//------------------------------------------------------------------------------
class Cache : public ICache
{
public:
    explicit Cache();
    virtual ~Cache() override;

    virtual bool Init( const AString & cachePath,
                       const AString & cachePathMountPoint,
                       bool cacheRead,
                       bool cacheWrite,
                       bool cacheVerbose,
                       const AString & pluginDLLConfig ) override;
    virtual void Shutdown() override;
    virtual bool Publish( const AString & cacheId, const void * data, size_t dataSize ) override;
    virtual bool Retrieve( const AString & cacheId, void * & data, size_t & dataSize ) override;
    virtual void FreeMemory( void * data, size_t dataSize ) override;
    virtual bool OutputInfo( bool showProgress ) override;
    virtual bool Trim( bool showProgress, uint32_t sizeMiB ) override;
private:
    void GetCacheFiles( bool showProgress, Array< FileIO::FileInfo > & outInfo, uint64_t & outTotalSize ) const;
    void GetFullPathForCacheEntry( const AString & cacheId, AString & outFullPath ) const;

    AString m_CachePath;
};

//------------------------------------------------------------------------------
