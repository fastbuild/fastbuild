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
#include "Tools/FBuild/FBuildCore/FBuildVersion.h"
#include "Tools/FBuild/FBuildCore/Protocol/Protocol.h"
#include "Tools/FBuild/FBuildCore/Protocol/Server.h"
#include "Tools/FBuild/FBuildCore/WorkerPool/JobQueueRemote.h"
#include "Tools/FBuild/FBuildCore/WorkerPool/WorkerThreadRemote.h"

#include "Core/Env/Env.h"
#include "Core/FileIO/FileIO.h"
#include "Core/Network/NetworkStartupHelper.h"
#include "Core/Process/Process.h"
#include "Core/Profile/Profile.h"
#include "Core/Strings/AStackString.h"
#include "Core/Tracing/Tracing.h"

// system
#include <stdio.h>

// CONSTRUCTOR
//------------------------------------------------------------------------------
Worker::Worker( void * hInstance, const AString & args )
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
    m_NetworkStartupHelper = FNEW( NetworkStartupHelper );
    m_JobQueueRemote = FNEW( JobQueueRemote( Env::GetNumProcessors() ) );
    #if defined( __WINDOWS__ )
        m_MainWindow = FNEW( WorkerWindow( hInstance ) );
    #else
        (void)hInstance;
    #endif
    m_ConnectionPool = FNEW( Server );

    Env::GetExePath( m_BaseExeName );
    if ( m_BaseExeName.Replace( ".copy", "" ) != 1 )
    {
        m_BaseExeName.Clear(); // not running from copy, disable restart detection
    }
    m_BaseArgs.Replace( "-subprocess", "" );

    StatusMessage( "FBuildWorker %s (%s)", FBUILD_VERSION_STRING, FBUILD_VERSION_PLATFORM );
}

// DESTRUCTOR
//------------------------------------------------------------------------------
Worker::~Worker()
{
    FDELETE m_NetworkStartupHelper;
    FDELETE m_ConnectionPool;
    FDELETE m_MainWindow;
    FDELETE m_JobQueueRemote;
    FDELETE m_WorkerSettings;

    if ( m_RestartNeeded )
    {
        Process p;
        p.Spawn( m_BaseExeName.Get(),
                 m_BaseArgs.Get(),
                 nullptr,   // default workingDir
                 nullptr ); // default env
        p.Detach();
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
        VERIFY( FileIO::GetTempDir( tmpPath ) );
        #if defined( __WINDOWS__ )
            tmpPath += ".fbuild.tmp\\target\\include";
        #else
            tmpPath += "_fbuild.tmp/target/include";
        #endif
        if ( !FileIO::EnsurePathExists( tmpPath ) )
        {
            ErrorMessage( "Failed to initialize tmp folder.  Error: 0x%x", Env::GetLastErr() );
            return -2;
        }
        #if defined( __WINDOWS__ )
            tmpPath += "\\.lock";
        #else
            tmpPath += "/_lock";
        #endif
        if ( !m_TargetIncludeFolderLock.Open( tmpPath.Get(), FileStream::WRITE_ONLY ) )
        {
            ErrorMessage( "Failed to lock tmp folder.  Error: 0x%x", Env::GetLastErr() );
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

        Thread::Sleep( 100 );
    }

    // allow to UI to shutdown
    // the application MUST NOT try to update the UI from this point on
    m_MainWindow->SetAllowQuit();

    m_WorkerBrokerage.SetAvailability( false );

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
        VERIFY( FileIO::GetTempDir( tmpPath ) );
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
    switch( ws.GetMode() )
    {
        case WorkerSettings::WHEN_IDLE:
        {
            if ( m_IdleDetection.IsIdle() == false )
            {
                numCPUsToUse = 0;
            }
            break;
        }
        case WorkerSettings::DEDICATED:
        {
            break; // use all allocated cpus
        }
        case WorkerSettings::DISABLED:
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

    m_WorkerBrokerage.SetAvailability( numCPUsToUse > 0);
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
        StatusMessage( status.Get() );
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

    /// get the current last write time
    uint64_t lastWriteTime = FileIO::GetFileLastWriteTime( m_BaseExeName );

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

// ErrorMessage
//------------------------------------------------------------------------------
void Worker::ErrorMessage( const char * fmtString, ... ) const
{
    AStackString<> buffer;

    va_list args;
    va_start(args, fmtString);
    buffer.VFormat( fmtString, args );
    va_end( args );

    if ( InConsoleMode() )
    {
        // Forward to console
        StatusMessage( "%s", buffer.Get() );
        return;
    }

    // Display interactive Message Box
    #if defined( __WINDOWS__ )
        ::MessageBox( nullptr, buffer.Get(), "FBuild Worker", MB_OK );
    #elif defined( __APPLE__ )
        // TODO:MAC Implement ErrorMessage for non-console mode
    #elif defined( __LINUX__ )
        // TODO:LINUX Implement ErrorMessage for non-console mode
    #else
        #error Unknown Platform
    #endif
}

//------------------------------------------------------------------------------
