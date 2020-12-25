// WorkerThread - object to process and manage jobs on a thread
//------------------------------------------------------------------------------
#pragma once

// Includes
//------------------------------------------------------------------------------
#include "Core/Env/Types.h"
#include "Core/Process/Mutex.h"
#include "Core/Process/Semaphore.h"
#include "Core/Strings/AStackString.h"
#include "Core/Strings/AString.h"

// Forward Declarations
//------------------------------------------------------------------------------
class FileStream;

// WorkerThread
//------------------------------------------------------------------------------
class WorkerThread
{
public:
    explicit WorkerThread( uint16_t threadIndex );
    void Init();
    virtual ~WorkerThread();

    static void InitTmpDir( bool remote = false );

    void Stop();
    bool HasExited() const;
    void WaitForStop();

    static uint16_t GetThreadIndex();

    static void GetTempFileDirectory( AString & tmpFileDirectory );

    static void CreateTempFilePath( const char * fileName,
                                    AString & tmpFileName );
    static bool CreateTempFile( const AString & tmpFileName,
                                FileStream & file );
    static void CreateThreadLocalTmpDir();
protected:
    // allow update from the main thread when in -j0 mode
    friend class FBuild;
    static bool Update();

    // worker thread main loop
    static uint32_t ThreadWrapperFunc( void * param );
    virtual void Main();

    // signal to exit thread
    volatile bool m_ShouldExit;
    volatile bool m_Exited;
    uint16_t      m_ThreadIndex;
    Semaphore     m_MainThreadWaitForExit; // Used by main thread to wait for exit of worker

    static Mutex s_TmpRootMutex; // s_TmpRoot is shared by local and remote queues in tests
    static AStackString<> s_TmpRoot;
};

//------------------------------------------------------------------------------
