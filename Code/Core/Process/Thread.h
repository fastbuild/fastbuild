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
    #define INVALID_THREAD_ID ( 0 )
    enum : uint32_t { kDefaultStackSize = ( 64 * 1024 ) };

    Thread();
    ~Thread();

    // Lifetime management
    void        Start( ThreadEntryFunction func,
                       const char * threadName = nullptr,
                       void * userData = nullptr,
                       uint32_t stackSizeBytes = kDefaultStackSize );
    uint32_t    Join();
    bool        IsRunning() const;

    // Thread Identification
    static ThreadId GetCurrentThreadId();
    static ThreadId GetMainThreadId() { return s_MainThreadId; }
    static bool IsThread( ThreadId threadId ) { return ( GetCurrentThreadId() == threadId ); }
    static bool IsMainThread() { return GetCurrentThreadId() == s_MainThreadId; }

    // Sleeps
    static void Sleep( uint32_t ms );

    // Legacy Functions - TODO:B Remove these unsafe functions
    void        Detach(); // TODO:B Remove this unsafe function
    uint32_t    JoinWithTimeout( uint32_t timeoutMS, bool & outTimedOut ); // TODO:B Remove this unsafe API

    // Debugging
    static void SetThreadName( const char * name );

private:
    ThreadHandle    m_Handle = INVALID_THREAD_HANDLE;

    static ThreadId s_MainThreadId;
};

//------------------------------------------------------------------------------
