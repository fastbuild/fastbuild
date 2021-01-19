// FBuild.cpp : Defines the entry point for the console application.
//------------------------------------------------------------------------------

// Includes
//------------------------------------------------------------------------------
#include "Tools/FBuild/FBuildCore/FBuild.h"
#include "Tools/FBuild/FBuildCore/FLog.h"
#include "Tools/FBuild/FBuildCore/Helpers/BuildProfiler.h"
#include "Tools/FBuild/FBuildCore/Helpers/CtrlCHandler.h"

#include "Core/Process/Process.h"
#include "Core/Process/SharedMemory.h"
#include "Core/Process/SystemMutex.h"
#include "Core/Process/Thread.h"
#include "Core/Profile/Profile.h"
#include "Core/Strings/AStackString.h"
#include "Core/Tracing/Tracing.h"

#include <memory.h>
#include <stdio.h>
#if defined( __WINDOWS__ )
    #include "Core/Env/WindowsHeader.h"
#endif

// Return Codes
//------------------------------------------------------------------------------
enum ReturnCodes
{
    FBUILD_OK                               = 0,
    FBUILD_BUILD_FAILED                     = -1,
    FBUILD_ERROR_LOADING_BFF                = -2,
    FBUILD_BAD_ARGS                         = -3,
    FBUILD_ALREADY_RUNNING                  = -4,
    FBUILD_FAILED_TO_SPAWN_WRAPPER          = -5,
    FBUILD_FAILED_TO_SPAWN_WRAPPER_FINAL    = -6,
    FBUILD_WRAPPER_CRASHED                  = -7,
    FBUILD_FAILED_TO_WSL_WRAPPER            = -8,
    FBUILD_FAILED_TO_WRITE_PROFILE_JSON     = -9,
};

// Headers
//------------------------------------------------------------------------------
int WrapperMainProcess( const AString & args, const FBuildOptions & options, SystemMutex & finalProcess );
int WrapperIntermediateProcess( const FBuildOptions & options );
int32_t WrapperModeForWSL( const FBuildOptions & options );
int Main( int argc, char * argv[] );

// Misc
//------------------------------------------------------------------------------
// data passed between processes in "wrapper" mode
struct SharedData
{
    bool    Started;
    int     ReturnCode;
};

// Global
//------------------------------------------------------------------------------
SharedMemory g_SharedMemory;

// main
//------------------------------------------------------------------------------
int main( int argc, char * argv[] )
{
    // This wrapper is purely for profiling scope
    int result = Main( argc, argv );
    PROFILE_SYNCHRONIZE // make sure no tags are active and do one final sync
    return result;
}

