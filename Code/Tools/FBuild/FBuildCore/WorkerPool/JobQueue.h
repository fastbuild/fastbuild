// JobQueue - list of pending jobs
//------------------------------------------------------------------------------
#pragma once

// Includes
//------------------------------------------------------------------------------
#include "Core/Containers/Array.h"
#include "Core/Containers/Singleton.h"

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

    inline uint32_t GetCount() const { return m_Count; }

    // jobs pushed by the main thread
    void QueueJobs( Array< Node * > & nodes );

    // jobs consumed by workers
    Job * RemoveJob();
private:
    uint32_t    m_Count;    // access the current count
    Mutex       m_Mutex;    // lock to add/remove jobs
    Array< Job * > m_Jobs;  // Sorted, most expensive at end
};

// JobQueue
//------------------------------------------------------------------------------
class JobQueue : public Singleton< JobQueue >
{
public:
    explicit JobQueue( uint32_t numWorkerThreads );
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
    size_t GetNumDistributableJobsAvailable() const;
    inline size_t GetDistributableJobsMemUsage() const { return m_DistributableJobsMemoryUsage; }

    void GetJobStats( uint32_t & numJobs, uint32_t & numJobsActive,
                      uint32_t & numJobsDist, uint32_t & numJobsDistActive ) const;

private:
    // worker threads call these
    friend class WorkerThread;
    void        WorkerThreadWait( uint32_t maxWaitMS );
    Job *       GetJobToProcess();
    Job *       GetDistributableJobToRace();
    static Node::BuildResult DoBuild( Job * job );
    void        FinishedProcessingJob( Job * job, bool result, bool wasARemoteJob, bool localRaceOfRemoteJob );

    void    QueueJob2( Job * job );

    // client side of protocol consumes jobs via this interface
    friend class Client;
    Job *       GetDistributableJobToProcess( bool remote );
    Job *       OnReturnRemoteJob( uint32_t jobId, bool & cancelled );
    void        ReturnUnfinishedDistributableJob( Job * job, bool systemError = false );

    // Semaphore to manage work
    Semaphore           m_WorkerThreadSemaphore;

    // Jobs available for local processing
    Array< Node * >     m_LocalJobs_Staging;
    JobSubQueue         m_LocalJobs_Available;

    // Jobs in progress locally
    uint32_t            m_NumLocalJobsActive;

    // Jobs available for distribution
    mutable Mutex       m_DistributableAvailableJobsMutex;
    Array< Job * >      m_DistributableAvailableJobs;
    size_t              m_DistributableJobsMemoryUsage;

    // Jobs in progress remotely
    mutable Mutex       m_DistributedJobsMutex;
    Array< Job * >      m_DistributedJobsRemote;        // Distributable job in progress remotely
    Array< Job * >      m_DistributedJobsLocal;         // Distributable job in progress locally
    Array< Job * >      m_DistributedJobsBeingRaced;    // Distributable job in progress remotely & locally
    struct CancelledJob
    {
        inline CancelledJob( Job * job, uint32_t jobId ) : m_JobId( jobId ), m_Job( job ) {}
        inline bool operator == ( uint32_t jobId ) const { return ( m_JobId == jobId ); }
        inline bool operator == ( const Job * job ) const { return ( m_Job == job ); }
        uint32_t    m_JobId;
        Job *       m_Job;
    };
    Array< CancelledJob > m_DistributedJobsCancelled;       // Distirbutable job in progress remotely, which should be discarded upon completion

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
};

//------------------------------------------------------------------------------
