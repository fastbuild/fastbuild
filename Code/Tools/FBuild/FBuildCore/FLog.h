// FLog
//------------------------------------------------------------------------------
#pragma once

// Includes
//------------------------------------------------------------------------------
#include "Core/Strings/AStackString.h"


// Macros
//------------------------------------------------------------------------------
#define FLOG_INFO( fmtString, ... )                 \
    do {                                            \
        if ( FLog::ShowInfo() )                     \
        {                                           \
            FLog::Info( fmtString, ##__VA_ARGS__ ); \
        }                                           \
    PRAGMA_DISABLE_PUSH_MSVC(4127)                  \
    } while ( false );                              \
    PRAGMA_DISABLE_POP_MSVC

#define FLOG_BUILD( fmtString, ... )                \
    do {                                            \
        if ( FLog::ShowBuildCommands() )            \
        {                                           \
            FLog::Build( fmtString, ##__VA_ARGS__ );\
        }                                           \
    PRAGMA_DISABLE_PUSH_MSVC(4127)                  \
    } while ( false );                              \
    PRAGMA_DISABLE_POP_MSVC

#define FLOG_BUILD_DIRECT( message )                \
    do {                                            \
        if ( FLog::ShowBuildCommands() )            \
        {                                           \
            FLog::BuildDirect( message );           \
        }                                           \
    PRAGMA_DISABLE_PUSH_MSVC(4127)                  \
    } while ( false );                              \
    PRAGMA_DISABLE_POP_MSVC

#define FLOG_MONITOR( fmtString, ... )              \
    do {                                            \
        if ( FLog::IsMonitorEnabled() )             \
        {                                           \
            FLog::Monitor( fmtString, ##__VA_ARGS__ ); \
        }                                           \
    PRAGMA_DISABLE_PUSH_MSVC(4127)                  \
    } while ( false );                              \
    PRAGMA_DISABLE_POP_MSVC

#define FLOG_WARN( fmtString, ... )                 \
    do {                                            \
        FLog::Warning( fmtString, ##__VA_ARGS__ );  \
    PRAGMA_DISABLE_PUSH_MSVC(4127)                  \
    } while ( false );                              \
    PRAGMA_DISABLE_POP_MSVC

#define FLOG_ERROR( fmtString, ... )                \
    do {                                            \
        FLog::Error( fmtString, ##__VA_ARGS__ );    \
    PRAGMA_DISABLE_PUSH_MSVC(4127)                  \
    } while ( false );                              \
    PRAGMA_DISABLE_POP_MSVC

#define FLOG_ERROR_DIRECT( message )                \
    do {                                            \
        FLog::ErrorDirect( message );               \
    PRAGMA_DISABLE_PUSH_MSVC(4127)                  \
    } while ( false );                              \
    PRAGMA_DISABLE_POP_MSVC

// FLog class
//------------------------------------------------------------------------------
class FLog
{
public:
    inline static bool ShowInfo() { return s_ShowInfo; }
    inline static bool ShowBuildCommands() { return s_ShowBuildCommands; }
    inline static bool ShowErrors() { return s_ShowErrors; }
    inline static bool IsMonitorEnabled() { return s_MonitorEnabled; }

    static void Info( const char * formatString, ... ) FORMAT_STRING( 1, 2 );
    static void Build( const char * formatString, ... ) FORMAT_STRING( 1, 2 );
    static void Warning( const char * formatString, ... ) FORMAT_STRING( 1, 2 );
    static void Error( const char * formatString, ... ) FORMAT_STRING( 1, 2 );
    static void Monitor( const char * formatString, ... ) FORMAT_STRING( 1, 2 );

    // for large, already formatted messages
    static void BuildDirect( const char * message );
    static void ErrorDirect( const char * message );

    static void StartBuild();
    static void StopBuild();

    static void OutputProgress( float time, float percentage, uint32_t numJobs, uint32_t numJobsActive, uint32_t numJobsDist, uint32_t numJobsDistActive );
    static void ClearProgress();

private:
    friend class FBuild;
    static inline void SetShowInfo( bool showInfo ) { s_ShowInfo = showInfo; }
    static inline void SetShowBuildCommands( bool showBuildCommands ) { s_ShowBuildCommands = showBuildCommands;}
    static inline void SetShowErrors( bool showErrors ) { s_ShowErrors = showErrors; }
    static inline void SetShowProgress( bool showProgress ) { s_ShowProgress = showProgress; }
    static inline void SetMonitorEnabled( bool enabled ) { s_MonitorEnabled = enabled; }

    static void Output( const char * type, const char * message );

    static bool TracingOutputCallback( const char * message );

    static bool s_ShowInfo;
    static bool s_ShowBuildCommands;
    static bool s_ShowErrors;
    static bool s_ShowProgress;
    static bool s_MonitorEnabled;

    static AStackString< 64 > m_ProgressText;
};

//------------------------------------------------------------------------------
