// Process.cpp
//------------------------------------------------------------------------------

// Includes
//------------------------------------------------------------------------------
#include "Process.h"

#include "Core/Env/Assert.h"
#include "Core/FileIO/FileIO.h"
#include "Core/Math/Conversions.h"
#include "Core/Process/Atomic.h"
#include "Core/Process/Thread.h"
#include "Core/Profile/Profile.h"
#include "Core/Strings/AStackString.h"
#include "Core/Strings/AString.h"
#include "Core/Time/Timer.h"
#include "Core/Tracing/Tracing.h"

#if defined( __WINDOWS__ )
    #include "Core/Env/WindowsHeader.h"
    #include <TlHelp32.h>
#endif

#if defined( __LINUX__ ) || defined( __APPLE__ )
    #include <errno.h>
    #include <fcntl.h>
    #include <signal.h>
    #include <spawn.h>
    #include <stdio.h>
    #include <stdlib.h>
    #include <string.h>
    #include <sys/wait.h>
    #include <unistd.h>
#endif

// Static Data
//------------------------------------------------------------------------------

// CONSTRUCTOR
//------------------------------------------------------------------------------
Process::Process( const volatile bool * mainAbortFlag,
                  const volatile bool * abortFlag )
    : m_Started( false )
#if defined( __WINDOWS__ )
    , m_SharingHandles( false )
    , m_RedirectHandles( true )
    , m_StdOutRead( INVALID_HANDLE_VALUE )
    , m_StdErrRead( INVALID_HANDLE_VALUE )
    , m_StdInWrite( INVALID_HANDLE_VALUE )
#endif
#if defined( __LINUX__ ) || defined( __APPLE__ )
    , m_ChildPID( -1 )
    , m_HasAlreadyWaitTerminated( false )
#endif
    , m_MainAbortFlag( mainAbortFlag )
    , m_AbortFlag( abortFlag )
{
#if defined( __WINDOWS__ )
    static_assert( sizeof( m_ProcessInfo ) == sizeof( PROCESS_INFORMATION ), "Unexpected sizeof(PROCESS_INFORMATION)" );
#endif
}

// DESTRUCTOR
//------------------------------------------------------------------------------
Process::~Process()
{
    if ( m_Started )
    {
        WaitForExit();
    }
}

// KillProcessTreeInternal
//------------------------------------------------------------------------------
#if defined( __WINDOWS__ )
void Process::KillProcessTreeInternal( const void * hProc,
                                       const uint32_t processID,
                                       const uint64_t processCreationTime )
{
    PROCESSENTRY32 pe;

    memset( &pe, 0, sizeof( PROCESSENTRY32 ) );
    pe.dwSize = sizeof( PROCESSENTRY32 );

    const HANDLE hSnap = ::CreateToolhelp32Snapshot( TH32CS_SNAPPROCESS, 0 );

    if ( ::Process32First( hSnap, &pe ) )
    {
        // find child processes
        do
        {
            // Ignore any process that is not a child
            if ( pe.th32ParentProcessID != processID )
            {
                continue;
            }

            // Handle pid re-use by ensuring process started after parent
            const uint32_t childProcessId = pe.th32ProcessID;
            const HANDLE hChildProc = ::OpenProcess( PROCESS_ALL_ACCESS, FALSE, childProcessId );
            if ( hChildProc )
            {
                const uint64_t childProcessCreationTime = GetProcessCreationTime( hChildProc );
                if ( childProcessCreationTime < processCreationTime )
                {
                    ::CloseHandle( hChildProc );
                    continue; // Cannot be a child because it was created before the parent
                }

                // We should never see the main process because that's handled above
                ASSERT( childProcessId != GetCurrentProcessId() );

                // Recursion
                KillProcessTreeInternal( hChildProc, childProcessId, childProcessCreationTime );

                ::CloseHandle( hChildProc );
            }
        } while ( ::Process32Next( hSnap, &pe ) );
    }

    ::CloseHandle( hSnap );

    // kill this process on the way back up the recursion
    ::TerminateProcess( (HANDLE)hProc, 1 );
}
#endif

