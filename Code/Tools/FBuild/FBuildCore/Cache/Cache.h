// Cache - Default cache implementation
//------------------------------------------------------------------------------
#pragma once

// Includes
//------------------------------------------------------------------------------
#include "ICache.h"
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
private:
    void GetCacheFileName( const AString & cacheId, AString & path ) const;

    AString m_CachePath;
};

//------------------------------------------------------------------------------
