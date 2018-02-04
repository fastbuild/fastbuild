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
    virtual ~Cache();

    virtual bool Init( const AString & cachePath );
    virtual void Shutdown();
    virtual bool Publish( const AString & cacheId, const void * data, size_t dataSize );
    virtual bool Retrieve( const AString & cacheId, void * & data, size_t & dataSize );
    virtual void FreeMemory( void * data, size_t dataSize );
    virtual bool OutputInfo( bool showProgress );
    virtual bool Trim( bool showProgress, uint32_t sizeMiB );
private:
    void GetCacheFiles( bool showProgress, Array< FileIO::FileInfo > & outInfo, uint64_t & outTotalSize ) const;
    void GetCacheFileName( const AString & cacheId, AString & path ) const;

    AString m_CachePath;
};

//------------------------------------------------------------------------------
