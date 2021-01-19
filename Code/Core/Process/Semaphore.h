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
    // Windows supports a max count, but OSX and Linux does not
    #if defined( __WINDOWS__ )
        explicit Semaphore( uint32_t maxCount );
    #endif

    Semaphore();
    ~Semaphore();

    void Signal();                  // Signal once
    void Signal( uint32_t num );    // Signal multiple times

    void Wait();                    // Wait until signalled
    bool Wait( uint32_t timeoutMS );// Wait until signalled or timeout. Returns true if signalled

private:
    #if defined( __WINDOWS__ )
        void * m_Semaphore;
        bool m_HasMaxCount;
    #elif defined( __APPLE__ )
        dispatch_semaphore_t m_Semaphore;
    #elif defined( __LINUX__ )
        sem_t m_Semaphore;
    #endif
};

//------------------------------------------------------------------------------
