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

extern "C" {

// CacheInit (Optional)
//------------------------------------------------------------------------------
// Setup access to cache at start of build.
//
// In:  cachePath- cache path provided from bff config
// Out: bool     - (return) success.  If false is returned, cache will be disabled
//typedef bool (STDCALL * CacheInitFunc)( const char * cachePath );
using CacheInitFunc = bool (STDCALL *)( const char * cachePath );

// CacheOutputFunc
//------------------------------------------------------------------------------
// A function the cache plugin can call to output the the stdout. Ensures that
// output it correctly interleaved with other FASTBuild output. The plugin should
// not implement this function, but rather should call it if/when needed. The
// function pointer is provided via CacheInitEx below.
using CacheOutputFunc = void (STDCALL*)( const char * message );

// CacheInitEx (Optional)
//------------------------------------------------------------------------------
// Setup access to cache at start of build.
//
// In:  cachePath   - cache path provided from bff config
//      cacheRead   - is -cacheread enabled for this build
//      cacheWrite  - is -cachewrite enabled for this build
//      cacheVerbose- is -cacheverbose enabled for this build
//      userConfig  - user provided confuguration string (from .CachePluginDLLConfig)
// Out: bool        - (return) success.  If false is returned, cache will be disabled
using CacheInitExFunc = bool (STDCALL *)( const char * cachePath,
                                          bool cacheRead,
                                          bool cacheWrite,
                                          bool cacheVerbose,
                                          const char * userConfig,
                                          CacheOutputFunc outputFunc );

// CacheShutdown (Required)
//------------------------------------------------------------------------------
// Perform any required cleanup
using CacheShutdownFunc = void (STDCALL *)();

// CachePublish (Required)
//------------------------------------------------------------------------------
// Store an item to the cache.  Return true on success.
//
// In:  cacheId  - string name of cache entry
//      data     - data to store to cache
//      dataSize - size in bytes of data to store
// Out: bool     - (return) Indicates if item was stored to cache.
using CachePublishFunc = bool (STDCALL *)( const char * cacheId, const void * data, unsigned long long dataSize );

// CacheRetrieve (Required)
//------------------------------------------------------------------------------
// Retrieve a previously stored item.  Returns true on succes.
//
// In:  cacheId  - string name of cache entry.
// Out: data     - on success, retreived data
//      dataSize - on success, size in bytes of retrieved data
using CacheRetrieveFunc = bool (STDCALL *)( const char * cacheId, void * & data, unsigned long long & dataSize );

// CacheFreeMemory (Required)
//------------------------------------------------------------------------------
// Free memory provided by CacheRetrieve
//
// In: data     - memory previously allocated by CacheRetrieve
//     dataSize - size in bytes of said memory
using CacheFreeMemoryFunc = void (STDCALL *)( void * data, unsigned long long dataSize );

// CacheOutputInfo (Optional)
//------------------------------------------------------------------------------
// Print information about the contents of the cache
//
// In: showProgress - emit progress messages for long operations
using CacheOutputInfoFunc = bool (STDCALL *)( bool showProgress );

// CacheTrim (Optional)
//------------------------------------------------------------------------------
// Trim data in the cache to the desired size
//
// In: showProgress - emit progress messages for long operations
//     sizeMiB      - desired size in MiB
using CacheTrimFunc = bool (STDCALL *)( bool showProgress, unsigned int sizeMiB );

} //extern "C"

//------------------------------------------------------------------------------
