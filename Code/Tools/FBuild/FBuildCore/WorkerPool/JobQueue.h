// JobQueue - list of pending jobs
//------------------------------------------------------------------------------
#pragma once

// Includes
//------------------------------------------------------------------------------
#include "Core/Containers/Array.h"
#include "Core/Containers/Singleton.h"
#include "Core/Containers/Tags.h"

#include "Tools/FBuild/FBuildCore/Graph/Node.h"
#include "Core/Process/Semaphore.h"
#include "Core/Process/Mutex.h"

// Forward Declarations
//------------------------------------------------------------------------------
class Node;
class Job;
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
    void QueueJobs( Array< Node * > & nodes );

    // jobs consumed by workers
    Job * RemoveJob();
    void  DeleteJobs();
    
private:
    uint32_t       m_Count;    // access the current count
    Mutex          m_Mutex;    // lock to add/remove jobs
    Array< Job * > m_Jobs;  // Sorted, most expensive at end
};

// JobQueue
//------------------------------------------------------------------------------
class JobQueue : public Singleton< JobQueue >
{
public:
    explicit JobQueue( const uint32_t numWorkerThreads,
        const Tags & localWorkerTags );
    ~JobQueue();

    // main thread calls these
    void AddJobToBatch( Node * node );  // Add new job to the staging queue
    void FlushJobBatch();               // Sort and flush the staging queue
    void FinalizeCompletedJobs( NodeGraph & nodeGraph );
    void MainThreadWait( uint32_t maxWaitMS );

    // main thread can be signalled
    inline void WakeMainThread() { m_MainThreadSemaphore.Signal(); }

    // handle shutting down
    void SignalStopWorkers();
    bool HaveWorkersStopped() const;

    // access state
    inline const Tags & GetLocalWorkerTags() const { return m_LocalWorkerTags; };
    void GetDistributableJobWorkerTags(
        Array< Tags > & requiredTagsPerJob ) const;
    void GetNumDistributableJobsAvailable(
        const Tags & workerTags,
        uint32_t & numJobsAvailable,
        uint32_t & numJobsAvailableForWorker ) const;

    void  CheckUnmatchedJob( Job * unmatchedJob, bool & errored );
    void GetJobStats( uint32_t & numJobs, uint32_t & numJobsActive,
                      uint32_t & numJobsDist, uint32_t & numJobsDistActive ) const;

private:
    // worker threads call these
    friend class WorkerThread;
    void         WorkerThreadWait( uint32_t maxWaitMS );
    static bool  CanBuildOnWorker( const Node * node, const Tags & workerTags );
    static Job * GetJobForWorker( Job * jobToCheck, const Tags & workerTags );
    Job *        GetJobToProcess();
    Job *        GetDistributableJobToRace( const Tags & workerTags );
    static Node::BuildResult DoBuild( Job * job );
    void         FinishedProcessingJob( Job * job, bool result, bool wasARemoteJob );

    void         QueueDistributableJob( Job * job );

    // client side of protocol consumes jobs via this interface
    friend class Client;
    Job * GetDistributableJobToProcess( const Tags & workerTags, const bool remote, bool & errored );
    void  CheckUnmatchedJobs( bool & errored );
    Job * OnReturnRemoteJob( uint32_t jobId );
    void  ReturnUnfinishedDistributableJob( Job * job );
    void  SetRetryingWorkers( const size_t numRemoteWorkers, const bool retryingWorkers );
    void  UpdateWorkerRecords( const Tags & workerTags );
    void  RemoveWorkerRecords( const AString & workerName );

    // Semaphore to manage work
    Semaphore           m_WorkerThreadSemaphore;

    // Jobs available for local processing
    Array< Node * >     m_LocalJobs_Staging;
    JobSubQueue         m_LocalJobs_Available;

    // number of workers that we may connect to
    size_t              m_NumRemoteWorkers;
    
    // Jobs in progress locally
    uint32_t            m_NumLocalJobsActive;

    // Jobs available for distributed processing (can also be done locally)
    mutable Mutex       m_DistributedJobsMutex;
    Array< Job * >      m_DistributableJobs_Available;  // Available, not in progress anywhere
    Array< Job * >      m_DistributableJobs_InProgress; // In progress remotely, locally or both

    // Semaphore to manage thread idle
    Semaphore           m_MainThreadSemaphore;

    // completed jobs
    mutable Mutex       m_CompletedJobsMutex;
    Array< Job * >      m_CompletedJobs;
    Array< Job * >      m_CompletedJobsFailed;

    // we have pair of arrays to enable a swap, avoiding locking the mutex too long
    Array< Job * >      m_CompletedJobs2;
    Array< Job * >      m_CompletedJobsFailed2;

    Array< WorkerThread * > m_Workers;
    bool                    m_RetryingWorkers;
    Tags                    m_LocalWorkerTags;
};

//------------------------------------------------------------------------------
