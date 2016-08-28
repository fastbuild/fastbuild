// Process.h
//------------------------------------------------------------------------------
#pragma once

// Includes
//------------------------------------------------------------------------------
#include "Core/Env/Types.h"
#include "Core/Containers/AutoPtr.h"

// Process
//------------------------------------------------------------------------------
class Process
{
public:
    Process();
    ~Process();

    bool Spawn( const char * executable,
                const char * args,
                const char * workingDir,
                const char * environment,
                bool shareHandles = false );
    bool IsRunning() const;
    int WaitForExit();
    void Detach();

    // Read all data from the process until it exits
    // NOTE: Owner must free the returned memory!
    bool ReadAllData( AutoPtr< char > & memOut, uint32_t * memOutSize,
                      AutoPtr< char > & errOut, uint32_t * errOutSize,
                      uint32_t timeOutMS = 0 );

    #if defined( __WINDOWS__ )
        // Read all available data
        // NOTE: Owner must free the returned memory!
        char * ReadStdOut( uint32_t * bytesRead = nullptr );
        char * ReadStdErr( uint32_t * bytesRead = nullptr );

        // read available data into a limited user buffer
        uint32_t ReadStdOut( char * outputBuffer, uint32_t outputBufferSize );
        uint32_t ReadStdErr( char * outputBuffer, uint32_t outputBufferSize );

        // Prevent handles being redirected
        inline void DisableHandleRedirection() { m_RedirectHandles = false; }
    #endif

    static uint32_t GetCurrentId();
private:
    #if defined( __WINDOWS__ )
        void Read( void * handle, AutoPtr< char > & buffer, uint32_t & sizeSoFar, uint32_t & bufferSize );
        char * Read( void * handle, uint32_t * bytesRead );
        uint32_t Read( void * handle, char * outputBuffer, uint32_t outputBufferSize );
    #else
        void Read( int handle, AutoPtr< char > & buffer, uint32_t & sizeSoFar, uint32_t & bufferSize );
    #endif

    void Terminate();

    #if defined( __WINDOWS__ )
        // This messyness is to avoid including windows.h in this file
        inline struct _PROCESS_INFORMATION & GetProcessInfo() const
        {
            return (_PROCESS_INFORMATION &)m_ProcessInfo;
        }
    #endif

    #if defined ( WIN64 )
        uint32_t m_ProcessInfo[ 2 + 2 + 1 + 1 ]; // PROCESS_INFORMATION
    #elif defined ( WIN32 )
        uint32_t m_ProcessInfo[ 1 + 1 + 1 + 1 ]; // PROCESS_INFORMATION
    #endif

    bool m_Started;
    #if defined( __WINDOWS__ )
        bool m_SharingHandles;
        bool m_RedirectHandles;
    #endif

    #if defined( __WINDOWS__ )
        void * m_StdOutRead;    // HANDLE
        void * m_StdOutWrite;   // HANDLE
        void * m_StdErrRead;    // HANDLE
        void * m_StdErrWrite;   // HANDLE
    #endif

    #if defined( __LINUX__ ) || defined( __APPLE__ )
        int m_ChildPID;
        mutable bool m_HasAlreadyWaitTerminated;
        mutable int m_ReturnStatus;
        int m_StdOutRead;
        int m_StdErrRead;
    #endif
};

//------------------------------------------------------------------------------
