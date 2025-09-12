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

    [[nodiscard]] bool Spawn( const char * executable,
                              const char * args,
                              const char * workingDir,
                              const char * environment,
                              bool shareHandles = false );
    [[nodiscard]] bool IsRunning() const;
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
    void DisableHandleRedirection() { m_RedirectHandles = false; }
#endif
    [[nodiscard]] bool HasAborted() const;
    [[nodiscard]] static uint32_t GetCurrentId();

private:
#if defined( __APPLE__ ) || defined( __LINUX__ )
    [[nodiscard]] bool SpawnUsingFork( int32_t stdOutPipeFDs[ 2 ],
                                       int32_t stdErrPipeFDs[ 2 ],
                                       const char * executable,
                                       char * const * argV,
                                       const char * workingDir,
                                       char * const * envV );
    [[nodiscard]] bool CanUsePosixSpawn( const char * workingDir ) const;
    [[nodiscard]] bool SpawnUsingPosixSpawn( int32_t stdOutPipeFDs[ 2 ],
                                             int32_t stdErrPipeFDs[ 2 ],
                                             const char * executable,
                                             char * const * argV,
                                             const char * workingDir,
                                             char * const * envV );
#endif

#if defined( __WINDOWS__ )
    void KillProcessTreeInternal( const void * hProc, // HANDLE
                                  const uint32_t processID,
                                  const uint64_t processCreationTime );
    [[nodiscard]] static uint64_t GetProcessCreationTime( const void * hProc ); // HANDLE
    void Read( void * handle, AString & buffer );
#elif defined( __LINUX__ ) || defined( __APPLE__ )
    void Read( int32_t stdOutHandle,
               int32_t stdErrHandle,
               AString & inoutOutBuffer,
               AString & inoutErrBuffer );
    void ReadCommon( int32_t handle, AString & inoutBuffer );
#endif

    void Terminate();

#if defined( __WINDOWS__ )
    // This messiness is to avoid including windows.h in this file
    struct _PROCESS_INFORMATION & GetProcessInfo() const
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
    void * m_StdOutRead; // HANDLE
    void * m_StdErrRead; // HANDLE
    void * m_StdInWrite; // HANDLE
#endif

#if defined( __LINUX__ ) || defined( __APPLE__ )
    int m_ChildPID;
    mutable bool m_HasAlreadyWaitTerminated;
    mutable int m_ReturnStatus;
    int m_StdOutRead;
    int m_StdErrRead;
#endif
    const volatile bool * m_MainAbortFlag; // This member is set when we must cancel processes asap when the main process dies.
    const volatile bool * m_AbortFlag;
};

//------------------------------------------------------------------------------
