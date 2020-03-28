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
    #if defined ( WIN64 )
        uint64_t m_CriticalSection[ 5 ]; // CRITICAL_SECTION
    #elif defined ( WIN32 )
        uint32_t m_CriticalSection[ 6 ]; // CRITICAL_SECTION
    #endif

    #if defined( __LINUX__ ) || defined( __APPLE__ )
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