// GetProcessStartTime
//------------------------------------------------------------------------------
#if defined( __WINDOWS__ )
/*static*/ uint64_t Process::GetProcessCreationTime( const void * hProc )
{
    if ( hProc == nullptr )
    {
        return 0;
    }

    // Get process start time
    FILETIME creationFileTime, unused;
    VERIFY( GetProcessTimes( (HANDLE)hProc, &creationFileTime, &unused, &unused, &unused ) );

    // Return start time in a more convenient format
    const uint64_t childProcessCreationTime = ( (uint64_t)creationFileTime.dwHighDateTime << 32 ) | creationFileTime.dwLowDateTime;
    return childProcessCreationTime;
}
#endif

// KillProcessTree
//------------------------------------------------------------------------------
void Process::KillProcessTree()
{
#if defined( __WINDOWS__ )
    const uint32_t childProcessId = GetProcessInfo().dwProcessId;
    const HANDLE hChildProc = ::OpenProcess( PROCESS_ALL_ACCESS, FALSE, childProcessId );
    if ( hChildProc )
    {
        KillProcessTreeInternal( hChildProc, childProcessId, GetProcessCreationTime( hChildProc ) );
        ::CloseHandle( hChildProc );
    }
#elif defined( __LINUX__ ) || defined( __APPLE__ )
    // Kill all processes in the process group of the child process.
    kill( -m_ChildPID, SIGKILL );
#else
    #error Unknown platform
#endif
}

// Externs
//------------------------------------------------------------------------------
#if defined( __LINUX__ ) || defined( __APPLE__ )
extern char ** environ;
#endif

