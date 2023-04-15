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

    void Lock();
    void Unlock();

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

//------------------------------------------------------------------------------
