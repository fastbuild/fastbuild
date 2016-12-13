// Process.cpp
//------------------------------------------------------------------------------

// Includes
//------------------------------------------------------------------------------
#include "Core/PrecompiledHeader.h"

#include "Process.h"

#include "Core/Env/Assert.h"
#include "Core/FileIO/FileIO.h"
#include "Core/Math/Conversions.h"
#include "Core/Process/Thread.h"
#include "Core/Profile/Profile.h"
#include "Core/Time/Timer.h"
#include "Core/Strings/AStackString.h"
#include "Core/Strings/AString.h"

#if defined( __WINDOWS__ )
    #include <windows.h>
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

// CONSTRUCTOR
//------------------------------------------------------------------------------
Process::Process()
: m_Started( false )
#if defined( __WINDOWS__ )
    , m_SharingHandles( false )
    , m_RedirectHandles( true )
    , m_StdOutRead( nullptr )
    , m_StdOutWrite( nullptr )
    , m_StdErrRead( nullptr )
    , m_StdErrWrite( nullptr )
#endif
#if defined( __LINUX__ ) || defined( __APPLE__ )
    , m_ChildPID( -1 )
    , m_HasAlreadyWaitTerminated( false )
#endif
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

// Spawn
//------------------------------------------------------------------------------
bool Process::Spawn( const char * executable,
                     const char * args,
                     const char * workingDir,
                     const char * environment,
                     bool shareHandles )
{
    PROFILE_FUNCTION

    ASSERT( !m_Started );
    ASSERT( executable );

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
                if ( ! CreatePipe( &m_StdOutRead, &m_StdOutWrite, &sa, MEGABYTE ) )
                {
                    return false;
                }
                SetHandleInformation( m_StdOutRead, HANDLE_FLAG_INHERIT, 0 );

                if ( ! CreatePipe( &m_StdErrRead, &m_StdErrWrite, &sa, MEGABYTE ) )
                {
                    VERIFY( CloseHandle( m_StdOutRead ) );
                    VERIFY( CloseHandle( m_StdOutWrite ) );
                    return false;
                }
                SetHandleInformation( m_StdErrRead, HANDLE_FLAG_INHERIT, 0 );

                si.hStdOutput = m_StdOutWrite;
                si.hStdError = m_StdErrWrite;
                si.hStdInput = GetStdHandle(STD_INPUT_HANDLE);
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

        m_Started = true;
        return true;
    #elif defined( __LINUX__ ) || defined( __APPLE__ )
        (void)shareHandles; // unsupported

        // create StdOut and StdErr pipes to capture output of spawned process
        int stdOutPipeFDs[ 2 ];
        int stdErrPipeFDs[ 2 ];
        VERIFY( pipe( stdOutPipeFDs ) == 0 );
        VERIFY( pipe( stdErrPipeFDs ) == 0 );

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
            for ( auto & arg : splitArgs )
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
            case WAIT_OBJECT_0:
                return false;

            case WAIT_TIMEOUT:
                return true;
        }
        ASSERT( false ); // we should never get here
        return false;
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
        m_ReturnStatus = WEXITSTATUS(status);
        m_HasAlreadyWaitTerminated = true;
        return false; // no longer running
    #else
        #error Unknown platform
    #endif
}

