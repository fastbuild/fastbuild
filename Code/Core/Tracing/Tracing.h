// Tracing.h
//------------------------------------------------------------------------------
#pragma once

// Includes
//------------------------------------------------------------------------------
#include "Core/Containers/Array.h"
#include "Core/Env/MSVCStaticAnalysis.h"
#include "Core/Env/Types.h"
#include "Core/Process/Mutex.h"

// Macros
//------------------------------------------------------------------------------
#ifdef DEBUG
    #define DEBUGSPAM( ... )    Tracing::DebugSpamFormat( __VA_ARGS__ )
    #define WARNING( ... )      Tracing::WarningFormat( __FILE__, __LINE__, ##__VA_ARGS__ )
#else
    #define DEBUGSPAM( ... )    Tracing::DoNothing()
    #define WARNING( ... )      Tracing::DoNothing()
#endif
#define OUTPUT( ... )           Tracing::OutputFormat( __VA_ARGS__ )
#define FATALERROR( ... )       Tracing::FatalErrorFormat( __VA_ARGS__ )

// Tracing
//------------------------------------------------------------------------------
class Tracing
{
public:
    static inline void DoNothing() {}

    #ifdef DEBUG
        static void DebugSpam( const char * message );
        static void DebugSpamFormat( MSVC_SAL_PRINTF const char * fmtString, ... ) FORMAT_STRING( 1, 2 );
        static void Warning( const char * file, uint32_t line, const char * message );
        static void WarningFormat( MSVC_SAL_PRINTF const char * file, uint32_t line, const char * fmtString, ... ) FORMAT_STRING( 3, 4 );
    #endif
    static void Output( const char * message );
    static void OutputFormat( MSVC_SAL_PRINTF const char * fmtString, ... ) FORMAT_STRING( 1, 2 );
    static void FatalError( const char * message );
    static void FatalErrorFormat( MSVC_SAL_PRINTF const char * fmtString, ... ) FORMAT_STRING( 1, 2 );

    typedef bool Callback( const char * mesage );
    static void AddCallbackDebugSpam( Callback * callback )     { s_Callbacks.AddCallbackDebugSpam( callback ); }
    static void AddCallbackOutput( Callback * callback )        { s_Callbacks.AddCallbackOutput( callback ); }
    static void RemoveCallbackDebugSpam( Callback * callback )  { s_Callbacks.RemoveCallbackDebugSpam( callback ); }
    static void RemoveCallbackOutput( Callback * callback )     { s_Callbacks.RemoveCallbackOutput( callback ); }

private:
    class Callbacks
    {
    public:
        Callbacks();
        ~Callbacks();

        void AddCallbackDebugSpam( Callback* callback );
        void AddCallbackOutput( Callback* callback );
        void RemoveCallbackDebugSpam( Callback* callback );
        void RemoveCallbackOutput( Callback* callback );

        bool DispatchCallbacksDebugSpam( const char * message );
        bool DispatchCallbacksOutput( const char* message );

    protected:
        // Tracing can occur during static initialization or shutdown
        // so we need a way to detect that to prevent unsafe access
        static bool         s_Valid;

        Mutex               m_CallbacksMutex;
        bool                m_InCallbackDispatch;
        Array< Callback * > m_CallbacksDebugSpam;
        Array< Callback * > m_CallbacksOutput;
    };
    static Callbacks        s_Callbacks;
};

//------------------------------------------------------------------------------
