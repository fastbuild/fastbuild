// Tracing
//------------------------------------------------------------------------------

// Includes
//------------------------------------------------------------------------------
#include "Tracing.h"
#include "Core/Env/Assert.h"

#include "Core/Profile/Profile.h"
#include "Core/Strings/AStackString.h"

#include <stdarg.h>
#include <stdio.h>
#if defined( __WINDOWS__ )
    #ifdef DEBUG
        #include "Core/Env/WindowsHeader.h" // for OutputDebugStringA
    #endif
#endif

// Defines
//------------------------------------------------------------------------------

// Static Data
//------------------------------------------------------------------------------
/*static*/ Tracing::Callbacks Tracing::s_Callbacks;
/*static*/ bool Tracing::Callbacks::s_Valid( false );

#ifdef DEBUG
    // DebugSpam
    //------------------------------------------------------------------------------
    /*static*/ void Tracing::DebugSpam( const char * message )
    {
        PROFILE_FUNCTION;

        const bool messageConsumed = s_Callbacks.DispatchCallbacksDebugSpam( message );
        if ( messageConsumed )
        {
            return; // Callback swallowed output
        }

        // normal output that goes to the TTY
        fputs( message, stdout );

        // emit to the debugger as well if possible
        #if defined( __WINDOWS__ )
            OutputDebugStringA( message );
        #endif
    }

    // DebugSpamFormat
    //------------------------------------------------------------------------------
    /*static*/ void Tracing::DebugSpamFormat( MSVC_SAL_PRINTF const char * fmtString, ... )
    {
        AStackString< 8192 > buffer;

        va_list args;
        va_start( args, fmtString );
        buffer.VFormat( fmtString, args );
        va_end( args );

        DebugSpam( buffer.Get() );
    }

    // Warning
    //------------------------------------------------------------------------------
    /*static*/ void Tracing::Warning( const char * file, uint32_t line, const char * message )
    {
        // format a double clickable line
        AStackString< 8192 > buffer;
        buffer.Format( "%s(%u): %s\n", file, line, message );

        // normal output that goes to the TTY
        puts( buffer.Get() );

        // emit to the debugger as well if possible
        #if defined( __WINDOWS__ )
            OutputDebugStringA( buffer.Get() );
        #endif
    }

    // WarningFormat
    //------------------------------------------------------------------------------
    /*static*/ void Tracing::WarningFormat( MSVC_SAL_PRINTF const char * file, uint32_t line, const char * fmtString, ... )
    {
        AStackString<> buffer;

        va_list args;
        va_start( args, fmtString );
        buffer.VFormat( fmtString, args );
        va_end( args );

        Warning( file, line, buffer.Get() );
    }
#endif

// Output
//------------------------------------------------------------------------------
/*static*/ void Tracing::Output( const char * message )
{
    const bool messageConsumed = s_Callbacks.DispatchCallbacksOutput( message );
    if ( messageConsumed )
    {
        return; // Callback swallowed output
    }

    // normal output that goes to the TTY
    fputs( message, stdout );

    // emit to the debugger as well if possible
    #if defined( __WINDOWS__ )
        #ifdef DEBUG
            OutputDebugStringA( message );
        #endif
    #endif
}

// OutputFormat
//------------------------------------------------------------------------------
/*static*/ void Tracing::OutputFormat( MSVC_SAL_PRINTF const char * fmtString, ... )
{
    AStackString< 8192 > buffer;

    va_list args;
    va_start( args, fmtString );
    buffer.VFormat( fmtString, args );
    va_end( args );

    Output( buffer.Get() );
}

// Error
//------------------------------------------------------------------------------
/*static*/ void Tracing::FatalError( const char * message )
{
    // tty output
    puts( message );

    // to the debugger if available
    #if defined( __WINDOWS__ )
        #ifdef DEBUG
            OutputDebugStringA( message );
        #endif
    #endif

    // for now, we'll just break
    BREAK_IN_DEBUGGER;
}

