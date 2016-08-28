// Worker
//------------------------------------------------------------------------------
#pragma once

// Includes
//------------------------------------------------------------------------------
#include "IdleDetection.h"

// FBuild
#include "Tools/FBuild/FBuildCore/WorkerPool/WorkerBrokerage.h"

// Core
#include "Core/FileIO/FileStream.h"

// Forward Declarations
//------------------------------------------------------------------------------
class Server;
class WorkerWindow;
class JobQueueRemote;
class NetworkStartupHelper;
class WorkerSettings;

// WorkerWindow
//------------------------------------------------------------------------------
class Worker
{
public:
    explicit Worker( void * hInstance, const AString & args );
    ~Worker();

    int Work();

private:
    void UpdateAvailability();
    void UpdateUI();
    void CheckForExeUpdate();
    bool HasEnoughDiskSpace();

    inline bool InConsoleMode() const { return ( m_MainWindow == nullptr ); }

    void StatusMessage( const char * fmtString, ... ) const;
    void ErrorMessage( const char * fmtString, ... ) const;

    WorkerWindow        * m_MainWindow;
    Server              * m_ConnectionPool;
    JobQueueRemote      * m_JobQueueRemote;
    NetworkStartupHelper * m_NetworkStartupHelper;
    WorkerSettings      * m_WorkerSettings;
    IdleDetection       m_IdleDetection;
    WorkerBrokerage     m_WorkerBrokerage;
    AString             m_BaseExeName;
    AString             m_BaseArgs;
    uint64_t            m_LastWriteTime;
    bool                m_RestartNeeded;
    Timer               m_UIUpdateTimer;
    FileStream          m_TargetIncludeFolderLock;
    #if defined( __WINDOWS__ )
        Timer               m_TimerLastDiskSpaceCheck;
        int32_t             m_LastDiskSpaceResult;      // -1 : No check done yet. 0=Not enough space right now. 1=OK for now.
    #endif
    mutable AString     m_LastStatusMessage;
};

//------------------------------------------------------------------------------
