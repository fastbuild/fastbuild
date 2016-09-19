// Timer.cpp
//------------------------------------------------------------------------------

// Includes
//------------------------------------------------------------------------------
#include "Core/PrecompiledHeader.h"

#include "Timer.h"
#include "Core/Env/Assert.h"

// system
#if defined( __WINDOWS__ )
    #include <Windows.h>
#endif
#if defined( __APPLE__ )
    #include <mach/mach.h>
    #include <mach/mach_time.h>
#endif
#if defined( __LINUX__ )
    #include <time.h>
#endif

// Static
//------------------------------------------------------------------------------
int64_t Timer::s_Frequency = 0;
float Timer::s_FrequencyInvFloat = 0.0f;
float Timer::s_FrequencyInvFloatMS = 0.0f;

// helper to initialize frequency at startup
class GlobalTimerFrequencyInitializer
{
public:
    GlobalTimerFrequencyInitializer()
    {
        #if defined( __WINDOWS__ )
            ASSERT( Timer::s_Frequency == 0 );
            LARGE_INTEGER freq;
            VERIFY( QueryPerformanceFrequency( &freq ) );
            Timer::s_Frequency = freq.QuadPart;
        #endif
        #if defined( __APPLE__ )
            mach_timebase_info_data_t info;
            mach_timebase_info( &info );
            Timer::s_Frequency = (int64_t)( info.numer / info.denom ) * 1000000000;
        #endif
        #if defined( __LINUX__ )
            Timer::s_Frequency = 1000000000ULL;
        #endif
        Timer::s_FrequencyInvFloat = (float)( 1.0 / (double)Timer::s_Frequency );
        Timer::s_FrequencyInvFloatMS = (float)( 1.0 / (double)Timer::s_Frequency * 1000.0 );
    }
};
GlobalTimerFrequencyInitializer g_GlobalTimerFrequencyInitializer;

// GetNow
//------------------------------------------------------------------------------
int64_t Timer::GetNow()
{
    #if defined( __WINDOWS__ )
        ASSERT( s_Frequency );
        LARGE_INTEGER now;
        VERIFY( QueryPerformanceCounter( &now ) );
        return now.QuadPart;
    #elif defined( __APPLE__ )
        return (int64_t)mach_absolute_time();
    #elif defined( __LINUX__ )
        timespec ts;
        VERIFY( clock_gettime( CLOCK_REALTIME, &ts ) == 0 );
        return ( (uint64_t)ts.tv_sec * 1000000000ULL ) + (uint64_t)ts.tv_nsec;
    #else
        #error Unknown platform
    #endif
}

//------------------------------------------------------------------------------
