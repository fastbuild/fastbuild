// Env.cpp
//------------------------------------------------------------------------------

// Includes
//------------------------------------------------------------------------------
#include "Env.h"

// Core
#include "Core/Containers/Array.h"
#include "Core/Process/Atomic.h"
#include "Core/Strings/AStackString.h"

#if defined( __WINDOWS__ )
    #include "Core/Env/WindowsHeader.h"
    #include <lmcons.h>
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
    #include <mach-o/dyld.h>
    extern "C"
    {
        int * _NSGetArgc( void );
        char *** _NSGetArgv( void );
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

        // As of Windows 7 -> Windows 10 / Windows Server 2016, the below information is valid:
        // On Systems with <= 64 Logical Processors, NUMBER_OF_PROCESSORS == "Logical Processor Count"
        // On Systems with >  64 Logical Processors, NUMBER_OF_PROCESSORS != "Logical Processor Count"
        // In the latter case, NUMBER_OF_PROCESSORS == "Logical Processor Count in this thread's NUMA Node"
        // So, we will check to see how many NUMA Nodes the system has here, and proceed accordingly:
        ULONG uNumNodes = 0;
        PULONG pNumNodes = &uNumNodes;
        GetNumaHighestNodeNumber( pNumNodes );

        const uint32_t numNodes = uNumNodes;
        if ( numNodes == 0 )
        {
            // The number of logical processors in the system all exist in one NUMA Node,
            // This means that NUMBER_OF_PROCESSORS represents the number of logical processors
            return numProcessors;
        }

        // NUMBER_OF_PROCESSORS is incorrect for our system, so loop over all NUMA Nodes and accumulate logical core counts
        uint32_t numProcessorsInAllGroups = 0;
        for( USHORT nodeID = 0; nodeID <= numNodes; ++nodeID )
        {
            GROUP_AFFINITY groupProcessorMask;
            memset( &groupProcessorMask, 0, sizeof( GROUP_AFFINITY ) );

            GetNumaNodeProcessorMaskEx( nodeID, &groupProcessorMask );

            // ULONG maxLogicalProcessorsInThisGroup = KeQueryMaximumProcessorCountEx( NodeID );
            // Each NUMA Node has a maximum of 32 cores on 32-bit systems and 64 cores on 64-bit systems
            size_t maxLogicalProcessorsInThisGroup = sizeof( size_t ) * 8; // ( NumBits = NumBytes * 8 )
            uint32_t numProcessorsInThisGroup = 0;

            for ( size_t processorID = 0; processorID < maxLogicalProcessorsInThisGroup; ++processorID )
            {
                numProcessorsInThisGroup += ( ( groupProcessorMask.Mask & ( uint64_t(1) << processorID ) ) != 0 ) ? 1 : 0;
            }

            numProcessorsInAllGroups += numProcessorsInThisGroup;
        }

        // If this ever fails, we might have encountered a weird new configuration
        ASSERT( numProcessorsInAllGroups >= numProcessors );

        // If we computed more processors via NUMA Groups, use that number
        // Otherwise, fallback to returning NUMBER_OF_PROCESSORS
        // Even though we never expect the numa detection to be less, ensure we don't
        // behave badly by using NUMBER_OF_PROCESSORS in that case
        return ( numProcessorsInAllGroups > numProcessors ) ? numProcessorsInAllGroups : numProcessors;
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
        // Get the required buffer size
        uint32_t bufferSize = 0;
        VERIFY( _NSGetExecutablePath( nullptr, &bufferSize ) == -1 );
        ASSERT( bufferSize > 0 ); // Updated by _NSGetExecutablePath
    
        // Reserve enough space (-1 since bufferSize includes the null)
        output.SetLength( bufferSize - 1 );
        VERIFY( _NSGetExecutablePath( output.Get(), &bufferSize ) == 0 );
    #else
        char path[ PATH_MAX ];
        const ssize_t length = readlink( "/proc/self/exe", path, PATH_MAX );
        ASSERT( length != -1 );
        ASSERT( length <= PATH_MAX );
        output.Assign( path, path + length );
    #endif
}