// Spawn
//------------------------------------------------------------------------------
bool Process::Spawn( const char * executable,
                     const char * args,
                     const char * workingDir,
                     const char * environment,
                     bool shareHandles )
{
    PROFILE_FUNCTION;

    ASSERT( !m_Started );
    ASSERT( executable );

    if ( m_MainAbortFlag && AtomicLoadRelaxed( m_MainAbortFlag ) )
    {
        // Once main process has aborted, we no longer permit spawning sub-processes.
        return false;
    }

#if defined( __WINDOWS__ )
    // Set up the start up info struct.
    STARTUPINFO si;
    ZeroMemory( &si, sizeof( STARTUPINFO ) );
    si.cb = sizeof( STARTUPINFO );
    si.dwFlags |= STARTF_USESHOWWINDOW;
    si.wShowWindow = SW_HIDE;

    SECURITY_ATTRIBUTES sa;
    ZeroMemory( &sa, sizeof( SECURITY_ATTRIBUTES ) );
    sa.nLength = sizeof( SECURITY_ATTRIBUTES );
    sa.bInheritHandle = TRUE;
    sa.lpSecurityDescriptor = nullptr;

    m_SharingHandles = shareHandles;

    if ( m_RedirectHandles )
    {
            // create the pipes
        if ( shareHandles )
        {
            si.hStdOutput = GetStdHandle( STD_OUTPUT_HANDLE );
            si.hStdError = GetStdHandle( STD_ERROR_HANDLE );
            si.hStdInput = GetStdHandle( STD_INPUT_HANDLE );
        }
        else
        {
            HANDLE stdOutWrite = INVALID_HANDLE_VALUE;
            HANDLE stdErrWrite = INVALID_HANDLE_VALUE;
            HANDLE stdInRead = INVALID_HANDLE_VALUE;

            bool ok = true;
            ok = ok && CreatePipe( &m_StdOutRead, &stdOutWrite, &sa, MEGABYTE );
            ok = ok && CreatePipe( &m_StdErrRead, &stdErrWrite, &sa, MEGABYTE );
            ok = ok && CreatePipe( &stdInRead, &m_StdInWrite, &sa, MEGABYTE );

            // Handle failure
            if ( !ok )
            {
                if ( m_StdOutRead != INVALID_HANDLE_VALUE )
                {
                    ::CloseHandle( m_StdOutRead );
                }
                if ( m_StdErrRead != INVALID_HANDLE_VALUE )
                {
                    ::CloseHandle( m_StdErrRead );
                }
                if ( m_StdInWrite != INVALID_HANDLE_VALUE )
                {
                    ::CloseHandle( m_StdInWrite );
                }
                if ( stdOutWrite != INVALID_HANDLE_VALUE )
                {
                    ::CloseHandle( stdOutWrite );
                }
                if ( stdErrWrite != INVALID_HANDLE_VALUE )
                {
                    ::CloseHandle( stdErrWrite );
                }
                if ( stdInRead != INVALID_HANDLE_VALUE )
                {
                    ::CloseHandle( stdInRead );
                }
                return false;
            }

            // Prevent child inheriting handles, to avoid deadlocks
            VERIFY( SetHandleInformation( m_StdOutRead, HANDLE_FLAG_INHERIT, 0 ) );
            VERIFY( SetHandleInformation( m_StdErrRead, HANDLE_FLAG_INHERIT, 0 ) );
            VERIFY( SetHandleInformation( m_StdInWrite, HANDLE_FLAG_INHERIT, 0 ) );

            si.hStdOutput = stdOutWrite;
            si.hStdError = stdErrWrite;
            si.hStdInput = stdInRead;
        }
        si.dwFlags |= STARTF_USESTDHANDLES;
    }

    // Make sure the first arg is the executable
    // We also need to make a copy, as CreateProcess can write back to this string
    AStackString<1024> fullArgs;
    fullArgs += '\"';
    fullArgs += executable;
    fullArgs += '\"';
    if ( args )
    {
        fullArgs += ' ';
        fullArgs += args;
    }

    // create the child
    PRAGMA_DISABLE_PUSH_MSVC( 6335 ) // False positive: Leaking process information handle '%s'
    if ( !CreateProcess( nullptr, //executable,
                         fullArgs.Get(),
                         nullptr,
                         nullptr,
                         (BOOL)m_RedirectHandles, // inherit handles
                         0,
                         (void *)environment,
                         workingDir,
                         &si,
                         (LPPROCESS_INFORMATION)&m_ProcessInfo ) )
    {
        return false;
    }
    PRAGMA_DISABLE_POP_MSVC // 6335

    if ( m_RedirectHandles && !shareHandles )
    {
        // Close the "other" end of the pipes to avoid deadlocks
        ::CloseHandle( si.hStdOutput );
        ::CloseHandle( si.hStdError );
        ::CloseHandle( si.hStdInput );
    }

    m_Started = true;
    return true;
#elif defined( __LINUX__ ) || defined( __APPLE__ )
    (void)shareHandles; // unsupported

        // create StdOut and StdErr pipes to capture output of spawned process
    int stdOutPipeFDs[ 2 ];
    int stdErrPipeFDs[ 2 ];
    VERIFY( pipe( stdOutPipeFDs ) == 0 );
    VERIFY( pipe( stdErrPipeFDs ) == 0 );

    // Increase buffer sizes to reduce stalls
    #if defined( __LINUX__ )
    // On systems with many CPU cores, this can fail due to per-process
    // limits being reached, so consider this a hint only.
    // (We only increase the size of the stdout to avoid "wasting" memory
    // accelerating the stderr, which is the uncommon case to write to)
    fcntl( stdOutPipeFDs[ 1 ], F_SETPIPE_SZ, ( 512 * 1024 ) );
    #endif

    // prepare args in a single buffer
    AStackString<2048> argVectorBuffer;
    StackArray<const char *, 128> argVector;
    argVector.Append( executable ); // first arg is exe name
    if ( args )
    {
        // Tokenize
        StackArray<AString::TokenRange, 128> tokenRanges;
        const AStackString argsCopy( args );
        argsCopy.Tokenize( tokenRanges );

        // Allocate worst case space for processed args. They can only be
        // the same size or smaller than the original args.
        // (If entire string is exactly one arg we need an extra byte for
        //  a null terminator char to keep the loop simple)
        argVectorBuffer.SetReserved( argsCopy.GetLength() + 1 );

        // Build Vector
        for ( const AString::TokenRange & tokenRange : tokenRanges )
        {
            // Remove quotes from split args. Unlike Windows, on Linux/OSX we're
            // passing the arg vector essentially directly to the process and
            // it's not split/de-quoted by the API used for process spawning
            AStackString arg( ( argsCopy.Get() + tokenRange.m_StartIndex ),
                              ( argsCopy.Get() + tokenRange.m_EndIndex ) );
            arg.RemoveQuotes();

            // Add to pointer array
            const char * argPos = argVectorBuffer.GetEnd();
            argVector.EmplaceBack( argPos );

            // copy procesed arg into buffer
            argVectorBuffer.Append( arg );
            argVectorBuffer.Append( static_cast<char>( 0 ) ); // null terminator
        }
    }
    argVector.Append( nullptr ); // argv must have be nullptr terminated

    // prepare environment
    StackArray<const char *> envVector;
    if ( environment )
    {
        // Iterate double-null terminated string vector
        while ( *environment != 0 )
        {
            envVector.Append( environment );
            environment += strlen( environment );
            environment += 1; // skip null terminator for string
        }
    }
    envVector.Append( nullptr ); // env must be terminated with a nullptr

    char * const * argV = const_cast<char * const *>( argVector.Begin() );
    char * const * envV = const_cast<char * const *>( environment ? envVector.Begin()
                                                                  : const_cast<char * const *>( environ ) );

    // Use posix_spawn when available and safe to improve process spawning
    // performance.
    if ( CanUsePosixSpawn( workingDir ) )
    {
        return SpawnUsingPosixSpawn( stdOutPipeFDs,
                                     stdErrPipeFDs,
                                     executable,
                                     argV,
                                     workingDir,
                                     envV );
    }

    return SpawnUsingFork( stdOutPipeFDs,
                           stdErrPipeFDs,
                           executable,
                           argV,
                           workingDir,
                           envV );
#endif
}

