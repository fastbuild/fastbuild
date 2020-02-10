// CachePlugin - Wrapper around external cache plugin DLL
//------------------------------------------------------------------------------

// Includes
//------------------------------------------------------------------------------
#include "CachePlugin.h"

// FBuild
#include "Tools/FBuild/FBuildCore/FLog.h"

// Core
#include "Core/Env/ErrorFormat.h"
#include "Core/Mem/Mem.h"
#include "Core/Tracing/Tracing.h"

// system
#if defined( __WINDOWS__ )
    #include "Core/Env/WindowsHeader.h"
#endif

#if defined(__LINUX__) || defined(__APPLE__)
    #include <dlfcn.h>
#endif
// CONSTRUCTOR
//------------------------------------------------------------------------------
/*explicit*/ CachePlugin::CachePlugin( const AString & dllName ) :
        m_DLL( nullptr ),
        m_InitFunc( nullptr ),
        m_ShutdownFunc( nullptr ),
        m_PublishFunc( nullptr ),
        m_RetrieveFunc( nullptr ),
        m_FreeMemoryFunc( nullptr )
{
    #if defined( __WINDOWS__ )
        m_DLL = ::LoadLibrary( dllName.Get() );
        if ( !m_DLL )
        {
            FLOG_WARN( "Cache plugin load failed. Error: %s Plugin: %s", LAST_ERROR_STR, dllName.Get() );
            return;
        }

        m_InitFunc      = (CacheInitFunc)       GetFunction( "CacheInit",       "?CacheInit@@YA_NPEBD@Z" );
        m_ShutdownFunc  = (CacheShutdownFunc)   GetFunction( "CacheShutdown",   "?CacheShutdown@@YAXXZ"  );
        m_PublishFunc   = (CachePublishFunc)    GetFunction( "CachePublish",    "?CachePublish@@YA_NPEBDPEBX_K@Z" );
        m_RetrieveFunc  = (CacheRetrieveFunc)   GetFunction( "CacheRetrieve",   "?CacheRetrieve@@YA_NPEBDAEAPEAXAEA_K@Z" );
        m_FreeMemoryFunc= (CacheFreeMemoryFunc) GetFunction( "CacheFreeMemory", "?CacheFreeMemory@@YAXPEAX_K@Z" );
        m_OutputInfoFunc= (CacheOutputInfoFunc) GetFunction( "CacheOutputInfo", "?CacheOutputInfo@@YA_N_N@Z", true ); // Optional
        m_TrimFunc      = (CacheTrimFunc)       GetFunction( "CacheTrim",       "?CacheTrim@@YA_N_NI@Z", true ); // Optional
    #elif defined( __APPLE__ ) || defined( __LINUX__ )
        m_DLL = dlopen(dllName.Get(), RTLD_NOW);
        if ( !m_DLL )
        {
            FLOG_WARN( "Cache plugin load failed. Error: %s '%s' Plugin: %s", LAST_ERROR_STR, dlerror(), dllName.Get() );
            return;
        }
        m_InitFunc       = (CacheInitFunc)          GetFunction( "CacheInit" );
        m_ShutdownFunc   = (CacheShutdownFunc)      GetFunction( "CacheShutdown" );
        m_PublishFunc    = (CachePublishFunc)       GetFunction( "CachePublish" );
        m_RetrieveFunc   = (CacheRetrieveFunc)      GetFunction( "CacheRetrieve" );
        m_FreeMemoryFunc = (CacheFreeMemoryFunc)    GetFunction( "CacheFreeMemory" );
        m_OutputInfoFunc = (CacheOutputInfoFunc)    GetFunction( "CacheOutputInfo", nullptr, true ); // Optional
        m_TrimFunc       = (CacheTrimFunc)          GetFunction( "CacheTrim", nullptr, true ); // Optional
    #else
        #error Unknown platform
    #endif
}

// DESTRUCTOR
//------------------------------------------------------------------------------
/*virtual*/ CachePlugin::~CachePlugin() = default;

// GetFunction
//------------------------------------------------------------------------------
void * CachePlugin::GetFunction( const char * friendlyName, const char * mangledName, bool optional ) const
{
    #if defined( __WINDOWS__ )
        ASSERT( m_DLL );
        FARPROC func = ::GetProcAddress( (HMODULE)m_DLL, mangledName );
        if ( !func && !optional )
        {
            FLOG_WARN( "Missing CachePluginDLL function '%s' (Mangled: %s)", friendlyName, mangledName );
        }
        return reinterpret_cast<void*>( func );
    #elif defined( __APPLE__ ) || defined( __LINUX__ )
        (void)mangledName;
        (void)optional;
        return dlsym( m_DLL, friendlyName );
    #else
        #error Unknown platform
    #endif
}

// Shutdown
//------------------------------------------------------------------------------
/*virtual*/ void CachePlugin::Shutdown()
{
    if ( m_ShutdownFunc )
    {
        (*m_ShutdownFunc)();
    }

    #if defined( __WINDOWS__ )
        if ( m_DLL )
        {
            ::FreeLibrary( (HMODULE)m_DLL );
        }
   #elif defined( __APPLE__ ) || defined( __LINUX__ )
        if ( m_DLL )
        {
            dlclose( m_DLL );
        }
    #else
        #error Unknown platform
    #endif
}

// Init
//------------------------------------------------------------------------------
/*virtual*/ bool CachePlugin::Init( const AString & cachePath, const AString & /*cachePathMountPoint*/ )
{
    // ensure all functions were found
    if ( m_InitFunc && m_ShutdownFunc && m_PublishFunc && m_RetrieveFunc && m_FreeMemoryFunc )
    {
        // and try to init
        return (*m_InitFunc)( cachePath.Get() );
    }

    return false;
}

// Publish
//------------------------------------------------------------------------------
/*virtual*/ bool CachePlugin::Publish( const AString & cacheId, const void * data, size_t dataSize )
{
    if ( m_PublishFunc )
    {
        return (*m_PublishFunc)( cacheId.Get(), data, dataSize );
    }
    return false;
}

// Retrieve
//------------------------------------------------------------------------------
/*virtual*/ bool CachePlugin::Retrieve( const AString & cacheId, void * & data, size_t & dataSize )
{
    if ( m_RetrieveFunc )
    {
        unsigned long long size;
        bool ok = (*m_RetrieveFunc)( cacheId.Get(), data, size );
        dataSize = (size_t)size;
        return ok;
    }
    return false;
}

// FreeMemory
//------------------------------------------------------------------------------
/*virtual*/ void CachePlugin::FreeMemory( void * data, size_t dataSize )
{
    ASSERT( m_FreeMemoryFunc ); // should never get here without being valid
    (*m_FreeMemoryFunc)( data, dataSize );
}

// OutputInfo
//------------------------------------------------------------------------------
/*virtual*/ bool CachePlugin::OutputInfo( bool showProgress )
{
    // OutputInfo is optional
    if ( m_OutputInfoFunc )
    {
        return (*m_OutputInfoFunc)( showProgress );
    }

    OUTPUT( "CachePlugin does not support OutputInfo.\n" );
    return false;
}

// Trim
//------------------------------------------------------------------------------
/*virtual*/ bool CachePlugin::Trim( bool showProgress, uint32_t sizeMiB )
{
    // Trim is optional
    if ( m_TrimFunc )
    {
        return (*m_TrimFunc)( showProgress , sizeMiB );
    }

    OUTPUT( "CachePlugin does not support Trim.\n" );
    return false;
}

//------------------------------------------------------------------------------
