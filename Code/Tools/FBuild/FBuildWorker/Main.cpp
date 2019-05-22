// Main
//------------------------------------------------------------------------------

// Includes
//------------------------------------------------------------------------------
#include "FBuildWorkerOptions.h"
#include "Worker/Worker.h"
#include "Core/Env/Assert.h"
#include "Core/Env/Env.h"
#include "Core/Env/ErrorFormat.h"
#include "Core/FileIO/FileIO.h"
#include "Core/Mem/MemTracker.h"
#include "Core/Process/Process.h"
#include "Core/Process/SystemMutex.h"
#include "Core/Process/Thread.h"
#include "Core/Strings/AStackString.h"

// system
#if defined( __WINDOWS__ )
    #include "Core/Env/WindowsHeader.h"
#endif

// Global Data
//------------------------------------------------------------------------------
// only allow 1 worker per system
SystemMutex g_OneProcessMutex( "Global\\FBuildWorker" );

// Functions
//------------------------------------------------------------------------------
int MainCommon( const AString & args, void * hInstance );
#if defined( __WINDOWS__ )
    int LaunchSubProcess( const AString & args );
#endif

//------------------------------------------------------------------------------
void ShowMsgBox( const char * msg )
{
    #if defined( __WINDOWS__ )
        MessageBoxA( nullptr, msg, "FBuildWorker", MB_OK );
    #elif defined( __APPLE__ )
        (void)msg; // TODO:MAC Implement ShowMsgBox
    #elif defined( __LINUX__ )
        (void)msg; // TODO:LINUX Implement ShowMsgBox
    #else
        #error Unknown Platform
    #endif
}

//------------------------------------------------------------------------------
#if defined( __WINDOWS__ )
    PRAGMA_DISABLE_PUSH_MSVC( 28251 ) // don't complain about missing annotations on WinMain
    int WINAPI WinMain( HINSTANCE hInstance,
                        HINSTANCE /*hPrevInstance*/,
                        LPSTR lpCmdLine,
                        int /*nCmdShow*/ )
    {
        AStackString<> args( lpCmdLine );
        return MainCommon( args, hInstance );
    }
    PRAGMA_DISABLE_POP_MSVC
#else
    int main( int argc, char** argv )
    {
        AStackString<> args;
        for ( int i=1; i<argc; ++i ) // NOTE: Skip argv[0] exe name
        {
            if ( i > 0 )
            {
                args += ' ';
            }
            args += argv[ i ];
        }
        return MainCommon( args, nullptr );
    }
#endif

#include <stdio.h>

