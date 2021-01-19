// FBuild - the main application
//------------------------------------------------------------------------------

// Includes
//------------------------------------------------------------------------------
// FBuildCore
#include "FBuildOptions.h"
#include "Tools/FBuild/FBuildCore/FBuildVersion.h"
#include "Tools/FBuild/FBuildCore/FLog.h"

// Core
#include "Core/Env/Env.h"
#include "Core/FileIO/FileIO.h"
#include "Core/FileIO/PathUtils.h"
#include "Core/Math/xxHash.h"
#include "Core/Tracing/Tracing.h"

// system
#include <stdio.h> // for sscanf
#if defined( __WINDOWS__ )
    #include "Core/Env/WindowsHeader.h" // for QueryDosDeviceA
#endif

// CONSTRUCTOR - FBuildOptions
//------------------------------------------------------------------------------
FBuildOptions::FBuildOptions()
{
#ifdef DEBUG
    //m_ShowInfo = true; // uncomment this to enable spam when debugging
#endif

    // Default to NUMBER_OF_PROCESSORS
    m_NumWorkerThreads = Env::GetNumProcessors();

    // Default working dir is the system working dir
    AStackString<> workingDir;
    VERIFY( FileIO::GetCurrentDir( workingDir ) );
    SetWorkingDir( workingDir );
}

