// ProfileManager
//------------------------------------------------------------------------------

// Includes
//------------------------------------------------------------------------------
#include "ProfileManager.h"

#ifdef PROFILING_ENABLED

#include "Core/FileIO/FileStream.h"
#include "Core/Math/xxHash.h"
#include "Core/Mem/Mem.h"
#include "Core/Process/Mutex.h"
#include "Core/Process/Thread.h"
#include "Core/Profile/Profile.h"
#include "Core/Strings/AStackString.h"
#include "Core/Time/Timer.h"
#include "Core/Tracing/Tracing.h"

// Static Data
//------------------------------------------------------------------------------
/*static*/ Array< ProfileManager::ProfileEventInfo > ProfileManager::s_ProfileEventInfo( 0, true );

// Global Data
//------------------------------------------------------------------------------
Mutex g_ProfileManagerMutex;
FileStream g_ProfileEventLog;

// ProfileEvent
//------------------------------------------------------------------------------
struct ProfileEvent
{
    const char *    m_Id;
    int64_t         m_TimeStamp;
};

// FormatU64
//------------------------------------------------------------------------------
void FormatU64( uint64_t value, char * outBuffer )
{
    if ( value == 0 )
    {
        *outBuffer = '0';
        ++outBuffer;
    }
    else
    {
        char tmp[ 24 ]; // 20 bytes needed for max U64 value: 18,446,744,073,709,551,615
        char * pos = tmp;
        while ( value )
        {
            *pos = ( '0' + (char)( value % 10 ) );
            ++pos;
            value /= 10;
        }
        while ( pos > tmp )
        {
            --pos;
            *outBuffer = *pos;
            ++outBuffer;
        }
    }
    *outBuffer = 0;
}

// Per-Thread structure
//------------------------------------------------------------------------------
struct ProfileEventBuffer
{
    inline void Start( const char * profileId );
    inline void Stop();
    inline void SetThreadName( const char * threadName );

    NO_INLINE ProfileEvent * AllocateEventStorage();

    size_t          m_CurrentDepth;

    // keep an exapanding buffer of events
    ProfileEvent *  m_Begin;
    ProfileEvent *  m_Current;
    ProfileEvent *  m_MaxEnd;

    enum { MAX_THREAD_NAME_LEN = 31 };
    char                m_ThreadName[ MAX_THREAD_NAME_LEN + 1 ];

    // when allocating memory to track events, do it in blocks
    enum{ NUM_EVENTS_PER_BLOCK = 8192 }; // 64KiB pages with 8 bytes events
};
THREAD_LOCAL ProfileEventBuffer tls_ProfileEventBuffer = { 0, nullptr, nullptr, nullptr, "" };

// ProfileEventBuffer::Start
//------------------------------------------------------------------------------
void ProfileEventBuffer::Start( const char * id )
{
    // ensure we have enough space
    ProfileEvent * e = m_Current;
    if ( e == m_MaxEnd )
    {
        e = AllocateEventStorage();
    }

    m_CurrentDepth++;

    e->m_Id = id;
    e->m_TimeStamp =  Timer::GetNow();

    ++e;
    m_Current = e;
}

// Stop
//------------------------------------------------------------------------------
void ProfileEventBuffer::Stop()
{
    ASSERT( m_CurrentDepth > 0 );

    ProfileEvent * e = m_Current;
    if ( e == m_MaxEnd )
    {
        e = AllocateEventStorage();
    }

    e->m_Id = nullptr;
    e->m_TimeStamp = Timer::GetNow();

    ++e;
    m_Current = e;

    // push events if back to root
    size_t currentDepth = m_CurrentDepth;
    if ( --currentDepth == 0 )
    {
        const ProfileEvent * events = m_Begin;
        ProfileManager::PushThreadEvents( events, (size_t)( m_Current - events ), m_ThreadName );
        m_Begin = nullptr;
        m_Current = nullptr;
        m_MaxEnd = nullptr;
    }
    m_CurrentDepth = currentDepth;
}

// ProfileEventBuffer::AllocateEventStorage
//------------------------------------------------------------------------------
ProfileEvent * ProfileEventBuffer::AllocateEventStorage()
{
    // do we have any events accumulated
    ProfileEvent * events = m_Begin;
    if ( events )
    {
        // ProfileManager now owns the memory
        ProfileManager::PushThreadEvents( events, (size_t)( m_Current - events ), m_ThreadName );
    }

    // allocate a fresh block
    MEMTRACKER_DISABLE_THREAD
    {
        events = FNEW_ARRAY( ProfileEvent[ NUM_EVENTS_PER_BLOCK ] );
    }
    MEMTRACKER_ENABLE_THREAD
    m_Begin = events;
    m_Current = events;
    m_MaxEnd = events + NUM_EVENTS_PER_BLOCK;

    return events;
}

// Start
//------------------------------------------------------------------------------
/*static*/ void ProfileManager::Start( const char * id )
{
    ProfileEventBuffer & buffer = tls_ProfileEventBuffer;
    buffer.Start( id );
}

// Stop
//------------------------------------------------------------------------------
/*static*/ void ProfileManager::Stop()
{
    ProfileEventBuffer & buffer = tls_ProfileEventBuffer;
    buffer.Stop();
}