// WaitForExit
//------------------------------------------------------------------------------
int Process::WaitForExit()
{
    ASSERT( m_Started );
    m_Started = false;

    #if defined( __WINDOWS__ )
        // wait for it to finish
        VERIFY( WaitForSingleObject( GetProcessInfo().hProcess, INFINITE ) == WAIT_OBJECT_0 );

        // get the result code
        DWORD exitCode = 0;
        VERIFY( GetExitCodeProcess( GetProcessInfo().hProcess, (LPDWORD)&exitCode ) );

        // cleanup
        VERIFY( CloseHandle( GetProcessInfo().hProcess ) );
        VERIFY( CloseHandle( GetProcessInfo().hThread ) );

        if ( !m_SharingHandles && m_RedirectHandles )
        {
            VERIFY( CloseHandle( m_StdOutRead ) );
            VERIFY( CloseHandle( m_StdOutWrite ) );
            VERIFY( CloseHandle( m_StdErrRead ) );
            VERIFY( CloseHandle( m_StdErrWrite ) );
        }

        return exitCode;
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
                m_ReturnStatus = WEXITSTATUS(status);
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
        VERIFY( CloseHandle( GetProcessInfo().hProcess ) );
        VERIFY( CloseHandle( GetProcessInfo().hThread ) );

        if ( !m_SharingHandles && m_RedirectHandles )
        {
            VERIFY( CloseHandle( m_StdOutRead ) );
            VERIFY( CloseHandle( m_StdOutWrite ) );
            VERIFY( CloseHandle( m_StdErrRead ) );
            VERIFY( CloseHandle( m_StdErrWrite ) );
        }
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
bool Process::ReadAllData( AutoPtr< char > & outMem, uint32_t * outMemSize,
                           AutoPtr< char > & errMem, uint32_t * errMemSize,
                           uint32_t timeOutMS )
{
    // we'll capture into these growing buffers
    uint32_t outSize = 0;
    uint32_t errSize = 0;
    uint32_t outBufferSize = 0;
    uint32_t errBufferSize = 0;

    Timer t;

    bool processExited = false;
    for ( ;; )
    {
        uint32_t prevOutSize = outSize;
        uint32_t prevErrSize = errSize;
        Read( m_StdOutRead, outMem, outSize, outBufferSize );
        Read( m_StdErrRead, errMem, errSize, errBufferSize );

        // did we get some data?
        if ( ( prevOutSize != outSize ) || ( prevErrSize != errSize ) )
        {
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
                    if ( ( timeOutMS > 0 ) && ( t.GetElapsedMS() >= timeOutMS ) )
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
                // TODO:C Replace this sleep with event-based wait
                Thread::Sleep( 15 );
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

    // if owner asks for pointers, they now own the mem
    if ( outMemSize ) { *outMemSize = outSize; }
    if ( errMemSize ) { *errMemSize = errSize; }
    return true;
}

// Read
//------------------------------------------------------------------------------
#if defined( __WINDOWS__ )
    void Process::Read( HANDLE handle, AutoPtr< char > & buffer, uint32_t & sizeSoFar, uint32_t & bufferSize )
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

        // will it fit in the buffer we have?
        if ( ( sizeSoFar + bytesAvail ) > bufferSize )
        {
            // no - allocate a bigger buffer (also handles the first time with no buffer)

            // TODO:B look at a new container type (like a linked list of 1mb buffers) to avoid the wasteage here
            // The caller has to take a copy to avoid the overhead if they want to hang onto the data
            // grow buffer in at least 16MB chunks, to prevent future reallocations
            uint32_t newBufferSize = Math::Max< uint32_t >( sizeSoFar + bytesAvail, bufferSize + ( 16 * MEGABYTE ) );
            char * newBuffer = (char *)ALLOC( newBufferSize + 1 ); // +1 so we can always add a null char
            if ( buffer.Get() )
            {
                // transfer and free old buffer
                memcpy( newBuffer, buffer.Get(), sizeSoFar );
            }
            buffer = newBuffer; // will take care of deletion of old buffer
            bufferSize = newBufferSize;
            buffer.Get()[ sizeSoFar ] = '\000';
        }

        ASSERT( sizeSoFar + bytesAvail <= bufferSize ); // sanity check

        // read the new data
        DWORD bytesReadNow = 0;
        if ( !::ReadFile( handle, buffer.Get() + sizeSoFar, bytesAvail, (LPDWORD)&bytesReadNow, 0 ) )
        {
            return;
        }
        ASSERT( bytesReadNow == bytesAvail );
        sizeSoFar += bytesReadNow;

        // keep data null char terminated for caller convenience
        buffer.Get()[ sizeSoFar ] = '\000';
    }
#endif


// Read
//------------------------------------------------------------------------------
#if defined( __LINUX__ ) || defined( __APPLE__ )
    void Process::Read( int handle, AutoPtr< char > & buffer, uint32_t & sizeSoFar, uint32_t & bufferSize )
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
        uint32_t spaceInBuffer = ( bufferSize - sizeSoFar );
        if ( spaceInBuffer == 0 )
        {
            // allocate a bigger buffer (also handles the first time with no buffer)

            // TODO:B look at a new container type (like a linked list of 1mb buffers) to avoid the wasteage here
            // The caller has to take a copy to avoid the overhead if they want to hang onto the data
            // grow buffer in at least 16MB chunks, to prevent future reallocations
            uint32_t newBufferSize = ( sizeSoFar + ( 16 * MEGABYTE ) );
            char * newBuffer = (char *)ALLOC( newBufferSize + 1 ); // +1 so we can always add a null char
            if ( buffer.Get() )
            {
                // transfer and free old buffer
                memcpy( newBuffer, buffer.Get(), sizeSoFar );
            }
            buffer = newBuffer; // will take care of deletion of old buffer
            bufferSize = newBufferSize;
            spaceInBuffer = ( bufferSize - sizeSoFar );
            buffer.Get()[ sizeSoFar ] = '\000';
        }

        // read the new data
        ssize_t result = read( handle, buffer.Get() + sizeSoFar, spaceInBuffer );
        if ( result == -1 )
        {
            ASSERT( false ); // error!
            return;
        }

        // account for newly read bytes
        sizeSoFar += (uint32_t)result;

        // keep data null char terminated for caller convenience
        buffer.Get()[ sizeSoFar ] = '\000';
    }
#endif

// ReadStdOut
//------------------------------------------------------------------------------
#if defined( __WINDOWS__ )
    char * Process::ReadStdOut( uint32_t * bytesRead )
    {
        return Read( m_StdOutRead, bytesRead );
    }
#endif

// ReadStdOut
//------------------------------------------------------------------------------
#if defined( __WINDOWS__ )
    char * Process::ReadStdErr( uint32_t * bytesRead )
    {
        return Read( m_StdErrRead, bytesRead );
    }
#endif

// ReadStdOut
//------------------------------------------------------------------------------
#if defined( __WINDOWS__ )
    uint32_t Process::ReadStdOut( char * outputBuffer, uint32_t outputBufferSize )
    {
        return Read( m_StdOutRead, outputBuffer, outputBufferSize );
    }
#endif

// ReadStdErr
//------------------------------------------------------------------------------
#if defined( __WINDOWS__ )
    uint32_t Process::ReadStdErr( char * outputBuffer, uint32_t outputBufferSize )
    {
        return Read( m_StdErrRead, outputBuffer, outputBufferSize );
    }
#endif

// Read
//------------------------------------------------------------------------------
#if defined( __WINDOWS__ )
    char * Process::Read( HANDLE handle, uint32_t * bytesRead )
    {
        // see if there's anything in the pipe
        DWORD bytesAvail;
        VERIFY( PeekNamedPipe( handle, nullptr, 0, nullptr, (LPDWORD)&bytesAvail, nullptr ) );
        if ( bytesAvail == 0 )
        {
            if ( bytesRead )
            {
                *bytesRead = 0;
            }
            return nullptr;
        }

        // allocate output buffer
        char * mem = (char *)ALLOC( bytesAvail + 1 ); // null terminate for convenience
        mem[ bytesAvail ] = 0;

        // read the data
        DWORD bytesReadNow = 0;
        VERIFY( ReadFile( handle, mem, bytesAvail, (LPDWORD)&bytesReadNow, 0 ) );
        ASSERT( bytesReadNow == bytesAvail );
        if ( bytesRead )
        {
            *bytesRead = bytesReadNow;
        }
        return mem;
    }
#endif

// Read
//------------------------------------------------------------------------------
#if defined( __WINDOWS__ )
    uint32_t Process::Read( HANDLE handle, char * outputBuffer, uint32_t outputBufferSize )
    {
        // see if there's anything in the pipe
        DWORD bytesAvail;
        VERIFY( PeekNamedPipe( handle, nullptr, 0, 0, (LPDWORD)&bytesAvail, 0 ) );
        if ( bytesAvail == 0 )
        {
            return 0;
        }

        // if there is more available than we have space for, just read as much as we can
        uint32_t bytesToRead = Math::Min<uint32_t>( outputBufferSize, bytesAvail );

        // read the data
        DWORD bytesReadNow = 0;
        VERIFY( ReadFile( handle, outputBuffer, bytesToRead, (LPDWORD)&bytesReadNow, 0 ) );
        ASSERT( bytesReadNow == bytesToRead );
        return bytesToRead;
    }
#endif

// GetCurrentId
//------------------------------------------------------------------------------
/*static*/ uint32_t Process::GetCurrentId()
{
    #if defined( __WINDOWS__ )
        return ::GetCurrentProcessId();
    #elif defined( __LINUX__ )
        return 0; // TODO: Implement GetCurrentId()
    #elif defined( __OSX__ )
        return 0; // TODO: Implement GetCurrentId()
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