// ProcessCommandLine
//------------------------------------------------------------------------------
FBuildOptions::OptionsResult FBuildOptions::ProcessCommandLine( int argc, char * argv[] )
{
    // Store executable name
    AStackString<> programName( "FBuild.exe" );
    if ( argc > 0 )
    {
        AStackString<> programPath( argv[0] );
        if ( !programPath.IsEmpty() )
        {
            const char* slash = programPath.FindLast( NATIVE_SLASH );
            programName = ( slash ? slash + 1 : programPath.Get() );
        }
    }

    bool progressOptionSpecified = false;

    // Parse options
    for ( int32_t i=1; i<argc; ++i ) // start from 1 to skip exe name
    {
        AStackString<> thisArg( argv[ i ] );

        // Check WSL wrapper
        #if defined( __WINDOWS__ )
            if ( ( m_WrapperMode  == WRAPPER_MODE_NONE ) && ( thisArg == "-wsl" ) )
            {
                // -wsl must be the first arg
                if ( i != 1 )
                {
                    OUTPUT( "FBuild: Error: -wsl must be the first argument\n" );
                    OUTPUT( "Try \"%s -help\"\n", programName.Get() );
                    return OPTIONS_ERROR;
                }

                m_WrapperMode = WRAPPER_MODE_WINDOWS_SUBSYSTEM_FOR_LINUX;
                const int32_t wslExeIndex = ( i + 1 );
                if ( wslExeIndex >= argc )
                {
                    OUTPUT( "FBuild: Error: Missing <wslPath> for '-wsl' argument\n" );
                    OUTPUT( "Try \"%s -help\"\n", programName.Get() );
                    return OPTIONS_ERROR;
                }
                m_WSLPath = argv[ wslExeIndex ];
                i++; // Skip extra arg we've consumed for the wsl exe

                continue;
            }
        #endif

        // Store into full arg string
        if ( m_Args.IsEmpty() == false )
        {
            m_Args += ' ';
        }
        m_Args += thisArg;

        // Don't validate args for WSL forwarding
        if ( m_WrapperMode == WRAPPER_MODE_WINDOWS_SUBSYSTEM_FOR_LINUX )
        {
            continue;
        }

        // options start with a '-'
        if ( thisArg.BeginsWith( '-' ) )
        {
            if ( thisArg == "-continueafterdbmove" )
            {
                m_ContinueAfterDBMove = true;
                continue;
            }
            else if ( thisArg == "-cache" )
            {
                m_UseCacheRead = true;
                m_UseCacheWrite = true;
                continue;
            }
            else if ( thisArg == "-cacheread" )
            {
                m_UseCacheRead = true;
                continue;
            }
            else if ( thisArg == "-cachewrite" )
            {
                m_UseCacheWrite = true;
                continue;
            }
            else if ( thisArg == "-cacheinfo" )
            {
                m_CacheInfo = true;
                continue;
            }
            else if ( thisArg == "-cachetrim" )
            {
                const int sizeIndex = ( i + 1 );
                PRAGMA_DISABLE_PUSH_MSVC( 4996 ) // This function or variable may be unsafe...
                PRAGMA_DISABLE_PUSH_CLANG_WINDOWS( "-Wdeprecated-declarations" ) // 'sscanf' is deprecated: This function or variable may be unsafe...
                if ( ( sizeIndex >= argc ) ||
                     ( sscanf( argv[ sizeIndex ], "%u", &m_CacheTrim ) ) != 1 ) // TODO:C Consider using sscanf_s
                PRAGMA_DISABLE_POP_CLANG_WINDOWS // -Wdeprecated-declarations
                PRAGMA_DISABLE_POP_MSVC // 4996
                {
                    OUTPUT( "FBuild: Error: Missing or bad <sizeMiB> for '-cachetrim' argument\n" );
                    OUTPUT( "Try \"%s -help\"\n", programName.Get() );
                    return OPTIONS_ERROR;
                }
                i++; // skip extra arg we've consumed

                // add to args we might pass to subprocess
                m_Args += ' ';
                m_Args += argv[ sizeIndex ];
                continue;
            }
            else if ( thisArg == "-cacheverbose" )
            {
                m_CacheVerbose = true;
                continue;
            }
            else if ( thisArg == "-cachecompressionlevel" )
            {
                const int sizeIndex = ( i + 1 );
                PRAGMA_DISABLE_PUSH_MSVC( 4996 ) // This function or variable may be unsafe...
                PRAGMA_DISABLE_PUSH_CLANG_WINDOWS( "-Wdeprecated-declarations" ) // 'sscanf' is deprecated: This function or variable may be unsafe...
                if ( ( sizeIndex >= argc ) ||
                     ( sscanf( argv[ sizeIndex ], "%i", &m_CacheCompressionLevel ) != 1 ) || // TODO:C Consider using sscanf_s
                     ( ( m_CacheCompressionLevel < -128 ) || ( m_CacheCompressionLevel > 12 ) ) ) // See Compressor for valid ranges
                PRAGMA_DISABLE_POP_CLANG_WINDOWS // -Wdeprecated-declarations
                PRAGMA_DISABLE_POP_MSVC // 4996
                {
                    OUTPUT( "FBuild: Error: Missing or bad <level> for '-cachecompressionlevel' argument\n" );
                    OUTPUT( "Try \"%s -help\"\n", programName.Get() );
                    return OPTIONS_ERROR;
                }
                i++; // skip extra arg we've consumed
                
                // add to args we might pass to subprocess
                m_Args += ' ';
                m_Args += argv[ sizeIndex ];
                continue;
            }
            else if ( thisArg == "-clean" )
            {
                m_ForceCleanBuild = true;
                continue;
            }
            else if ( thisArg == "-compdb" )
            {
                m_GenerateCompilationDatabase = true;
                continue;
            }
            else if ( thisArg == "-config" )
            {
                const int32_t pathIndex = ( i + 1 );
                if ( pathIndex >= argc )
                {
                    OUTPUT( "FBuild: Error: Missing <path> for '-config' argument\n" );
                    OUTPUT( "Try \"%s -help\"\n", programName.Get() );
                    return OPTIONS_ERROR;
                }
                m_ConfigFile = argv[ pathIndex ];
                i++; // skip extra arg we've consumed

                // add to args we might pass to subprocess
                m_Args += ' ';
                m_Args += '"'; // surround config file with quotes to avoid problems with spaces in the path
                m_Args += m_ConfigFile;
                m_Args += '"';
                continue;
            }
            #if defined( __WINDOWS__ )
                else if ( thisArg == "-debug" )
                {
                    Env::ShowMsgBox( "FBuild", "Please attach debugger and press ok\n\n(-debug command line used)" );
                    continue;
                }
            #endif
            else if ( thisArg == "-dist" )
            {
                m_AllowDistributed = true;
                continue;
            }
            else if ( thisArg == "-distverbose" )
            {
                m_AllowDistributed = true;
                m_DistVerbose = true;
                m_ShowBuildReason = true;
                continue;
            }
            else if ( thisArg == "-dot" )
            {
                m_GenerateDotGraph = true;
                m_GenerateDotGraphFull = false;
                continue;
            }
            else if ( thisArg == "-dotfull" )
            {
                m_GenerateDotGraph = true;
                m_GenerateDotGraphFull = true;
                continue;
            }
            else if ( thisArg == "-fastcancel" )
            {
                // This is on by default now
                OUTPUT( "FBuild: Warning: -fastcancel is deprecated. (\"fastcancel\" is on by default)\n" );
                continue;
            }
            else if ( thisArg == "-fixuperrorpaths" )
            {
                m_FixupErrorPaths = true;
                continue;
            }
            else if ( thisArg == "-forceremote" )
            {
                m_AllowDistributed = true;
                m_NoLocalConsumptionOfRemoteJobs = true; // ensure all jobs happen on the remote worker
                m_AllowLocalRace = false;
                continue;
            }
            else if ( thisArg == "-help" )
            {
                DisplayHelp( programName );
                return OPTIONS_OK_AND_QUIT; // exit app
            }
            else if ( ( thisArg == "-ide" ) || ( thisArg == "-vs" ) )
            {
                m_ShowProgress = false;
                progressOptionSpecified = true;
                #if defined( __WINDOWS__ )
                    m_FixupErrorPaths = true;
                    m_WrapperMode = WRAPPER_MODE_MAIN_PROCESS;
                #endif
                continue;
            }
            PRAGMA_DISABLE_PUSH_MSVC( 4996 ) // This function or variable may be unsafe...
            PRAGMA_DISABLE_PUSH_CLANG_WINDOWS( "-Wdeprecated-declarations" ) // 'sscanf' is deprecated: This function or variable may be unsafe...
            else if ( thisArg.BeginsWith( "-j" ) &&
                      sscanf( thisArg.Get(), "-j%u", &m_NumWorkerThreads ) == 1 ) // TODO:C Consider using sscanf_s
            PRAGMA_DISABLE_POP_CLANG_WINDOWS // -Wdeprecated-declarations
            PRAGMA_DISABLE_POP_MSVC // 4996
            {
                // only accept within sensible range
                if ( m_NumWorkerThreads <= 256 )
                {
                    continue; // 'numWorkers' will contain value now
                }
            }
            else if ( thisArg == "-monitor" )
            {
                m_EnableMonitor = true;
                continue;
            }
            else if ( thisArg == "-nofastcancel" )
            {
                m_FastCancel = false;
                continue;
            }
            else if (thisArg == "-nolocalrace")
            {
                m_AllowLocalRace = false;
                continue;
            }
            else if ( thisArg == "-noprogress" )
            {
                m_ShowProgress = false;
                progressOptionSpecified = true;
                continue;
            }
            else if ( thisArg == "-nostoponerror")
            {
                m_StopOnFirstError = false;
                continue;
            }
            else if ( thisArg == "-nosummaryonerror" )
            {
                m_ShowSummary = true;
                m_NoSummaryOnError = true;
                continue;
            }
            else if ( thisArg == "-nounity" )
            {
                m_NoUnity = true;
                continue;
            }
            else if ( thisArg == "-profile" )
            {
                m_Profile = true;
                continue;
            }
            else if ( thisArg == "-progress" )
            {
                m_ShowProgress = true;
                progressOptionSpecified = true;
                continue;
            }
            else if ( thisArg == "-quiet" )
            {
                m_ShowCommandSummary = false;
                m_ShowTotalTimeTaken = false;
                m_ShowVerbose = false;
                continue;
            }
            else if ( thisArg == "-report" )
            {
                m_GenerateReport = true;
                continue;
            }
            else if ( thisArg == "-showcmds" )
            {
                m_ShowCommandLines = true;
                continue;
            }
            else if ( thisArg == "-showcmdoutput" )
            {
                m_ShowCommandOutput = true;
                continue;
            }
            else if ( thisArg == "-showdeps" )
            {
                m_DisplayDependencyDB = true;
                continue;
            }
            else if ( thisArg == "-showtargets" )
            {
                m_DisplayTargetList = true;
                continue;
            }
            else if ( thisArg == "-showalltargets" )
            {
                m_DisplayTargetList = true;
                m_ShowHiddenTargets = true;
                continue;
            }
            else if ( thisArg == "-summary" )
            {
                m_ShowSummary = true;
                continue;
            }
            else if ( thisArg == "-verbose" )
            {
                m_ShowVerbose = true;
                m_ShowCommandLines = true;
                m_ShowCommandOutput = true;
                m_CacheVerbose = true;
                continue;
            }
            else if ( thisArg == "-version" )
            {
                DisplayVersion();
                return OPTIONS_OK_AND_QUIT; // exit app
            }
            // -vs : see -ide
            else if ( thisArg == "-wait" )
            {
                m_WaitMode = true;
                continue;
            }
            else if ( thisArg == "-why" )
            {
                m_ShowBuildReason = true;
                continue;
            }
            #if defined( __WINDOWS__ )
                else if ( thisArg == "-wrapper")
                {
                    m_WrapperMode = WRAPPER_MODE_MAIN_PROCESS;
                    continue;
                }
                else if ( thisArg == "-wrapperintermediate") // Internal use only
                {
                    m_WrapperMode = WRAPPER_MODE_INTERMEDIATE_PROCESS;
                    continue;
                }
                else if ( thisArg == "-wrapperfinal") // Internal use only
                {
                    m_WrapperMode = WRAPPER_MODE_FINAL_PROCESS;
                    continue;
                }
            #endif

            // can't use FLOG_ERROR as FLog is not initialized
            OUTPUT( "FBuild: Error: Unknown argument '%s'\n", thisArg.Get() );
            OUTPUT( "Try \"%s -help\"\n", programName.Get() );
            return OPTIONS_ERROR;
        }
        else
        {
            // assume target
            m_Targets.Append( thisArg );
        }
    }

    if ( progressOptionSpecified == false )
    {
        // By default show progress bar only if stdout goes to the terminal
        m_ShowProgress = ( Env::IsStdOutRedirected() == false );
    }

    // Default to build "all"
    if ( m_Targets.IsEmpty() )
    {
        FLOG_VERBOSE( "No target specified, defaulting to target 'all'" );
        m_Targets.EmplaceBack( "all" );
    }

    // When building multiple targets, try to build as much as possible
    if ( m_Targets.GetSize() > 1 )
    {
        m_StopOnFirstError = false;
    }

    // cache mode environment variable (if not supplied on cmd line)
    if ( ( m_UseCacheRead == false ) && ( m_UseCacheWrite == false ) )
    {
        AStackString<> cacheMode;
        if ( Env::GetEnvVariable( "FASTBUILD_CACHE_MODE", cacheMode ) )
        {
            if ( cacheMode == "r" )
            {
                m_UseCacheRead = true;
            }
            else if ( cacheMode == "w" )
            {
                m_UseCacheWrite = true;
            }
            else if ( cacheMode == "rw" )
            {
                m_UseCacheRead = true;
                m_UseCacheWrite = true;
            }
            else
            {
                OUTPUT( "FASTBUILD_CACHE_MODE is invalid (%s)\n", cacheMode.Get() );
                return OPTIONS_ERROR;
            }
        }
    }

    // Global mutex names depend on workingDir which is managed by FBuildOptions
    m_ProgramName = programName;

    return OPTIONS_OK;
}

