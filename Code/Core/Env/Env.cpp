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
    #include <stdlib.h>
    #include <unistd.h>
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
            ASSERT( false ); // this should this whould never fail
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

// GetCmdLine
//------------------------------------------------------------------------------
/*static*/ const char * Env::GetCmdLine()
{
	#if defined( __WINDOWS__ )
		return ::GetCommandLine();
	#elif defined( __APPLE__ )
		ASSERT( false ); // TODO:MAC Implement GetCmdLine
		return nullptr;
	#elif defined( __LINUX__ )
		ASSERT( false ); // TODO:LINUX Implement GetCmdLine
		return nullptr;
	#else
        #error Unknown platform
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
		ASSERT( false ); // TODO:MAC Implement GetExePath
	#elif defined( __LINUX__ )
		ASSERT( false ); // TODO:LINUX Implement GetExePath
	#else
        #error Unknown platform
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