//------------------------------------------------------------------------------
#if defined( __LINUX__ ) || defined( __APPLE__ )
bool Process::SpawnUsingFork( int32_t stdOutPipeFDs[ 2 ],
                              int32_t stdErrPipeFDs[ 2 ],
                              const char * executable,
                              char * const * argV,
                              const char * workingDir,
                              char * const * envV )
{
    // fork the process
    const pid_t childProcessPid = fork();
    if ( childProcessPid == -1 )
    {
        // cleanup pipes
        VERIFY( close( stdOutPipeFDs[ 0 ] ) == 0 );
        VERIFY( close( stdOutPipeFDs[ 1 ] ) == 0 );
        VERIFY( close( stdErrPipeFDs[ 0 ] ) == 0 );
        VERIFY( close( stdErrPipeFDs[ 1 ] ) == 0 );

        ASSERT( false ); // fork failed - should not happen in normal operation
        return false;
    }

    const bool isChild = ( childProcessPid == 0 );
    if ( isChild )
    {
        // Put child process into its own process group.
        // This will allow as to send signals to the whole group which we use to implement KillProcessTree.
        // The new process group will have ID equal to the PID of the child process.
        VERIFY( setpgid( 0, 0 ) == 0 );

        VERIFY( dup2( stdOutPipeFDs[ 1 ], STDOUT_FILENO ) != -1 );
        VERIFY( dup2( stdErrPipeFDs[ 1 ], STDERR_FILENO ) != -1 );

        VERIFY( close( stdOutPipeFDs[ 0 ] ) == 0 );
        VERIFY( close( stdOutPipeFDs[ 1 ] ) == 0 );
        VERIFY( close( stdErrPipeFDs[ 0 ] ) == 0 );
        VERIFY( close( stdErrPipeFDs[ 1 ] ) == 0 );

        if ( workingDir )
        {
            VERIFY( chdir( workingDir ) == 0 );
        }

        // transfer execution to new executable
        if ( envV )
        {
            execve( executable, argV, envV );
        }
        else
        {
            execv( executable, argV );
        }

        exit( -1 ); // only get here if execv fails
    }
    else
    {
        // close write pipes (we never write anything)
        VERIFY( close( stdOutPipeFDs[ 1 ] ) == 0 );
        VERIFY( close( stdErrPipeFDs[ 1 ] ) == 0 );

        // keep pipes for reading child process
        m_StdOutRead = stdOutPipeFDs[ 0 ];
        m_StdErrRead = stdErrPipeFDs[ 0 ];
        m_ChildPID = (int)childProcessPid;

        // TODO: How can we tell if child spawn failed?
        m_Started = true;
        m_HasAlreadyWaitTerminated = false;
        return true;
    }
}
#endif

