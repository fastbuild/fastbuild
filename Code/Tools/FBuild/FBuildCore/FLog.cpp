// FLog
//------------------------------------------------------------------------------

// Includes
//------------------------------------------------------------------------------
#include "FLog.h"

#include "Tools/FBuild/FBuildCore/WorkerPool/WorkerThread.h"
#include "Tools/FBuild/FBuildCore/FBuild.h"

#include "Core/Env/Types.h"
#include "Core/FileIO/FileIO.h"
#include "Core/FileIO/FileStream.h"
#include "Core/Process/Mutex.h"
#include "Core/Process/Process.h"
#include "Core/Profile/Profile.h"
#include "Core/Time/Time.h"
#include "Core/Tracing/Tracing.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#if defined( __WINDOWS__ ) && defined( DEBUG )
    #include "Core/Env/WindowsHeader.h" // for OutputDebugStringA
#endif
#if defined( __LINUX__ ) || defined( __APPLE__ )
    // TODO:LINUX TODO:MAC Clean up this _itoa_s mess
    void _itoa_s( int value, char * buffer, int bufferSize, int base )
    {
        (void)bufferSize;
        ASSERT( base == 10 ); (void)base;
        sprintf( buffer, "%i", value );
    }
#endif

// Static Data
//------------------------------------------------------------------------------
/*static*/ bool FLog::s_ShowVerbose = false;
/*static*/ bool FLog::s_ShowBuildReason = false;
/*static*/ bool FLog::s_ShowErrors = true;
/*static*/ bool FLog::s_ShowProgress = false;
/*static*/ bool FLog::s_MonitorEnabled = false;
/*static*/ AStackString< 64 > FLog::m_ProgressText;
static AStackString< 72 > g_ClearLineString( "\r                                                               \r" );
static AStackString< 64 > g_OutputString( "\r99.9 % [....................] " );
static Mutex g_MonitorMutex;
static FileStream * g_MonitorFileStream = nullptr;

// Defines
//------------------------------------------------------------------------------
#define FBUILD_MONITOR_VERSION uint32_t( 1 )

// Info
//------------------------------------------------------------------------------
/*static*/ void FLog::Verbose( MSVC_SAL_PRINTF const char * formatString, ... )
{
    AStackString< 8192 > buffer;

    va_list args;
    va_start(args, formatString);
    buffer.VFormat( formatString, args );
    va_end( args );

    OutputInternal( "Info:", buffer.Get() );
}

// Output
//------------------------------------------------------------------------------
/*static*/ void FLog::Output( MSVC_SAL_PRINTF const char * formatString, ... )
{
    AStackString< 8192 > buffer;

    va_list args;
    va_start(args, formatString);
    buffer.VFormat( formatString, args );
    va_end( args );

    if ( buffer.IsEmpty() ) // Ignore empty messages for caller convenience
    {
        return;
    }

    Tracing::Output( buffer.Get() );
}

// Monitor
//------------------------------------------------------------------------------
/*static*/ void FLog::Monitor( MSVC_SAL_PRINTF const char * formatString, ... )
{
    // Is monitoring enabled?
    if ( g_MonitorFileStream == nullptr )
    {
        return; // No - nothing to do
    }

    PROFILE_SECTION( "FLog::Monitor" );

    AStackString< 1024 > buffer;
    va_list args;
    va_start( args, formatString );
    buffer.VFormat( formatString, args );
    va_end( args );

    AStackString< 1024 > finalBuffer;
    finalBuffer.Format( "%" PRIu64 " %s", Time::GetCurrentFileTime(), buffer.Get() );

    MutexHolder lock( g_MonitorMutex );
    g_MonitorFileStream->WriteBuffer( finalBuffer.Get(), finalBuffer.GetLength() );
}

// Output
//------------------------------------------------------------------------------
/*static*/ void FLog::Output( const AString & message )
{
    if ( message.IsEmpty() ) // Ignore empty messages for caller convenience
    {
        return;
    }

    Tracing::Output( message.Get() );
}

// Warning
//------------------------------------------------------------------------------
/*static*/ void FLog::Warning( MSVC_SAL_PRINTF const char * formatString, ... )
{
    AStackString< 8192 > buffer;

    va_list args;
    va_start(args, formatString);
    buffer.VFormat( formatString, args );
    va_end( args );

    OutputInternal( "Warning:", buffer.Get() );
}

