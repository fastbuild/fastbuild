// Thread
//------------------------------------------------------------------------------

// Includes
//------------------------------------------------------------------------------
#include "Thread.h"
#include "Core/Env/Assert.h"
#include "Core/Mem/Mem.h"
#include "Core/Profile/Profile.h"

// system
#if defined( __WINDOWS__ )
    #include "Core/Env/WindowsHeader.h"
#endif
#if defined( __APPLE__ ) || defined( __LINUX__ )
    #include <errno.h>
    #include <limits.h>
    #include <pthread.h>
    #include <unistd.h>
#endif

#if !defined( __has_feature )
    #define __has_feature( ... ) 0
#endif

// Debug Structure for thread name setting
//------------------------------------------------------------------------------
#if defined( __WINDOWS__ )
    #ifdef DEBUG
        #pragma pack(push,8)
        typedef struct tagTHREADNAME_INFO
        {
            DWORD dwType; // Must be 0x1000.
            LPCSTR szName; // Pointer to name (in user addr space).
            DWORD dwThreadID; // Thread ID (-1=caller thread).
            DWORD dwFlags; // Reserved for future use, must be zero.
        } THREADNAME_INFO;
        #pragma pack(pop)
    #endif
#endif

// ThreadStartInfo
//------------------------------------------------------------------------------
struct ThreadStartInfo
{
public:
    ThreadStartInfo( Thread::ThreadEntryFunction entryFunc, void * userData, const char * threadName )
        : m_UserEntryFunction( entryFunc )
        , m_UserData( userData )
        , m_ThreadName( threadName )
    {}

    Thread::ThreadEntryFunction m_UserEntryFunction;
    void *          m_UserData;
    const char *    m_ThreadName;

    #if defined( __WINDOWS__ )
        static uint32_t WINAPI ThreadStartFunction( void * userData )
    #else
        static void * ThreadStartFunction( void * userData )
    #endif
    {
        // take a copy
        ThreadStartInfo * originalInfo = (ThreadStartInfo *)userData;
        Thread::ThreadEntryFunction realFunction = originalInfo->m_UserEntryFunction;
        void * realUserData = originalInfo->m_UserData;

        // Set thread name for debugging
        #ifdef DEBUG
            if ( originalInfo->m_ThreadName )
            {
                Thread::SetThreadName( originalInfo->m_ThreadName );
            }
        #endif

        // done with ThreadStartInfo
        MemoryBarrier();
        FDELETE( originalInfo );

        // enter into real thread function
        #if defined( __WINDOWS__ )
            return (*realFunction)( realUserData );
        #else
            return (void *)(size_t)( (*realFunction)( realUserData ) );
        #endif
    }
};

// Static Data
//------------------------------------------------------------------------------
/*static*/ Thread::ThreadId Thread::s_MainThreadId( Thread::GetCurrentThreadId() );

// GetCurrentThreadId
//------------------------------------------------------------------------------
/*static*/ Thread::ThreadId Thread::GetCurrentThreadId()
{
    #if defined( __WINDOWS__ )
        return (Thread::ThreadId) ::GetCurrentThreadId();
    #elif defined( __APPLE__ ) || defined( __LINUX__ )
        return pthread_self();
    #else
        #error Unknown platform GetCurrentThreadId
    #endif
}

//  Sleep
//------------------------------------------------------------------------------
/*static*/ void Thread::Sleep( uint32_t ms )
{
    PROFILE_FUNCTION;

    #if defined( WIN32 ) || defined( WIN64 )
        ::Sleep( ms );
    #elif defined( __APPLE__ ) || defined( __LINUX__ )
        usleep( ms * 1000 );
    #else
        #error Unknown platform
    #endif
}

// CreateThread
//------------------------------------------------------------------------------
/*static*/ Thread::ThreadHandle Thread::CreateThread( ThreadEntryFunction entryFunc,
                                                      const char * threadName,
                                                      uint32_t stackSize,
                                                      void * userData )
{
    ThreadStartInfo & info = *FNEW( ThreadStartInfo( entryFunc, userData, threadName ) );
    MemoryBarrier();

    #if defined( __WINDOWS__ )
        HANDLE h = ::CreateThread( nullptr,     // LPSECURITY_ATTRIBUTES lpThreadAttributes
                                   stackSize,       // SIZE_T dwStackSize
                                   (LPTHREAD_START_ROUTINE)ThreadStartInfo::ThreadStartFunction,    // LPTHREAD_START_ROUTINE lpStartAddress
                                   &info,           // LPVOID lpParameter
                                   0,               // DWORD dwCreationFlags
                                   nullptr      // LPDWORD lpThreadId
                                 );
        ASSERT( h != nullptr );
    #elif defined( __LINUX__ ) || defined( __APPLE__ )
        #if __has_feature( address_sanitizer ) || defined( __SANITIZE_ADDRESS__ )
            // AddressSanitizer instruments objects created on the stack by inserting redzones around them.
            // This greatly increases the amount of stack space used by the program.
            // To account for that double the requested stack size for the thread.
            stackSize *= 2;
        #endif
        // Necessary on Aarch64, where it's 131072 in my tests. Sometimes we ask for 65536.
        if ( stackSize < PTHREAD_STACK_MIN )
        {
            stackSize = PTHREAD_STACK_MIN;
        }
        pthread_t h( 0 );
        pthread_attr_t threadAttr;
        VERIFY( pthread_attr_init( &threadAttr ) == 0 );
        VERIFY( pthread_attr_setstacksize( &threadAttr, stackSize ) == 0 );
        VERIFY( pthread_attr_setdetachstate( &threadAttr, PTHREAD_CREATE_JOINABLE ) == 0 );
        VERIFY( pthread_create( &h, &threadAttr, ThreadStartInfo::ThreadStartFunction, &info ) == 0 );
        ASSERT( h != (pthread_t)0 );
    #else
        #error Unknown platform
    #endif

    return (Thread::ThreadHandle)h;
}

