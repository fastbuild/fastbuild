// Process.cpp
//------------------------------------------------------------------------------

// Includes
//------------------------------------------------------------------------------
#include "Process.h"

#include "Core/Env/Assert.h"
#include "Core/FileIO/FileIO.h"
#include "Core/Math/Constants.h"
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
    , m_HasAborted( false )
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
    void Process::KillProcessTreeInternal( const void * hProc, const uint32_t processID, const uint64_t processCreationTime )
    {
        PROCESSENTRY32 pe;

        memset( &pe, 0, sizeof( PROCESSENTRY32) );
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
            }
            while ( ::Process32Next( hSnap, &pe ) );
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
        ZeroMemory( &si, sizeof(STARTUPINFO) );
        si.cb = sizeof( STARTUPINFO );
        si.dwFlags |= STARTF_USESHOWWINDOW;
        si.wShowWindow = SW_HIDE;

        SECURITY_ATTRIBUTES sa;
        ZeroMemory( &sa, sizeof( SECURITY_ATTRIBUTES ) );
        sa.nLength = sizeof(SECURITY_ATTRIBUTES);
        sa.bInheritHandle = TRUE;
        sa.lpSecurityDescriptor = nullptr;

        m_SharingHandles = shareHandles;

        if ( m_RedirectHandles )
        {
            // create the pipes
            if ( shareHandles )
            {
                si.hStdOutput = GetStdHandle(STD_OUTPUT_HANDLE);
                si.hStdError = GetStdHandle(STD_ERROR_HANDLE);
                si.hStdInput = GetStdHandle(STD_INPUT_HANDLE);
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
                    if ( m_StdOutRead != INVALID_HANDLE_VALUE ) { ::CloseHandle( m_StdOutRead ); }
                    if ( m_StdErrRead != INVALID_HANDLE_VALUE ) { ::CloseHandle( m_StdErrRead ); }
                    if ( m_StdInWrite != INVALID_HANDLE_VALUE ) { ::CloseHandle( m_StdInWrite ); }
                    if ( stdOutWrite != INVALID_HANDLE_VALUE ) { ::CloseHandle( stdOutWrite ); }
                    if ( stdErrWrite != INVALID_HANDLE_VALUE ) { ::CloseHandle( stdErrWrite ); }
                    if ( stdInRead != INVALID_HANDLE_VALUE ) { ::CloseHandle( stdInRead ); }
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
        AStackString< 1024 > fullArgs;
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

        // prepare args
        Array< AString > splitArgs( 64, true );
        Array< const char * > argVector( 64, true );
        argVector.Append( executable ); // first arg is exe name
        if ( args )
        {
            // Tokenize
            AStackString<> argCopy( args );
            argCopy.Tokenize( splitArgs );

            // Build Vector
            for ( AString & arg : splitArgs )
            {
                if ( arg.BeginsWith( '"' ) && arg.EndsWith( '"' ) )
                {
                    // strip quotes
                    arg.SetLength( arg.GetLength() - 1 ); // trim end quote
                    argVector.Append( arg.Get() + 1 ); // skip start quote
                    continue;
                }
                argVector.Append( arg.Get() ); // leave arg as-is
            }
        }
        argVector.Append( nullptr ); // argv must have be nullptr terminated

        // prepare environment
        Array< const char* > envVector( 8, true );
        if ( environment )
        {
            // Iterate double-null terminated string vector
            while( *environment != 0 )
            {
                envVector.Append( environment );
                environment += strlen( environment );
                environment += 1; // skip null terminator for string
            }
        }
        envVector.Append( nullptr ); // env must be terminated with a nullptr

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
            char * const * argV = (char * const *)argVector.Begin();
            if ( environment )
            {
                char * const * envV = (char * const *)envVector.Begin();
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
    #else
        #error Unknown platform
    #endif
}

// IsRunning
//----------------------------------------------------------
bool Process::IsRunning() const
{
    ASSERT( m_Started );

    #if defined( __WINDOWS__ )
        switch ( WaitForSingleObject( GetProcessInfo().hProcess, 0 ) )
        {
            case WAIT_OBJECT_0: return false;
            case WAIT_TIMEOUT:  return true;
            default:            ASSERT( false ); return false; // we should never get here
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
        ASSERT ( result != -1 ); // usage error
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

        if ( m_HasAborted == false )
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
            for( ;; )
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
        if ( m_StdOutRead != INVALID_HANDLE_VALUE ) { ::CloseHandle( m_StdOutRead ); }
        if ( m_StdErrRead != INVALID_HANDLE_VALUE ) { ::CloseHandle( m_StdErrRead ); }
        if ( m_StdInWrite != INVALID_HANDLE_VALUE ) { ::CloseHandle( m_StdInWrite ); }
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
    Timer t;

    #if defined( __LINUX__ )
        // Start with a short sleep interval to allow rapid termination of
        // short-lived processes. The timeout increases during periods of
        // no output and reset when receiving output to balance responsiveness
        // with overhead.
        uint32_t sleepIntervalMS = 1;
    #endif

    bool processExited = false;
    for ( ;; )
    {
        const bool mainAbort = ( m_MainAbortFlag && AtomicLoadRelaxed( m_MainAbortFlag ) );
        const bool abort = ( m_AbortFlag && AtomicLoadRelaxed( m_AbortFlag ) );
        if ( abort || mainAbort )
        {
            PROFILE_SECTION( "Abort" );
            KillProcessTree();
            m_HasAborted = true;
            break;
        }

        const uint32_t prevOutSize = outMem.GetLength();
        const uint32_t prevErrSize = errMem.GetLength();
        Read( m_StdOutRead, outMem );
        Read( m_StdErrRead, errMem );

        // did we get some data?
        if ( ( prevOutSize != outMem.GetLength() ) || ( prevErrSize != errMem.GetLength() ) )
        {
            #if defined( __LINUX__ )
                // Reset sleep interval            
                sleepIntervalMS = 1;
            #endif
            continue; // try reading again right away incase there is more
        }

        // nothing to read right now
        #if defined( __WINDOWS__ )
            if ( processExited == false )
            {
                DWORD result = WaitForSingleObject( GetProcessInfo().hProcess, 15 );
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
                if ( ( timeOutMS > 0 ) && ( t.GetElapsedMS() >= timeOutMS ) )
                {
                    Terminate();
                    return false; // Timed out
                }

                // no data available, but process is still going, so wait
                #if defined( __OSX__ )
                    // On OSX there seems to be no way to set the pipe bufffer
                    // size so we must instead wake up frequently to avoid the
                    // writer being blocked.
                    Thread::Sleep( 2 );
                #else
                    // TODO:C Investigate waiting on an event when process terminates
                    // to reduce overall process spawn time
                    Thread::Sleep( sleepIntervalMS );

                    // Increase sleep interval upto limit
                    sleepIntervalMS = Math::Min<uint32_t>( sleepIntervalMS * 2, 8 );
                #endif
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
            const uint32_t newBufferSize = Math::Max< uint32_t >( newSize, buffer.GetReserved() + ( 16 * MEGABYTE ) );
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


// Read
//------------------------------------------------------------------------------
#if defined( __LINUX__ ) || defined( __APPLE__ )
    void Process::Read( int handle, AString & buffer )
    {
        // any data available?
        timeval timeout;
        timeout.tv_sec = 0;
        timeout.tv_usec = 0;
        fd_set fdSet;
        FD_ZERO( &fdSet );
        FD_SET( handle, &fdSet );
        int ret = select( handle+1, &fdSet, nullptr, nullptr, &timeout );
        if ( ret == -1 )
        {
            ASSERT( false ); // usage error?
            return;
        }
        if ( ret == 0 )
        {
            return; // no data available
        }

        // how much space do we have left for reading into?
        const uint32_t spaceInBuffer = ( buffer.GetReserved() - buffer.GetLength() );
        if ( spaceInBuffer == 0 )
        {
            // Expand buffer for new data in large chunks
            const uint32_t newBufferSize = ( buffer.GetReserved() + ( 16 * MEGABYTE ) );
            buffer.SetReserved( newBufferSize );
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
    #elif defined( __LINUX__ )
        return ::getpid();
    #elif defined( __OSX__ )
        return ::getpid();
    #endif
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
