// Worker
//------------------------------------------------------------------------------

// Includes
//------------------------------------------------------------------------------
#if defined( __WINDOWS__ )
    #include <winsock2.h> // this must be here to avoid windows include order problems
#endif

#include "Worker.h"
#include "WorkerWindow.h"
#include "WorkerSettings.h"

// FBuild
#include "Tools/FBuild/FBuildCore/FBuild.h"
#include "Tools/FBuild/FBuildCore/FBuildVersion.h"
#include "Tools/FBuild/FBuildCore/Protocol/Protocol.h"
#include "Tools/FBuild/FBuildCore/Protocol/Server.h"
#include "Tools/FBuild/FBuildCore/WorkerPool/JobQueueRemote.h"
#include "Tools/FBuild/FBuildCore/WorkerPool/WorkerThreadRemote.h"
#include "Tools/FBuild/FBuildCore/Graph/WorkerSettingsNode.h"

#include "Core/Env/Env.h"
#include "Core/FileIO/FileIO.h"
#include "Core/FileIO/PathUtils.h"
#include "Core/Network/NetworkStartupHelper.h"
#include "Core/Process/Process.h"
#include "Core/Profile/Profile.h"
#include "Core/Strings/AStackString.h"
#include "Core/Tracing/Tracing.h"

// system
#include <stdio.h>

// CONSTRUCTOR
//------------------------------------------------------------------------------
Worker::Worker( const AString & args )
    : m_MainWindow( nullptr )
    , m_ConnectionPool( nullptr )
    , m_NetworkStartupHelper( nullptr )
    , m_BaseArgs( args )
    , m_LastWriteTime( 0 )
    , m_RestartNeeded( false )
    #if defined( __WINDOWS__ )
        , m_LastDiskSpaceResult( -1 )
    #endif
{
    m_WorkerSettings = FNEW( WorkerSettings );
}

// Initialize
//------------------------------------------------------------------------------
void Worker::Initialize( void * hInstance, const bool consoleMode )
{
    m_NetworkStartupHelper = FNEW( NetworkStartupHelper );

    Server::Options serverOptions;
    serverOptions.m_NumThreadsInJobQueue = 0;
    serverOptions.m_SandboxEnabled = m_WorkerSettings->GetSandboxEnabled();
    serverOptions.m_SandboxTmp = m_WorkerSettings->GetSandboxTmp();
    serverOptions.m_WorkerTags = m_WorkerSettings->GetWorkerTags();

    m_ConnectionPool = FNEW( Server ( serverOptions ) );

    if ( consoleMode == true )
    {
        #if __WINDOWS__
            VERIFY( ::AllocConsole() );
            (void)freopen("CONOUT$", "w", stdout);
        #endif
    }
    else
    {
        m_MainWindow = FNEW( WorkerWindow( hInstance ) );
    }

    Env::GetExePath( m_BaseExeName );
    if ( m_BaseExeName.Replace( ".copy", "" ) != 1 )
    {
        m_BaseExeName.Clear(); // not running from copy, disable restart detection
    }
    m_BaseArgs.Replace( "-subprocess", "" );

    StatusMessage( "FBuildWorker %s", FBUILD_VERSION_STRING );
}

// DESTRUCTOR
//------------------------------------------------------------------------------
Worker::~Worker()
{
    FDELETE m_NetworkStartupHelper;
    FDELETE m_ConnectionPool;
    FDELETE m_MainWindow;
    FDELETE m_WorkerSettings;

    if ( m_RestartNeeded )
    {
        Process p;
        size_t tryCount = 10;
        for ( ;; )
        {
            if ( p.Spawn( m_BaseExeName.Get(),
                          m_BaseArgs.Get(),
                          nullptr,      // default workingDir
                          nullptr ) )   // default env
            {
                p.Detach();
                break;
            }

            --tryCount;
            if ( tryCount == 0 )
            {
                break;
            }

            // wait before trying again
            Thread::Sleep( 1000 );
        }
    }
}

