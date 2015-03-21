// ProfileManager
//------------------------------------------------------------------------------

// Includes
//------------------------------------------------------------------------------
#include "Core/PrecompiledHeader.h"

#include "ProfileManager.h"

#ifdef PROFILING_ENABLED

#include "Core/FileIO/FileStream.h"
#include "Core/Mem/Mem.h"
#include "Core/Process/Mutex.h"
#include "Core/Process/Thread.h"
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
	const char *	m_Id;
	uint64_t		m_TimeStamp;
};

// Per-Thread structure
//------------------------------------------------------------------------------
struct ProfileEventBuffer
{
	inline void Start( const char * profileId );
	inline void Stop();

	NO_INLINE ProfileEvent * AllocateEventStorage();

	size_t			m_CurrentDepth;

	// keep an exapanding buffer of events
	ProfileEvent *	m_Begin;
	ProfileEvent *	m_Current;
	ProfileEvent *	m_MaxEnd;

	bool m_Registered;

	// when allocating memory to track events, do it in blocks
	enum{ NUM_EVENTS_PER_BLOCK = 512 };
};
THREAD_LOCAL ProfileEventBuffer tls_ProfileEventBuffer = { 0 };

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
		ProfileEvent * events = m_Begin;
		ProfileManager::PushThreadEvents( events, m_Current-events );
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
	// first time registration
	if ( !m_Registered )
	{
		ProfileManager::RegisterThread();
		m_Registered = true;
	}

	// do we have any events accumulated
	ProfileEvent * events = m_Begin;
	if ( events )
	{
		// ProfileManager now owns the memory
		ProfileManager::PushThreadEvents( events, m_Current-events );
	}

	// allocate a fresh block
	events = FNEW_ARRAY( ProfileEvent[ NUM_EVENTS_PER_BLOCK ] );
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

// RegisterThread
//------------------------------------------------------------------------------
/*static*/ void ProfileManager::RegisterThread()
{
	MutexHolder mh( g_ProfileManagerMutex );
	// TODO: Take note of the thread id/name
}

//------------------------------------------------------------------------------
/*static*/ void ProfileManager::PushThreadEvents( const ProfileEvent * events, size_t num )
{
	MutexHolder mh( g_ProfileManagerMutex );

	if ( s_ProfileEventInfo.GetCapacity() == 0 )
	{
		s_ProfileEventInfo.SetCapacity( 64 );
	}

	ProfileEventInfo pfi;
	pfi.m_ThreadId = Thread::GetCurrentThreadId();
	pfi.m_Events = events;
	pfi.m_NumEvents = num;
	s_ProfileEventInfo.Append( pfi );
}

// Synchronize
//------------------------------------------------------------------------------
/*static*/ void ProfileManager::Synchronize()
{
	Array< ProfileEventInfo > infos( 0, true );
	{
		MutexHolder mh( g_ProfileManagerMutex );
		infos.Swap( s_ProfileEventInfo );
	}

	// first time? open log file
	if ( g_ProfileEventLog.IsOpen() == false )
	{
		g_ProfileEventLog.Open( "c:\\test\\profile.log", FileStream::WRITE_ONLY | FileStream::NO_RETRY_ON_SHARING_VIOLATION );
	}

	// write all the events we have
	const ProfileEventInfo * const end = infos.End();
	for ( const ProfileEventInfo * it = infos.Begin(); it != end; ++it )
	{
		const ProfileEventInfo & info = *it;
		if ( g_ProfileEventLog.IsOpen() )
		{
			g_ProfileEventLog.Write( info.m_ThreadId );
			g_ProfileEventLog.Write( (uint32_t)info.m_NumEvents );
			g_ProfileEventLog.Write( info.m_Events, sizeof( ProfileEventInfo ) * info.m_NumEvents );
		}
		FDELETE [] info.m_Events;
	}
}

//------------------------------------------------------------------------------
#endif // PROFILING_ENABLED
