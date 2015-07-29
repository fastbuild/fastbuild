// Plugin - Test external cache plugin
//------------------------------------------------------------------------------

// Includes
//------------------------------------------------------------------------------
#include <stdio.h>

// The FASTBuild DLL Interface
//------------------------------------------------------------------------------
#define CACHEPLUGIN_DLL_EXPORT __declspec(dllexport)
#include "../../../FBuildCore/Cache/CachePluginInterface.h"

// CacheInit
//------------------------------------------------------------------------------
bool __stdcall CacheInit( const char * settings )
{
	printf( "Init : %s\n", settings );
	return true;
}

// CacheShutdown
//------------------------------------------------------------------------------
void __stdcall CacheShutdown()
{
	printf( "Shutdown\n" );
}

// CachePublish
//------------------------------------------------------------------------------
bool __stdcall CachePublish( const char * cacheId, const void * data, unsigned long long dataSize )
{
	printf( "Publish : %s, %p, %llu\n", cacheId, data, dataSize );
	return true;
}

// CacheRetrieve
//------------------------------------------------------------------------------
bool __stdcall CacheRetrieve( const char * cacheId, void * & data, unsigned long long & dataSize )
{
	(void)data;
	(void)dataSize;
	printf( "Retrieve : %s\n", cacheId );
	return false;
}

// CacheFreeMemory
//------------------------------------------------------------------------------
void __stdcall CacheFreeMemory( void * data, unsigned long long dataSize )
{
	printf( "FreeMemory : %p, %llu\n", data, dataSize );
}

//------------------------------------------------------------------------------