// ErrorFormat
//------------------------------------------------------------------------------
/*static*/ void Tracing::FatalErrorFormat( MSVC_SAL_PRINTF const char * fmtString, ... )
{
    AStackString< 8192 > buffer;

    va_list args;
    va_start( args, fmtString );
    buffer.VFormat( fmtString, args );
    va_end( args );

    FatalError( buffer.Get() );
}

// Tracing::Callback (CONSTRUCTOR)
//------------------------------------------------------------------------------
Tracing::Callbacks::Callbacks()
    : m_CallbacksMutex()
    , m_InCallbackDispatch( false )
    , m_CallbacksDebugSpam( 2, true )
    , m_CallbacksOutput( 2, true )
{
    // Callbacks can now be modified or dispatched
    s_Valid = true;
}

// Tracing::Callback (DESTRUCTOR)
//------------------------------------------------------------------------------
Tracing::Callbacks::~Callbacks()
{
    // Callbacks can no longer be modified or dispatched
    s_Valid = false;
}

//------------------------------------------------------------------------------
void Tracing::Callbacks::AddCallbackDebugSpam( Callback * callback )
{
    // It is not valid to modify callbacks during static init/destruction
    ASSERT( s_Valid );

    MutexHolder lock( m_CallbacksMutex );
    ASSERT( m_CallbacksDebugSpam.Find( callback ) == nullptr );
    m_CallbacksDebugSpam.Append( callback );
}

//------------------------------------------------------------------------------
void Tracing::Callbacks::AddCallbackOutput( Callback * callback )
{
    // It is not valid to modify callbacks during static init/destruction
    ASSERT( s_Valid );

    MutexHolder lock( m_CallbacksMutex );
    ASSERT( m_CallbacksOutput.Find( callback ) == nullptr );
    m_CallbacksOutput.Append( callback );
}

//------------------------------------------------------------------------------
void Tracing::Callbacks::RemoveCallbackDebugSpam( Callback * callback )
{
    // It is not valid to modify callbacks during static init/destruction
    ASSERT( s_Valid );

    MutexHolder lock( m_CallbacksMutex );
    VERIFY( m_CallbacksDebugSpam.FindAndErase( callback ) );
}

//------------------------------------------------------------------------------
void Tracing::Callbacks::RemoveCallbackOutput( Callback * callback )
{
    // It is not valid to modify callbacks during static init/destruction
    ASSERT( s_Valid );

    MutexHolder lock( m_CallbacksMutex );
    VERIFY( m_CallbacksOutput.FindAndErase( callback ) );
}

// Callbacks::DispatchCallbacksDebugSpam
//------------------------------------------------------------------------------
bool Tracing::Callbacks::DispatchCallbacksDebugSpam( const char * message )
{
    // Callbacks are unavailable?
    if ( s_Valid == false )
    {
        return false;
    }

    MutexHolder lock( m_CallbacksMutex );

    // Prevent re-entrancy (if as ASSERT fires during a callback for example)
    if ( m_InCallbackDispatch )
    {
        return false;
    }
    m_InCallbackDispatch = true;

    for ( Tracing::Callback * cb : m_CallbacksDebugSpam )
    {
        if ( (*cb)( message ) == false )
        {
            m_InCallbackDispatch = false;
            return true; // callback wants msg supressed
        }
    }

    m_InCallbackDispatch = false;
    return false;
}

// Callbacks::DispatchCallbacksOutput
//------------------------------------------------------------------------------
bool Tracing::Callbacks::DispatchCallbacksOutput( const char * message )
{
    // Callbacks are unavailable?
    if ( s_Valid == false )
    {
        return false;
    }

    MutexHolder lock( m_CallbacksMutex );

    // Prevent re-entrancy (if as ASSERT fires during a callback for example)
    if ( m_InCallbackDispatch )
    {
        return false;
    }
    m_InCallbackDispatch = true;

    for ( Tracing::Callback * cb : m_CallbacksOutput )
    {
        if ( (*cb)( message ) == false )
        {
            m_InCallbackDispatch = false;
            return true; // callback wants msg supressed
        }
    }

    m_InCallbackDispatch = false;
    return false;
}

//------------------------------------------------------------------------------
