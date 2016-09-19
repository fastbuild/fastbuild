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
        unsigned char m_CriticalSection[ 8 + 4 + 4 + 8 + 8 + 8 ]; // CRITICAL_SECTION
    #elif defined ( WIN32 )
        unsigned char m_CriticalSection[ 4 + 4 + 4 + 4 + 4 + 4 ]; // CRITICAL_SECTION
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
    void operator = ( MutexHolder & ) {} // avoid C4512 - 'class' : assignment operator could not be generated

    Mutex & m_Mutex;
};

//------------------------------------------------------------------------------
