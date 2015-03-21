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
bool CacheInit( const char * settings )
{
	printf( "Init : %s\n", settings );
	return true;
}

// CacheShutdown
//------------------------------------------------------------------------------
void CacheShutdown()
{
	printf( "Shutdown\n" );
}

// CachePublish
//------------------------------------------------------------------------------
bool CachePublish( const char * cacheId, const void * data, unsigned long long dataSize )
{
	printf( "Publish : %s, %p, %llu\n", cacheId, data, dataSize );
	return true;
}

// CacheRetrieve
//------------------------------------------------------------------------------
bool CacheRetrieve( const char * cacheId, void * & data, unsigned long long & dataSize )
{
	(void)data;
	(void)dataSize;
	printf( "Retrieve : %s\n", cacheId );
	return false;
}

// CacheFreeMemory
//------------------------------------------------------------------------------
void CacheFreeMemory( void * data, unsigned long long dataSize )
{
	printf( "FreeMemory : %p, %llu\n", data, dataSize );
}

//------------------------------------------------------------------------------