// Main
//------------------------------------------------------------------------------
int Main( int argc, char * argv[] )
{
    PROFILE_FUNCTION;

    Timer t;

    // Register Ctrl-C Handler
    CtrlCHandler ctrlCHandler;

    // handle cmd line args
    FBuildOptions options;
    options.m_SaveDBOnCompletion = true; // Override default
    options.m_ShowProgress = true; // Override default
    switch ( options.ProcessCommandLine( argc, argv ) )
    {
        case FBuildOptions::OPTIONS_OK:             break;
        case FBuildOptions::OPTIONS_OK_AND_QUIT:    return FBUILD_OK;
        case FBuildOptions::OPTIONS_ERROR:          return FBUILD_BAD_ARGS;
    }

    const FBuildOptions::WrapperMode wrapperMode = options.m_WrapperMode;
    if ( wrapperMode == FBuildOptions::WRAPPER_MODE_INTERMEDIATE_PROCESS )
    {
        return WrapperIntermediateProcess( options );
    }
    if ( wrapperMode == FBuildOptions::WRAPPER_MODE_WINDOWS_SUBSYSTEM_FOR_LINUX )
    {
        return WrapperModeForWSL( options );
    }

    #if defined( __WINDOWS__ )
        // TODO:MAC Implement SetPriorityClass
        // TODO:LINUX Implement SetPriorityClass
        VERIFY( SetPriorityClass( GetCurrentProcess(), BELOW_NORMAL_PRIORITY_CLASS ) );
    #endif

    // don't buffer output
    VERIFY( setvbuf( stdout, nullptr, _IONBF, 0 ) == 0 );
    VERIFY( setvbuf( stderr, nullptr, _IONBF, 0 ) == 0 );

    // ensure only one FASTBuild instance is running at a time
    SystemMutex mainProcess( options.GetMainProcessMutexName().Get() );

    // in "wrapper" mode, Main process monitors life of final process using this
    // (when main process can acquire, final process has terminated)
    SystemMutex finalProcess( options.GetFinalProcessMutexName().Get() );

    // only 1 instance running at a time
    if ( ( wrapperMode == FBuildOptions::WRAPPER_MODE_MAIN_PROCESS ) ||
         ( wrapperMode == FBuildOptions::WRAPPER_MODE_NONE ) )
    {
        if ( mainProcess.TryLock() == false )
        {
            if ( options.m_WaitMode == false )
            {
                OUTPUT( "FBuild: Error: Another instance of FASTBuild is already running in '%s'.\n", options.GetWorkingDir().Get() );
                return FBUILD_ALREADY_RUNNING;
            }

            OUTPUT( "FBuild: Waiting for another FASTBuild to terminate due to -wait option.\n" );
            while( mainProcess.TryLock() == false )
            {
                Thread::Sleep( 1000 );
                if ( FBuild::GetStopBuild() )
                {
                    return FBUILD_BUILD_FAILED;
                }
            }
        }
    }

    if ( wrapperMode == FBuildOptions::WRAPPER_MODE_MAIN_PROCESS )
    {
        return WrapperMainProcess( options.m_Args, options, finalProcess );
    }

    ASSERT( ( wrapperMode == FBuildOptions::WRAPPER_MODE_NONE ) ||
            ( wrapperMode == FBuildOptions::WRAPPER_MODE_FINAL_PROCESS ) );

    SharedData * sharedData = nullptr;
    if ( wrapperMode == FBuildOptions::WRAPPER_MODE_FINAL_PROCESS )
    {
        while ( !finalProcess.TryLock() )
        {
            OUTPUT( "FBuild: Waiting for another FASTBuild to terminate...\n" );
            if ( mainProcess.TryLock() )
            {
                // main process has aborted, terminate
                return FBUILD_FAILED_TO_SPAWN_WRAPPER_FINAL;
            }
            Thread::Sleep( 1000 );
        }

        g_SharedMemory.Open( options.GetSharedMemoryName().Get(), sizeof( SharedData ) );

        // signal to "main" process that we have started
        sharedData = (SharedData *)g_SharedMemory.GetPtr();
        if ( sharedData == nullptr )
        {
            // main process was killed while we were waiting
            return FBUILD_FAILED_TO_SPAWN_WRAPPER_FINAL;
        }
        sharedData->Started = true;
    }

    FBuild fBuild( options );

    // load the dependency graph if available
    if ( !fBuild.Initialize() )
    {
        if ( sharedData )
        {
            sharedData->ReturnCode = FBUILD_ERROR_LOADING_BFF;
        }
        ctrlCHandler.DeregisterHandler(); // Ensure this happens before FBuild is destroyed
        return FBUILD_ERROR_LOADING_BFF;
    }

    bool result = false;
    if ( options.m_DisplayTargetList )
    {
        fBuild.DisplayTargetList( options.m_ShowHiddenTargets );
        result = true; // DisplayTargetList cannot fail
    }
    else if ( options.m_DisplayDependencyDB )
    {
        result = fBuild.DisplayDependencyDB( options.m_Targets );
    }
    else if ( options.m_GenerateDotGraph )
    {
        result = fBuild.GenerateDotGraph( options.m_Targets, options.m_GenerateDotGraphFull );
    }
    else if ( options.m_GenerateCompilationDatabase )
    {
        result = fBuild.GenerateCompilationDatabase( options.m_Targets );
    }
    else if ( options.m_CacheInfo )
    {
        result = fBuild.CacheOutputInfo();
    }
    else if ( options.m_CacheTrim )
    {
        result = fBuild.CacheTrim();
    }
    else
    {
        result = fBuild.Build( options.m_Targets );
    }


    // Build Profiling enabled?
    bool problemSavingBuildProfileJSON = false;
    if ( options.m_Profile )
    {
        if ( BuildProfiler::Get().SaveJSON( options, "fbuild_profile.json" ) == false )
        {
            problemSavingBuildProfileJSON = true;
        }
    }

    if ( sharedData )
    {
        sharedData->ReturnCode = ( result == true ) ? FBUILD_OK : FBUILD_BUILD_FAILED;
    }

    // final line of output - status of build
    if ( options.m_ShowTotalTimeTaken )
    {
        float totalBuildTime = t.GetElapsed();
        const uint32_t minutes = uint32_t( totalBuildTime / 60.0f );
        const float seconds = ( totalBuildTime - (float)( minutes * 60 ) );
        if ( minutes > 0 )
        {
            FLOG_OUTPUT( "Time: %um %05.3fs\n", minutes, (double)seconds );
        }
        else
        {
            FLOG_OUTPUT( "Time: %05.3fs\n", (double)seconds );
        }
    }

    ctrlCHandler.DeregisterHandler(); // Ensure this happens before FBuild is destroyed

    if ( problemSavingBuildProfileJSON )
    {
        return FBUILD_FAILED_TO_WRITE_PROFILE_JSON;
    }    
    return ( result == true ) ? FBUILD_OK : FBUILD_BUILD_FAILED;
}

