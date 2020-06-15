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
void * gMemory = nullptr;
unsigned long long gMemorySize = 0;

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
    ADD_WINDOWS_DLL_EXPORT_FOR_THIS_FUNCTION

    // Store output function
    gOutputFunction = outputFunc;

    (*gOutputFunction)( "CacheInitEx Called" );
    return true;
}

// CacheShutdown
//------------------------------------------------------------------------------
void STDCALL CacheShutdown()
{
    ADD_WINDOWS_DLL_EXPORT_FOR_THIS_FUNCTION

    (*gOutputFunction)( "CacheShutdown Called" );
}

// CachePublish
//------------------------------------------------------------------------------
bool STDCALL CachePublish( const char * /*cacheId*/, const void * data, unsigned long long dataSize )
{
    ADD_WINDOWS_DLL_EXPORT_FOR_THIS_FUNCTION

    (*gOutputFunction)( "CachePublish Called" );

    // Take a copy of the cache data
    gMemory = malloc( dataSize );
    memcpy( gMemory, data, dataSize );
    gMemorySize = dataSize;

    return true;
}

// CacheRetrieve
//------------------------------------------------------------------------------
bool STDCALL CacheRetrieve( const char * /*cacheId*/, void * & /*data*/, unsigned long long & /*dataSize*/ )
{
    ADD_WINDOWS_DLL_EXPORT_FOR_THIS_FUNCTION

    (*gOutputFunction)( "CacheRetrieve Called" );

    return false;
}

// CacheFreeMemory
//------------------------------------------------------------------------------
void STDCALL CacheFreeMemory( void * /*data*/, unsigned long long /*dataSize*/ )
{
    ADD_WINDOWS_DLL_EXPORT_FOR_THIS_FUNCTION

    (*gOutputFunction)( "CacheFreeMemory Called" );
}

// CacheOutputInfo
//------------------------------------------------------------------------------
bool STDCALL CacheOutputInfo( bool /*showProgress*/ )
{
    ADD_WINDOWS_DLL_EXPORT_FOR_THIS_FUNCTION

    (*gOutputFunction)( "CacheOutputInfo Called" );
    return true; // Success
}

// CacheTrim
//------------------------------------------------------------------------------
bool STDCALL CacheTrim( bool /*showProgress*/, unsigned int /*sizeMiB*/ )
{
    ADD_WINDOWS_DLL_EXPORT_FOR_THIS_FUNCTION

    (*gOutputFunction)( "CacheTrim Called" );
    return true; // Success
}

//------------------------------------------------------------------------------

}// extern "C"