// WaitForThread
//------------------------------------------------------------------------------
/*static*/ int32_t Thread::WaitForThread( ThreadHandle handle )
{
    #if defined( __WINDOWS__ )
        bool timedOut = true; // default is true to catch cases when timedOut wasn't set by WaitForThread()
        int32_t ret = WaitForThread( handle, INFINITE, timedOut );

        if ( timedOut )
        {
            // something is wrong - we were waiting an INFINITE time
            ASSERT( false );
            return 0;
        }

        return ret;
    #elif defined( __APPLE__ ) || defined( __LINUX__ )
        void * ret;
        if ( pthread_join( (pthread_t)handle, &ret ) == 0 )
        {
            return (int)( (size_t)ret );
        }
        ASSERT( false ); // usage failure
        return 0;
    #else
        #error Unknown platform
    #endif
}

// WaitForThread
//------------------------------------------------------------------------------
/*static*/ int32_t Thread::WaitForThread( ThreadHandle handle, uint32_t timeoutMS, bool & timedOut )
{
    #if defined( __WINDOWS__ )
        // wait for thread to finish
        DWORD waitResult = WaitForSingleObject( handle, timeoutMS );

        // timed out ?
        if ( waitResult == WAIT_TIMEOUT )
        {
            timedOut = true;
            return 0;
        }
        if ( waitResult != WAIT_OBJECT_0 )
        {
            // something is wrong - invalid handle?
            ASSERT( false );
            timedOut = false;
            return 0;
        }

        // get actual return code
        DWORD ret( 0 );
        if ( ::GetExitCodeThread( handle, (LPDWORD)&ret ) )
        {
            timedOut = false;
            return (int32_t)ret;
        }
        ASSERT( false ); // invalid thread handle
        timedOut = false;
        return -1;
    #elif __has_feature( thread_sanitizer ) || defined( __SANITIZE_THREAD__ )
        // ThreadSanitizer doesn't support pthread_timedjoin_np yet (as of February 2018)
        timedOut = false;
        (void)timeoutMS;
        return WaitForThread( handle );
    #elif defined( __APPLE__ )
        timedOut = false;
        (void)timeoutMS; // TODO:MAC Implement timeout support
        return WaitForThread( handle );
    #elif defined( __LINUX__ )
        // timeout is specified in absolute time
        // - get current time
        struct timespec abstime;
        VERIFY( clock_gettime( CLOCK_REALTIME, &abstime ) == 0 );

        // - add timeout
        abstime.tv_sec += ( timeoutMS / 1000 );
        abstime.tv_nsec += ( timeoutMS % 1000 ) * 1000000;

        // handle nanosecond overflow
        if ( abstime.tv_nsec > 1000000000 )
        {
            abstime.tv_nsec -= 1000000000;
            abstime.tv_sec += 1;
        }

        // join thread with timeout
        void * ret;
        int retVal = pthread_timedjoin_np( (pthread_t)handle, &ret, &abstime );
        if ( ( retVal == EBUSY ) || ( retVal == ETIMEDOUT ) )
        {
            timedOut = true;
            return 0;
        }
        if ( retVal == 0 )
        {
            timedOut = false;
            return (int)( (size_t)ret );
        }

        ASSERT( false ); // a non-timeout error indicates usage failure
        timedOut = false;
        return 0;
    #else
        #error Unknown platform
    #endif
}

// DetachThread
//------------------------------------------------------------------------------
/*static*/ void Thread::DetachThread( ThreadHandle handle )
{
    #if defined( __WINDOWS__ )
        (void)handle; // Nothing to do
    #elif defined( __APPLE__ ) || defined( __LINUX__ )
        VERIFY( pthread_detach( (pthread_t)handle ) == 0 );
    #else
        #error Unknown platform
    #endif
}

// CloseHandle
//------------------------------------------------------------------------------
/*static*/ void Thread::CloseHandle( ThreadHandle h )
{
    #if defined( __WINDOWS__ )
        ::CloseHandle( h );
    #elif defined( __APPLE__ ) || defined( __LINUX__ )
        (void)h; // Nothing to do
    #else
        #error Unknown platform
    #endif
}

// SetThreadName
//------------------------------------------------------------------------------
#ifdef DEBUG
    /*static*/ void Thread::SetThreadName( const char * name )
    {
        ASSERT( name );

        #if defined( __WINDOWS__ )
            #if defined( __clang__ )
                // Clang for windows (3.7.1) crashes trying to compile this
            #else
                const DWORD MS_VC_EXCEPTION=0x406D1388;

                THREADNAME_INFO info;
                info.dwType = 0x1000;
                info.szName = name;
                info.dwThreadID = GetCurrentThreadId();
                info.dwFlags = 0;

                __try
                {
                    RaiseException( MS_VC_EXCEPTION, 0, sizeof(info) / sizeof(ULONG_PTR), (ULONG_PTR*)&info );
                }
                PRAGMA_DISABLE_PUSH_MSVC( 6320 ) // Exception-filter expression is the constant EXCEPTION_EXECUTE_HANDLER
                __except( EXCEPTION_EXECUTE_HANDLER )
                PRAGMA_DISABLE_POP_MSVC
                {
                    (void)0;
                }
            #endif
        #elif defined( __LINUX__ )
            pthread_setname_np( (pthread_t)GetCurrentThreadId(), name );
        #elif defined( __APPLE__ )
            pthread_setname_np( name );
        #else
            #error Unknown platform
        #endif
    }
#endif

//------------------------------------------------------------------------------