// Work
//------------------------------------------------------------------------------
int Worker::Work()
{
    // start listening
    StatusMessage( "Listening on port %u\n", Protocol::PROTOCOL_PORT );
    if ( m_ConnectionPool->Listen( Protocol::PROTOCOL_PORT ) == false )
    {
        ErrorMessage( "Failed to listen on port %u.  Check port is not in use.", Protocol::PROTOCOL_PORT );
        return -1;
    }

    // Special folder for Orbis Clang
    // We just create this folder whether it's needed or not
    {
        AStackString<> tmpPath;
        if ( m_WorkerSettings->GetSandboxEnabled() )
        {
            if ( !FileIO::EnsurePathExists( m_WorkerSettings->GetSandboxTmp() ) )
            {
                ErrorMessage( "Failed to create tmp folder %s (error 0x%x)", m_WorkerSettings->GetSandboxTmp().Get(), Env::GetLastErr() );
                return -2;
            }
            tmpPath = m_WorkerSettings->GetObfuscatedSandboxTmp();
            PathUtils::EnsureTrailingSlash( tmpPath );
        }
        else
        {
            VERIFY( FBuild::GetTempDir( tmpPath ) );
        #if defined( __WINDOWS__ )
            tmpPath += ".fbuild.tmp\\";
        #else
            tmpPath += "_fbuild.tmp/";
        #endif
        }
        #if defined( __WINDOWS__ )
            tmpPath += "target\\include";
        #else
            tmpPath += "target/include";
        #endif

        if ( m_WorkerSettings->GetSandboxEnabled() )
        {
            AStackString<> errorMsg;
            if ( !FileIO::SetLowIntegrity(
                m_WorkerSettings->GetSandboxTmp(), errorMsg ) )  // pass in root sandbox tmp dir, so we can secure it
            {
                ErrorMessageString( errorMsg.Get() );
                return -2;
            }
            if ( !FileIO::EnsurePathExists( tmpPath ) )
            {
                // print the root sandbox tmp dir, not the obfuscated dir
                // so we can hide the obfuscated dir from other processes
                ErrorMessage( "Failed to create sandbox dir under %s (error 0x%x)",
                    m_WorkerSettings->GetSandboxTmp().Get(), Env::GetLastErr() );
                return -2;
            }
        }
        else
        {
            if ( !FileIO::EnsurePathExists( tmpPath ) )
            {
                ErrorMessage( "Failed to create tmp folder %s (error 0x%x)", tmpPath.Get(), Env::GetLastErr() );
                return -2;
            }
        }

        #if defined( __WINDOWS__ )
            tmpPath += "\\.lock";
        #else
            tmpPath += "/_lock";
        #endif
        if ( !m_TargetIncludeFolderLock.Open( tmpPath.Get(), FileStream::WRITE_ONLY ) )
        {
            ErrorMessage( "Failed to lock tmp folder (error 0x%x)", Env::GetLastErr() );
            return -2;
        }
    }

    for(;;)
    {
        if ( InConsoleMode() )
        {
            // TODO: Handle Ctrl+C gracefully to remove worker token etc
        }
        else
        {
            if ( WorkerWindow::Get().WantToQuit() )
            {
                break;
            }
        }

        UpdateAvailability();

        UpdateUI();

        CheckForExeUpdate();

        PROFILE_SYNCHRONIZE

        Thread::Sleep( 500 );
    }

    // allow to UI to shutdown
    // the application MUST NOT try to update the UI from this point on
    m_MainWindow->SetAllowQuit();

    m_WorkerBrokerage.SetUnavailable();

    return 0;
}

// HasEnoughDiskSpace
//------------------------------------------------------------------------------
bool Worker::HasEnoughDiskSpace()
{
    #if defined( __WINDOWS__ )
        // Only check disk space every few seconds
        float elapsedTime = m_TimerLastDiskSpaceCheck.GetElapsedMS();
        if ( ( elapsedTime < 15000.0f ) && ( m_LastDiskSpaceResult != -1 ) )
        {
            return ( m_LastDiskSpaceResult != 0 );
        }
        m_TimerLastDiskSpaceCheck.Start();

        static const uint64_t MIN_DISK_SPACE = 1024 * 1024 * 1024; // 1 GiB

        unsigned __int64 freeBytesAvailable = 0;
        unsigned __int64 totalNumberOfBytes = 0;
        unsigned __int64 totalNumberOfFreeBytes = 0;

        // Check available disk space of temp path
        AStackString<> tmpPath;
        if ( m_WorkerSettings->GetSandboxEnabled() )
        {
            tmpPath = m_WorkerSettings->GetObfuscatedSandboxTmp();
        }
        else
        {
            VERIFY( FBuild::GetTempDir( tmpPath ) );
        }
        BOOL result = GetDiskFreeSpaceExA( tmpPath.Get(), (PULARGE_INTEGER)&freeBytesAvailable, (PULARGE_INTEGER)&totalNumberOfBytes, (PULARGE_INTEGER)&totalNumberOfFreeBytes );
        if ( result && ( freeBytesAvailable >= MIN_DISK_SPACE ) )
        {
            m_LastDiskSpaceResult = 1;
            return true;
        }

        // The drive doesn't have enough free space or could not be queried. Exclude this machine from worker pool.
        m_LastDiskSpaceResult = 0;
        return false;
    #else
        return true; // TODO:MAC TODO:LINUX Implement disk space checks
    #endif
}

// UpdateAvailability
//------------------------------------------------------------------------------
void Worker::UpdateAvailability()
{
    // Check disk space
    bool hasEnoughDiskSpace = HasEnoughDiskSpace();

    m_IdleDetection.Update();

    WorkerSettings & ws = WorkerSettings::Get();
    uint32_t numCPUsToUse = ws.GetNumCPUsToUse();
    switch( ws.GetWorkMode() )
    {
        case WorkerSettingsNode::WHEN_IDLE:
        {
            if ( m_IdleDetection.IsIdle() == false )
            {
                numCPUsToUse = 0;
            }
            break;
        }
        case WorkerSettingsNode::DEDICATED:
        {
            break; // use all allocated cpus
        }
        case WorkerSettingsNode::DISABLED:
        {
            numCPUsToUse = 0;
            break;
        }
    }

    // don't accept any new work while waiting for a restart
    if ( m_RestartNeeded || ( hasEnoughDiskSpace == false ) )
    {
        numCPUsToUse = 0;
    }

    WorkerThreadRemote::SetNumCPUsToUse( numCPUsToUse );
    
    if ( numCPUsToUse > 0 )
    {
        m_WorkerBrokerage.SetAvailable( ws.GetWorkerTags() );
    }
    else
    {
        m_WorkerBrokerage.SetUnavailable();
    }
}

