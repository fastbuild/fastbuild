// FLog
//------------------------------------------------------------------------------
#pragma once
#ifndef FBUILD_FLOG_H
#define FBUILD_FLOG_H

// Includes
//------------------------------------------------------------------------------
#include "Core/Strings/AStackString.h"


// Macros
//------------------------------------------------------------------------------
#define FLOG_INFO( fmtString, ... )					\
	do {											\
		if ( FLog::ShowInfo() )						\
		{											\
			FLog::Info( fmtString, ##__VA_ARGS__ );	\
		}											\
	PRAGMA_DISABLE_PUSH_MSVC(4127)					\
	} while ( false );								\
	PRAGMA_DISABLE_POP_MSVC

#define FLOG_BUILD( fmtString, ... )				\
	do {											\
		FLog::Build( fmtString, ##__VA_ARGS__ );		\
	PRAGMA_DISABLE_PUSH_MSVC(4127)					\
	} while ( false );								\
	PRAGMA_DISABLE_POP_MSVC

#define FLOG_BUILD_DIRECT( message )			    \
	do {											\
		FLog::BuildDirect( message );               \
	PRAGMA_DISABLE_PUSH_MSVC(4127)					\
	} while ( false );								\
	PRAGMA_DISABLE_POP_MSVC

#define FLOG_WARN( fmtString, ... )					\
	do {											\
		FLog::Warning( fmtString, ##__VA_ARGS__ );	\
	PRAGMA_DISABLE_PUSH_MSVC(4127)					\
	} while ( false );								\
	PRAGMA_DISABLE_POP_MSVC

#define FLOG_ERROR( fmtString, ... )				\
	do {											\
		FLog::Error( fmtString, ##__VA_ARGS__ );		\
	PRAGMA_DISABLE_PUSH_MSVC(4127)					\
	} while ( false );								\
	PRAGMA_DISABLE_POP_MSVC

#define FLOG_ERROR_DIRECT( message )                \
	do {											\
		FLog::ErrorDirect( message );				\
	PRAGMA_DISABLE_PUSH_MSVC(4127)					\
	} while ( false );								\
	PRAGMA_DISABLE_POP_MSVC

// FLog class
//------------------------------------------------------------------------------
class FLog
{
public:	
	inline static bool ShowInfo() { return s_ShowInfo; }
	inline static bool ShowErrors() { return s_ShowErrors; }

	static void Info( const char * formatString, ... );
	static void Build( const char * formatString, ... );
	static void Warning( const char * formatString, ... );
	static void Error( const char * formatString, ... );

	// for large, already formatted messages
    static void BuildDirect( const char * message );
	static void ErrorDirect( const char * message );

	static void StartBuild();
	static void StopBuild();

	static void OutputProgress( float time, float percentage, uint32_t numJobs, uint32_t numJobsActive, uint32_t numJobsDist, uint32_t numJobsDistActive );

private:
	friend class FBuild;
	static inline void SetShowInfo( bool showInfo ) { s_ShowInfo = showInfo; }
	static inline void SetShowErrors( bool showErrors ) { s_ShowErrors = showErrors; }
	static inline void SetShowProgress( bool showProgress ) { s_ShowProgress = showProgress; }

	static void Output( const char * type, const char * message );

	static bool TracingOutputCallback( const char * message );

	static bool s_ShowInfo;
	static bool s_ShowErrors;
	static bool s_ShowProgress;

	static AStackString< 64 > m_ProgressText;
};

//------------------------------------------------------------------------------
#endif // FBUILD_FLOG_H 
