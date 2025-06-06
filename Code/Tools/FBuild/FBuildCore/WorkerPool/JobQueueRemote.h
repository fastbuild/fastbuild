// JobQueueRemote - list of pending jobs
//------------------------------------------------------------------------------
#pragma once

// Includes
//------------------------------------------------------------------------------
// FBuildCore
#include "Tools/FBuild/FBuildCore/Graph/Node.h"

// Core
#include "Core/Containers/Array.h"
#include "Core/Containers/Singleton.h"
#include "Core/Process/Mutex.h"
#include "Core/Process/Semaphore.h"

// Forward Declarations
//------------------------------------------------------------------------------
class Node;
class Job;
class ThreadPool;
class WorkerThread;

// JobQueueRemote
//------------------------------------------------------------------------------
class JobQueueRemote : public Singleton<JobQueueRemote>
{
public:
    explicit JobQueueRemote( uint32_t numWorkerThreads );
    ~JobQueueRemote();

    // main thread calls these
    void QueueJob( Job * job );
    Job * GetCompletedJob( Node::BuildResult & outResult );
    void CancelJobsWithUserData( void * userData );

    // handle shutting down
    void SignalStopWorkers();
    bool HaveWorkersStopped() const;

    size_t GetNumWorkers() const { return m_Workers.GetSize(); }
    void GetWorkerStatus( size_t index, AString & hostName, AString & status, bool & isIdle ) const;

    void MainThreadWait( uint32_t timeoutMS );
    void WakeMainThread();

    // Worker threads can wait:
    void WorkerThreadWait();    // Wait for a job to be available (active thread)
    void WorkerThreadSleep();   // Sleep (inactive thread)

private:
    // worker threads call these
    friend class WorkerThread;
    friend class WorkerThreadRemote;
    Job *       GetJobToProcess();
    static Node::BuildResult DoBuild( Job * job, bool racingRemoteJob );
    void        FinishedProcessingJob( Job * job, Node::BuildResult result );

    // internal helpers
    static bool ReadResults( Job * job );

    mutable Mutex       m_PendingJobsMutex;
    Array<Job *>        m_PendingJobs;
    mutable Mutex       m_InFlightJobsMutex;
    Array<Job *>        m_InFlightJobs;
    Mutex               m_CompletedJobsMutex;
    Array<Job *>        m_CompletedJobs;
    Array<Job *>        m_CompletedJobsFailed;
    Array<Job *>        m_CompletedJobsAborted;

    Semaphore           m_MainThreadSemaphore;
    Semaphore           m_WorkerThreadSemaphore;
    Semaphore           m_WorkerThreadSleepSemaphore;

    ThreadPool *        m_ThreadPool;
    Array<WorkerThread *> m_Workers;
};

//------------------------------------------------------------------------------
