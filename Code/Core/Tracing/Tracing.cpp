// Tracing
//------------------------------------------------------------------------------

// Includes
//------------------------------------------------------------------------------
#include "Core/PrecompiledHeader.h"

#include "Tracing.h"
#include "Core/Env/Assert.h"

#include "Core/Profile/Profile.h"
#include "Core/Strings/AStackString.h"

#include <stdio.h>
#include <stdarg.h>
#if defined( __WINDOWS__ )
    #ifdef DEBUG
        #include <windows.h> // for OutputDebugStringA
    #endif
#endif

// Defines
//------------------------------------------------------------------------------

// Static Data
//------------------------------------------------------------------------------
/*static*/ Tracing::Callback * Tracing::m_CallbackDebugSpam = nullptr;
/*static*/ Tracing::Callback * Tracing::m_CallbackOutput = nullptr;

#ifdef DEBUG
	// DebugSpam
	//------------------------------------------------------------------------------
	/*static*/ void Tracing::DebugSpam( const char * message )
	{
        PROFILE_FUNCTION

		// pass through callback if there is one
		if ( m_CallbackDebugSpam )
		{
			if ( (*m_CallbackDebugSpam)( message ) == false )
			{
				return; // callback wants msg supressed
			}
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
	/*static*/ void Tracing::DebugSpamFormat( const char * fmtString, ... )
	{
		AStackString< 8192 > buffer;

		va_list args;
		va_start(args, fmtString);
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
	/*static*/ void Tracing::WarningFormat( const char * file, uint32_t line, const char * fmtString, ... )
	{
		AStackString<> buffer;

		va_list args;
		va_start(args, fmtString);
		buffer.VFormat( fmtString, args );
		va_end( args );

		Warning( file, line, buffer.Get() );
	}
#endif

// Output
//------------------------------------------------------------------------------
/*static*/ void Tracing::Output( const char * message )
{
    PROFILE_FUNCTION

	// pass through callback if there is one
	if ( m_CallbackOutput )
	{
		if ( (*m_CallbackOutput)( message ) == false )
		{
			return; // callback wants msg supressed
		}
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
/*static*/ void Tracing::OutputFormat( const char * fmtString, ... )
{
	AStackString< 8192 > buffer;

	va_list args;
	va_start(args, fmtString);
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
/*static*/ void Tracing::FatalErrorFormat( const char * fmtString, ... )
{
	AStackString< 8192 > buffer;

	va_list args;
	va_start(args, fmtString);
	buffer.VFormat( fmtString, args );
	va_end( args );

	FatalError( buffer.Get() );
}

// SetCallbackDebugSpam
//------------------------------------------------------------------------------
/*static*/ void Tracing::SetCallbackDebugSpam( Callback callback )
{
	m_CallbackDebugSpam = callback;
}

// SetCallbackOutput
//------------------------------------------------------------------------------
/*static*/ void Tracing::SetCallbackOutput( Callback callback )
{
	m_CallbackOutput = callback;
}

//------------------------------------------------------------------------------
