// Thread
//------------------------------------------------------------------------------

// Includes
//------------------------------------------------------------------------------
#include "Thread.h"
#include "Core/Env/Assert.h"
#include "Core/Mem/Mem.h"
#include "Core/Process/Atomic.h"
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
    {
        if ( threadName )
        {
            m_ThreadName = threadName;
        }
    }

    Thread::ThreadEntryFunction m_UserEntryFunction;
    void *          m_UserData;
    AString         m_ThreadName;

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

        if ( originalInfo->m_ThreadName.IsEmpty() == false )
        {
            // Set thread name for debugging
            #ifdef DEBUG
                Thread::SetThreadName( originalInfo->m_ThreadName.Get() );
            #endif

            // Set thread name for profiling
            PROFILE_SET_THREAD_NAME( originalInfo->m_ThreadName.Get() );
        }

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

    void operator =(const ThreadStartInfo& other) = delete;
};

// Static Data
//------------------------------------------------------------------------------
/*static*/ Thread::ThreadId Thread::s_MainThreadId( Thread::GetCurrentThreadId() );

// CONSTRUCTOR
//------------------------------------------------------------------------------
Thread::Thread() = default;

// DESTRUCTOR
//------------------------------------------------------------------------------
Thread::~Thread()
{
    // Thread must be joined before being destroyed
    ASSERT( m_Handle == INVALID_THREAD_HANDLE );
}

// Start
//------------------------------------------------------------------------------
void Thread::Start( ThreadEntryFunction func,
                    const char * threadName,
                    void * userData,
                    uint32_t stackSizeBytes )
{
    // Can only start if not already started
    ASSERT( !IsRunning() );

    // Create structure to pass to thread
    ThreadStartInfo & info = *FNEW( ThreadStartInfo( func, userData, threadName ) );
    MemoryBarrier();

    // Create thread
    #if defined( __WINDOWS__ )
        HANDLE h = ::CreateThread( nullptr,         // LPSECURITY_ATTRIBUTES lpThreadAttributes
                                   stackSizeBytes,  // SIZE_T dwStackSize
                                   (LPTHREAD_START_ROUTINE)ThreadStartInfo::ThreadStartFunction,    // LPTHREAD_START_ROUTINE lpStartAddress
                                   &info,           // LPVOID lpParameter
                                   0,               // DWORD dwCreationFlags
                                   nullptr          // LPDWORD lpThreadId
                                 );
        ASSERT( h != nullptr );
    #elif defined( __LINUX__ ) || defined( __APPLE__ )
        #if __has_feature( address_sanitizer ) || defined( __SANITIZE_ADDRESS__ )
            // AddressSanitizer instruments objects created on the stack by inserting redzones around them.
            // This greatly increases the amount of stack space used by the program.
            // To account for that double the requested stack size for the thread.
            stackSizeBytes *= 2;
        #endif
        // Necessary on Aarch64, where it's 131072 in my tests. Sometimes we ask for 65536.
        if ( stackSizeBytes < PTHREAD_STACK_MIN )
        {
            stackSizeBytes = PTHREAD_STACK_MIN;
        }
        pthread_t h( 0 );
        pthread_attr_t threadAttr;
        VERIFY( pthread_attr_init( &threadAttr ) == 0 );
        VERIFY( pthread_attr_setstacksize( &threadAttr, stackSizeBytes ) == 0 );
        VERIFY( pthread_attr_setdetachstate( &threadAttr, PTHREAD_CREATE_JOINABLE ) == 0 );
        VERIFY( pthread_create( &h, &threadAttr, ThreadStartInfo::ThreadStartFunction, &info ) == 0 );
        ASSERT( h != (pthread_t)0 );
    #else
        #error Unknown platform
    #endif
    m_Handle = (Thread::ThreadHandle)h;

    ASSERT( IsRunning() );
}

