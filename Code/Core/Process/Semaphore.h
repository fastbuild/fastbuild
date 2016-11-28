// Semaphore.h
//------------------------------------------------------------------------------
#pragma once

// Includes
//------------------------------------------------------------------------------
#include "Core/Env/Types.h"

#if defined( __APPLE__ )
    #include <dispatch/dispatch.h>
#elif defined( __LINUX__ )
    #include <semaphore.h>
#endif

// Semaphore
//------------------------------------------------------------------------------
class Semaphore
{
public:
    Semaphore();
    ~Semaphore();

    void Signal();                  // Signal once
    void Signal( uint32_t num );    // Signal multiple times
    void Wait( uint32_t timeoutMS = 0 ); // Infinite timeout by default

private:
    #if defined( __WINDOWS__ )
        void * m_Semaphore;
    #elif defined( __APPLE__ )
        dispatch_semaphore_t m_Semaphore;
    #elif defined( __LINUX__ )
        sem_t m_Semaphore;
    #endif
};

//------------------------------------------------------------------------------
