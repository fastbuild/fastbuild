// CachePlugin - Wrapper around external cache plugin DLL
//------------------------------------------------------------------------------
#pragma once

// Includes
//------------------------------------------------------------------------------
#include "ICache.h"
#include "CachePluginInterface.h"

// Forward Declarations
//------------------------------------------------------------------------------
class AString;

// Cache
//------------------------------------------------------------------------------
class CachePlugin : public ICache
{
public:
    explicit CachePlugin( const AString & dllName );
    virtual ~CachePlugin() override;

    virtual bool Init( const AString & cachePath, const AString & cachePathMountPoint ) override;
    virtual void Shutdown() override;
    virtual bool Publish( const AString & cacheId, const void * data, size_t dataSize ) override;
    virtual bool Retrieve( const AString & cacheId, void * & data, size_t & dataSize ) override;
    virtual void FreeMemory( void * data, size_t dataSize ) override;
    virtual bool OutputInfo( bool showProgress ) override;
    virtual bool Trim( bool showProgress, uint32_t sizeMiB ) override;
private:
    void * GetFunction( const char * friendlyName, const char * mangledName = nullptr, bool optional = false ) const;

    void *              m_DLL;
    CacheInitFunc       m_InitFunc;
    CacheShutdownFunc   m_ShutdownFunc;
    CachePublishFunc    m_PublishFunc;
    CacheRetrieveFunc   m_RetrieveFunc;
    CacheFreeMemoryFunc m_FreeMemoryFunc;
    CacheOutputInfoFunc m_OutputInfoFunc;
    CacheTrimFunc       m_TrimFunc;
};

//------------------------------------------------------------------------------