// Error
//------------------------------------------------------------------------------
/*static*/ void FLog::Error( MSVC_SAL_PRINTF const char * formatString, ... )
{
    // we prevent output here, rather than where the macros is inserted
    // as an error being output is not the normal code path, and a check
    // before calling this function would bloat the code
    if ( FLog::ShowErrors() == false )
    {
        return;
    }

    AStackString< 8192 > buffer;

    va_list args;
    va_start(args, formatString);
    buffer.VFormat( formatString, args );
    va_end( args );

    OutputInternal( "Error:", buffer.Get() );
}

// ErrorDirect
//------------------------------------------------------------------------------
/*static*/ void FLog::ErrorDirect( const char * message )
{
    if ( FLog::ShowErrors() == false )
    {
        return;
    }

    Tracing::Output( message );
}

// Output - write to stdout and debugger
//------------------------------------------------------------------------------
/*static*/ void FLog::OutputInternal( const char * type, const char * message )
{
    if ( type == nullptr )
    {
        OUTPUT( "%s", message );
        return;
    }

    AStackString< 1024 > buffer( message );
    if ( buffer.IsEmpty() )
    {
        return;
    }
    if ( buffer[ buffer.GetLength() - 1 ] != '\n' )
    {
        buffer += '\n';
    }

    OUTPUT( "%s", buffer.Get() );
}

// StartBuild
//------------------------------------------------------------------------------
/*static*/ void FLog::StartBuild()
{
    if ( FBuild::Get().GetOptions().m_EnableMonitor )
    {
        // TODO:B Change the monitoring log path
        //  - it's not uniquified per instance
        //  - we already have a .fbuild.tmp folder we should use
        AStackString<> fullPath;
        FBuild::GetTempDir( fullPath );
        fullPath += "FastBuild";
        if ( FileIO::DirectoryCreate( fullPath ) )
        {
            fullPath += "/FastBuildLog.log";

            ASSERT( g_MonitorFileStream == nullptr );
            MutexHolder lock( g_MonitorMutex );
            g_MonitorFileStream = new FileStream();
            if ( g_MonitorFileStream->Open( fullPath.Get(), FileStream::WRITE_ONLY ) )
            {
                Monitor( "START_BUILD %u %u\n", FBUILD_MONITOR_VERSION, Process::GetCurrentId() );
            }
            else
            {
                Error( "Couldn't open monitor file for write at %s", fullPath.Get() );

                delete g_MonitorFileStream;
                g_MonitorFileStream = nullptr;
            }
        }
        else
        {
            Error( "Couldn't create directory for monitor file at %s", fullPath.Get() );
        }
    }

    Tracing::AddCallbackOutput( &TracingOutputCallback );
}

// StopBuild
//------------------------------------------------------------------------------
/*static*/ void FLog::StopBuild()
{
    if ( g_MonitorFileStream )
    {
        MutexHolder lock( g_MonitorMutex );
        Monitor( "STOP_BUILD\n" );
        g_MonitorFileStream->Close();

        delete g_MonitorFileStream;
        g_MonitorFileStream = nullptr;
    }

    Tracing::RemoveCallbackOutput( &TracingOutputCallback );

    if ( s_ShowProgress )
    {
        fputs( g_ClearLineString.Get(), stdout );
        m_ProgressText.Clear();
    }
}

