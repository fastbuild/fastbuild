// CachePlugin - Wrapper around external cache plugin DLL
//------------------------------------------------------------------------------

// Includes
//------------------------------------------------------------------------------
#include "Tools/FBuild/FBuildCore/PrecompiledHeader.h"

#include "CachePlugin.h"

// FBuild
#include "Tools/FBuild/FBuildCore/FLog.h"

// Core
#include "Core/Containers/AutoPtr.h"
#include "Core/FileIO/FileIO.h"
#include "Core/FileIO/FileStream.h"
#include "Core/Mem/Mem.h"
#include "Core/Strings/AStackString.h"

// system
#if defined( __WINDOWS__ )
    #include <windows.h>
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
            FLOG_WARN( "Cache plugin '%s' load failed (0x%x).", dllName.Get(), ::GetLastError() );
            return;
        }

        #if defined( WIN64 )
            m_InitFunc      = (CacheInitFunc)       GetFunction( "CacheInit",       "?CacheInit@@YA_NPEBD@Z" );
            m_ShutdownFunc  = (CacheShutdownFunc)   GetFunction( "CacheShutdown",   "?CacheShutdown@@YAXXZ"  );
            m_PublishFunc   = (CachePublishFunc)    GetFunction( "CachePublish",    "?CachePublish@@YA_NPEBDPEBX_K@Z" );
            m_RetrieveFunc  = (CacheRetrieveFunc)   GetFunction( "CacheRetrieve",   "?CacheRetrieve@@YA_NPEBDAEAPEAXAEA_K@Z" );
            m_FreeMemoryFunc= (CacheFreeMemoryFunc) GetFunction( "CacheFreeMemory", "?CacheFreeMemory@@YAXPEAX_K@Z" );
        #else
            m_InitFunc      = (CacheInitFunc)       GetFunction( "CacheInit",       "?CacheInit@@YG_NPBD@Z" );
            m_ShutdownFunc  = (CacheShutdownFunc)   GetFunction( "CacheShutdown",   "?CacheShutdown@@YGXXZ"  );
            m_PublishFunc   = (CachePublishFunc)    GetFunction( "CachePublish",    "?CachePublish@@YG_NPBDPBX_K@Z" );
            m_RetrieveFunc  = (CacheRetrieveFunc)   GetFunction( "CacheRetrieve",   "?CacheRetrieve@@YG_NPBDAAPAXAA_K@Z" );
            m_FreeMemoryFunc= (CacheFreeMemoryFunc) GetFunction( "CacheFreeMemory", "?CacheFreeMemory@@YGXPAX_K@Z" );
        #endif

    #elif defined( __APPLE__ ) || defined( __LINUX__ )
        m_DLL = dlopen(dllName.Get(), RTLD_NOW);
        if ( !m_DLL )
        {
            FLOG_WARN( "Cache plugin '%s' load failed (0x%x).", dllName.Get(), dlerror() );
            return;
        }
        m_InitFunc       = (CacheInitFunc)          GetFunction( "CacheInit" );
        m_ShutdownFunc   = (CacheShutdownFunc)      GetFunction( "CacheShutdown" );
        m_PublishFunc    = (CachePublishFunc)       GetFunction( "CachePublish" );
        m_RetrieveFunc   = (CacheRetrieveFunc)      GetFunction( "CacheRetrieve" );
        m_FreeMemoryFunc = (CacheFreeMemoryFunc)    GetFunction( "CacheFreeMemory" );
    #else
        #error Unknown platform
    #endif
}

// DESTRUCTOR
//------------------------------------------------------------------------------
/*virtual*/ CachePlugin::~CachePlugin() = default;

// GetFunction
//------------------------------------------------------------------------------
void * CachePlugin::GetFunction( const char * friendlyName, const char * mangledName ) const
{
    #if defined( __WINDOWS__ )
        ASSERT( m_DLL );
        void * func = ::GetProcAddress( (HMODULE)m_DLL, mangledName );
        if ( !func )
        {
            FLOG_WARN( "Missing CachePluginDLL function '%s' (Mangled: %s)", friendlyName, mangledName );
        }
        return func;
    #elif defined( __APPLE__ ) || defined( __LINUX__ )
        (void)mangledName;
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
/*virtual*/ bool CachePlugin::Init( const AString & cachePath )
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
    if ( m_FreeMemoryFunc )
    {
        (*m_FreeMemoryFunc)( data, dataSize );
    }
}

//------------------------------------------------------------------------------