// Join
//------------------------------------------------------------------------------
uint32_t Thread::Join()
{
    // Must only join if running and not already joined
    ASSERT( IsRunning() );

    // Wait for thread and obtain return result
    #if defined( __WINDOWS__ )
        // Wait for thread to finish
        VERIFY( WaitForSingleObject( m_Handle, INFINITE ) == WAIT_OBJECT_0 );

        // Ge teturn code
        DWORD ret = 0;
        VERIFY( ::GetExitCodeThread( m_Handle, (LPDWORD)&ret ) );
        VERIFY( ::CloseHandle( m_Handle ) );
        m_Handle = INVALID_THREAD_HANDLE;
        return ret;
    #elif defined( __APPLE__ ) || defined( __LINUX__ )
        void * ret;
        if ( pthread_join( (pthread_t)m_Handle, &ret ) == 0 )
        {
            m_Handle = INVALID_THREAD_HANDLE;
            return static_cast<uint32_t>( (size_t)ret );
        }
        ASSERT( false ); // usage failure
        m_Handle = INVALID_THREAD_HANDLE;
        return 0;
    #else
        #error Unknown platform
    #endif
}

// IsRunning
//------------------------------------------------------------------------------
bool Thread::IsRunning() const
{
    return ( m_Handle != INVALID_THREAD_HANDLE );
}

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

    #if defined( __WINDOWS__ )
        ::Sleep( ms );
    #elif defined( __APPLE__ ) || defined( __LINUX__ )
        usleep( ms * 1000 );
    #else
        #error Unknown platform
    #endif
}

// Detach
//------------------------------------------------------------------------------
void Thread::Detach()
{
    // TODO:B Remove this unsafe function

    // Must only detach if running and not already joined or detached
    ASSERT( IsRunning() );

    #if defined( __WINDOWS__ )
        VERIFY( ::CloseHandle( m_Handle ) );
    #elif defined( __APPLE__ ) || defined( __LINUX__ )
        VERIFY( pthread_detach( (pthread_t)m_Handle ) == 0 );
    #else
        #error Unknown platform
    #endif

    m_Handle = INVALID_THREAD_HANDLE;
}

// JoinWithTimeout
//------------------------------------------------------------------------------
uint32_t Thread::JoinWithTimeout( uint32_t timeoutMS, bool & outTimedOut )
{
    // TODO:B Remove this unsafe function

    #if defined( __WINDOWS__ )
        // Wait for thread to finish
        const DWORD waitResult = WaitForSingleObject( m_Handle, timeoutMS );

        // Timed out?
        if ( waitResult == WAIT_TIMEOUT )
        {
            outTimedOut = true;
            return 0;
        }
        VERIFY( waitResult == WAIT_OBJECT_0 ); // Invalid handle?
        outTimedOut = false;

        // Get return code
        DWORD ret = 0;
        VERIFY( ::GetExitCodeThread( m_Handle, (LPDWORD)&ret ) );
        VERIFY( ::CloseHandle( m_Handle ) );
        m_Handle = INVALID_THREAD_HANDLE;
        return ret;
    #elif __has_feature( thread_sanitizer ) || defined( __SANITIZE_THREAD__ )
        // ThreadSanitizer doesn't support pthread_timedjoin_np yet (as of February 2018)
        outTimedOut = false;
        (void)timeoutMS;
        return Join();
    #elif defined( __APPLE__ )
        outTimedOut = false;
        (void)timeoutMS; // TODO:MAC Implement timeout support
        return Join();
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
        int retVal = pthread_timedjoin_np( (pthread_t)m_Handle, &ret, &abstime );
        if ( ( retVal == EBUSY ) || ( retVal == ETIMEDOUT ) )
        {
            outTimedOut = true;
            return 0;
        }
        VERIFY( retVal == 0 );
        m_Handle = INVALID_THREAD_HANDLE;
        outTimedOut = false;
        return static_cast<uint32_t>( (size_t)ret );
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