// OutputProgress
//------------------------------------------------------------------------------
/*static*/ void FLog::OutputProgress( float time,
                                      float percentage,
                                      uint32_t numJobs,
                                      uint32_t numJobsActive,
                                      uint32_t numJobsDist,
                                      uint32_t numJobsDistActive )
{
    PROFILE_FUNCTION;

    ASSERT( s_ShowProgress );

    // format progress % (we know it never goes above 99.9%)
    uint32_t intPerc = (uint32_t)( percentage * 10.0f ); // 0 to 999
    const uint32_t hundreds = ( intPerc / 100 ); intPerc -= ( hundreds * 100 );
    uint32_t tens = ( intPerc / 10 ); intPerc -= ( tens * 10 );
    uint32_t ones = intPerc;
    m_ProgressText = g_OutputString;
    m_ProgressText[ 1 ] = ( hundreds > 0 ) ? ( '0' + (char)hundreds ) : ' ';
    m_ProgressText[ 2 ] = '0' + (char)tens;
    m_ProgressText[ 4 ] = '0' + (char)ones;

    // 20 column output (100/20 = 5% per char)
    const uint32_t numStarsDone = (uint32_t)( percentage * 20.0f / 100.0f ); // 20 columns
    for ( uint32_t i=0; i<20; ++i )
    {
        m_ProgressText[ 9 + i ] = ( i < numStarsDone ) ? '*' : '-';
    }

    // time " [%um] %02us"
    const uint32_t timeTakenMinutes = uint32_t( time / 60.0f );
    const uint32_t timeTakenSeconds = (uint32_t)time - ( timeTakenMinutes * 60 );
    if ( timeTakenMinutes > 0 )
    {
        char buffer[ 8 ];
        _itoa_s( (int32_t)timeTakenMinutes, buffer, 8, 10 );
        m_ProgressText += buffer;
        m_ProgressText.Append( "m ", 2 );
    }
    char buffer[ 8 ];
    _itoa_s( (int32_t)timeTakenSeconds, buffer, 8, 10 );
    if ( timeTakenSeconds < 10 ) { m_ProgressText += '0'; }
    m_ProgressText += buffer;
    m_ProgressText += 's';

    // active/available jobs " (%u/%u)"
    m_ProgressText.Append( " (", 2 );
    _itoa_s( (int32_t)numJobsActive, buffer, 8, 10 );
    m_ProgressText += buffer;
    m_ProgressText += '/';
    _itoa_s( (int32_t)( numJobsActive + numJobs ), buffer, 8, 10 );
    m_ProgressText += buffer;
    m_ProgressText += ')';

    // distributed status "+(%u/%u)"
    if ( FBuild::Get().GetOptions().m_AllowDistributed )
    {
        m_ProgressText.Append( "+(", 2 );
        _itoa_s( (int32_t)numJobsDistActive, buffer, 8, 10 );
        m_ProgressText += buffer;
        m_ProgressText += '/';
        _itoa_s( (int32_t)( numJobsDistActive + numJobsDist ), buffer, 8, 10 );
        m_ProgressText += buffer;
        m_ProgressText += ')';
    }

    m_ProgressText.Append( "    \b\b\b", 7 ); // extra whitespace when string shortens

    // if build aborting, override all output
    if ( FBuild::Get().GetStopBuild() )
    {
        m_ProgressText.Format( "\rBUILD ABORTED - STOPPING (%u) ", numJobsActive );
        m_ProgressText += "                                  \b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b";
    }

    // animation
    static int animIndex = 0;
    static char anim[] = { '\\', '|', '/', '-', '\\', '|', '/', '-' };
    m_ProgressText += anim[ ( animIndex++ ) % 8 ];

    // finally print it
    fwrite( m_ProgressText.Get(), 1, m_ProgressText.GetLength(), stdout );
}

// ClearProgress
//------------------------------------------------------------------------------
/*static*/ void FLog::ClearProgress()
{
    fwrite( g_ClearLineString.Get(), 1, g_ClearLineString.GetLength(), stdout );
}

// TracingOutputCallback
//------------------------------------------------------------------------------
/*static*/ bool FLog::TracingOutputCallback( const char * message )
{
    const uint32_t threadIndex = WorkerThread::GetThreadIndex();

    AStackString< 2048 > tmp;

    if ( s_ShowProgress )
    {
        // clear previous progress message
        tmp += g_ClearLineString;
    }

    // print output and then progress
    if ( threadIndex > 0 )
    {
        char buffer[ 8 ];
        _itoa_s( (int32_t)threadIndex, buffer, 8, 10 );
        tmp += buffer;
        tmp += '>';
        if ( threadIndex < 10 )
        {
            tmp += ' '; // keep output aligned when there are > 9 threads
        }
    }

    tmp += message;

    // output to debugger if present
    #ifdef DEBUG
        #ifdef __WINDOWS__
            OutputDebugStringA( message );
        #endif
    #endif

    tmp += m_ProgressText;

    fwrite( tmp.Get(), 1, tmp.GetLength(), stdout );

    return false; // tell tracing not to output it again
}

//------------------------------------------------------------------------------
