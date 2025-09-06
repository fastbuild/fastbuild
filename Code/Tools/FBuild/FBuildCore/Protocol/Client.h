// Client.h - Handles Client Side connections
//------------------------------------------------------------------------------
#pragma once

// Includes
//------------------------------------------------------------------------------
// FBuild
#include "Tools/FBuild/FBuildCore/WorkerPool/WorkerBrokerageClient.h"

// Core
#include "Core/Containers/Array.h"
#include "Core/Containers/UniquePtr.h"
#include "Core/Process/Atomic.h"
#include "Core/Process/Thread.h"
#include "Core/Strings/AString.h"
#include "Core/Time/Timer.h"

// Forward Declarations
//------------------------------------------------------------------------------
class ClientToWorkerConnection;
class ClientWorkerInfo;
class Job;
class MemoryStream;
class MultiBuffer;
namespace Protocol
{
    class IMessage;
    class MsgConnectionAck;
    class MsgJobResult;
    class MsgJobResultCompressed;
    class MsgRequestJob;
    class MsgRequestManifest;
    class MsgRequestFile;
    class MsgServerStatus;
}
class ToolManifest;

// Client
//------------------------------------------------------------------------------
class Client
{
public:
    Client( const Array<AString> & explicitWorkerList,
            uint16_t port,
            uint32_t workerConnectionLimit,
            bool detailedLogging );
    ~Client();

    uint16_t GetPort() const { return m_Port; }

    uint32_t GetNumConnections() const;

private:
    Client & operator=( const Client & other ) = delete;

    static uint32_t ThreadFuncStatic( void * param );
    void ThreadFunc();

    void FindPotentialWorkers();
    void RegisterFoundWorkers( const Array<AString> & workerList,
                               const AString * brokeragePaths );
    void ConnectToWorkers();
    void CommunicateJobAvailability();

    // Worker pool
    bool m_WorkerDiscoveryDone = false;
    Array<AString> m_StaticWorkerList; // optional explicit list to use (overrides discovery)
    Array<UniquePtr<ClientWorkerInfo>> m_WorkerPool; // workers to potentially connect to
    uint32_t m_NextWorkerIndex = 0; // next worker we will attempt to connect to
    WorkerBrokerageClient m_WorkerBrokerage; // Helper for access to brokerage

    // Primary "Client" thread
    Atomic<bool> m_ShouldExit{ false }; // signal from main thread
    Thread m_Thread; // the thread to find and manage workers

    // state
    Timer m_StatusUpdateTimer;

    // Workers we are connected to (or establishing/cleaning up a connection to)
    Array<UniquePtr<ClientToWorkerConnection>> m_ActiveConnections;

    // State that remains constant throughout the build
    const bool m_DetailedLogging;
    const uint32_t m_WorkerConnectionLimit;
    const uint16_t m_Port;
};

//------------------------------------------------------------------------------
