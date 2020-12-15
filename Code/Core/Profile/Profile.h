// Profile.h
//------------------------------------------------------------------------------
#pragma once

// Includes
//------------------------------------------------------------------------------
#include "ProfileManager.h"

// Defines
//------------------------------------------------------------------------------
#if !defined(PROFILING_ENABLED)
    #define PROFILE_SET_THREAD_NAME( threadName ) (void)0
    #define PROFILE_FUNCTION (void)0
    #define PROFILE_SECTION( sectionName ) (void)0
    #define PROFILE_SYNCHRONIZE
#else
    #define PROFILE_SET_THREAD_NAME( threadName ) ProfileManager::SetThreadName( threadName )

    #define PASTE_HELPER( a, b ) a ## b
    #define PASTE( a, b ) PASTE_HELPER( a, b )

    #define PROFILE_SECTION( sectionName ) const ProfileHelper PASTE( ph, __LINE__ )( sectionName )
    #define PROFILE_FUNCTION PROFILE_SECTION( __FUNCTION__ )

    #define PROFILE_SYNCHRONIZE ProfileManager::Synchronize();

    // RAII helper to manage Start/Stop of a profile section
    class ProfileHelper
    {
    public:
        inline ProfileHelper( const char * id )
        {
            ProfileManager::Start( id );
        }
        inline ~ProfileHelper()
        {
            ProfileManager::Stop();
        }
    private:
    };
#endif // PROFILING_ENABLED

//------------------------------------------------------------------------------
