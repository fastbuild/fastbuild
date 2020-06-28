// Plugin - Test external cache plugin
//------------------------------------------------------------------------------

// Includes
//------------------------------------------------------------------------------
#include <stdio.h>

// The FASTBuild DLL Interface
//------------------------------------------------------------------------------
#if defined(__WINDOWS__)
#define CACHEPLUGIN_DLL_EXPORT __declspec(dllexport)
#elif defined(__LINUX__) || defined(__APPLE__)
#define CACHEPLUGIN_DLL_EXPORT
#endif

#include "Tools/FBuild/FBuildCore/Cache/CachePluginInterface.h"

// System
#include <memory.h>
#include <stdlib.h>

// Globals
//------------------------------------------------------------------------------
CacheOutputFunc gOutputFunction = nullptr;

// CacheInit
//------------------------------------------------------------------------------
extern "C" {

bool STDCALL CacheInitEx( const char * /*cachePath*/,
                          bool /*cacheRead*/,
                          bool /*cacheWrite*/,
                          bool /*cacheVerbose*/,
                          const char * /*userConfig*/,
                          CacheOutputFunc outputFunc )
{
    // DLL Export for Windows
    #if defined( __WINDOWS__ )
        #pragma comment(linker, "/EXPORT:" __FUNCTION__"=" __FUNCDNAME__)
    #endif

    // Store output function
    gOutputFunction = outputFunc;

    (*gOutputFunction)( "CacheInitEx Called" );
    return true;
}

// CacheShutdown
//------------------------------------------------------------------------------
void STDCALL CacheShutdown()
{
    // DLL Export for Windows
    #if defined( __WINDOWS__ )
        #pragma comment(linker, "/EXPORT:" __FUNCTION__"=" __FUNCDNAME__)
    #endif

    (*gOutputFunction)( "CacheShutdown Called" );
}

// CachePublish
//------------------------------------------------------------------------------
bool STDCALL CachePublish( const char * /*cacheId*/, const void * /*data*/, unsigned long long /*dataSize*/ )
{
    // DLL Export for Windows
    #if defined( __WINDOWS__ )
        #pragma comment(linker, "/EXPORT:" __FUNCTION__"=" __FUNCDNAME__)
    #endif

    (*gOutputFunction)( "CachePublish Called" );

    return true;
}

// CacheRetrieve
//------------------------------------------------------------------------------
bool STDCALL CacheRetrieve( const char * /*cacheId*/, void * & /*data*/, unsigned long long & /*dataSize*/ )
{
    // DLL Export for Windows
    #if defined( __WINDOWS__ )
        #pragma comment(linker, "/EXPORT:" __FUNCTION__"=" __FUNCDNAME__)
    #endif

    (*gOutputFunction)( "CacheRetrieve Called" );

    return false;
}

// CacheFreeMemory
//------------------------------------------------------------------------------
void STDCALL CacheFreeMemory( void * /*data*/, unsigned long long /*dataSize*/ )
{
    // DLL Export for Windows
    #if defined( __WINDOWS__ )
        #pragma comment(linker, "/EXPORT:" __FUNCTION__"=" __FUNCDNAME__)
    #endif

    (*gOutputFunction)( "CacheFreeMemory Called" );
}

// CacheOutputInfo
//------------------------------------------------------------------------------
bool STDCALL CacheOutputInfo( bool /*showProgress*/ )
{
    // DLL Export for Windows
    #if defined( __WINDOWS__ )
        #pragma comment(linker, "/EXPORT:" __FUNCTION__"=" __FUNCDNAME__)
    #endif

    (*gOutputFunction)( "CacheOutputInfo Called" );
    return true; // Success
}

// CacheTrim
//------------------------------------------------------------------------------
bool STDCALL CacheTrim( bool /*showProgress*/, unsigned int /*sizeMiB*/ )
{
    // DLL Export for Windows
    #if defined( __WINDOWS__ )
        #pragma comment(linker, "/EXPORT:" __FUNCTION__"=" __FUNCDNAME__)
    #endif

    (*gOutputFunction)( "CacheTrim Called" );
    return true; // Success
}

//------------------------------------------------------------------------------

}// extern "C"
