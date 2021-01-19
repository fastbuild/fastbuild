// Worker
//------------------------------------------------------------------------------

// Includes
//------------------------------------------------------------------------------
#if defined( __WINDOWS__ )
    #include <WinSock2.h> // this must be here to avoid windows include order problems
#endif

#include "Worker.h"

// FBuildWorker
#include "Tools/FBuild/FBuildWorker/Worker/WorkerSettings.h"
#include "Tools/FBuild/FBuildWorker/Worker/WorkerWindow.h"

// FBuild
#include "Tools/FBuild/FBuildCore/FBuild.h"
#include "Tools/FBuild/FBuildCore/FBuildVersion.h"
#include "Tools/FBuild/FBuildCore/Protocol/Protocol.h"
#include "Tools/FBuild/FBuildCore/Protocol/Server.h"
#include "Tools/FBuild/FBuildCore/WorkerPool/JobQueueRemote.h"
#include "Tools/FBuild/FBuildCore/WorkerPool/WorkerThreadRemote.h"

// Core
#include "Core/Env/Env.h"
#include "Core/Env/ErrorFormat.h"
#include "Core/Env/Types.h"
#include "Core/FileIO/FileIO.h"
#include "Core/Network/NetworkStartupHelper.h"
#include "Core/Process/Process.h"
#include "Core/Profile/Profile.h"
#include "Core/Strings/AStackString.h"
#include "Core/Tracing/Tracing.h"

// system
#if defined( __WINDOWS__ )
    #include <Psapi.h>
#endif
#include <stdio.h>

