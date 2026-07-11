// ThreadPool
//------------------------------------------------------------------------------
#pragma once

// Includes
//------------------------------------------------------------------------------
// Core
#include "Core/Containers/Array.h"
#include "Core/Containers/UniquePtr.h"
#include "Core/Process/Atomic.h"
#include "Core/Process/Mutex.h"
#include "Core/Process/Semaphore.h"
#include "Core/Process/Thread.h"

// ThreadPool
//------------------------------------------------------------------------------
class ThreadPool
{
public:
    explicit ThreadPool( uint32_t numThreads );
    ~ThreadPool();

    // Enqueue jobs
    using ThreadJobFunc = void ( * )( void * param );
    void EnqueueJob( ThreadJobFunc func, void * userData = nullptr );

    uint32_t GetNumThreads() const { return m_NumThreads; }

protected:
    void operator=( const ThreadPool & other ) = delete;

    static uint32_t ThreadFuncWrapper( void * userData );
    void ThreadFunc();

    // Job tracking structure
    class ThreadJob
    {
    public:
        ThreadJobFunc m_Function = nullptr;
        void * m_UserData = nullptr;
    };

    // Thread structure
    class ThreadPoolThread
    {
    public:
        ThreadPoolThread( uint32_t threadId, ThreadPool & ownerPool );
        ~ThreadPoolThread();
        void ThreadFunc();

        void operator=( const ThreadPoolThread & other ) = delete;

    protected:
        uint32_t m_ThreadId = 0;
        Thread m_Thread;
        ThreadPoolThread * m_Next = nullptr;
        ThreadPool & m_OwnerPool;
    };

    // Thread management
    const uint32_t m_NumThreads = 0;
    Atomic<bool> m_WantToQuit;
    Semaphore m_WakeSemaphore;
    ThreadPoolThread * m_FirstThread;

    // Job queue
    Mutex m_JobQueueMutex;
    Array<ThreadJob> m_JobQueue;
};

//------------------------------------------------------------------------------