// FBuildOptions::SetWorkingDir
//------------------------------------------------------------------------------
void FBuildOptions::SetWorkingDir( const AString & path )
{
    ASSERT( !path.IsEmpty() );
    m_WorkingDir = path;

    // clean path
    PathUtils::FixupFolderPath( m_WorkingDir );

    // no trailing slash
    if ( m_WorkingDir.EndsWith( NATIVE_SLASH ) )
    {
        m_WorkingDir.SetLength( m_WorkingDir.GetLength() - 1 );
    }

    #if defined( __WINDOWS__ )
        // so C:\ and c:\ are treated the same on Windows, for better cache hits
        // make the drive letter always uppercase
        if ( ( m_WorkingDir.GetLength() >= 2 ) &&
             ( m_WorkingDir[ 1 ] == ':' ) &&
             ( m_WorkingDir[ 0 ] >= 'a' ) &&
             ( m_WorkingDir[ 0 ] <= 'z' ) )
        {
            m_WorkingDir[ 0 ] = ( 'A' + ( m_WorkingDir[ 0 ] - 'a' ) );
        }
    #endif

    // Generate Mutex/SharedMemory names
    #if defined( __WINDOWS__ ) || defined( __OSX__ )
        #if defined( __WINDOWS__ )
            // convert subst drive mappings to the read path
            // (so you can't compile from the real path and the subst path at the
            // same time which would cause problems)
            AStackString<> canonicalPath;
            if ( ( m_WorkingDir.GetLength() >= 2 ) &&
                 ( m_WorkingDir[ 1 ] == ':' ) &&
                 ( m_WorkingDir[ 0 ] >= 'A' ) &&
                 ( m_WorkingDir[ 0 ] <= 'Z' ) )
            {
                // get drive letter without slash
                AStackString<> driveLetter( m_WorkingDir.Get(), m_WorkingDir.Get() + 2 );

                // get real path for drive letter (could be the same, or a subst'd path)
                char actualPath[ MAX_PATH ];
                actualPath[ 0 ] = '\000';
                VERIFY( QueryDosDeviceA( driveLetter.Get(), actualPath, MAX_PATH ) );

                // if returned path is of format "\??\C:\Folder"...
                if ( AString::StrNCmp( actualPath, "\\??\\", 4 ) == 0 )
                {
                    // .. then it means the working dir is a subst folder
                    // trim the "\\??\\" and use the real path as a base
                    canonicalPath = &actualPath[ 4 ];
                    canonicalPath += ( m_WorkingDir.Get() + 2 ); // add everything after the subst drive letter
                }
                else
                {
                    // The path was already a real path (QueryDosDevice returns the volume only)
                    canonicalPath = m_WorkingDir;
                }
            }
            else
            {
                // a UNC or other funky path - just leave it as is
                canonicalPath = m_WorkingDir;
            }
        #elif defined( __OSX__ )
            AStackString<> canonicalPath( m_WorkingDir );
        #endif

        // case insensitive
        canonicalPath.ToLower();
    #elif defined( __LINUX__ )
        // case sensitive
        AStackString<> canonicalPath( m_WorkingDir );
    #endif

    m_WorkingDirHash = xxHash::Calc32( canonicalPath );
    m_ProcessMutexName.Format( "Global\\FASTBuild-0x%08x", m_WorkingDirHash );
    m_FinalProcessMutexName.Format( "Global\\FASTBuild_Final-0x%08x", m_WorkingDirHash );
    m_SharedMemoryName.Format( "FASTBuildSharedMemory_%08x", m_WorkingDirHash );
}

