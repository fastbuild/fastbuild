// FBuild.cpp : Defines the entry point for the console application.
//------------------------------------------------------------------------------

// Includes
//------------------------------------------------------------------------------
#include "Tools/FBuild/FBuildCore/FBuild.h"
#include "Tools/FBuild/FBuildCore/FBuildVersion.h"
#include "Tools/FBuild/FBuildCore/FLog.h"

#include "Core/Containers/Array.h"
#include "Core/Env/Env.h"
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
    #include <windows.h> 
#endif

// Return Codes
//------------------------------------------------------------------------------
enum ReturnCodes
{
	FBUILD_OK								= 0,
	FBUILD_BUILD_FAILED						= -1,
	FBUILD_ERROR_LOADING_BFF				= -2,
	FBUILD_BAD_ARGS							= -3,
	FBUILD_ALREADY_RUNNING					= -4,
	FBUILD_FAILED_TO_SPAWN_WRAPPER			= -5,
	FBUILD_FAILED_TO_SPAWN_WRAPPER_FINAL	= -6
};

// Headers
//------------------------------------------------------------------------------
void DisplayHelp();
void DisplayVersion();
#if defined( __WINDOWS__ )
    BOOL CtrlHandler( DWORD fdwCtrlType ); // Handle Ctrl+C etc
#else
    // TODO:MAC Implement CtrlHandler
    // TODO:LINUX Implement CtrlHandler
#endif
int WrapperMainProcess( const AString & args, const FBuildOptions & options, SystemMutex & finalProcess );
int WrapperIntermediateProcess( const AString & args, const FBuildOptions & options );
int Main(int argc, char * argv[]);

// Misc
//------------------------------------------------------------------------------
enum WrapperMode
{
	WRAPPER_MODE_NONE,
	WRAPPER_MODE_MAIN_PROCESS,
	WRAPPER_MODE_INTERMEDIATE_PROCESS,
	WRAPPER_MODE_FINAL_PROCESS
};

// data passed between processes in "wrapper" mode
struct SharedData
{
	bool	Started;
	int		ReturnCode;
};

// Global
//------------------------------------------------------------------------------
SharedMemory g_SharedMemory;

// main
//------------------------------------------------------------------------------
int main(int argc, char * argv[])
{
    // This wrapper is purely for profiling scope
    int result = Main( argc, argv );
    PROFILE_SYNCHRONIZE // make sure no tags are active and do one final sync
    return result;
}

