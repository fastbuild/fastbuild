// Main
//------------------------------------------------------------------------------

// Includes
//------------------------------------------------------------------------------
#include "FBuildWorkerOptions.h"
#include "Worker/Worker.h"
#include "Core/Env/Assert.h"
#include "Core/Env/Env.h"
#include "Core/FileIO/FileIO.h"
#include "Core/Mem/MemTracker.h"
#include "Core/Process/Process.h"
#include "Core/Process/SystemMutex.h"
#include "Core/Process/Thread.h"
#include "Core/Strings/AStackString.h"

// system
#if defined( __WINDOWS__ )
    #include <windows.h>
#endif

// Global Data
//------------------------------------------------------------------------------
// only allow 1 worker per system
SystemMutex g_OneProcessMutex( "Global\\FBuildWorker" );

// Functions
//------------------------------------------------------------------------------
int MainCommon( const AString & args, void * hInstance );
int LaunchSubProcess( const AString & args );

//------------------------------------------------------------------------------
void ShowMsgBox( const char * msg )
{
    #if defined( __WINDOWS__ )
        MessageBoxA( nullptr, msg, "FBuildWorker", MB_OK );
    #elif defined( __APPLE__ )
        // TODO:MAC Implement ShowMsgBox
    #elif defined( __LINUX__ )
        // TODO:LINUX Implement ShowMsgBox
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
        for ( int i=0; i<argc; ++argc )
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

// MainCommon
//------------------------------------------------------------------------------
int MainCommon( const AString & args, void * hInstance )
{
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

	if ( options.m_UseSubprocess && !options.m_IsSubprocess )
	{
		return LaunchSubProcess( args );
	}

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
		Worker worker( hInstance, args );
		if ( options.m_OverrideCPUAllocation )
		{
			WorkerSettings::Get().SetNumCPUsToUse( options.m_CPUAllocation );
		}
		if ( options.m_OverrideWorkMode )
		{
			WorkerSettings::Get().SetMode( options.m_WorkMode );
		}
		ret = worker.Work();
	}

	MEMTRACKER_DUMP_ALLOCATIONS

	return ret;
}

// LaunchSubProcess
//------------------------------------------------------------------------------
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
			msg.Format( "Failed to make sub-process copy - error: %u (0x%x)\n\nSrc: %s\nDst: %s\n", Env::GetLastErr(), Env::GetLastErr(), exeName.Get(), exeNameCopy.Get() );
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

//------------------------------------------------------------------------------
