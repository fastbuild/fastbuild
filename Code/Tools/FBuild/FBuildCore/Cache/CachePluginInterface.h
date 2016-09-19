// ICache - Interface for Cache plugins
//------------------------------------------------------------------------------
#pragma once

#if __WINDOWS__
    #define STDCALL __stdcall
#elif defined(__LINUX__) || defined(__OSX__)
    #define STDCALL
#else
    #error Unknown Platform
#endif

// DLL Export
//------------------------------------------------------------------------------
// Externally compiled DLL should define this appropriately
// e.g.
//#define CACHEPLUGIN_DLL_EXPORT __declspec(dllexport)

#if !defined(__WINDOWS__) //TODO:Windows : Use unmangled name on windows.
extern "C" {
#endif

// CacheInit
//------------------------------------------------------------------------------
// Setup access to cache at start of build.
//
// In:  cachePath- cache path provided from bff config
// Out: bool     - (return) success.  If false is returned, cache will be disabled
typedef bool (*CacheInitFunc)( const char * cachePath );
#ifdef CACHEPLUGIN_DLL_EXPORT
    CACHEPLUGIN_DLL_EXPORT bool STDCALL CacheInit( const char * settings );
#endif

// CacheShutdown
//------------------------------------------------------------------------------
// Perform any required cleanup
typedef void (*CacheShutdownFunc)();
#ifdef CACHEPLUGIN_DLL_EXPORT
    CACHEPLUGIN_DLL_EXPORT void STDCALL CacheShutdown();
#endif

// CachePublish
//------------------------------------------------------------------------------
// Store an item to the cache.  Return true on success.
//
// In:  cacheId  - string name of cache entry
//      data     - data to store to cache
//      dataSize - size in bytes of data to store
// Out: bool     - (return) Indicates if item was stored to cache.
typedef bool (*CachePublishFunc)( const char * cacheId, const void * data, unsigned long long dataSize );
#ifdef CACHEPLUGIN_DLL_EXPORT
    CACHEPLUGIN_DLL_EXPORT bool STDCALL CachePublish( const char * cacheId, const void * data, unsigned long long dataSize );
#endif

// CacheRetrieve
//------------------------------------------------------------------------------
// Retrieve a previously stored item.  Returns true on succes.
//
// In:  cacheId  - string name of cache entry.
// Out: data     - on success, retreived data
//      dataSize - on success, size in bytes of retrieved data
typedef bool (*CacheRetrieveFunc)( const char * cacheId, void * & data, unsigned long long & dataSize );
#ifdef CACHEPLUGIN_DLL_EXPORT
    CACHEPLUGIN_DLL_EXPORT bool STDCALL CacheRetrieve( const char * cacheId, void * & data, unsigned long long & dataSize );
#endif

// CacheFreeMemory
//------------------------------------------------------------------------------
// Free memory provided by CacheRetrieve
//
// In: data     - memory previously allocated by CacheRetrieve
//     dataSize - size in bytes of said memory
typedef void (*CacheFreeMemoryFunc)( void * data, unsigned long long dataSize );
#ifdef CACHEPLUGIN_DLL_EXPORT
    CACHEPLUGIN_DLL_EXPORT void STDCALL CacheFreeMemory( void * data, unsigned long long dataSize );
#endif

#if !defined(__WINDOWS__)//TODO:Windows : Use unmangled name on windows.
} //extern "C"
#endif

//------------------------------------------------------------------------------
