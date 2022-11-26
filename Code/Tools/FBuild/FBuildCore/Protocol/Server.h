// Server.h - Handles Server Side connections
//------------------------------------------------------------------------------
#pragma once

// Includes
//------------------------------------------------------------------------------
#include "Core/Network/TCPConnectionPool.h"
#include "Core/Process/Atomic.h"
#include "Core/Time/Timer.h"

// Forward Declarations
//------------------------------------------------------------------------------
class Job;
class JobQueueRemote;
namespace Protocol
{
    class IMessage;
    class MsgConnection;
    class MsgJob;
    class MsgManifest;
    class MsgNoJobAvailable;
    class MsgStatus;
    class MsgFile;
}
class ToolManifest;

// Protocol
//------------------------------------------------------------------------------
class Server : public TCPConnectionPool
{
public:
    Server( uint32_t numThreadsInJobQueue = 0 );
    virtual ~Server() override;

    static void GetHostForJob( const Job * job, AString & hostName );

    bool IsSynchingTool( AString & statusStr ) const;

private:
    // TCPConnection interface
    virtual void OnConnected( const ConnectionInfo * connection ) override;
    virtual void OnDisconnected( const ConnectionInfo * connection ) override;
    virtual void OnReceive( const ConnectionInfo * connection, void * data, uint32_t size, bool & keepMemory ) override;

    // helpers to handle messages
    void Process( const ConnectionInfo * connection, const Protocol::MsgConnection * msg );
    void Process( const ConnectionInfo * connection, const Protocol::MsgStatus * msg );
    void Process( const ConnectionInfo * connection, const Protocol::MsgNoJobAvailable * msg );
    void Process( const ConnectionInfo * connection, const Protocol::MsgJob * msg, const void * payload, size_t payloadSize );
    void Process( const ConnectionInfo * connection, const Protocol::MsgManifest * msg, const void * payload, size_t payloadSize );
    void Process( const ConnectionInfo * connection, const Protocol::MsgFile * msg, const void * payload, size_t payloadSize );

    static uint32_t ThreadFuncStatic( void * param );
    void            ThreadFunc();

    void            FindNeedyClients();
    void            FinalizeCompletedJobs();
    void            TouchToolchains();
    void            CheckWaitingJobs( const ToolManifest * manifest );

    void            RequestMissingFiles( const ConnectionInfo * connection, ToolManifest * manifest ) const;

    struct ClientState
    {
        explicit ClientState( const ConnectionInfo * ci )
            : m_Connection( ci )
            , m_WaitingJobs( 16, true )
        {}

        inline bool operator < ( const ClientState & other ) const { return ( m_NumJobsAvailable > other.m_NumJobsAvailable ); }

        Mutex                   m_Mutex;

        const Protocol::IMessage * m_CurrentMessage = nullptr;
        const ConnectionInfo *  m_Connection = nullptr;
        uint32_t                m_NumJobsAvailable = 0;
        uint32_t                m_NumJobsRequested = 0;
        uint32_t                m_NumJobsActive = 0;

        uint8_t                 m_ProtocolVersionMinor = 0;
        AString                 m_HostName;

        Array< Job * >          m_WaitingJobs; // jobs waiting for manifests/toolchains

        Timer                   m_StatusTimer;
    };

    JobQueueRemote *        m_JobQueueRemote;

    Atomic<bool>            m_ShouldExit;   // signal from main thread
    Thread::ThreadHandle    m_Thread;       // the thread to manage workload
    Mutex                   m_ClientListMutex;
    Array< ClientState * >  m_ClientList;

    mutable Mutex           m_ToolManifestsMutex;
    Array< ToolManifest * > m_Tools;
    
    #if defined( __OSX__ ) || defined( __LINUX__ )
        Timer                   m_TouchToolchainTimer;
    #endif
};

//------------------------------------------------------------------------------
