// Client.h - Handles Client Side connections
//------------------------------------------------------------------------------
#pragma once

// Includes
//------------------------------------------------------------------------------
#include "Core/Containers/Array.h"
#include "Core/Network/TCPConnectionPool.h"
#include "Core/Process/Thread.h"
#include "Core/Strings/AString.h"
#include "Core/Time/Timer.h"
#include "Tools/FBuild/FBuildCore/WorkerPool/WorkerBrokerage.h"

// Forward Declarations
//------------------------------------------------------------------------------
class Job;
class MemoryStream;
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
    typedef Array< AString > Workers;

    class ServerState
    {
        public:
            ServerState( const bool detailedLogging );
            ~ServerState();
            void Disconnect();

            const ConnectionInfo *     m_Connection;
            AString                    m_RemoteName;

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
    void Process( const ConnectionInfo * connection, const Protocol::MsgServerStatus * msg );

    const ToolManifest * FindManifest( const ConnectionInfo * connection, uint64_t toolId ) const;
    bool WriteFileToDisk( const AString & fileName, const char * data, const uint32_t dataSize ) const;

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
    static void GetWorkerChanges(
                            const Array< AString > & previousWorkers,
                            const Array< AString > & currentWorkers,
                            Array< AString > & addedWorkers );
    void        ExcludeWorker( Client::ServerState & ss, Array< AString > & newlyExcludedWorkers );
    void        HandleExcludedWorkers( const Array< AString > & newlyExcludedWorkers );
    bool        UpdateServerList( const Array< AString > & removedWorkers, const Array< AString > & addedWorkers );
    size_t      GetNumValidWorkers() const;
    void        OutputNumWorkers();
    void        LookForWorkers();
    void        CommunicateJobAvailability();
    void        CheckForTimeouts();

    // More verbose name to avoid conflict with windows.h SendMessage
    void            SendMessageInternal( const ConnectionInfo * connection, const Protocol::IMessage & msg );
    void            SendMessageInternal( const ConnectionInfo * connection, const Protocol::IMessage & msg, const MemoryStream & memoryStream );

    WorkerBrokerage  m_WorkerBrokerage;
    Array< AString > m_SettingsWorkers;  // workers from settings
    Workers          m_WorkerList;       // workers to use
    Array< AString > m_ExcludedWorkers;  // worker to no longer try
    bool             m_RetryingWorkers;
    Timer            m_WorkerListRefreshTimer;
    uint32_t         m_WorkerListRefreshIntervalSec;     // seconds between worker list retries
    float            m_WorkerListRefreshElapsedTimeSec;  // seconds since we started the worker list retries
    int32_t          m_WorkerListRefreshLimitSec;        // max seconds elapsed until stop retrying worker list
    int32_t          m_WorkerConnectionRetryLimitSec;    // max seconds elapsed until stop retrying connection to worker
    volatile bool    m_ShouldExit;                       // signal from main thread
    volatile bool    m_Exited;                           // flagged on exit
    bool             m_DetailedLogging;
    Thread::ThreadHandle m_Thread;                       // the thread to find and manage workers

    // state
    Timer                 m_StatusUpdateTimer;

    Mutex                 m_ServerListMutex;
    Array< ServerState* > m_ServerList;
    uint32_t              m_WorkerConnectionLimit;
    uint16_t              m_Port;
};

//------------------------------------------------------------------------------