// WrapperMainProcess
//------------------------------------------------------------------------------
int WrapperMainProcess( const AString & args, const FBuildOptions & options, SystemMutex & finalProcess )
{
    // Create SharedMemory to communicate between Main and Final process
    SharedMemory sm;
    g_SharedMemory.Create( options.GetSharedMemoryName().Get(), sizeof( SharedData ) );
    SharedData * sd = (SharedData *)g_SharedMemory.GetPtr();
    memset( sd, 0, sizeof( SharedData ) );
    sd->ReturnCode = FBUILD_WRAPPER_CRASHED;

    // launch intermediate process
    AStackString<> argsCopy( args );
    argsCopy += " -wrapperintermediate";

    Process p;
    if ( !p.Spawn( options.m_ProgramName.Get(), argsCopy.Get(), options.GetWorkingDir().Get(), nullptr, true ) ) // true = forward output to our tty
    {
        return FBUILD_FAILED_TO_SPAWN_WRAPPER;
    }

    // the intermediate process will exit immediately after launching the final
    // process
    const int32_t result = p.WaitForExit();
    if ( result == FBUILD_FAILED_TO_SPAWN_WRAPPER_FINAL )
    {
        OUTPUT( "FBuild: Error: Intermediate process failed to spawn the final process.\n" );
        return result;
    }
    else if ( result != FBUILD_OK )
    {
        OUTPUT( "FBuild: Error: Intermediate process failed (%i).\n", result );
        return result;
    }

    // wait for final process to signal as started
    while ( sd->Started == false )
    {
        Thread::Sleep( 1 );
    }

    // wait for final process to exit
    for ( ;; )
    {
        if ( finalProcess.TryLock() == true )
        {
            break; // final process has released the mutex
        }
        Thread::Sleep( 1 );
    }

    return sd->ReturnCode;
}

// WrapperIntermediateProcess
//------------------------------------------------------------------------------
int WrapperIntermediateProcess( const FBuildOptions & options )
{
    // launch final process
    AStackString<> argsCopy( options.m_Args );
    argsCopy += " -wrapperfinal";

    Process p;
    if ( !p.Spawn( options.m_ProgramName.Get(), argsCopy.Get(), options.GetWorkingDir().Get(), nullptr, true ) ) // true = forward output to our tty
    {
        return FBUILD_FAILED_TO_SPAWN_WRAPPER_FINAL;
    }

    // don't wait for the final process (the main process will do that)
    p.Detach();
    return FBUILD_OK;
}

// WrapperModeForWSL
//------------------------------------------------------------------------------
int32_t WrapperModeForWSL( const FBuildOptions & options )
{
    // launch final process
    Process p;
    if ( !p.Spawn( options.m_WSLPath.Get(), options.m_Args.Get(), options.GetWorkingDir().Get(), nullptr, true ) ) // true = forward output to our tty
    {
        return FBUILD_FAILED_TO_WSL_WRAPPER;
    }

    // Return the result from the WSL process, which will itself forward the
    // result of the target process
    return p.WaitForExit();
}

//------------------------------------------------------------------------------