// SetThreadName
//------------------------------------------------------------------------------
/*static*/ void ProfileManager::SetThreadName( const char * threadName )
{
    ProfileEventBuffer & buffer = tls_ProfileEventBuffer;

    // Take a copy of the name
    AString::Copy( threadName, buffer.m_ThreadName, Math::Min< size_t >( AString::StrLen( threadName ), ProfileEventBuffer::MAX_THREAD_NAME_LEN ) );
}

//------------------------------------------------------------------------------
/*static*/ void ProfileManager::PushThreadEvents( const ProfileEvent * events, size_t num, const char * threadName )
{
    MEMTRACKER_DISABLE_THREAD
    {
        MutexHolder mh( g_ProfileManagerMutex );

        if ( s_ProfileEventInfo.GetCapacity() == 0 )
        {
            s_ProfileEventInfo.SetCapacity( 64 );
        }

        ProfileEventInfo pfi;
        pfi.m_ThreadId = Thread::GetCurrentThreadId();
        pfi.m_ThreadName = threadName;
        pfi.m_Events = events;
        pfi.m_NumEvents = num;
        s_ProfileEventInfo.Append( pfi );
    }
    MEMTRACKER_ENABLE_THREAD
}

// Synchronize
//------------------------------------------------------------------------------
/*static*/ void ProfileManager::Synchronize()
{
    PROFILE_SECTION( "ProfileManager::Synchronize" );
    SynchronizeNoTag();
}

// SynchronizeNoTag
//------------------------------------------------------------------------------
/*static*/ void ProfileManager::SynchronizeNoTag()
{
    Array< ProfileEventInfo > infos( 0, true );
    {
        MutexHolder mh( g_ProfileManagerMutex );
        infos.Swap( s_ProfileEventInfo );
    }

    // first time? open log file
    if ( g_ProfileEventLog.IsOpen() == false )
    {
        if ( g_ProfileEventLog.Open( "profile.json", FileStream::WRITE_ONLY | FileStream::NO_RETRY_ON_SHARING_VIOLATION ) )
        {
            g_ProfileEventLog.WriteBuffer( "[ ", 2 ); // json array opening tag
        }
    }

    // write all the events we have
    const ProfileEventInfo * const end = infos.End();
    AStackString< 8192 > buffer;
    const double freqMul = ( (double)Timer::GetFrequencyInvFloatMS() * 1000.0 );
    if ( g_ProfileEventLog.IsOpen() )
    {
        for ( const ProfileEventInfo * it = infos.Begin(); it != end; ++it )
        {
            const ProfileEventInfo & info = *it;
            uint64_t threadId = (uint64_t)info.m_ThreadId;
            char threadIsAsString[ 32 ];

            // Thread Name
            if ( ( info.m_ThreadName.IsEmpty() == false ) || ( info.m_ThreadId == Thread::GetMainThreadId() ) )
            {
                if ( info.m_ThreadName.IsEmpty() == false )
                {
                    threadId = xxHash::Calc32( info.m_ThreadName );
                }
                FormatU64( threadId, threadIsAsString );

                // SetThreadName event
                // {"name": "thread_name", "ph": "M", "pid": 0, "tid": 164, "args": { "name" : "ThreadName" }},
                buffer += "{\"name\":\"thread_name\",\"ph\":\"M\",\"pid\":0,\"tid\":";
                buffer += threadIsAsString;
                buffer += ", \"args\":{\"name\":\"";
                buffer += info.m_ThreadName.IsEmpty() ? "_MainThread" : info.m_ThreadName.Get();
                buffer += "\"}},\n";

                if ( buffer.GetLength() > ( 8192 - 256 ) )
                {
                    g_ProfileEventLog.WriteBuffer( buffer.Get(), buffer.GetLength() );
                    buffer.Clear();
                }
            }
            else
            {
                FormatU64( threadId, threadIsAsString );
            }

            const size_t numEvents( info.m_NumEvents );
            for ( size_t i = 0; i < numEvents; ++i )
            {
                const ProfileEvent & e = info.m_Events[ i ];

                // {"name": "Asub", "ph": "B", "pid": 22630, "tid": 22630, "ts": 829},
                if ( e.m_Id )
                {
                    buffer += "{\"name\":\"";
                    buffer += e.m_Id;
                    buffer += "\",\"ph\":\"B\",\"pid\":0,\"tid\":";
                }
                else
                {
                    buffer += "{\"name\":\"\",\"ph\":\"E\",\"pid\":0,\"tid\":";
                }
                buffer += threadIsAsString;
                buffer += ",\"ts\":";
                const uint64_t eventTime = (uint64_t)( (double)e.m_TimeStamp * freqMul );
                char eventTimeBuffer[ 32 ];
                FormatU64( eventTime, eventTimeBuffer );
                buffer += eventTimeBuffer;
                buffer += "},\n";

                if ( buffer.GetLength() > ( 8192 - 256 ) )
                {
                    g_ProfileEventLog.WriteBuffer( buffer.Get(), buffer.GetLength() );
                    buffer.Clear();
                }
            }
        }

        // Flush remaining
        if ( buffer.GetLength() > 0 )
        {
            g_ProfileEventLog.WriteBuffer( buffer.Get(), buffer.GetLength() );
            buffer.Clear();
        }
    }

    for ( const ProfileEventInfo * it = infos.Begin(); it != end; ++it )
    {
        const ProfileEventInfo & info = *it;
        FDELETE[] info.m_Events;
    }
}

//------------------------------------------------------------------------------
#endif // PROFILING_ENABLED
