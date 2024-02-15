// main.cpp
//------------------------------------------------------------------------------

// Includes
//------------------------------------------------------------------------------
// system
#if defined( __WINDOWS__ )
    #include <Windows.h>
#endif
#if defined( __LINUX__ ) || defined( __APPLE__ )
    #include <errno.h>
    #include <sys/file.h>
    #include <unistd.h>
#endif
#include <stdio.h>
#include <stdlib.h>

// LockSystemMutex
//------------------------------------------------------------------------------
bool LockSystemMutex( const char * name )
{
    // NOTE: This function doesn't cleanup failures to simplify the test
    //       (the process will be terminated anyway)

    // Acquire system mutex which should be uncontested
    #if defined( __WINDOWS__ )
        CreateMutex( nullptr, TRUE, name );
        if ( GetLastError() == ERROR_ALREADY_EXISTS )
        {
            return false; // Test fails
        }
        return true;
    #elif defined( __LINUX__ ) || defined( __APPLE__ )
        char tempFileName[256];
        sprintf( tempFileName, "/tmp/%s.lock", name );
        int handle = open( tempFileName, O_CREAT | O_RDWR, 0666 );
        if ( handle < 0 )
        {
            return false; // Test fails
        }
        int rc = flock( handle, LOCK_EX | LOCK_NB );
        if ( rc )
        {
            return false; // Test fails
        }
        return true;
    #else
        #error Unknown platform
    #endif
}

// Spawn
//------------------------------------------------------------------------------
bool Spawn( const char * exe, const char * mutexId )
{
    // NOTE: This function doesn't cleanup failures to simplify the test
    //       (the process will be terminated anyway)

    #if defined( __WINDOWS__ )
        // Set up the start up info struct.
        STARTUPINFO si;
        ZeroMemory( &si, sizeof(STARTUPINFO) );
        si.cb = sizeof( STARTUPINFO );
        si.dwFlags |= STARTF_USESHOWWINDOW;
        si.wShowWindow = SW_HIDE;

        // Prepare args
        char fullArgs[256];
        sprintf_s( fullArgs, "\"%s\" %s", exe, mutexId );

        // create the child
        LPPROCESS_INFORMATION processInfo;
        if ( !CreateProcess( nullptr,
                             fullArgs,
                             nullptr,
                             nullptr,
                             false, // inherit handles
                             0,
                             nullptr,
                             nullptr,
                             &si,
                             (LPPROCESS_INFORMATION)&processInfo ) )
        {
            return false;
        }
        return true;
    #else
        // prepare args
        const char * args[ 3 ] = { exe, mutexId, nullptr };

        // fork the process
        const pid_t childProcessPid = fork();
        if ( childProcessPid == -1 )
        {
            return false;
        }

        const bool isChild = ( childProcessPid == 0 );
        if ( isChild )
        {
            // transfer execution to new executable
            execv( const_cast<char *>( exe ), const_cast<char **>( args ) );
            exit( -1 ); // only get here if execv fails
        }
        else
        {
            return true;
        }
    #endif
}

// Sleep
//------------------------------------------------------------------------------
#if !defined( __WINDOWS__ )
void Sleep( unsigned int milliseconds )
{
    // Provide API that matches Windows for convenience
    usleep( milliseconds * 1000 );
}
#endif

// Main
//------------------------------------------------------------------------------
int main( int argc, char ** argv )
{
    // Check args
    if ( argc < 2 )
    {
        printf( "Bad args (1)\n" );
        return 1;
    }

    // Get the mutex id passed on the command line
    const int mutexId = atoi( argv[ 1 ] );
    if ( ( mutexId < 1 ) || ( mutexId > 4 ) )
    {
        printf( "Bad args (2)\n" );
        return 2;
    }

    // Spawn child if we're not the last one
    if ( mutexId < 4 )
    {
        char mutexIdString[2] = { ' ', 0 };
        mutexIdString[ 0 ] = (char)( '0' + mutexId + 1 );
        if ( !Spawn( argv[ 0 ], mutexIdString ) )
        {
            printf( "Failed to spawn child '%s'\n", mutexIdString );
            return 3;
        }
    }

    // Acquire SystemMutex which test uses to check our lifetimes
    const char * const mutexNames[4] =
    {
        "FASTBuildFastCancelTest1",
        "FASTBuildFastCancelTest2",
        "FASTBuildFastCancelTest3",
        "FASTBuildFastCancelTest4"
    };
    const char * const mutexName = mutexNames[ mutexId - 1 ];

    // Try to acquire repeatedly to manage races with the test that
    // is monitoring these processes
    int tryCount = 0;
    while ( LockSystemMutex( mutexName ) == false )
    {
        ::Sleep( 1 );
        ++tryCount;
        if ( tryCount == 100 )
        {
            printf( "Failed to acquire: %s\n", mutexName );
            return 4;
        }
    }

    // Spin forever - test will terminate
    int count = 0;
    for (;;)
    {
        ::Sleep( 1000 );

        // If we haven't been terminated in a sensible time frame
        // quit to avoid zombie processes. Useful when debugging
        // the test also
        if ( count > 10 )
        {
            printf( "Alive for too long: %s\n", mutexName );
            return 5;
        }
        ++count;
    }
}

//------------------------------------------------------------------------------