// CONSTRUCTOR
//------------------------------------------------------------------------------
Worker::Worker( const AString & args, bool consoleMode )
    : m_ConsoleMode( consoleMode )
    , m_MainWindow( nullptr )
    , m_ConnectionPool( nullptr )
    , m_NetworkStartupHelper( nullptr )
    , m_BaseArgs( args )
    , m_LastWriteTime( 0 )
    , m_WantToQuit( false )
    , m_RestartNeeded( false )
    #if defined( __WINDOWS__ )
        , m_LastDiskSpaceResult( -1 )
        , m_LastMemoryCheckResult( -1 )
    #endif
{
    m_WorkerSettings = FNEW( WorkerSettings );
    m_NetworkStartupHelper = FNEW( NetworkStartupHelper );
    m_ConnectionPool = FNEW( Server );

    Env::GetExePath( m_BaseExeName );
    #if defined( __WINDOWS__ )
        if ( m_BaseExeName.Replace( ".copy", "" ) != 1 )
        {
            m_BaseExeName.Clear(); // not running from copy, disable restart detection
        }
        m_BaseArgs.Replace( "-subprocess", "" );
    #endif
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
int32_t Worker::Work()
{
    PROFILE_FUNCTION;

    // Open GUI or setup console
    if ( InConsoleMode() )
    {
        #if __WINDOWS__
            VERIFY( ::AllocConsole() );
            PRAGMA_DISABLE_PUSH_MSVC( 4996 ) // This function or variable may be unsafe...
            PRAGMA_DISABLE_PUSH_CLANG_WINDOWS( "-Wdeprecated-declarations" ) // 'freopen' is deprecated: This function or variable may be unsafe...
            VERIFY( freopen( "CONOUT$", "w", stdout ) ); // TODO:C consider using freopen_s
            PRAGMA_DISABLE_POP_CLANG_WINDOWS // -Wdeprecated-declarations
            PRAGMA_DISABLE_POP_MSVC // 4996
        #endif

        // TODO: Block until Ctrl+C
    }
    else
    {
        // Create UI
        m_MainWindow = FNEW( WorkerWindow() );
        m_MainWindow->SetStatus( m_WorkerBrokerage.GetHostName(), AStackString<>( "Idle" ) );
    }

    // spawn work thread
    m_WorkThread = Thread::CreateThread( &WorkThreadWrapper,
                                         "WorkerThread",
                                         ( 256 * KILOBYTE ),
                                         this );
    ASSERT( m_WorkThread != INVALID_THREAD_HANDLE );

    // Run the UI message loop if we're not in console mode
    if ( m_MainWindow )
    {
        m_MainWindow->Work(); // Blocks until exit
    }

    // Join work thread and get exit code
    return Thread::WaitForThread( m_WorkThread );
}

// WorkThreadWrapper
//------------------------------------------------------------------------------
/*static*/ uint32_t Worker::WorkThreadWrapper( void * userData )
{
    PROFILE_SET_THREAD_NAME( "WorkerThread" );

    Worker * worker = reinterpret_cast<Worker *>( userData );
    return worker->WorkThread();
}

// WorkThread
//------------------------------------------------------------------------------
uint32_t Worker::WorkThread()
{
    PROFILE_FUNCTION;

    // Initial status message
    StatusMessage( "FBuildWorker %s", FBUILD_VERSION_STRING );

    // start listening
    StatusMessage( "Listening on port %u\n", Protocol::PROTOCOL_PORT );
    if ( m_ConnectionPool->Listen( Protocol::PROTOCOL_PORT ) == false )
    {
        ErrorMessage( "Failed to listen on port %u.  Check port is not in use.", Protocol::PROTOCOL_PORT );
        return (uint32_t)-1;
    }

    // Special folder for Orbis Clang
    // We just create this folder whether it's needed or not
    {
        AStackString<> tmpPath;
        VERIFY( FBuild::GetTempDir( tmpPath ) );
        #if defined( __WINDOWS__ )
            tmpPath += ".fbuild.tmp\\target\\include";
        #else
            tmpPath += "_fbuild.tmp/target/include";
        #endif
        if ( !FileIO::EnsurePathExists( tmpPath ) )
        {
            ErrorMessage( "Failed to initialize tmp folder. Error: %s", LAST_ERROR_STR );
            return (uint32_t)-2;
        }
        #if defined( __WINDOWS__ )
            tmpPath += "\\.lock";
        #else
            tmpPath += "/_lock";
        #endif
        if ( !m_TargetIncludeFolderLock.Open( tmpPath.Get(), FileStream::WRITE_ONLY ) )
        {
            ErrorMessage( "Failed to lock tmp folder. Error: %s", LAST_ERROR_STR );
            return (uint32_t)-2;
        }
    }

    // Main Loop
    for ( ;; )
    {
        UpdateAvailability();

        UpdateUI();

        CheckForExeUpdate();

        PROFILE_SYNCHRONIZE

        // Check if we want to exit
        if ( m_WantToQuit )
        {
            break;
        }

        Thread::Sleep( 500 );
    }

    // Now that we will no longer interact with the UI, we can stop the message pump
    m_MainWindow->StopMessagePump();

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

        static constexpr uint64_t MIN_DISK_SPACE = 1024 * 1024 * 1024; // 1 GiB

        uint64_t freeBytesAvailable = 0;
        uint64_t totalNumberOfBytes = 0;
        uint64_t totalNumberOfFreeBytes = 0;

        // Check available disk space of temp path
        AStackString<> tmpPath;
        VERIFY( FBuild::GetTempDir( tmpPath ) );
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

// HasEnoughMemory
//------------------------------------------------------------------------------
bool Worker::HasEnoughMemory()
{
    #if defined( __WINDOWS__ )
        // Only check free memory every few seconds
        float elapsedTime = m_TimerLastMemoryCheck.GetElapsedMS();
        if ( ( elapsedTime < 1000.0f ) && ( m_LastMemoryCheckResult != -1 ) )
        {
            return ( m_LastMemoryCheckResult != 0 );
        }
        m_TimerLastMemoryCheck.Start();
    
        PERFORMANCE_INFORMATION memInfo;
        memInfo.cb = sizeof( memInfo );
        if ( GetPerformanceInfo( &memInfo, sizeof( memInfo ) ) )
        {
            const uint64_t limitMemSize = memInfo.CommitLimit * memInfo.PageSize;
            const uint64_t currentMemSize = memInfo.CommitTotal * memInfo.PageSize;
    
            // Calculate the free memory in MiB.
            const uint64_t freeMemSize = ( limitMemSize - currentMemSize ) / MEGABYTE;
    
            // Check if the free memory is high enough
            WorkerSettings & ws = WorkerSettings::Get();
            if ( freeMemSize > ws.GetMinimumFreeMemoryMiB() )
            {
                m_LastMemoryCheckResult = 1;
                return true;
            }
        }
    
        // The machine doesn't have enough memory or query failed. Exclude this machine from worker pool.
        m_LastMemoryCheckResult = 0;
        return false;
    #else
        return true; // TODO:LINUX TODO:OSX Implement
    #endif
}

// UpdateAvailability
//------------------------------------------------------------------------------
void Worker::UpdateAvailability()
{
    PROFILE_FUNCTION;

    // Check disk space
    const bool hasEnoughDiskSpace = HasEnoughDiskSpace();
    const bool hasEnoughMemory = HasEnoughMemory();

    WorkerSettings & ws = WorkerSettings::Get();

    m_IdleDetection.Update( ws.GetIdleThresholdPercent() );

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
        case WorkerSettings::PROPORTIONAL:
        {
            if ( ( m_IdleDetection.IsIdleFloat() >= 0.0f ) && ( m_IdleDetection.IsIdleFloat() <= 1.0f ) )
            {
                numCPUsToUse = uint32_t( (float)numCPUsToUse * m_IdleDetection.IsIdleFloat() );
            }
            else
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
    if ( m_RestartNeeded || ( hasEnoughDiskSpace == false ) || ( hasEnoughMemory == false ) )
    {
        numCPUsToUse = 0;
    }

    WorkerThreadRemote::SetNumCPUsToUse( numCPUsToUse );

    m_WorkerBrokerage.SetAvailability( numCPUsToUse > 0 );
}

// UpdateUI
//------------------------------------------------------------------------------
void Worker::UpdateUI()
{
    PROFILE_FUNCTION;

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
        m_MainWindow->SetStatus( m_WorkerBrokerage.GetHostName(), status );
    }

    if ( InConsoleMode() == false )
    {
        // thread output
        JobQueueRemote & jqr = JobQueueRemote::Get();
        const size_t numWorkers = jqr.GetNumWorkers();
        for ( size_t i = 0; i < numWorkers; ++i )
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
    PROFILE_FUNCTION;

    // if a restart is pending, can we restart yet?
    if ( m_RestartNeeded )
    {
        // can we restart yet?
        if ( JobQueueRemote::Get().HaveWorkersStopped() )
        {
            m_WantToQuit = true;
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
void Worker::StatusMessage( MSVC_SAL_PRINTF const char * fmtString, ... ) const
{
    // Status Messages are only shown in console mode
    if ( InConsoleMode() == false )
    {
        return;
    }

    AStackString<> buffer;

    va_list args;
    va_start( args, fmtString );
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
void Worker::ErrorMessage( MSVC_SAL_PRINTF const char * fmtString, ... ) const
{
    AStackString<> buffer;

    va_list args;
    va_start( args, fmtString );
    buffer.VFormat( fmtString, args );
    va_end( args );

    if ( InConsoleMode() )
    {
        // Forward to console
        StatusMessage( "%s", buffer.Get() );
        return;
    }

    // Display interactive Message Box
    Env::ShowMsgBox( "FBuildWorker", buffer.Get() );
}

//------------------------------------------------------------------------------
