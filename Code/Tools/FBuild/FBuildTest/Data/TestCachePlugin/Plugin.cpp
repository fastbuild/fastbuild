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

#include "../../../FBuildCore/Cache/CachePluginInterface.h"

// CacheInit
//------------------------------------------------------------------------------
#if !defined(__WINDOWS__) // TODO:Windows : Use unmangled names on windows
extern "C" {
#endif

bool STDCALL CacheInit( const char * settings )
{
    printf( "Init : %s\n", settings );
    return true;
}

// CacheShutdown
//------------------------------------------------------------------------------
void STDCALL CacheShutdown()
{
    printf( "Shutdown\n" );
}

// CachePublish
//------------------------------------------------------------------------------
bool STDCALL CachePublish( const char * cacheId, const void * data, unsigned long long dataSize )
{
    printf( "Publish : %s, %p, %llu\n", cacheId, data, dataSize );
    return true;
}

// CacheRetrieve
//------------------------------------------------------------------------------
bool STDCALL CacheRetrieve( const char * cacheId, void * & data, unsigned long long & dataSize )
{
    (void)data;
    (void)dataSize;
    printf( "Retrieve : %s\n", cacheId );
    return false;
}

// CacheFreeMemory
//------------------------------------------------------------------------------
void STDCALL CacheFreeMemory( void * data, unsigned long long dataSize )
{
    printf( "FreeMemory : %p, %llu\n", data, dataSize );
}

//------------------------------------------------------------------------------

#if !defined(__WINDOWS__)//TODO:Windows : Use unmangled name on windows.
}// extern "C"
#endif