// IsStdOutRedirectedInternal
//------------------------------------------------------------------------------
static bool IsStdOutRedirectedInternal()
{
    #if defined( __WINDOWS__ )
        HANDLE h = GetStdHandle( STD_OUTPUT_HANDLE );
        ASSERT( h != INVALID_HANDLE_VALUE );
        if ( h == nullptr )
        {
            return true; // There is no handle associated with stdout, this is essentially a redirection to NUL
        }

        // Check if the handle belongs to a console window
        DWORD unused;
        if ( GetConsoleMode( h, (LPDWORD)&unused ) )
        {
            return false; // Console window exists, so there is no redirection
        }

        // Check if the handle belongs to a pipe used by Cygwin/MSYS to forward output to a terminal
        if ( GetFileType( h ) != FILE_TYPE_PIPE )
        {
            return true; // Redirected to something that is not a pipe
        }

        alignas( __alignof( FILE_NAME_INFO ) ) char buffer[ sizeof( FILE_NAME_INFO ) + MAX_PATH * sizeof( wchar_t ) ];
        if ( ! GetFileInformationByHandleEx( h, FileNameInfo, buffer, sizeof( buffer ) ) )
        {
            return true; // Redirected to something that doesn't have a name
        }

        FILE_NAME_INFO * info = reinterpret_cast< FILE_NAME_INFO * >( buffer );
        info->FileName[ info->FileNameLength / sizeof( wchar_t ) ] = L'\0';

        // Check if name of the pipe matches pattern "\cygwin-%llx-pty%d-to-master" or "\msys-%llx-pty%d-to-master"
        const wchar_t * p = nullptr;
        if ( ( p = wcsstr( info->FileName, L"\\cygwin-" ) ) != nullptr )
        {
            p += 7;
        }
        else if ( ( p = wcsstr( info->FileName, L"\\msys-" ) ) != nullptr )
        {
            p += 5;
        }
        else
        {
            return true; // Redirected to a pipe that is not related to Cygwin/MSYS
        }
        int nChars = 0;
        PRAGMA_DISABLE_PUSH_MSVC( 4996 ) // This function or variable may be unsafe...
        PRAGMA_DISABLE_PUSH_CLANG_WINDOWS( "-Wdeprecated-declarations" ) // 'swscanf' is deprecated: This function or variable may be unsafe...
        if ( ( swscanf( p, L"%*llx-pty%*d-to-master%n", &nChars ) == 0 ) && ( nChars > 0 ) ) // TODO:C Consider using swscanf_s
        PRAGMA_DISABLE_POP_CLANG_WINDOWS // -Wdeprecated-declarations
        PRAGMA_DISABLE_POP_MSVC // 4996
        {
            return false; // Pipe name matches the pattern, stdout is forwarded to a terminal by Cygwin/MSYS
        }

        return true;
    #elif defined( __LINUX__ ) || defined( __APPLE__ )
        return ( isatty( STDOUT_FILENO ) == 0 );
    #else
        #error Unknown platform
    #endif
}

// GetUserName
//------------------------------------------------------------------------------
/*static*/ bool Env::GetLocalUserName( AString & outUserName )
{
    #if defined( __WINDOWS__ )
        char userName[ UNLEN + 1 ];
        DWORD bufferSize = sizeof(userName);
        if ( ::GetUserNameA( userName, &bufferSize ) == FALSE )
        {
            return false;
        }
        outUserName = userName;
        return true;
    #else
        return GetEnvVariable( "USER", outUserName );
    #endif
}

// IsStdOutRedirected
//------------------------------------------------------------------------------
/*static*/ bool Env::IsStdOutRedirected( const bool recheck )
{
    static volatile int32_t sCachedResult = 0; // 0 - not checked, 1 - true, 2 - false
    const int32_t result = AtomicLoadRelaxed( &sCachedResult );
    if ( recheck || ( result == 0 ) )
    {
        if ( IsStdOutRedirectedInternal() )
        {
            AtomicStoreRelaxed( &sCachedResult, 1 );
            return true;
        }
        else
        {
            AtomicStoreRelaxed( &sCachedResult, 2 );
            return false;
        }
    }
    else
    {
        return ( result == 1 );
    }
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

// AllocEnvironmentString
//------------------------------------------------------------------------------
/*static*/ const char * Env::AllocEnvironmentString( const Array< AString > & environment )
{
    size_t len = 0;
    const size_t numEnvVars = environment.GetSize();
    for ( size_t i = 0; i < numEnvVars; ++i )
    {
        len += environment[i].GetLength() + 1;
    }
    len += 1; // for double null

    // Now that the environment string length is calculated, allocate and fill.
    char * mem = (char *)ALLOC( len );
    const char * environmentString = mem;
    for ( size_t i = 0; i < numEnvVars; ++i )
    {
        const AString & envVar = environment[i];
        AString::Copy( envVar.Get(), mem, envVar.GetLength() + 1 );
        mem += ( envVar.GetLength() + 1 );
    }
    *mem = 0;

    return environmentString;
}

// ShowMsgBox
//------------------------------------------------------------------------------
void Env::ShowMsgBox( const char * title, const char * msg )
{
    #if defined( __WINDOWS__ )
        MessageBoxA( nullptr, msg, title, MB_OK );
    #elif defined( __APPLE__ )
        (void)title;
        (void)msg; // TODO:MAC Implement ShowMsgBox
    #elif defined( __LINUX__ )
        (void)title;
        (void)msg; // TODO:LINUX Implement ShowMsgBox
    #else
        #error Unknown Platform
    #endif
}

//------------------------------------------------------------------------------
