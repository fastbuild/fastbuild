// ProfileManager.h
//------------------------------------------------------------------------------
#pragma once

// Main Define
//------------------------------------------------------------------------------
#ifdef PROFILING_ENABLED // defined globally through build system

// Includes
//------------------------------------------------------------------------------
#include "Core/Containers/Array.h"
#include "Core/Env/Types.h"
#include "Core/Process/Thread.h"
#include "Core/Strings/AStackString.h"

// Forward Declarations
//------------------------------------------------------------------------------
struct ProfileSection;
struct ProfileEvent;

// ProfileManager
//------------------------------------------------------------------------------
class ProfileManager
{
public:
    // call once per frame (or other synchronization point)
    static void Synchronize();
    static void SynchronizeNoTag(); // don't push a tag around synchronization

    // macros usually wrap these, but they can be called directly
    // NOTE: id must be valid for lifetime of application!
    static void Start( const char * id );
    static void Stop();

    // Assign human readable name to current thread
    static void SetThreadName( const char * threadName );

private:
    // when a thread is finished with an event buffer (full or forced syncrhonization)
    // it's passed to the ProfileManager to
    friend struct ProfileEventBuffer;
    static void PushThreadEvents( const ProfileEvent * events, size_t num, const char * threadName );

    struct ProfileEventInfo
    {
        Thread::ThreadId        m_ThreadId;
        AStackString< 32 >      m_ThreadName;
        const ProfileEvent *    m_Events;
        size_t                  m_NumEvents;
    };
    static Array< ProfileEventInfo > s_ProfileEventInfo;
};

//------------------------------------------------------------------------------
#endif // PROFILING_ENBLED

//------------------------------------------------------------------------------
