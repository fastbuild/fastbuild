// Env.cpp
//------------------------------------------------------------------------------

// Includes
//------------------------------------------------------------------------------
#include "Core/PrecompiledHeader.h"

#include "Env.h"

// Core
#include "Core/Strings/AStackString.h"

#if defined( __WINDOWS__ )
    #include <windows.h>
    #include <stdio.h>
#endif

#if defined( __LINUX__ ) || defined( __APPLE__ )
    #include <errno.h>
    #include <stdio.h>
    #include <stdlib.h>
    #include <unistd.h>
#endif

#if defined( __LINUX__ )
    #include <linux/limits.h>
#endif

#if defined( __APPLE__ )
    extern "C"
    {
        int *_NSGetArgc(void);
        char ***_NSGetArgv(void);
    };
#endif

// GetNumProcessors
//------------------------------------------------------------------------------
/*static*/ uint32_t Env::GetNumProcessors()
{
    #if defined( __WINDOWS__ )
        // Default to NUMBER_OF_PROCESSORS
        uint32_t numProcessors = 1;

        AStackString< 32 > var;
        if ( GetEnvVariable( "NUMBER_OF_PROCESSORS", var ) )
        {
            if ( sscanf_s( var.Get(), "%u", &numProcessors ) != 1 )
            {
                numProcessors = 1;
            }
        }

        return numProcessors;
    #elif defined( __LINUX__ ) || defined( __APPLE__ )
        long numCPUs = sysconf( _SC_NPROCESSORS_ONLN );
        if ( numCPUs <= 0 )
        {
            ASSERT( false ); // this should never fail
            numCPUs = 1;
        }
        return ( uint32_t )numCPUs;
    #else
        #error Unknown platform
    #endif
}

// GetEnvVariable
//------------------------------------------------------------------------------
/*static*/ bool Env::GetEnvVariable( const char * envVarName, AString & envVarValue )
{
    #if defined( __WINDOWS__ )
        // try to get the env variable into whatever storage we have available
        uint32_t maxSize = envVarValue.GetReserved();
        DWORD ret = ::GetEnvironmentVariable( envVarName, envVarValue.Get(), maxSize );

        // variable does not exist
        if ( ret == 0 )
        {
            return false;
        }

        // variable exists - was there enough space?
        if ( ret > maxSize )
        {
            // not enough space, allocate enough
            envVarValue.SetReserved( ret );
            maxSize = envVarValue.GetReserved();
            ret = ::GetEnvironmentVariable( envVarName, envVarValue.Get(), maxSize );
            ASSERT( ret <= maxSize ); // should have fit
        }

        // make string aware of valid buffer length as populated by ::GetEnvironmentVariable
        envVarValue.SetLength( ret );

        return true;
    #elif defined( __LINUX__ ) || defined( __APPLE__ )
        const char * envVar = ::getenv( envVarName );
        if ( envVar )
        {
            envVarValue = envVar;
            return true;
        }
        return false;
    #else
        #error Unknown platform
    #endif
}

// SetEnvVariable
//------------------------------------------------------------------------------
/*static*/ bool Env::SetEnvVariable( const char * envVarName, const AString & envVarValue )
{
    #if defined( __WINDOWS__ )
        return ::SetEnvironmentVariable( envVarName, envVarValue.Get() ) != 0;
    #elif defined( __LINUX__ ) || defined( __APPLE__ )
        return ::setenv( envVarName, envVarValue.Get(), 1 ) == 0;
    #else
        #error Unknown platform
    #endif
}

// GetCmdLine
//------------------------------------------------------------------------------
/*static*/ void Env::GetCmdLine( AString & cmdLine )
{
    #if defined( __WINDOWS__ )
        cmdLine = ::GetCommandLine();
    #elif defined( __APPLE__ )
        int argc = *_NSGetArgc();
        const char ** argv = const_cast< const char ** >( *_NSGetArgv() );
        for ( int i=0; i<argc; ++i )
        {
            if ( i > 0 )
            {
                cmdLine += ' ';
            }
            cmdLine += argv[i];
        }
    #else
        FILE* f = fopen( "/proc/self/cmdline", "rb" );
        VERIFY( f != 0 );
        char buffer[ 4096 ];
        for (;;)
        {
            int size = fread( buffer, 1, 4096, f );
            if ( size == 0 )
            {
                break;
            }

            // Append
            for ( int i=0; i<size; ++i )
            {
                const char c = buffer[ i ];
                cmdLine += ( c ? c : ' ' ); // convert nulls in between args back into spaces
            }
        }
        VERIFY( fclose( f ) == 0 );

    #endif
}

// GetExePath
//------------------------------------------------------------------------------
void Env::GetExePath( AString & output )
{
    #if defined( __WINDOWS__ )
        HMODULE hModule = GetModuleHandleA( nullptr );
        char path[ MAX_PATH ];
        GetModuleFileNameA( hModule, path, MAX_PATH );
        output = path;
    #elif defined( __APPLE__ )
        const char ** argv = const_cast< const char ** >( *_NSGetArgv() );
        output = argv[0];
    #else
        char path[ PATH_MAX ];
        VERIFY( readlink( "/proc/self/exe", path, PATH_MAX ) != -1 );
        output = path;
    #endif
}

// GetLastErr
//------------------------------------------------------------------------------
/*static*/ uint32_t Env::GetLastErr()
{
    #if defined( __WINDOWS__ )
        return ::GetLastError();
    #elif defined( __LINUX__ ) || defined( __APPLE__ )
        return errno;
    #else
        #error Unknown platform
    #endif
}

//------------------------------------------------------------------------------
