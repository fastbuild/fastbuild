// ThreadPool
//------------------------------------------------------------------------------

// Includes
//------------------------------------------------------------------------------
#include "ThreadPool.h"

// Core
#include "Core/Process/Thread.h"
#include "Core/Profile/Profile.h"
#include "Core/Strings/AStackString.h"

// CONSTRUCTOR
//------------------------------------------------------------------------------
ThreadPool::ThreadPool( uint32_t numThreads )
    : m_NumThreads( numThreads )
#if defined( __WINDOWS__ )
    , m_WakeSemaphore( numThreads ) // On Windows, take advantage of signalling limit
#else
    , m_WakeSemaphore()
#endif
{
    PROFILE_FUNCTION;

    // Doesn't make sense to create a pool with no threads
    ASSERT( m_NumThreads > 0 );

    // Start first thread (it will create additional threads)
    const uint32_t threadId = 1;
    m_FirstThread = FNEW( ThreadPoolThread( threadId, *this ) );
}

// DESTRUCTOR
//------------------------------------------------------------------------------
ThreadPool::~ThreadPool()
{
    PROFILE_FUNCTION;

    // Signal worker threads to exit
    m_WantToQuit.Store( true );
    m_WakeSemaphore.Signal( m_NumThreads );

    // Destroy first thread (it will destroy additional threads)
    FDELETE m_FirstThread;

    // All jobs should have been completed
    ASSERT( m_JobQueue.IsEmpty() );
}

// EnqueueJob
//------------------------------------------------------------------------------
void ThreadPool::EnqueueJob( ThreadJobFunc func, void * userData )
{
    // Add job to queue
    {
        MutexHolder mh( m_JobQueueMutex );
        ThreadJob & job = m_JobQueue.EmplaceBack();
        job.m_Function = func;
        job.m_UserData = userData;
    }

    // Wake threads if sleeping
    m_WakeSemaphore.Signal();
}

// ThreadFuncWrapper
//------------------------------------------------------------------------------
/*static*/ uint32_t ThreadPool::ThreadFuncWrapper( void * userData )
{
    ThreadPoolThread * threadPoolThread = static_cast<ThreadPoolThread *>( userData );
    threadPoolThread->ThreadFunc();
    return 0;
}

// ThreadFunc
//------------------------------------------------------------------------------
void ThreadPool::ThreadFunc()
{
    for ( ;; )
    {
        // See if a job is available
        // (Jobs might be taken by other threads even if we were woken)
        ThreadJob job;
        {
            MutexHolder mh( m_JobQueueMutex );
            if ( m_JobQueue.IsEmpty() == false )
            {
                // Remove Job from queue, processing them in order of enqueuing
                // Popping from the front of an Array is inefficient, but current
                // uses have few jobs (typically one per-core)
                job = m_JobQueue[ 0 ];
                m_JobQueue.PopFront();
            }
        }

        // Job available?
        if ( job.m_Function )
        {
            // Execute job
            ( job.m_Function )( job.m_UserData );

            // Immediately get another job. Ensure all jobs are consumed
            // before shutting down.
            continue;
        }

        // Shutting down?
        if ( m_WantToQuit.Load() )
        {
            break;
        }

        // Wait for work to be available
        m_WakeSemaphore.Wait();
    }
}

//------------------------------------------------------------------------------
ThreadPool::ThreadPoolThread::ThreadPoolThread( uint32_t threadId,
                                                ThreadPool & ownerPool )
    : m_ThreadId( threadId )
    , m_OwnerPool( ownerPool )
{
    // Start thread
    AStackString threadName;
    threadName.Format( "ThreadPool_%02u", threadId );
    m_Thread.Start( ThreadFuncWrapper, threadName.Get(), this, MEGABYTE );
}

//------------------------------------------------------------------------------
ThreadPool::ThreadPoolThread::~ThreadPoolThread()
{
    // Wait for thread to exit
    m_Thread.Join();
}

//------------------------------------------------------------------------------
void ThreadPool::ThreadPoolThread::ThreadFunc()
{
    PROFILE_FUNCTION;

    // First thread creates other threads
    if ( ( m_ThreadId == 1 ) && ( m_OwnerPool.GetNumThreads() > 1 ) )
    {
        PROFILE_SECTION( "CreateAdditionalThreads" );
        ThreadPoolThread * thread = this;
        for ( uint32_t i = 2; i <= m_OwnerPool.GetNumThreads(); ++i )
        {
            thread->m_Next = FNEW( ThreadPoolThread( i, m_OwnerPool ) );
            thread = thread->m_Next;
        }
    }

    // Process jobs
    m_OwnerPool.ThreadFunc();

    // First thread joins additional threads
    if ( ( m_ThreadId == 1 ) && ( m_OwnerPool.GetNumThreads() > 1 ) )
    {
        PROFILE_SECTION( "DestroyAdditionalThreads" );

        // Join
        ThreadPoolThread * thread = m_Next;
        while ( thread )
        {
            ThreadPoolThread * next = thread->m_Next;
            FDELETE thread;
            thread = next;
        }
    }
}

//------------------------------------------------------------------------------