//------------------------------------------------------------------------------
#if defined( __LINUX__ ) || defined( __APPLE__ )
// Safe use of posix_spawn requires posix_spawn_file_actions_addchdir_np if
// the working dir needs to be set
bool Process::CanUsePosixSpawn( const char * workingDir ) const
{
    #if defined( __APPLE__ ) && defined( __aarch64__ )
    // ARM is OSX 11 or later and has posix_spawn_file_actions_addchdir_np
    (void)workingDir;
    return true;
    #else
    // If we don't actually need to set the working dir, we don't need the action
    // TODO: We could dynamically check for the symbol
    return ( workingDir == nullptr );
    #endif
}
#endif

//------------------------------------------------------------------------------
#if defined( __LINUX__ ) || defined( __APPLE__ )
bool Process::SpawnUsingPosixSpawn( int32_t stdOutPipeFDs[ 2 ],
                                    int32_t stdErrPipeFDs[ 2 ],
                                    const char * executable,
                                    char * const * argV,
                                    const char * workingDir,
                                    char * const * envV )
{
    ASSERT( CanUsePosixSpawn( workingDir ) ); // Should not get here if not safe

    // Create spawn actions
    posix_spawn_file_actions_t spawnFileActions;
    VERIFY( posix_spawn_file_actions_init( &spawnFileActions ) == 0 );

    // Child will dup2 pipes
    VERIFY( posix_spawn_file_actions_adddup2( &spawnFileActions, stdOutPipeFDs[ 1 ], STDOUT_FILENO ) == 0 );
    VERIFY( posix_spawn_file_actions_adddup2( &spawnFileActions, stdErrPipeFDs[ 1 ], STDERR_FILENO ) == 0 );

    // Child will close the original pipes we don't want inherited, but will
    // keep dup2'd handles
    VERIFY( posix_spawn_file_actions_addclose( &spawnFileActions, stdOutPipeFDs[ 0 ] ) == 0 );
    VERIFY( posix_spawn_file_actions_addclose( &spawnFileActions, stdOutPipeFDs[ 1 ] ) == 0 );
    VERIFY( posix_spawn_file_actions_addclose( &spawnFileActions, stdErrPipeFDs[ 0 ] ) == 0 );
    VERIFY( posix_spawn_file_actions_addclose( &spawnFileActions, stdErrPipeFDs[ 1 ] ) == 0 );

    // Child will set working dir if needed
    if ( workingDir )
    {
    #if defined( __OSX__ ) && defined( __aarch64__ )
        // OSX ARM has at least OSX 11.0 which has posix_spawn_file_actions_addchdir_np
        VERIFY( posix_spawn_file_actions_addchdir_np( &spawnFileActions, workingDir ) == 0 );
    #else
        // We should not be calling this function if we want to set a working dir
        // but can't safely do so using posix_spawn_file_actions_addchdir_np
        ASSERT( false );
    #endif
    }

    // Create spawn attributes
    posix_spawnattr_t spawnAttributes;
    VERIFY( posix_spawnattr_init( &spawnAttributes ) == 0 );

    // Child will put itself into its own process group.
    // This allows us to send signals to the group to implement KillProcessTree.
    VERIFY( posix_spawnattr_setpgroup( &spawnAttributes, 0 ) == 0 );
    const int16_t flags = POSIX_SPAWN_SETPGROUP;
    VERIFY( posix_spawnattr_setflags( &spawnAttributes, flags ) == 0 );

    // Spawn process
    pid_t childProcessPid = -1;
    const int32_t result = posix_spawn( &childProcessPid,
                                        executable,
                                        &spawnFileActions,
                                        &spawnAttributes,
                                        argV,
                                        envV );

    // Free attributes and actions
    VERIFY( posix_spawnattr_destroy( &spawnAttributes ) == 0 );
    VERIFY( posix_spawn_file_actions_destroy( &spawnFileActions ) == 0 );

    // Close write pipes (we never write anything)
    VERIFY( close( stdOutPipeFDs[ 1 ] ) == 0 );
    VERIFY( close( stdErrPipeFDs[ 1 ] ) == 0 );

    if ( result != 0 )
    {
        // Close read pipes
        VERIFY( close( stdOutPipeFDs[ 0 ] ) == 0 );
        VERIFY( close( stdErrPipeFDs[ 0 ] ) == 0 );
        return false; // Failed to spawn
    }

    // Keep pipes for reading child process
    m_StdOutRead = stdOutPipeFDs[ 0 ];
    m_StdErrRead = stdErrPipeFDs[ 0 ];
    m_ChildPID = static_cast<int32_t>( childProcessPid );

    m_Started = true;
    m_HasAlreadyWaitTerminated = false;
    return true;
}
#endif

