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

// Forward Declarations
//------------------------------------------------------------------------------
class Job;
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
    explicit Client( const Array< AString > & workerList );
    ~Client();

private:
    virtual void OnDisconnected( const ConnectionInfo * connection );
    virtual void OnReceive( const ConnectionInfo * connection, void * data, uint32_t size, bool & keepMemory );

    void Process( const ConnectionInfo * connection, const Protocol::MsgRequestJob * msg );
    void Process( const ConnectionInfo * connection, const Protocol::MsgJobResult *, const void * payload, size_t payloadSize );
    void Process( const ConnectionInfo * connection, const Protocol::MsgRequestManifest * msg );
    void Process( const ConnectionInfo * connection, const Protocol::MsgRequestFile * msg );
    void Process( const ConnectionInfo * connection, const Protocol::MsgServerStatus * msg );

    const ToolManifest * FindManifest( const ConnectionInfo * connection, uint64_t toolId ) const;

    static uint32_t ThreadFuncStatic( void * param );
    void            ThreadFunc();

    void            LookForWorkers();
    void            CommunicateJobAvailability();
    void            CheckForTimeouts();

    Array< AString >    m_WorkerList;   // workers to connect to
    volatile bool       m_ShouldExit;   // signal from main thread
    volatile bool       m_Exited;       // flagged on exit
    Thread::ThreadHandle m_Thread;      // the thread to find and manage workers

    // state
    Timer               m_StatusUpdateTimer;

    struct ServerState
    {
        explicit ServerState();

        const ConnectionInfo *  m_Connection;
        AString                 m_RemoteName;

        Mutex                   m_Mutex;
        const Protocol::IMessage * m_CurrentMessage;
        Timer                   m_DelayTimer;
        uint32_t                m_NumJobsAvailable;     // num jobs we've told this server we have available
        Array< Job * >          m_Jobs;                 // jobs we've sent to this server

        Timer                   m_StatusTimer;

        bool                    m_Blacklisted;
    };
    Mutex                   m_ServerListMutex;
    Array< ServerState >    m_ServerList;
};

//------------------------------------------------------------------------------
