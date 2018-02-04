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

// CacheInit (Required)
//------------------------------------------------------------------------------
// Setup access to cache at start of build.
//
// In:  cachePath- cache path provided from bff config
// Out: bool     - (return) success.  If false is returned, cache will be disabled
typedef bool (STDCALL * CacheInitFunc)( const char * cachePath );
#ifdef CACHEPLUGIN_DLL_EXPORT
    CACHEPLUGIN_DLL_EXPORT bool STDCALL CacheInit( const char * cachePath );
#endif

// CacheShutdown (Required)
//------------------------------------------------------------------------------
// Perform any required cleanup
typedef void (STDCALL *CacheShutdownFunc)();
#ifdef CACHEPLUGIN_DLL_EXPORT
    CACHEPLUGIN_DLL_EXPORT void STDCALL CacheShutdown();
#endif

// CachePublish (Required)
//------------------------------------------------------------------------------
// Store an item to the cache.  Return true on success.
//
// In:  cacheId  - string name of cache entry
//      data     - data to store to cache
//      dataSize - size in bytes of data to store
// Out: bool     - (return) Indicates if item was stored to cache.
typedef bool (STDCALL *CachePublishFunc)( const char * cacheId, const void * data, unsigned long long dataSize );
#ifdef CACHEPLUGIN_DLL_EXPORT
    CACHEPLUGIN_DLL_EXPORT bool STDCALL CachePublish( const char * cacheId, const void * data, unsigned long long dataSize );
#endif

// CacheRetrieve (Required)
//------------------------------------------------------------------------------
// Retrieve a previously stored item.  Returns true on succes.
//
// In:  cacheId  - string name of cache entry.
// Out: data     - on success, retreived data
//      dataSize - on success, size in bytes of retrieved data
typedef bool (STDCALL *CacheRetrieveFunc)( const char * cacheId, void * & data, unsigned long long & dataSize );
#ifdef CACHEPLUGIN_DLL_EXPORT
    CACHEPLUGIN_DLL_EXPORT bool STDCALL CacheRetrieve( const char * cacheId, void * & data, unsigned long long & dataSize );
#endif

// CacheFreeMemory (Required)
//------------------------------------------------------------------------------
// Free memory provided by CacheRetrieve
//
// In: data     - memory previously allocated by CacheRetrieve
//     dataSize - size in bytes of said memory
typedef void (STDCALL *CacheFreeMemoryFunc)( void * data, unsigned long long dataSize );
#ifdef CACHEPLUGIN_DLL_EXPORT
    CACHEPLUGIN_DLL_EXPORT void STDCALL CacheFreeMemory( void * data, unsigned long long dataSize );
#endif

// CacheOutputInfo (Optional)
//------------------------------------------------------------------------------
// Print information about the contents of the cache
//
// In: showProgress - emit progress messages for long operations
typedef bool (STDCALL *CacheOutputInfoFunc)( bool showProgress );
#ifdef CACHEPLUGIN_DLL_EXPORT
    CACHEPLUGIN_DLL_EXPORT bool STDCALL CacheOutputInfo( bool showProgress );
#endif

// CacheTrim (Optional)
//------------------------------------------------------------------------------
// Trim data in the cache to the desired size
//
// In: showProgress - emit progress messages for long operations
//     sizeMiB      - desired size in MiB
typedef bool (STDCALL *CacheTrimFunc)( bool showProgress, unsigned int sizeMiB );
#ifdef CACHEPLUGIN_DLL_EXPORT
    CACHEPLUGIN_DLL_EXPORT bool STDCALL CacheTrim( bool showProgress, unsigned int sizeMiB );
#endif

#if !defined(__WINDOWS__)//TODO:Windows : Use unmangled name on windows.
} //extern "C"
#endif

//------------------------------------------------------------------------------