// IsRunning
//----------------------------------------------------------
bool Process::IsRunning() const
{
    ASSERT( m_Started );

#if defined( __WINDOWS__ )
    switch ( WaitForSingleObject( GetProcessInfo().hProcess, 0 ) )
    {
        case WAIT_OBJECT_0: return false;
        case WAIT_TIMEOUT: return true;
        default: ASSERT( false ); return false; // we should never get here
    }
#elif defined( __LINUX__ ) || defined( __APPLE__ )
    // already waited?
    if ( m_HasAlreadyWaitTerminated )
    {
        return false;
    }

    // non-blocking "wait"
    int status( -1 );
    pid_t result = waitpid( m_ChildPID, &status, WNOHANG );
    ASSERT( result != -1 ); // usage error
    if ( result == 0 )
    {
        return true; // Still running
    }

    // store wait result: can't call again if we just cleaned up process
    ASSERT( result == m_ChildPID );
    if ( WIFEXITED( status ) )
    {
        m_ReturnStatus = WEXITSTATUS( status ); // process terminated normally, use exit code
    }
    else if ( WIFSIGNALED( status ) )
    {
        m_ReturnStatus = -( WTERMSIG( status ) ); // process was terminated by a signal, use negative signal value
    }
    else if ( WIFSTOPPED( status ) )
    {
        return true; // process was stopped, it is not terminated yet
    }
    else
    {
        m_ReturnStatus = status; // some other unexpected state change, treat it as a failure
    }
    m_HasAlreadyWaitTerminated = true;
    return false; // no longer running
#else
    #error Unknown platform
#endif
}

// WaitForExit
//------------------------------------------------------------------------------
int32_t Process::WaitForExit()
{
    ASSERT( m_Started );
    m_Started = false;

#if defined( __WINDOWS__ )

    DWORD exitCode = 0;

    if ( HasAborted() == false )
    {
        // Don't wait if using jobs and the process has been aborted.
        // It will be killed along with the fbuild process if the TerminateProcess has failed for any reason and
        // it is useless to wait for it was anyways we are reporting a failing exit code.
        // Also, This accelerate further more the cancellation.

        // wait for it to finish
        VERIFY( WaitForSingleObject( GetProcessInfo().hProcess, INFINITE ) == WAIT_OBJECT_0 );

        // get the result code
        VERIFY( GetExitCodeProcess( GetProcessInfo().hProcess, (LPDWORD)&exitCode ) );
    }

    // cleanup
    VERIFY( ::CloseHandle( m_StdOutRead ) );
    VERIFY( ::CloseHandle( m_StdErrRead ) );
    VERIFY( ::CloseHandle( m_StdInWrite ) );
    VERIFY( ::CloseHandle( GetProcessInfo().hProcess ) );
    VERIFY( ::CloseHandle( GetProcessInfo().hThread ) );

    return (int32_t)exitCode;
#elif defined( __LINUX__ ) || defined( __APPLE__ )
    VERIFY( close( m_StdOutRead ) == 0 );
    VERIFY( close( m_StdErrRead ) == 0 );
    if ( m_HasAlreadyWaitTerminated == false )
    {
        int status;
        for ( ;; )
        {
            pid_t ret = waitpid( m_ChildPID, &status, 0 );
            if ( ret == -1 )
            {
                if ( errno == EINTR )
                {
                    continue; // Try again
                }
                ASSERT( false ); // Usage error
            }
            ASSERT( ret == m_ChildPID );
            if ( WIFEXITED( status ) )
            {
                m_ReturnStatus = WEXITSTATUS( status ); // process terminated normally, use exit code
            }
            else if ( WIFSIGNALED( status ) )
            {
                m_ReturnStatus = -( WTERMSIG( status ) ); // process was terminated by a signal, use negative signal value
            }
            else if ( WIFSTOPPED( status ) )
            {
                continue; // process was stopped, keep waiting for termination
            }
            else
            {
                m_ReturnStatus = status; // some other unexpected state change, treat it as a failure
            }
            break;
        }
    }

    return m_ReturnStatus;
#else
    #error Unknown platform
#endif
}

