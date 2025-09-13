// CachePlugin - Wrapper around external cache plugin DLL
//------------------------------------------------------------------------------
#pragma once

// Includes
//------------------------------------------------------------------------------
// FBuildCore
#include "Tools/FBuild/FBuildCore/Cache/CachePluginInterface.h"
#include "Tools/FBuild/FBuildCore/Cache/ICache.h"

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

    virtual bool Init( const AString & cachePath,
                       const AString & cachePathMountPoint,
                       bool cacheRead,
                       bool cacheWrite,
                       bool cacheVerbose,
                       const AString & pluginDLLConfig ) override;
    virtual void Shutdown() override;
    virtual bool Publish( const AString & cacheId, const void * data, size_t dataSize ) override;
    virtual bool Retrieve( const AString & cacheId, void *& data, size_t & dataSize ) override;
    virtual void FreeMemory( void * data, size_t dataSize ) override;
    virtual bool OutputInfo( bool showProgress ) override;
    virtual bool Trim( bool showProgress, uint32_t sizeMiB ) override;

private:
    void * GetFunction( const char * friendlyName, const char * mangledName = nullptr, bool optional = false );

    static void CacheOutputWrapper( const char * message );

    void * m_DLL = nullptr;
    bool m_Valid = false;
    CacheInitFunc m_InitFunc = nullptr;
    CacheInitExFunc m_InitExFunc = nullptr;
    CacheShutdownFunc m_ShutdownFunc = nullptr;
    CachePublishFunc m_PublishFunc = nullptr;
    CacheRetrieveFunc m_RetrieveFunc = nullptr;
    CacheFreeMemoryFunc m_FreeMemoryFunc = nullptr;
    CacheOutputInfoFunc m_OutputInfoFunc = nullptr;
    CacheTrimFunc m_TrimFunc = nullptr;
};

//------------------------------------------------------------------------------
