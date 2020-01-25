// Thread functionality
//------------------------------------------------------------------------------
#pragma once

// Includes
//------------------------------------------------------------------------------
#include "Core/Env/Types.h"

#if defined( __APPLE__ ) || defined( __LINUX__ )
    #include <pthread.h> // TODO:C Look at moving this out of header
#endif

// Thread
//------------------------------------------------------------------------------
class Thread
{
public:
    #if defined( __WINDOWS__ )
        typedef uint32_t ThreadId;
        typedef void * ThreadHandle;
        typedef uint32_t (*ThreadEntryFunction)( void * param );

        #define INVALID_THREAD_HANDLE ( nullptr )
    #elif defined( __APPLE__ ) || defined( __LINUX__ )
        typedef pthread_t ThreadId;
        typedef void * ThreadHandle;
        typedef uint32_t (*ThreadEntryFunction)( void * param );
        #define INVALID_THREAD_HANDLE ( nullptr )
    #else
        #error Unknown platform
    #endif

    static ThreadId GetCurrentThreadId();
    static ThreadId GetMainThreadId() { return s_MainThreadId; }
    static bool IsThread( ThreadId threadId ) { return ( GetCurrentThreadId() == threadId ); }
    static bool IsMainThread() { return GetCurrentThreadId() == s_MainThreadId; }

    static void Sleep( uint32_t ms );

    static ThreadHandle CreateThread( ThreadEntryFunction entryFunc,
                                      const char * threadName = nullptr,
                                      uint32_t stackSize = ( 64 * KILOBYTE ),
                                      void * userData = nullptr
                                    );
    static int32_t WaitForThread( ThreadHandle handle );
    static int32_t WaitForThread( ThreadHandle handle, uint32_t timeoutMS, bool & timedOut );
    static void DetachThread( ThreadHandle handle );
    static void CloseHandle( ThreadHandle h );

    static void SetThreadName( const char * name );

private:
    static ThreadId s_MainThreadId;
};

//------------------------------------------------------------------------------
