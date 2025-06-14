// JobQueue - list of pending jobs
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
class SettingsNode;
class ThreadPool;
class WorkerThread;

// JobSubQueue
//------------------------------------------------------------------------------
class JobSubQueue
{
public:
    JobSubQueue();
    ~JobSubQueue();

    uint32_t GetCount() const;

    // jobs pushed by the main thread
    void QueueJobs( Array<Node *> & nodes );

    // jobs consumed by workers
    Job * RemoveJob();

private:
    uint32_t m_Count; // access the current count
    Mutex m_Mutex; // lock to add/remove jobs
    Array<Job *> m_Jobs; // Sorted, most expensive at end
};

// JobQueue
//------------------------------------------------------------------------------
class JobQueue : public Singleton<JobQueue>
{
public:
    explicit JobQueue( uint32_t numWorkerThreads, ThreadPool * threadPool );
    ~JobQueue();

    // main thread calls these
    void AddJobToBatch( Node * node );  // Add new job to the staging queue
    void FlushJobBatch( const SettingsNode & settings ); // Sort and flush the staging queue
    bool HasJobsToFlush() const;
    void FinalizeCompletedJobs( NodeGraph & nodeGraph );
    void MainThreadWait( uint32_t maxWaitMS );

    // main thread can be signalled
    void WakeMainThread() { m_MainThreadSemaphore.Signal(); }

    // handle shutting down
    void SignalStopWorkers();
    bool HaveWorkersStopped() const;

    // access state
    size_t GetNumDistributableJobsAvailable() const;

    void GetJobStats( uint32_t & numJobs, uint32_t & numJobsActive,
                      uint32_t & numJobsDist, uint32_t & numJobsDistActive ) const;
    bool HasPendingCompletedJobs() const;

private:
    // worker threads call these
    friend class WorkerThread;
    void WorkerThreadWait( uint32_t maxWaitMS );
    Job * GetJobToProcess();
    Job * GetDistributableJobToRace();
    static Node::BuildResult DoBuild( Job * job );
    void FinishedProcessingJob( Job * job, Node::BuildResult result, bool wasARemoteJob );

    void QueueDistributableJob( Job * job );

    // client side of protocol consumes jobs via this interface
    friend class Client;
    Job * GetDistributableJobToProcess( bool remote, uint8_t workerMinorProtocolVersion );
    Job * OnReturnRemoteJob( uint32_t jobId,
                             bool systemError,
                             bool & outRaceLost,
                             bool & outRaceWon,
                             const Node *& outNode,
                             uint32_t & outJobSystemErrorCount );
    void ReturnUnfinishedDistributableJob( Job * job );

    // Semaphore to manage work
    Semaphore m_WorkerThreadSemaphore;

    // Jobs available for local processing
    class ConcurrencyGroupState
    {
    public:
        Array<Node *> m_LocalJobs_Staging; // Jobs ready to be made available
        uint32_t m_ActiveJobs = 0; // Jobs made available for processing
    };
    Array<ConcurrencyGroupState> m_ConcurrencyGroupsState;
    JobSubQueue m_LocalJobs_Available;

    // Jobs in progress locally
    uint32_t m_NumLocalJobsActive;

    // Jobs available for distributed processing (can also be done locally)
    mutable Mutex m_DistributedJobsMutex;
    Array<Job *> m_DistributableJobs_Available; // Available, not in progress anywhere
    Array<Job *> m_DistributableJobs_InProgress; // In progress remotely, locally or both

    // Semaphore to manage thread idle
    Semaphore m_MainThreadSemaphore;

    // completed jobs
    mutable Mutex m_CompletedJobsMutex;
    Array<Job *> m_CompletedJobs;
    Array<Job *> m_CompletedJobsAborted;
    Array<Job *> m_CompletedJobsFailed;

    // we have pair of arrays to enable a swap, avoiding locking the mutex too long
    Array<Job *> m_CompletedJobs2;
    Array<Job *> m_CompletedJobsAborted2;
    Array<Job *> m_CompletedJobsFailed2;

    Array<WorkerThread *> m_Workers;
};

//------------------------------------------------------------------------------
