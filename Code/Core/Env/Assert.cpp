// Assert.cpp
//------------------------------------------------------------------------------

// Includes
//------------------------------------------------------------------------------
#include "Assert.h"
#ifdef ASSERTS_ENABLED
    #include "Core/Env/Types.h"
    #include "Core/Strings/AStackString.h"
    #include "Core/Strings/AString.h"
    #include <stdarg.h>
    #include <stdio.h>
#endif
#if defined( __WINDOWS__ )
    #include "Core/Env/WindowsHeader.h"
#endif
#if defined( __OSX__ )
    #include <sys/sysctl.h>
    #include <sys/types.h>
    #include <unistd.h>
#endif

// Static
//------------------------------------------------------------------------------
#ifdef ASSERTS_ENABLED
    /*static*/ AssertHandler::AssertCallback * AssertHandler::s_AssertCallback( nullptr );
#endif

// NoReturn
//------------------------------------------------------------------------------
#if defined( ASSERTS_ENABLED ) && defined( __WINDOWS__ )
    void NoReturn()
    {
        #if defined( __clang__ )
            for (;;) {}
        #endif
    }
#endif

// IsDebugerAttached
//------------------------------------------------------------------------------
bool IsDebuggerAttached()
{
    #if defined( __WINDOWS__ )
        return ( IsDebuggerPresent() == TRUE );
    #elif defined( __APPLE__ )
        //
        // From: https://developer.apple.com/library/archive/qa/qa1361/_index.html
        //

        // Initialize the flags so that, if sysctl fails for some bizarre
        // reason, we get a predictable result.
        struct kinfo_proc info;
        info.kp_proc.p_flag = 0;
        
        // Initialize mib, which tells sysctl the info we want, in this case
        // we're looking for information about a specific process ID.
        int mib[ 4 ];
        mib[ 0 ] = CTL_KERN;
        mib[ 1 ] = KERN_PROC;
        mib[ 2 ] = KERN_PROC_PID;
        mib[ 3 ] = getpid();
        
        // Call sysctl
        size_t size = sizeof(info);
        VERIFY( sysctl( mib, sizeof(mib) / sizeof(*mib), &info, &size, NULL, 0 ) == 0 );
    
        // We're being debugged if the P_TRACED flag is set.
        return ( ( info.kp_proc.p_flag & P_TRACED ) != 0 );
    #elif defined( __LINUX__ )
        return false; // TODO:LINUX Implement IsDebugerAttached
    #else
        #error Unknown platform
    #endif
}

// Failure
//------------------------------------------------------------------------------
#ifdef ASSERTS_ENABLED
/*static*/ bool AssertHandler::Failure( const char * message,
                                        const char * file,
                                        const int line )
{
    #ifdef DEBUG
        const uint32_t BUFFER_SIZE( 4096 );
        char buffer[ BUFFER_SIZE ];
        #if defined( __APPLE__ ) || defined( __LINUX__ )
            sprintf( buffer,
        #else
            sprintf_s( buffer, BUFFER_SIZE,
        #endif
            "\n-------- ASSERTION FAILED --------\n%s(%i): Assert: %s\n-----^^^ ASSERTION FAILED ^^^-----\n",
            file, line, message );

        puts( buffer );
        fflush( stdout );

        #if defined( __WINDOWS__ )
            OutputDebugStringA( buffer );
        #endif

        // Trigger user callback if needed. This may not return.
        if ( s_AssertCallback )
        {
            (*s_AssertCallback)( buffer );
        }

        if ( IsDebuggerAttached() == false )
        {
            #if defined( __WINDOWS__ )
                // TODO:LINUX Fix MessageBox use
                // TODO:OSX Fix MessageBox use
                int res = MessageBox( nullptr, buffer, "Assertion Failed - Break Execution?", MB_YESNO | MB_ICONERROR );
                return ( res == IDYES );
            #endif
        }
        return true; // break execution
    #else
        return true; // break execution
    #endif
}

// FailureM
//------------------------------------------------------------------------------
/*static*/ bool AssertHandler::FailureM( const char * message,
                                         const char * file,
                                         const int line,
                                         const char * fmtString,
                                         ... )
{
    #ifdef DEBUG
        AStackString< 4096 > buffer( message );
        buffer += '\n';

        AStackString< 4096 > buffer2;
        va_list args;
        va_start( args, fmtString );
        buffer2.VFormat( fmtString, args );
        va_end( args );

        buffer += buffer2;

        return Failure( buffer.Get(), file, line );
    #else
        return true; // break execution
    #endif
}

//------------------------------------------------------------------------------
#endif // ASSERTS_ENABLED