// Detach
//------------------------------------------------------------------------------
void Process::Detach()
{
    ASSERT( m_Started );
    m_Started = false;

#if defined( __WINDOWS__ )
    // cleanup
    if ( m_StdOutRead != INVALID_HANDLE_VALUE )
    {
        ::CloseHandle( m_StdOutRead );
    }
    if ( m_StdErrRead != INVALID_HANDLE_VALUE )
    {
        ::CloseHandle( m_StdErrRead );
    }
    if ( m_StdInWrite != INVALID_HANDLE_VALUE )
    {
        ::CloseHandle( m_StdInWrite );
    }
    VERIFY( ::CloseHandle( GetProcessInfo().hProcess ) );
    VERIFY( ::CloseHandle( GetProcessInfo().hThread ) );
#elif defined( __APPLE__ )
    // TODO:MAC Implement Process
#elif defined( __LINUX__ )
    ASSERT( false ); // TODO:LINUX Implement Process
#else
    #error Unknown platform
#endif
}

// ReadAllData
//------------------------------------------------------------------------------
bool Process::ReadAllData( AString & outMem,
                           AString & errMem,
                           uint32_t timeOutMS )
{
    const Timer t;

    bool processExited = false;
    for ( ;; )
    {
        if ( HasAborted() )
        {
            PROFILE_SECTION( "Abort" );
            KillProcessTree();
            break;
        }

        const uint32_t prevOutSize = outMem.GetLength();
        const uint32_t prevErrSize = errMem.GetLength();
#if defined( __APPLE__ ) || defined( __LINUX__ )
        Read( m_StdOutRead, m_StdErrRead, outMem, errMem );
#else
        Read( m_StdOutRead, outMem );
        Read( m_StdErrRead, errMem );
#endif

        // did we get some data?
        if ( ( prevOutSize != outMem.GetLength() ) || ( prevErrSize != errMem.GetLength() ) )
        {
            continue; // try reading again right away incase there is more
        }

        // nothing to read right now
#if defined( __WINDOWS__ )
        if ( processExited == false )
        {
            const DWORD result = WaitForSingleObject( GetProcessInfo().hProcess, 15 );
            if ( result == WAIT_TIMEOUT )
            {
                // Check if timeout is hit
                if ( ( timeOutMS > 0 ) && ( t.GetElapsedMS() >= (float)timeOutMS ) )
                {
                    Terminate();
                    return false; // Timed out
                }

                continue; // still running - try to read
            }
            else
            {
                // exited - will do one more read
                ASSERT( result == WAIT_OBJECT_0 );
            }
        }
#else
        if ( IsRunning() )
        {
            // Check if timeout is hit
            if ( ( timeOutMS > 0 ) && ( t.GetElapsedMS() >= static_cast<float>( timeOutMS ) ) )
            {
                Terminate();
                return false; // Timed out
            }

            // no data available, but process is still going, so wait
            // if we got here it's because we woke periodically to check
            // if the process timed out (but it didn't as checked above)
            // or if the build was cancelled (after we continue the loop)
            continue;
        }
#endif

        // process exited - is this the first time to this point?
        if ( processExited == false )
        {
            processExited = true;
            continue; // get remaining output
        }

        break; // all done
    }

    return true;
}

