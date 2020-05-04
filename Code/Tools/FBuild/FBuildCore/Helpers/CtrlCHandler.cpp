// CtrlCHandler
//------------------------------------------------------------------------------

// Includes
//------------------------------------------------------------------------------
#include "CtrlCHandler.h"

// FBuild
#include "Tools/FBuild/FBuildCore/FBuild.h"

// Core
#include "Core/Tracing/Tracing.h"

// System
#if defined( __WINDOWS__ )
    #include "Core/Env/WindowsHeader.h"
#elif defined( __LINUX__ ) || defined( __OSX__ )
    #include <signal.h>
#endif

// System-Specific Callbacks
//------------------------------------------------------------------------------
#if defined( __WINDOWS__ )
    BOOL WINAPI CtrlHandlerFunc( DWORD fdwCtrlType );
#elif defined( __LINUX__ ) || defined( __OSX__ )
    void CtrlHandlerFunc( int dummy );
#endif

// CONSTRUCTOR
//------------------------------------------------------------------------------
CtrlCHandler::CtrlCHandler()
{
    RegisterHandler();
}

// DESTRUCTOR
//------------------------------------------------------------------------------
CtrlCHandler::~CtrlCHandler()
{
    DeregisterHandler();
}

// RegisterHandler
//------------------------------------------------------------------------------
void CtrlCHandler::RegisterHandler()
{
    ASSERT( m_IsRegistered == false );

    #if defined( __WINDOWS__ )
        VERIFY( SetConsoleCtrlHandler( (PHANDLER_ROUTINE)CtrlHandlerFunc, TRUE ) );
    #elif defined( __LINUX__ ) || defined( __OSX__ )
        signal( SIGINT, CtrlHandlerFunc );
    #endif

    m_IsRegistered = true;
}

// DeregisterHandler
//------------------------------------------------------------------------------
void CtrlCHandler::DeregisterHandler()
{
    // Handle manual deregistration
    if ( m_IsRegistered )
    {
        #if defined( __WINDOWS__ )
            VERIFY( SetConsoleCtrlHandler( (PHANDLER_ROUTINE)nullptr, TRUE ) );
        #elif defined( __LINUX__ )
            signal( SIGINT, SIG_DFL );
        #elif defined( __OSX__ )
            // TODO:MAC Implement
        #endif

        m_IsRegistered = false;
    }
}

// CtrlHandler
//------------------------------------------------------------------------------
#if defined( __WINDOWS__ )
    BOOL WINAPI CtrlHandlerFunc( DWORD /*fdwCtrlType*/ )
    {
        // tell FBuild we want to stop the build cleanly
        FBuild::AbortBuild();

        // only printf output for the first break received
        static bool received = false;
        if ( received == false )
        {
            received = true;

            // get the console colours
            CONSOLE_SCREEN_BUFFER_INFO consoleInfo;
            VERIFY( GetConsoleScreenBufferInfo( GetStdHandle( STD_OUTPUT_HANDLE ), &consoleInfo ) );

            // print a big red msg
            VERIFY( SetConsoleTextAttribute( GetStdHandle( STD_OUTPUT_HANDLE ), FOREGROUND_RED ) );
            OUTPUT( "<<<< ABORT SIGNAL RECEIVED >>>>\n" );

            // put the console back to normal
            VERIFY( SetConsoleTextAttribute( GetStdHandle( STD_OUTPUT_HANDLE ), consoleInfo.wAttributes ) );
        }

        return TRUE; // tell Windows we've "handled" it
    }
#elif defined( __LINUX__ ) || defined( __OSX__ )
    void CtrlHandlerFunc( int /*dummy*/ )
    {
        // tell FBuild we want to stop the build cleanly
        FBuild::AbortBuild();

        // only printf output for the first break received
        static bool received = false;
        if ( received == false )
        {
            received = true;
            OUTPUT( "<<<< ABORT SIGNAL RECEIVED >>>>\n" );
        }
    }
#endif

//------------------------------------------------------------------------------