// MainCommon
//------------------------------------------------------------------------------
int MainCommon( const AString & args, void * hInstance )
{
    // don't buffer output
    VERIFY( setvbuf(stdout, nullptr, _IONBF, 0) == 0 );
    VERIFY( setvbuf(stderr, nullptr, _IONBF, 0) == 0 );

    // process cmd line args
    FBuildWorkerOptions options;
    if ( options.ProcessCommandLine( args ) == false )
    {
        return -3;
    }

    // only allow 1 worker per system
    Timer t;
    while ( g_OneProcessMutex.TryLock() == false )
    {
        // retry for upto 2 seconds, to allow some time for old worker to close
        if ( t.GetElapsed() > 5.0f )
        {
            ShowMsgBox( "An FBuildWorker is already running!" );
            return -1;
        }
        Thread::Sleep(100);
    }

    #if defined( __WINDOWS__ )
        if ( options.m_UseSubprocess && !options.m_IsSubprocess )
        {
            return LaunchSubProcess( args );
        }
    #endif

    // prevent popups when launching tools with missing dlls
    #if defined( __WINDOWS__ )
        ::SetErrorMode( SEM_FAILCRITICALERRORS );
    #else
        // TODO:MAC SetErrorMode equivalent
        // TODO:LINUX SetErrorMode equivalent
    #endif

    #if defined( __WINDOWS__ )
        VERIFY( SetPriorityClass( GetCurrentProcess(), BELOW_NORMAL_PRIORITY_CLASS ) );
    #else
        // TODO:MAC SetPriorityClass equivalent
        // TODO:LINUX SetPriorityClass equivalent
    #endif

    // start the worker and wait for it to be closed
    int ret;
    {
        // construct worker
        Worker worker( args );

        // before initializing worker, set initial worker settings
        bool anyOverrides = false;
        WorkerSettings & workerSettings = WorkerSettings::Get();
        if ( options.m_OverrideCPUAllocation )
        {
            workerSettings.SetNumCPUsToUse( options.m_CPUAllocation );
            anyOverrides = true;
        }
        if ( options.m_OverrideWorkMode )
        {
            workerSettings.SetMode( options.m_WorkMode );
            anyOverrides = true;
        }
        if ( options.m_OverrideStartMinimized )
        {
            workerSettings.SetStartMinimized( options.m_StartMinimized );
            anyOverrides = true;
        }
        if ( options.m_OverrideSandboxEnabled )
        {
            workerSettings.SetSandboxEnabled( options.m_SandboxEnabled );
            anyOverrides = true;
        }
        if ( options.m_OverrideSandboxExe )
        {
            workerSettings.SetSandboxExe( options.m_SandboxExe );
            anyOverrides = true;
        }
        if ( options.m_OverrideSandboxArgs )
        {
            workerSettings.SetSandboxArgs( options.m_SandboxArgs );
            anyOverrides = true;
        }
        if ( options.m_OverrideSandboxTmp )
        {
            workerSettings.SetSandboxTmp( options.m_SandboxTmp );
            anyOverrides = true;
        }

        if ( anyOverrides )
        {
            // save our state out to the .settings file
            workerSettings.Save();
        }

        // error check settings
        if ( workerSettings.GetSandboxEnabled() )
        {
            const AString & absSandboxExe = workerSettings.GetAbsSandboxExe();
            if ( absSandboxExe.IsEmpty() ||
                 workerSettings.GetSandboxTmp().IsEmpty() )
            {
                AStackString<> sandboxError( "To enable the sandbox, please specify a non-empty sandbox exe " );
                sandboxError += "and a non-empty sandbox tmp via the ";
                sandboxError += "command line -sandboxexe and -sandboxtmp args\n";
                ShowMsgBox( sandboxError.Get() );
                return -1;
            }
            if ( !absSandboxExe.IsEmpty() && !FileIO::FileExists( absSandboxExe.Get() ) )
            {
                AStackString<> sandboxError;
                sandboxError.Format( "sandbox executable '%s' was not found on disk\n", absSandboxExe.Get() );
                ShowMsgBox( sandboxError.Get() );
                return -1;
            }
        }
        // the remaining sandbox init logic for the worker is in Worker.cpp 
        
        // worker settings are set, so initialize the worker
        worker.Initialize( hInstance, options.m_ConsoleMode );

        // do work
        ret = worker.Work();
    }

    return ret;
}

// LaunchSubProcess
//------------------------------------------------------------------------------
#if defined( __WINDOWS__ )
    int LaunchSubProcess( const AString & args )
    {
        // try to make a copy of our exe
        AStackString<> exeName;
        Env::GetExePath( exeName );
        AStackString<> exeNameCopy( exeName );
        exeNameCopy += ".copy";
        Timer t;
        while ( FileIO::FileCopy( exeName.Get(), exeNameCopy.Get() ) == false )
        {
            if ( t.GetElapsed() > 5.0f )
            {
                AStackString<> msg;
                msg.Format( "Failed to make sub-process copy. Error: %s\n\nSrc: %s\nDst: %s\n", LAST_ERROR_STR, exeName.Get(), exeNameCopy.Get() );
                ShowMsgBox( msg.Get() );
                return -2;
            }
            Thread::Sleep( 100 );
        }

        AStackString<> argsCopy( args );
        argsCopy += " -subprocess";

        // allow subprocess to access the mutex
        g_OneProcessMutex.Unlock();

        Process p;
        #if defined( __WINDOWS__ )
            p.DisableHandleRedirection(); // TODO:MAC TODO:LINUX is this needed?
        #endif
        p.Spawn( exeNameCopy.Get(), argsCopy.Get(), nullptr, nullptr );
        p.Detach();

        return 0;
    }
#endif

//------------------------------------------------------------------------------
