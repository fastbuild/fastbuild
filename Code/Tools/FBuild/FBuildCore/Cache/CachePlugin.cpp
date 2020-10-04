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
#if defined( __LINUX__ ) || defined( __APPLE__ )
    #include <dlfcn.h>
#endif

// CONSTRUCTOR
//------------------------------------------------------------------------------
/*explicit*/ CachePlugin::CachePlugin( const AString & dllName )
    : m_DLL( nullptr )
    , m_InitFunc( nullptr )
    , m_ShutdownFunc( nullptr )
    , m_PublishFunc( nullptr )
    , m_RetrieveFunc( nullptr )
    , m_FreeMemoryFunc( nullptr )
{
    #if defined( __WINDOWS__ )
        m_DLL = ::LoadLibrary( dllName.Get() );
        if ( !m_DLL )
        {
            FLOG_WARN( "Cache plugin load failed. Error: %s Plugin: %s", LAST_ERROR_STR, dllName.Get() );
            return;
        }
    #else
        m_DLL = dlopen(dllName.Get(), RTLD_NOW);
        if ( !m_DLL )
        {
            FLOG_WARN( "Cache plugin load failed. Error: %s '%s' Plugin: %s", LAST_ERROR_STR, dlerror(), dllName.Get() );
            return;
        }
    #endif

    // Failure to find a required function will mark us as invalid
    m_Valid = true;

    m_InitFunc      = (CacheInitFunc)       GetFunction( "CacheInit",       "?CacheInit@@YA_NPEBD@Z", true ); // Optional
    m_InitExFunc    = (CacheInitExFunc)     GetFunction( "CacheInitEx",     nullptr, true ); // Optional
    m_ShutdownFunc  = (CacheShutdownFunc)   GetFunction( "CacheShutdown",   "?CacheShutdown@@YAXXZ", true ); // Optional
    m_PublishFunc   = (CachePublishFunc)    GetFunction( "CachePublish",    "?CachePublish@@YA_NPEBDPEBX_K@Z" );
    m_RetrieveFunc  = (CacheRetrieveFunc)   GetFunction( "CacheRetrieve",   "?CacheRetrieve@@YA_NPEBDAEAPEAXAEA_K@Z" );
    m_FreeMemoryFunc= (CacheFreeMemoryFunc) GetFunction( "CacheFreeMemory", "?CacheFreeMemory@@YAXPEAX_K@Z" );
    m_OutputInfoFunc= (CacheOutputInfoFunc) GetFunction( "CacheOutputInfo", "?CacheOutputInfo@@YA_N_N@Z", true ); // Optional
    m_TrimFunc      = (CacheTrimFunc)       GetFunction( "CacheTrim",       "?CacheTrim@@YA_N_NI@Z", true ); // Optional
}

// DESTRUCTOR
//------------------------------------------------------------------------------
/*virtual*/ CachePlugin::~CachePlugin() = default;

// GetFunction
//------------------------------------------------------------------------------
void * CachePlugin::GetFunction( const char * name, const char * mangledName, bool optional )
{
    #if defined( __WINDOWS__ )
        ASSERT( m_DLL );

        // Try the unmangled name first
        void * func = reinterpret_cast<void*>( ::GetProcAddress( (HMODULE)m_DLL, name ) );

        // If that fails, check for the mangled name for backwards compat with existing plugins
        if ( !func && mangledName )
        {
            func = reinterpret_cast<void*>( ::GetProcAddress( (HMODULE)m_DLL, mangledName ) );
        }
    #else
        (void)mangledName; // Only used on Windows for backwards copatibility
        void * func = dlsym( m_DLL, name );
    #endif

    if ( !func && !optional )
    {
        FLOG_ERROR( "Missing required CachePluginDLL function '%s'", name );
        m_Valid = false;
    }

    return func;
}

// CacheOutputWrapper
//------------------------------------------------------------------------------
/*static*/ void CachePlugin::CacheOutputWrapper( const char * message )
{
    // Ensure message includes newline ending
    AStackString<> buffer( message );
    if ( buffer.EndsWith( '\n' ) == false )
    {
        buffer += '\n';
    }

    // Forward to normal output system
    FLOG_OUTPUT( "%s", buffer.Get() );
}

// Shutdown
//------------------------------------------------------------------------------
/*virtual*/ void CachePlugin::Shutdown()
{
    if ( m_Valid == false )
    {
        return;
    }

    if ( m_ShutdownFunc )
    {
        (*m_ShutdownFunc)();
    }

    if ( m_DLL )
    {
        #if defined( __WINDOWS__ )
            ::FreeLibrary( (HMODULE)m_DLL );
       #else
            dlclose( m_DLL );
        #endif
    }
}

// Init
//------------------------------------------------------------------------------
/*virtual*/ bool CachePlugin::Init( const AString & cachePath,
                                    const AString & /*cachePathMountPoint*/,
                                    bool cacheRead,
                                    bool cacheWrite,
                                    bool cacheVerbose,
                                    const AString & pluginDLLConfig )
{
    if ( m_Valid == false )
    {
        return false;
    }

    // Original Init
    if ( m_InitFunc )
    {
        return (*m_InitFunc)( cachePath.Get() );
    }

    // Extended Init
    if ( m_InitExFunc )
    {
        return (*m_InitExFunc)( cachePath.Get(), cacheRead, cacheWrite, cacheVerbose, pluginDLLConfig.Get(), &CacheOutputWrapper );
    }

    return false;
}

// Publish
//------------------------------------------------------------------------------
/*virtual*/ bool CachePlugin::Publish( const AString & cacheId, const void * data, size_t dataSize )
{
    if ( m_Valid == false )
    {
        return false;
    }

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
    if ( m_Valid == false )
    {
        return false;
    }

    if ( m_RetrieveFunc )
    {
        unsigned long long size;
        const bool ok = (*m_RetrieveFunc)( cacheId.Get(), data, size );
        dataSize = (size_t)size;
        return ok;
    }
    return false;
}

// FreeMemory
//------------------------------------------------------------------------------
/*virtual*/ void CachePlugin::FreeMemory( void * data, size_t dataSize )
{
    if ( m_Valid == false )
    {
        return;
    }

    ASSERT( m_FreeMemoryFunc ); // should never get here without being valid
    (*m_FreeMemoryFunc)( data, dataSize );
}

// OutputInfo
//------------------------------------------------------------------------------
/*virtual*/ bool CachePlugin::OutputInfo( bool showProgress )
{
    if ( m_Valid == false )
    {
        return false;
    }

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
    if ( m_Valid == false )
    {
        return false;
    }

    // Trim is optional
    if ( m_TrimFunc )
    {
        return (*m_TrimFunc)( showProgress , sizeMiB );
    }

    OUTPUT( "CachePlugin does not support Trim.\n" );
    return false;
}

//------------------------------------------------------------------------------
