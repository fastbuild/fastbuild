// Process.h
//------------------------------------------------------------------------------
#pragma once

// Includes
//------------------------------------------------------------------------------
#include "Core/Env/Types.h"

// Forward Declarations
//------------------------------------------------------------------------------
class AString;

// Process
//------------------------------------------------------------------------------
class Process
{
public:
    explicit Process( const volatile bool * mainAbortFlag = nullptr,
                      const volatile bool * abortFlag = nullptr );
    ~Process();

    bool Spawn( const char * executable,
                const char * args,
                const char * workingDir,
                const char * environment,
                bool shareHandles = false );
    bool IsRunning() const;
    int32_t WaitForExit();
    void Detach();
    void KillProcessTree();

    // Read all data from the process until it exits
    // NOTE: Owner must free the returned memory!
    bool ReadAllData( AString & memOut,
                      AString & errOut,
                      uint32_t timeOutMS = 0 );

    #if defined( __WINDOWS__ )
        // Prevent handles being redirected
        inline void DisableHandleRedirection() { m_RedirectHandles = false; }
    #endif
    bool HasAborted() const { return m_HasAborted; }
    static uint32_t GetCurrentId();

private:
    #if defined( __WINDOWS__ )
        void KillProcessTreeInternal( const void * hProc, // HANDLE
                                      const uint32_t processID,
                                      const uint64_t processCreationTime );
        static uint64_t GetProcessCreationTime( const void * hProc ); // HANDLE
        void Read( void * handle, AString & buffer );
    #else
        void Read( int handle, AString & buffer );
    #endif

    void Terminate();

    #if defined( __WINDOWS__ )
        // This messyness is to avoid including windows.h in this file
        inline struct _PROCESS_INFORMATION & GetProcessInfo() const
        {
            return (_PROCESS_INFORMATION &)m_ProcessInfo;
        }
    #endif

    #if defined( __WINDOWS__ )
        uint32_t m_ProcessInfo[ 2 + 2 + 1 + 1 ]; // PROCESS_INFORMATION
    #endif

    bool m_Started;
    #if defined( __WINDOWS__ )
        bool m_SharingHandles;
        bool m_RedirectHandles;
    #endif

    #if defined( __WINDOWS__ )
        void * m_StdOutRead;    // HANDLE
        void * m_StdErrRead;    // HANDLE
        void * m_StdInWrite;    // HANDLE
    #endif

    #if defined( __LINUX__ ) || defined( __APPLE__ )
        int m_ChildPID;
        mutable bool m_HasAlreadyWaitTerminated;
        mutable int m_ReturnStatus;
        int m_StdOutRead;
        int m_StdErrRead;
    #endif
    bool m_HasAborted;
    const volatile bool * m_MainAbortFlag; // This member is set when we must cancel processes asap when the main process dies.
    const volatile bool * m_AbortFlag;
};

//------------------------------------------------------------------------------
