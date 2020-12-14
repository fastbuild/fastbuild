// SystemMutex
//------------------------------------------------------------------------------

// Includes
//------------------------------------------------------------------------------
#include "SystemMutex.h"
#include "Core/Strings/AStackString.h"

// system
#if defined( __WINDOWS__ )
    #include "Core/Env/Assert.h"
    #include "Core/Env/WindowsHeader.h"
#endif
#if defined( __LINUX__ ) || defined( __APPLE__ )
    #include <errno.h>
    #include <fcntl.h>
    #include <sys/file.h>
    #include <unistd.h>
#endif

// CONSTRUCTOR
//------------------------------------------------------------------------------
SystemMutex::SystemMutex( const char * name )
    #if defined( __WINDOWS__ )
        : m_Handle( INVALID_HANDLE_VALUE )
    #elif defined( __LINUX__ ) || defined( __APPLE__ )
        : m_Handle( -1 )
    #endif
    , m_Name( name )
{
}

// DESTRUCTOR
//------------------------------------------------------------------------------
SystemMutex::~SystemMutex()
{
    if ( IsLocked() )
    {
        Unlock();
    }
}

// Lock
//------------------------------------------------------------------------------
bool SystemMutex::TryLock()
{
    ASSERT( !IsLocked() ); // Invalid to lock more than once

    #if defined( __WINDOWS__ )
        void * handle = (void *)CreateMutex( nullptr, TRUE, m_Name.Get() );
        if ( GetLastError() == ERROR_ALREADY_EXISTS )
        {
            if ( ( handle != INVALID_HANDLE_VALUE ) && ( handle != nullptr ) )
            {
                CloseHandle( handle );
            }
            return false;
        }
        m_Handle = handle;
        return true;
    #elif defined( __LINUX__ ) || defined( __APPLE__ )
        AStackString<> tempFileName;
        tempFileName.Format( "/tmp/%s.lock", m_Name.Get());
        int handle = open( tempFileName.Get(), O_CREAT | O_RDWR | O_CLOEXEC, 0666 );
        if ( handle < 0 )
        {
            ASSERT( false ); // unexpected problem
            return false;
        }
        int rc = flock( handle, LOCK_EX | LOCK_NB );
        if ( rc )
        {
            VERIFY( close( handle ) == 0 );
            if ( errno == EWOULDBLOCK || errno == EAGAIN )
            {
                return false; // locked by another process
            }
            ASSERT( false ); // Unexpected problem!
            return false;
        }
        m_Handle = handle;
        return true; // we own it now
    #else
        #error Unknown platform
    #endif
}

// IsLocked
//------------------------------------------------------------------------------
bool SystemMutex::IsLocked() const
{
    #if defined( __WINDOWS__ )
        return ( m_Handle != INVALID_HANDLE_VALUE );
    #elif defined( __LINUX__ ) || defined( __APPLE__ )
        return ( m_Handle != -1 );
    #else
        #error Unknown platform
    #endif
}

// Unlock
//------------------------------------------------------------------------------
void SystemMutex::Unlock()
{
    #if defined( __WINDOWS__ )
        ASSERT( m_Handle != INVALID_HANDLE_VALUE );
        CloseHandle( m_Handle );
        m_Handle = INVALID_HANDLE_VALUE;
    #elif defined( __LINUX__ ) || defined( __APPLE__ )
        ASSERT( m_Handle != -1 );
        VERIFY( flock( m_Handle, LOCK_UN ) == 0 );
        VERIFY( close( m_Handle ) == 0 );
        m_Handle = -1;
    #else
        #error Unknown platform
    #endif
}

//------------------------------------------------------------------------------
