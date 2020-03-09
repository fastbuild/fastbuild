// Client.h - Handles Client Side connections
//------------------------------------------------------------------------------
#pragma once

// Includes
//------------------------------------------------------------------------------
#include "Core/Containers/Array.h"
#include "Core/Containers/Tags.h"
#include "Core/Network/TCPConnectionPool.h"
#include "Core/Process/Thread.h"
#include "Core/Strings/AString.h"
#include "Core/Time/Timer.h"
#include "Tools/FBuild/FBuildCore/WorkerPool/WorkerBrokerage.h"

// Forward Declarations
//------------------------------------------------------------------------------
class Job;
class MemoryStream;
class MultiBuffer;
namespace Protocol
{
    class IMessage;
    class MsgJobResult;
    class MsgRequestJob;
    class MsgRequestManifest;
    class MsgRequestFile;
    class MsgServerStatus;
}
class ToolManifest;

// Client
//------------------------------------------------------------------------------
class Client : public TCPConnectionPool
{
public:
    Client( const Array< AString > & settingsWorkers,
            uint16_t port,
            int32_t workerListRefreshLimitSec,
            int32_t workerConnectionRetryLimitSec,
            uint32_t workerConnectionLimit,
            bool detailedLogging );
    ~Client();

private:
    typedef Array< Array< WorkerBrokerage::WorkerInfo > > WorkersPerJob;

    class ServerState
    {
        public:
            ServerState( const bool detailedLogging );
            ~ServerState();
            void Disconnect();

            const ConnectionInfo *     m_Connection;
            WorkerBrokerage::WorkerInfo m_RemoteWorker;

            Mutex                      m_Mutex;
            const Protocol::IMessage * m_CurrentMessage;
            Timer                      m_ConnectionDelayTimer;
            uint32_t                   m_ConnectionRetryIntervalSec;     // seconds between connection retries
            float                      m_ConnectionRetryElapsedTimeSec;  // secons since we started the connection retries
            uint32_t                   m_NumJobsAvailableForWorker;      // num jobs we've told this server we have available
            Array< Job * >    m_Jobs;  // jobs we've sent to this server
            Timer                      m_StatusTimer;
            bool                       m_Blacklisted;  // worker had a system error
            bool                       m_Excluded;  // worker was added to excluded workers list
            Tags                       m_RemoteWorkerTags;
            bool                       m_DetailedLogging;
        
        private:
            void ClearConnectionFields();
    };

    virtual void OnDisconnected( const ConnectionInfo * connection );
    virtual void OnReceive( const ConnectionInfo * connection, void * data, uint32_t size, bool & keepMemory );

    void Process( const ConnectionInfo * connection, const Protocol::MsgRequestJob * msg );
    void Process( const ConnectionInfo * connection, const Protocol::MsgJobResult *, const void * payload, size_t payloadSize );
    void Process( const ConnectionInfo * connection, const Protocol::MsgRequestManifest * msg );
    void Process( const ConnectionInfo * connection, const Protocol::MsgRequestFile * msg );
    void Process( const ConnectionInfo * connection, const Protocol::MsgServerStatus * msg, const void * payload, size_t payloadSize );

    const ToolManifest * FindManifest( const ConnectionInfo * connection, uint64_t toolId ) const;
    bool WriteFileToDisk( const AString& fileName, const MultiBuffer & multiBuffer, size_t index ) const;

    static uint32_t ThreadFuncStatic( void * param );
    void            ThreadFunc();

    static bool ShouldRetry(
                            const float timeSinceLastRetry,
                            const uint32_t retryIntervalSec,
                            const int32_t retryLimitSec,
                            float & retryElapsedTimeSec,
                            AString & timeIntervalString,
                            AString & timeLimitString );
    void        RetryWorkers();
    void        FlattenWorkersPerJob(
                            const WorkersPerJob & workersPerJob,
                            Array< WorkerBrokerage::WorkerInfo > & workers ) const;
    static void GetWorkerChanges(
                            const Array< WorkerBrokerage::WorkerInfo > & previousWorkers,
                            const Array< WorkerBrokerage::WorkerInfo > & currentWorkers,
                            Array< WorkerBrokerage::WorkerInfo > & addedWorkers );
    void        ExcludeWorker( Client::ServerState & ss, Array< WorkerBrokerage::WorkerInfo > & newlyExcludedWorkers );
    void        HandleExcludedWorkers( const Array< WorkerBrokerage::WorkerInfo > & newlyExcludedWorkers );
    bool        UpdateServerList( const Array< WorkerBrokerage::WorkerInfo > & removedWorkers, const Array< WorkerBrokerage::WorkerInfo > & addedWorkers );
    size_t      GetNumValidWorkers() const;
    void        OutputNumWorkers();
    void        NotifyJobQueue();
    void        LookForWorkers();
    void        CommunicateJobAvailability();

    // More verbose name to avoid conflict with windows.h SendMessage
    void            SendMessageInternal( const ConnectionInfo * connection, const Protocol::IMessage & msg );
    void            SendMessageInternal( const ConnectionInfo * connection, const Protocol::IMessage & msg, const MemoryStream & memoryStream );

    WorkerBrokerage  m_WorkerBrokerage;
    Array< WorkerBrokerage::WorkerInfo > m_SettingsWorkers;  // workers from settings
    WorkersPerJob    m_WorkerList;       // workers to use
    Array< WorkerBrokerage::WorkerInfo > m_ExcludedWorkers;  // worker to no longer try
    bool             m_RetryingWorkers;
    bool             m_NotifyJobQueue;
    Timer            m_WorkerListRefreshTimer;
    uint32_t         m_WorkerListRefreshIntervalSec;     // seconds between worker list retries
    float            m_WorkerListRefreshElapsedTimeSec;  // seconds since we started the worker list retries
    int32_t          m_WorkerListRefreshLimitSec;        // max seconds elapsed until stop retrying worker list
    int32_t          m_WorkerConnectionRetryLimitSec;    // max seconds elapsed until stop retrying connection to worker
    volatile bool       m_ShouldExit;   // signal from main thread
    volatile bool       m_Exited;         // flagged on exit
    bool                m_DetailedLogging;
    Thread::ThreadHandle m_Thread;      // the thread to find and manage workers

    // state
    Timer                 m_StatusUpdateTimer;

    Mutex                 m_ServerListMutex;
    Array< ServerState* > m_ServerList;
    uint32_t              m_WorkerConnectionLimit;
    uint16_t              m_Port;
};

//------------------------------------------------------------------------------