// Read
//------------------------------------------------------------------------------
#if defined( __WINDOWS__ )
void Process::Read( HANDLE handle, AString & buffer )
{
    // anything available?
    DWORD bytesAvail( 0 );
    if ( !::PeekNamedPipe( handle, nullptr, 0, nullptr, (LPDWORD)&bytesAvail, nullptr ) )
    {
        return;
    }
    if ( bytesAvail == 0 )
    {
        return;
    }

    // Will data fit in existing buffer?
    const uint32_t sizeSoFar = buffer.GetLength();
    const uint32_t newSize = ( sizeSoFar + bytesAvail );
    if ( newSize > buffer.GetReserved() )
    {
        // Expand buffer for new data in large chunks
        const uint32_t newBufferSize = Math::Max<uint32_t>( newSize, buffer.GetReserved() + ( 16 * MEGABYTE ) );
        buffer.SetReserved( newBufferSize );
    }

    // read the new data
    DWORD bytesReadNow = 0;
    if ( !::ReadFile( handle, buffer.Get() + sizeSoFar, bytesAvail, (LPDWORD)&bytesReadNow, nullptr ) )
    {
        ASSERT( false ); // error!
    }

    // Update length
    buffer.SetLength( sizeSoFar + bytesReadNow );
}
#endif

//------------------------------------------------------------------------------
#if defined( __APPLE__ ) || defined( __LINUX__ )
void Process::Read( int32_t stdOutHandle,
                    int32_t stdErrHandle,
                    AString & inoutOutBuffer,
                    AString & inoutErrBuffer )
{
    PROFILE_FUNCTION;

    // Break periodically so caller can:
    // - check timeouts (if used)
    // - terminate process if cancelling
    timeval timeout;
    timeout.tv_sec = 0;
    timeout.tv_usec = 500 * 1000; // 500ms

    // any data available in either handle?
    fd_set fdSet;
    FD_ZERO( &fdSet );
    FD_SET( stdOutHandle, &fdSet );
    FD_SET( stdErrHandle, &fdSet );
    int ret = select( Math::Max( stdOutHandle, stdErrHandle ) + 1,
                      &fdSet,
                      nullptr,
                      nullptr,
                      &timeout );
    if ( ret == -1 )
    {
        ASSERT( false ); // usage error?
        return;
    }
    if ( ret == 0 )
    {
        return; // no data available
    }

    ASSERT( ( ret == 1 ) || ( ret == 2 ) );
    if ( FD_ISSET( stdOutHandle, &fdSet ) )
    {
        ReadCommon( stdOutHandle, inoutOutBuffer );
    }
    if ( FD_ISSET( stdErrHandle, &fdSet ) )
    {
        ReadCommon( stdErrHandle, inoutErrBuffer );
    }
}
#endif

//------------------------------------------------------------------------------
#if defined( __LINUX__ ) || defined( __APPLE__ )
void Process::ReadCommon( int32_t handle, AString & buffer )
{
    // how much space do we have left for reading into?
    uint32_t spaceInBuffer = ( buffer.GetReserved() - buffer.GetLength() );
    if ( spaceInBuffer == 0 )
    {
        // Expand buffer for new data in large chunks
        const uint32_t newBufferSize = ( buffer.GetReserved() + ( 16 * MEGABYTE ) );
        buffer.SetReserved( newBufferSize );
        spaceInBuffer = ( buffer.GetReserved() - buffer.GetLength() );
    }

    // read the new data
    ssize_t result = read( handle, buffer.Get() + buffer.GetLength(), spaceInBuffer );
    if ( result == -1 )
    {
        ASSERT( false ); // error!
        result = 0; // no bytes read
    }

    // Update length
    buffer.SetLength( buffer.GetLength() + (uint32_t)result );
}
#endif

// GetCurrentId
//------------------------------------------------------------------------------
/*static*/ uint32_t Process::GetCurrentId()
{
#if defined( __WINDOWS__ )
    return ::GetCurrentProcessId();
#else
    return static_cast<uint32_t>( ::getpid() );
#endif
}

// HasAborted
//------------------------------------------------------------------------------
bool Process::HasAborted() const
{
    return ( m_MainAbortFlag && AtomicLoadRelaxed( m_MainAbortFlag ) ) ||
           ( m_AbortFlag && AtomicLoadRelaxed( m_AbortFlag ) );
}

// Terminate
//------------------------------------------------------------------------------
void Process::Terminate()
{
#if defined( __WINDOWS__ )
    VERIFY( ::TerminateProcess( GetProcessInfo().hProcess, 1 ) );
#else
    kill( m_ChildPID, SIGKILL );
#endif
}

//------------------------------------------------------------------------------
