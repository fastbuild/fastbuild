// Assert.cpp
//------------------------------------------------------------------------------

// Includes
//------------------------------------------------------------------------------
#include "Core/PrecompiledHeader.h"

#include "Assert.h"
#ifdef ASSERTS_ENABLED
    #include "Core/Env/Types.h"
    #include "Core/Strings/AString.h"
    #include "Core/Strings/AStackString.h"
    #include <stdarg.h>
    #include <stdio.h>
    #if defined( __WINDOWS__ )
        #include <windows.h>
    #endif
#endif

// Static
//------------------------------------------------------------------------------
#ifdef ASSERTS_ENABLED
/*static*/ bool AssertHandler::s_ThrowOnAssert( false );
#endif

// IsDebugerAttached
//------------------------------------------------------------------------------
bool IsDebuggerAttached()
{
    #if defined( __WINDOWS__ )
        return ( IsDebuggerPresent() == TRUE );
    #elif defined( __APPLE__ )
        return false; // TODO:MAC Implement IsDebugerAttached
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
        #if defined( __WINDOWS__ )
            OutputDebugStringA( buffer );
        #endif

        if ( s_ThrowOnAssert )
        {
            throw "AssertionFailed";
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
        va_start(args, fmtString);
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
