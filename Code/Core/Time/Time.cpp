// Time.cpp
//------------------------------------------------------------------------------

// Includes
//------------------------------------------------------------------------------
#include "Time.h"
#include "Core/Env/Assert.h"
#include "Core/Strings/AStackString.h"

// system
#if defined( __WINDOWS__ )
    #include "Core/Env/WindowsHeader.h"
#endif
#if defined( __APPLE__ ) || defined( __LINUX__ )
    #include <sys/time.h>
#endif

// GetCurrentFileTime
//------------------------------------------------------------------------------
uint64_t Time::GetCurrentFileTime()
{
    #if defined( __WINDOWS__ )
        SYSTEMTIME st;
        FILETIME ft;
        GetSystemTime( &st );
        if ( SystemTimeToFileTime( &st, &ft ) == FALSE )
        {
            ASSERT(false); // Should never fail
            return 0;
        }
        const uint64_t fileTimeNow = ( (uint64_t)ft.dwLowDateTime | ( (uint64_t)ft.dwHighDateTime << 32 ) );
        return fileTimeNow;
    #elif defined( __APPLE__ ) || defined( __LINUX__ )
        struct timeval currentTime;
        VERIFY( gettimeofday( &currentTime, nullptr )== 0 );
        const uint64_t fileTimeNow = ( (uint64_t)currentTime.tv_sec * 1000000000ULL ) + ( (uint64_t)currentTime.tv_usec * 1000 );
        return fileTimeNow;
    #else
        #error Unknown platform
    #endif
}

// FormatTime
//------------------------------------------------------------------------------
void Time::FormatTime( const float timeInSeconds, const bool outputFractionalDigits, AString & buffer )
{
    buffer.Clear();

    float mutableTimeInSeconds = timeInSeconds;

    uint32_t days = (uint32_t)( mutableTimeInSeconds / ( 24.0f * 60.0f * 60.0f ) );
    mutableTimeInSeconds -= ( (float)days * ( 24.0f * 60.0f * 60.0f ) );
    uint32_t hours = (uint32_t)( mutableTimeInSeconds / ( 60.0f * 60.0f ) );
    mutableTimeInSeconds -= ( (float)hours * ( 60.0f * 60.0f ) );
    uint32_t mins = (uint32_t)( mutableTimeInSeconds / 60.0f );
    mutableTimeInSeconds -= ( (float)mins * 60.0f );

    AStackString<> temp;

    if ( days > 0 )
    {
        temp.Format( "%u days, ", days );
        buffer += temp;
    }
    if ( hours > 0 )
    {
        temp.Format( "%uh:", hours );
        buffer += temp;
    }
    if ( mins > 0 )
    {
        temp.Format( "%um ", mins );
        buffer += temp;
    }

    if ( outputFractionalDigits )
    {
        temp.Format( "%2.3fs", mutableTimeInSeconds );
    }
    else
    {
        temp.Format( "%us", (uint32_t) mutableTimeInSeconds );
    }
    buffer += temp;
}

//------------------------------------------------------------------------------
