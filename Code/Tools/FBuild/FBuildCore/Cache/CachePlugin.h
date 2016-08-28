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
    virtual ~CachePlugin();

    virtual bool Init( const AString & cachePath );
    virtual void Shutdown();
    virtual bool Publish( const AString & cacheId, const void * data, size_t dataSize );
    virtual bool Retrieve( const AString & cacheId, void * & data, size_t & dataSize );
    virtual void FreeMemory( void * data, size_t dataSize );
private:
    void * GetFunction( const char * friendlyName, const char * mangledName = nullptr ) const;

    void *              m_DLL;
    CacheInitFunc       m_InitFunc;
    CacheShutdownFunc   m_ShutdownFunc;
    CachePublishFunc    m_PublishFunc;
    CacheRetrieveFunc   m_RetrieveFunc;
    CacheFreeMemoryFunc m_FreeMemoryFunc;
};

//------------------------------------------------------------------------------
