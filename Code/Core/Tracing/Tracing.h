// Tracing.h
//------------------------------------------------------------------------------
#pragma once
#ifndef CORE_TRACING_TRACING_H
#define CORE_TRACING_TRACING_H

// Includes
//------------------------------------------------------------------------------
#include "Core/Env/Types.h"

// Macros
//------------------------------------------------------------------------------
#ifdef DEBUG
	#define DEBUGSPAM( ... )	Tracing::DebugSpamFormat( __VA_ARGS__ )
	#define WARNING( ... )		Tracing::WarningFormat( __FILE__, __LINE__, ##__VA_ARGS__ )
#else
	#define DEBUGSPAM( ... )	Tracing::DoNothing()
	#define WARNING( ... )		Tracing::DoNothing()
#endif
#define OUTPUT( ... )			Tracing::OutputFormat( __VA_ARGS__ )
#define FATALERROR( ... )		Tracing::FatalErrorFormat( __VA_ARGS__ )

// Tracing
//------------------------------------------------------------------------------
class Tracing
{
public:
	static inline void DoNothing() {}

	#ifdef DEBUG
		static void DebugSpam( const char * message );
		static void DebugSpamFormat( const char * fmtString, ... );
		static void Warning( const char * file, uint32_t line, const char * message );
		static void WarningFormat( const char * file, uint32_t line, const char * fmtString, ... );
	#endif
	static void Output( const char * message );
	static void OutputFormat( const char * fmtString, ... );
	static void FatalError( const char * message );
	static void FatalErrorFormat( const char * fmtString, ... );

	typedef bool Callback( const char * mesage );
	static void SetCallbackDebugSpam( Callback callback );
	static void SetCallbackOutput( Callback callback );

private:
	static Callback * m_CallbackDebugSpam;
	static Callback * m_CallbackOutput;
};

//------------------------------------------------------------------------------
#endif // CORE_TRACING_TRACING_H