// Main
//------------------------------------------------------------------------------
int Main(int argc, char * argv[])
{
    PROFILE_FUNCTION

	Timer t;

    #if defined( __WINDOWS__ )
        VERIFY( SetConsoleCtrlHandler( (PHANDLER_ROUTINE)CtrlHandler, TRUE ) ); // Register
    #endif

	// handle cmd line args
	Array< AString > targets( 8, true );
	bool cleanBuild = false;
	bool verbose = false;
	bool progressBar = true;
	bool useCacheRead = false;
	bool useCacheWrite = false;
	bool allowDistributed = false;
	bool showCommands = false;
	bool showSummary = false;
	bool report = false;
	bool fixupErrorPaths = false;
	bool waitMode = false;
	int32_t numWorkers = -1;
	WrapperMode wrapperMode( WRAPPER_MODE_NONE );
	AStackString<> args;
	const char * configFile = nullptr;
	for ( int32_t i=1; i<argc; ++i ) // start from 1 to skip exe name
	{
		AStackString<> thisArg( argv[ i ] );
		args += thisArg;
		args += ' ';

		// options start with a '-'
		if ( thisArg.BeginsWith( '-' ) )
		{
			if ( thisArg == "-cache" )
			{
				useCacheRead = true;
				useCacheWrite = true;
				continue;
			}
			else if ( thisArg == "-cacheread" )
			{
				useCacheRead = true;
				continue;
			}
			else if ( thisArg == "-cachewrite" )
			{
				useCacheWrite = true;
				continue;
			}
			else if ( thisArg == "-clean" )
			{
				cleanBuild = true;
				continue;
			}
			else if ( thisArg == "-config" )
			{
				int pathIndex = ( i + 1 );
				if ( pathIndex >= argc )
				{
					OUTPUT( "FBuild: Error: Missing <path> for '-config' argument\n" );
					OUTPUT( "Try \"FBuild.exe -help\"\n" );
					return FBUILD_BAD_ARGS;
				}
				configFile = argv[ pathIndex ];
				i++; // skip extra arg we've consumed

				// add to args we might pass to subprocess
				args += configFile;
				args += ' ';
				continue;
			}
			#ifdef DEBUG
				else if ( thisArg == "-debug" )
				{
					ASSERT( false && "Break due to '-debug' argument - attach debugger!" );
					continue;
				}
			#endif
			else if ( thisArg == "-dist" )
			{
				allowDistributed = true;
				continue;
			}
			else if ( thisArg == "-fixuperrorpaths" )
			{
				fixupErrorPaths = true;
				continue;
			}
			else if ( thisArg == "-help" )
			{
				DisplayHelp();
				return FBUILD_OK; // exit app
			}
			else if ( thisArg.BeginsWith( "-j" ) &&
					  sscanf( thisArg.Get(), "-j%i", &numWorkers ) == 1 )
			{
				continue; // 'numWorkers' will contain value now
			}
			else if ( thisArg == "-nooutputbuffering" )
			{
				// this doesn't do anything any more
				OUTPUT( "FBuild: Warning: -nooutputbuffering is deprecated." );
				continue;
			}
			else if ( thisArg == "-noprogress" )
			{
				progressBar = false;
				continue;
			}
			else if ( thisArg == "-report" )
			{
				report = true;
				continue;
			}
			else if ( thisArg == "-showcmds" )
			{
				showCommands = true;
				continue;
			}
			else if ( thisArg == "-summary" )
			{
				showSummary = true;
				continue;
			}
			else if ( thisArg == "-verbose" ) 
			{
				verbose = true;
				continue;
			}
			else if ( thisArg == "-version" ) 
			{
				DisplayVersion();
				return FBUILD_OK; // exit app
			}
			else if ( ( thisArg == "-ide" ) || ( thisArg == "-vs" ) )
			{
				progressBar = false;
				fixupErrorPaths = true;
                #if defined( __WINDOWS__ )
                    wrapperMode = WRAPPER_MODE_MAIN_PROCESS;
                #endif
				continue;
			}
			else if ( thisArg == "-wait" )
			{
				waitMode = true;
				continue;
			}
   			else if ( thisArg == "-wrapper")
    		{
                #if defined( __WINDOWS__ )
		    		wrapperMode = WRAPPER_MODE_MAIN_PROCESS;
                #endif
		    	continue;
   			}
			else if ( thisArg == "-wrapperintermediate")
			{
                #if defined( __WINDOWS__ )
    				wrapperMode = WRAPPER_MODE_INTERMEDIATE_PROCESS;
                #endif
				continue;
			}
			else if ( thisArg == "-wrapperfinal")
			{
                #if defined( __WINDOWS__ )
    				wrapperMode = WRAPPER_MODE_FINAL_PROCESS;
                #endif
				continue;
			}

			// can't use FLOG_ERROR as FLog is not initialized
			OUTPUT( "FBuild: Error: Unknown argument '%s'\n", thisArg.Get() );
			OUTPUT( "Try \"FBuild.exe -help\"\n" );
			return FBUILD_BAD_ARGS;
		}
		else
		{
			// assume target
			targets.Append( AStackString<>( thisArg ) );
		}
	}

	// cache mode environment variable (if not supplied on cmd line)
	if ( ( useCacheRead == false ) && ( useCacheWrite == false ) )
	{
		AStackString<> cacheMode;
		if ( Env::GetEnvVariable( "FASTBUILD_CACHE_MODE", cacheMode ) )
		{
			if ( cacheMode == "r" )
			{
				useCacheRead = true;
			}
			else if ( cacheMode == "w" )
			{
				useCacheWrite = true;
			}
			else if ( cacheMode == "rw" )
			{
				useCacheRead = true;
				useCacheWrite = true;
			}
			else
			{
				OUTPUT( "FASTBUILD_CACHE_MODE is invalid (%s)\n", cacheMode.Get() );
				return FBUILD_BAD_ARGS;
			}
		}
	}

    // Global mutex names depend on workingDir which is managed by FBuildOptions
    FBuildOptions options;

	if ( wrapperMode == WRAPPER_MODE_INTERMEDIATE_PROCESS )
	{
		return WrapperIntermediateProcess( args, options );
	}

    #if defined( __WINDOWS__ )
        // TODO:MAC Implement SetPriorityClass
        // TODO:LINUX Implement SetPriorityClass
        VERIFY( SetPriorityClass( GetCurrentProcess(), BELOW_NORMAL_PRIORITY_CLASS ) );
    #endif

	// don't buffer output
	VERIFY( setvbuf(stdout, nullptr, _IONBF, 0) == 0 );
	VERIFY( setvbuf(stderr, nullptr, _IONBF, 0) == 0 );

    // ensure only one FASTBuild instance is running at a time
    SystemMutex mainProcess( options.GetMainProcessMutexName().Get() );

    // in "wrapper" mode, Main process monitors life of final process using this
    // (when main process can acquire, final process has terminated)
    SystemMutex finalProcess( options.GetFinalProcessMutexName().Get() );

	// only 1 instance running at a time
	if ( ( wrapperMode == WRAPPER_MODE_MAIN_PROCESS ) ||
		 ( wrapperMode == WRAPPER_MODE_NONE ) )
	{
		if ( mainProcess.TryLock() == false )
		{
			if ( waitMode == false )
			{
				OUTPUT( "FBuild: Error: Another instance of FASTBuild is already running in '%s'.", options.GetWorkingDir().Get() );
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

	if ( wrapperMode == WRAPPER_MODE_MAIN_PROCESS )
	{
		return WrapperMainProcess( args, options, finalProcess );
	}

	ASSERT( ( wrapperMode == WRAPPER_MODE_NONE ) ||
		    ( wrapperMode == WRAPPER_MODE_FINAL_PROCESS ) );

	SharedData * sharedData = nullptr;
	if ( wrapperMode == WRAPPER_MODE_FINAL_PROCESS )
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

	options.m_ShowProgress = progressBar;
	options.m_ShowInfo = verbose;
	options.m_ShowCommandLines = showCommands;
	options.m_UseCacheRead = useCacheRead;
	options.m_UseCacheWrite = useCacheWrite;
	if ( numWorkers >= 0 )
	{
		options.m_NumWorkerThreads = numWorkers;
	}
	options.m_ForceCleanBuild = cleanBuild; 
	options.m_AllowDistributed = allowDistributed;
	options.m_ShowSummary = showSummary;
	if ( configFile )
	{
		options.m_ConfigFile = configFile;
	}
	options.m_SaveDBOnCompletion = true;
	options.m_GenerateReport = report;
	options.m_WrapperChild = ( wrapperMode == WRAPPER_MODE_FINAL_PROCESS );
	options.m_FixupErrorPaths = fixupErrorPaths;
	if ( targets.GetSize() > 1 )
	{
		options.m_StopOnFirstError = false; // when building multiple targets, try to build as much as possible
	}
	FBuild fBuild( options );

	if ( targets.IsEmpty() )
	{
		FLOG_INFO( "No target specified, defaulting to target 'all'" );
		targets.Append( AString( "all" ) );
	}

	// load the dependency graph if available
	if ( !fBuild.Initialize( FBuild::GetDependencyGraphFileName() ) )
	{
		if ( sharedData )
		{
			sharedData->ReturnCode = FBUILD_ERROR_LOADING_BFF;
		}
		return FBUILD_ERROR_LOADING_BFF;
	}

	bool result = fBuild.Build( targets );

	if ( sharedData )
	{
		sharedData->ReturnCode = ( result == true ) ? FBUILD_OK : FBUILD_BUILD_FAILED;
	}

    // final line of output - status of build
    float totalBuildTime = t.GetElapsed(); // FBuildStats::GetTotalBuildTimeS();
	uint32_t minutes = uint32_t( totalBuildTime / 60.0f );
	totalBuildTime -= ( minutes * 60.0f );
	float seconds = totalBuildTime;
	if ( minutes > 0 )
	{
		FLOG_BUILD( "Time: %um %05.3fs\n", minutes, seconds );
	}
	else
	{
		FLOG_BUILD( "Time: %05.3fs\n", seconds );
	}

	return ( result == true ) ? FBUILD_OK : FBUILD_BUILD_FAILED;
}

// DisplayHelp
//------------------------------------------------------------------------------
void DisplayHelp()
{
	DisplayVersion();
	OUTPUT( "----------------------------------------------------------------------\n"
			"Usage: fbuild.exe [options] [target1]..[targetn]\n"
			"----------------------------------------------------------------------\n"
			"Options:\n"
			" -cache[read|write] Control use of the build cache.\n"
			" -clean	     Force a clean build.\n"
			" -config [path] Explicitly specify the config file to use\n" );
#ifdef DEBUG
	OUTPUT( " -debug         Break at startup, to attach debugger.\n" );
#endif
	OUTPUT( " -dist          Allow distributed compilation.\n"
	        " -fixuperrorpaths Reformat error paths to be VisualStudio friendly.\n"
			" -help          Show this help.\n"
            " -ide           Enable multiple options when building from an IDE.\n"
            "                Enables: -noprogress, -fixuperrorpaths &\n"
			"                -wrapper (Windows)\n"
			" -jX            Explicitly set worker thread count X, instead of\n"
			"                default of NUMBER_OF_PROCESSORS. Set to 0 to build\n"
			"                everything in the main thread.\n"
			" -noprogress    Don't show the progress bar while building.\n"
			" -report        Ouput a detailed report at the end of the build,\n"
			"                to report.html.  This will lengthen the total build\n"
			"                time.\n"
			" -showcmds      Show command lines used to launch external processes.\n"
			" -summary       Show a summary at the end of the build.\n"
			" -verbose       Show detailed diagnostic information. This will slow\n"
			"                down building.\n"
			" -version       Print version and exit. No other work will be\n"
			"                performed.\n"
			" -vs            VisualStudio mode. Same as -ide.\n"
			" -wait          Wait for a previous build to complete before starting.\n"
			"                (Slower than building both targets in one invocation).\n"
			" -wrapper       (Windows only) Spawn a sub-process to gracefully handle\n"
            "                termination from Visual Studio.\n"
			"----------------------------------------------------------------------\n" );
}

// DisplayVersion
//------------------------------------------------------------------------------
void DisplayVersion()
{
	#ifdef DEBUG
		#define VERSION_CONFIG " (DEBUG) "
	#else
		#define VERSION_CONFIG " "
	#endif
	OUTPUT( "FASTBuild - " FBUILD_VERSION_STRING " " FBUILD_VERSION_PLATFORM VERSION_CONFIG " - "
			"Copyright 2012-2015 Franta Fulin - http://www.fastbuild.org\n" );
	#undef VERSION_CONFIG
}

// CtrlHandler
//------------------------------------------------------------------------------
#if defined( __WINDOWS__ )
	BOOL CtrlHandler( DWORD UNUSED( fdwCtrlType ) ) 
	{
		// tell FBuild we want to stop the build cleanly
		FBuild::AbortBuild();

		// only printf output for the first break received
		static bool received = false;
		if ( received == false )
		{
			received = true;

			// get the console colours
			CONSOLE_SCREEN_BUFFER_INFO consoleInfo;
			VERIFY( GetConsoleScreenBufferInfo( GetStdHandle( STD_OUTPUT_HANDLE ), &consoleInfo ) );

			// print a big red msg
			VERIFY( SetConsoleTextAttribute( GetStdHandle( STD_OUTPUT_HANDLE ), FOREGROUND_RED ) );
			OUTPUT( "<<<< ABORT SIGNAL RECEIVED >>>>\n" );

			// put the console back to normal
			VERIFY( SetConsoleTextAttribute( GetStdHandle( STD_OUTPUT_HANDLE ), consoleInfo.wAttributes ) );
		}

		return TRUE; // tell Windows we've "handled" it
	}
#endif

// WrapperMainProcess
//------------------------------------------------------------------------------
int WrapperMainProcess( const AString & args, const FBuildOptions & options, SystemMutex & finalProcess )
{
	// Create SharedMemory to communicate between Main and Final process
	SharedMemory sm;
	g_SharedMemory.Create( options.GetSharedMemoryName().Get(), sizeof( SharedData ) );
	SharedData * sd = (SharedData *)g_SharedMemory.GetPtr();
	memset( sd, 0, sizeof( SharedData ) );

	// launch intermediate process
	AStackString<> argsCopy( args );
	argsCopy += " -wrapperintermediate";

	Process p;
	if ( !p.Spawn( "fbuild.exe", argsCopy.Get(), options.GetWorkingDir().Get(), nullptr, true ) ) // true = forward output to our tty
	{
		return FBUILD_FAILED_TO_SPAWN_WRAPPER;
	}

	// the intermediate process will exit immediately after launching the final
	// process
	p.WaitForExit();

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
int WrapperIntermediateProcess( const AString & args, const FBuildOptions & options  )
{
	// launch final process
	AStackString<> argsCopy( args );
	argsCopy += " -wrapperfinal";

	Process p;
	if ( !p.Spawn( "fbuild.exe", argsCopy.Get(), options.GetWorkingDir().Get(), nullptr, true ) ) // true = forward output to our tty
	{
		return FBUILD_FAILED_TO_SPAWN_WRAPPER_FINAL;
	}

	// don't wait for the final process (the main process will do that)
	p.Detach();
	return FBUILD_OK;
}

//------------------------------------------------------------------------------
