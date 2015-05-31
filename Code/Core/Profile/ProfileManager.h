// ProfileManager.h
//------------------------------------------------------------------------------
#pragma once
#ifndef CORE_PROFILE_PROFILEMANAGER_H
#define CORE_PROFILE_PROFILEMANAGER_H

// Master Define
//------------------------------------------------------------------------------
#ifdef PROFILING_ENABLED // defined globally through build system

// Includes
//------------------------------------------------------------------------------
#include "Core/Containers/Array.h"
#include "Core/Env/Types.h"
#include "Core/Process/Thread.h"

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
private:
	// the first time an event is recorded, the thread will register itself
	static void RegisterThread();

	// when a thread is finished with an event buffer (full or forced syncrhonization)
	// it's passed to the ProfileManager to
	friend struct ProfileEventBuffer;
	static void PushThreadEvents( const ProfileEvent * events, size_t num );

	struct ProfileEventInfo
	{
		Thread::ThreadId		m_ThreadId;
		const ProfileEvent  *	m_Events;
		size_t					m_NumEvents;
	};
	static Array< ProfileEventInfo > s_ProfileEventInfo;
};

//------------------------------------------------------------------------------
#endif // PROFILING_ENBLED

//------------------------------------------------------------------------------
#endif // CORE_PROFILE_PROFILEMANAGER_H