// UpdateUI
//------------------------------------------------------------------------------
void Worker::UpdateUI()
{
    // throttle UI updates
    if ( m_UIUpdateTimer.GetElapsed() < 0.25f )
    {
        return;
    }

    // title bar
    size_t numConnections = m_ConnectionPool->GetNumConnections();
    AStackString<> status;
    status.Format( "%u Connections", (uint32_t)numConnections );
    if ( m_RestartNeeded )
    {
        status += " (Restart Pending)";
    }
    #if defined( __WINDOWS__ )
        if ( m_LastDiskSpaceResult == 0 )
        {
            status += " (Low Disk Space)";
        }
    #endif
    if ( InConsoleMode() )
    {
        status += '\n';
        StatusMessage( "%s", status.Get() );
    }
    else
    {
        m_MainWindow->SetStatus( status.Get() );
    }


    if ( InConsoleMode() == false )
    {
        // thread output
        JobQueueRemote & jqr = JobQueueRemote::Get();
        const size_t numWorkers = jqr.GetNumWorkers();
        for ( size_t i=0; i<numWorkers; ++i )
        {
            // get status of worker
            AStackString<> workerStatus;
            AStackString<> hostName;
            bool isIdle;
            jqr.GetWorkerStatus( i, hostName, workerStatus, isIdle );

            // are we syncing tools?
            if ( isIdle )
            {
                AStackString<> statusStr;
                if ( m_ConnectionPool->IsSynchingTool( statusStr ) )
                {
                    // show status of synchronization
                    workerStatus = statusStr;
                }
            }

            // reflect in UI
            m_MainWindow->SetWorkerState( i, hostName, workerStatus );
        }
    }

    m_UIUpdateTimer.Start();
}

// CheckForExeUpdate
//------------------------------------------------------------------------------
void Worker::CheckForExeUpdate()
{
    // if a restart is pending, can we restart yet?
    if ( m_RestartNeeded )
    {
        // can we restart yet?
        if ( JobQueueRemote::Get().HaveWorkersStopped() )
        {
            WorkerWindow::Get().SetWantToQuit();
        }

        return;
    }

    if ( m_BaseExeName.IsEmpty() )
    {
        return; // not running as a copy to allow restarts
    }

    // get the current last write time
    uint64_t lastWriteTime = FileIO::GetFileLastWriteTime( m_BaseExeName );

    // If exe is has been deleted, but not replaced, do nothing
    // (may be part of two step delete/replace)
    if ( lastWriteTime == 0 )
    {
        return;
    }

    // store the time when we were launch (first time in this function)
    if ( m_LastWriteTime == 0 )
    {
        m_LastWriteTime = lastWriteTime;
        return;
    }

    // has exe changed?
    if ( lastWriteTime != m_LastWriteTime )
    {
        m_RestartNeeded = true;
        JobQueueRemote::Get().SignalStopWorkers();
    }
}

// StatusMessage
//------------------------------------------------------------------------------
void Worker::StatusMessage( const char * fmtString, ... ) const
{
    // Status Messages are only shown in console mode
    if ( InConsoleMode() == false )
    {
        return;
    }

    AStackString<> buffer;

    va_list args;
    va_start(args, fmtString);
    buffer.VFormat( fmtString, args );
    va_end( args );

    if ( buffer.EndsWith( '\n' ) == false )
    {
        buffer += '\n';
    }

    // don't spam when the status has not changed
    if ( m_LastStatusMessage == buffer )
    {
        return;
    }
    m_LastStatusMessage = buffer;

    OUTPUT( "%s", buffer.Get() );
}

// ErrorMessageString
//------------------------------------------------------------------------------
void Worker::ErrorMessageString( const char * message ) const
{
    if ( InConsoleMode() )
    {
        // Forward to console
        StatusMessage( "%s", message );
        return;
    }

    // Display interactive Message Box
    #if defined( __WINDOWS__ )
        ::MessageBox( nullptr, message, "FBuild Worker", MB_OK );
    #elif defined( __APPLE__ )
        // TODO:MAC Implement ErrorMessageString for non-console mode
    #elif defined( __LINUX__ )
        // TODO:LINUX Implement ErrorMessageString for non-console mode
    #else
        #error Unknown Platform
    #endif
}

// ErrorMessage
//------------------------------------------------------------------------------
void Worker::ErrorMessage( const char * fmtString, ... ) const
{
    AStackString<> buffer;

    va_list args;
    va_start(args, fmtString);
    buffer.VFormat( fmtString, args );
    va_end( args );

    ErrorMessageString( buffer.Get() );
}

//------------------------------------------------------------------------------
