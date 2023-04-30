// Mutex.h
//------------------------------------------------------------------------------
#pragma once

// Includes
//------------------------------------------------------------------------------
#include "Core/Env/Types.h"

#if defined( __LINUX__ ) || defined( __APPLE__ )
    #include <pthread.h>
#endif

// Mutex
//------------------------------------------------------------------------------
class Mutex
{
public:
    Mutex();
    ~Mutex();

    void                Lock();
    [[nodiscard]] bool  TryLock();
    void                Unlock();

private:
    // do this to avoid including windows.h
    #if defined( __WINDOWS__ )
        uint64_t m_CriticalSection[ 5 ]; // CRITICAL_SECTION
    #else
        pthread_mutex_t m_Mutex;
    #endif
};

// MutexHolder
//------------------------------------------------------------------------------
class MutexHolder
{
public:
    explicit MutexHolder( Mutex & mutex ) : m_Mutex( mutex )
    {
        mutex.Lock();
    }
    ~MutexHolder()
    {
        m_Mutex.Unlock();
    }

private:
    MutexHolder( const MutexHolder & other ) = delete;
    void operator = ( MutexHolder & ) = delete;

    Mutex & m_Mutex;
};

// TryMutexHolder
//------------------------------------------------------------------------------
class TryMutexHolder
{
public:
    explicit TryMutexHolder( Mutex & mutex )
        : m_Mutex( mutex )
        , m_Locked( mutex.TryLock() )
    {
    }
    ~TryMutexHolder()
    {
        if ( m_Locked )
        {
            m_Mutex.Unlock();
        }
    }

    [[nodiscard]] bool IsLocked() const { return m_Locked; }

private:
    TryMutexHolder( const TryMutexHolder & other ) = delete;
    void operator = ( TryMutexHolder & other ) = delete;

    Mutex &     m_Mutex;
    const bool  m_Locked;
};

//------------------------------------------------------------------------------
