// SystemMutex.h
//------------------------------------------------------------------------------
#pragma once

// Includes
//------------------------------------------------------------------------------
#include "Core/Strings/AString.h"

// SystemMutex
//------------------------------------------------------------------------------
class SystemMutex
{
public:
    explicit SystemMutex( const char * name );
    ~SystemMutex();

    bool TryLock();
    bool IsLocked() const;
    void Unlock();

private:
#if defined( __WINDOWS__ )
    void * m_Handle = reinterpret_cast<void *>( -1 );
#elif defined( __LINUX__ ) || defined( __APPLE__ )
    int m_Handle = -1;
#endif
    AString m_Name;
};

//------------------------------------------------------------------------------