// DisplayHelp
//------------------------------------------------------------------------------
void FBuildOptions::DisplayHelp( const AString & programName ) const
{
    DisplayVersion();
    OUTPUT( "--------------------------------------------------------------------------------\n"
            "Usage: %s [options] [target1]..[targetn]\n", programName.Get() );
    OUTPUT( "--------------------------------------------------------------------------------\n"
            "Options:\n"
            " -cache[read|write]\n"
            "                   Control use of the build cache.\n"
            " -cachecompressionlevel <level>\n"
            "                   Control compression for cache artifacts (default: -1)\n"
            "                   - <= -1 : less compression, with -128 being the lowest\n"
            "                   - ==  0 : disable compression\n"
            "                   - >=  1 : more compression, with 12 being the highest\n"
            " -cacheinfo        Output cache statistics.\n"
            " -cachetrim <size> Trim the cache to the given size in MiB.\n"
            " -cacheverbose     Emit details about cache interactions.\n"
            " -clean            Force a clean build.\n"
            " -compdb           Generate JSON compilation database for targets.\n"
            " -config <path>    Explicitly specify the config file to use.\n"
            " -continueafterdbmove\n"
            "       Allow builds after a DB move.\n"
            " -debug            (Windows) Break at startup, to attach debugger.\n"
            " -dist             Allow distributed compilation.\n"
            " -distverbose      Print detailed info for distributed compilation.\n"
            " -dot[full]        Emit known dependency tree info for specified targets to an\n"
            "                   fbuild.gv file in DOT format.\n"
            " -fixuperrorpaths  Reformat error paths to be Visual Studio friendly.\n"
            " -forceremote      Force distributable jobs to only be built remotely.\n"
            " -help             Show this help.\n"
            " -ide              Enable multiple options when building from an IDE.\n"
            "                   Enables: -noprogress, -fixuperrorpaths &\n"
            "                   -wrapper (Windows)\n"
            " -j<x>             Explicitly set LOCAL worker thread count X, instead of\n"
            "                   default of hardware thread count.\n"
            " -monitor          Emit a machine-readable file while building.\n"
            " -nofastcancel     Disable aborting other tasks as soon any task fails.\n"
            " -nolocalrace      Disable local race of remotely started jobs.\n"
            " -noprogress       Don't show the progress bar while building.\n"
            " -nounity          (Experimental) Build files individually, ignoring Unity.\n"
            " -nostoponerror    On error, favor building as much as possible.\n"
            " -nosummaryonerror Hide the summary if the build fails. Implies -summary.\n"
            " -profile          Output an fbuild_profiling.json describing the build.\n"
            " -progress         Show build progress bar even if stdout is redirected.\n"
            " -quiet            Don't show build output.\n"
            " -report           Ouput report.html at build end. (Increases build time)\n"
            " -showcmds         Show command lines used to launch external processes.\n"
            " -showcmdoutput    Show output of external processes.\n"
            " -showdeps         Show known dependency tree for specified targets.\n"
            " -showtargets      Display primary targets, excluding those marked \"Hidden\".\n"
            " -showalltargets   Display primary targets, including those marked \"Hidden\".\n"
            " -summary          Show a summary at the end of the build.\n"
            " -verbose          Show detailed diagnostic info. (Increases built time)\n"
            " -version          Print version and exit.\n"
            " -vs               VisualStudio mode. Same as -ide.\n"
            " -wait             Wait for a previous build to complete before starting.\n"
            "                   (Slower than building both targets in one invocation).\n"
            " -why              Show build reason for each item.\n"
            " -wrapper          (Windows) Spawn a sub-process to gracefully handle\n"
            "                   termination from Visual Studio.\n"
            " -wsl <wslPath> <args...>\n"
            "                   (Windows) Forward to the Windows Subsystem for Linux\n"
            "--------------------------------------------------------------------------------\n" );
}

// DisplayVersion
//------------------------------------------------------------------------------
void FBuildOptions::DisplayVersion() const
{
    #ifdef DEBUG
        #define VERSION_CONFIG "(DEBUG) "
    #else
        #define VERSION_CONFIG ""
    #endif
    OUTPUT( "FASTBuild " FBUILD_VERSION_STRING " " VERSION_CONFIG "- "
            "Copyright 2012-2021 Franta Fulin - https://www.fastbuild.org\n" );
    #undef VERSION_CONFIG
}

//------------------------------------------------------------------------------
