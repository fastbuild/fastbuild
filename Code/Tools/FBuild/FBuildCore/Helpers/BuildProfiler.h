// BuildProfiler
//------------------------------------------------------------------------------
#pragma once

// Includes
//------------------------------------------------------------------------------
// Core
#include "Core/Containers/Array.h"
#include "Core/Containers/Singleton.h"
#include "Core/Env/Types.h"
#include "Core/Process/Atomic.h"
#include "Core/Process/Mutex.h"
#include "Core/Process/Semaphore.h"
#include "Core/Process/Thread.h"
#include "Core/Strings/AString.h"
#include "Core/Time/Timer.h"

// Forward Declarations
//------------------------------------------------------------------------------
class FBuild;
class FBuildOptions;
class Job;

// BuildProfiler
//------------------------------------------------------------------------------
class BuildProfiler : public Singleton<BuildProfiler>
{
public:
    BuildProfiler();
    ~BuildProfiler();

    // Periodic gathering of global metrics
    void StartMetricsGathering();
    void StopMetricsGathering();

    // Record duration of a local step (BuildProfilerScope calls this)
    void RecordLocal( uint32_t threadId,
                      int64_t startTime,
                      int64_t endTime,
                      const char * stepName,
                      const char * targetName );

    // Record duration of a remote step
    void RecordRemote( uint32_t workedId,
                       const AString & workerName,
                       uint32_t remoteThreadId,
                       int64_t startTime,
                       int64_t endTime,
                       const char * stepName,
                       const char * targetName );

    // Capture data from FBuild that we have pointers to so that it can
    // be safely destroyed (along with BuildProfiler)
    // Must be called before SaveJSON()
    void Capture( const FBuild & fBuild );

    // Write the profiling info in Chrome tracing format
    static bool SaveJSON( const char * fileName );

protected:
    static uint32_t MetricsThreadWrapper( void * userData );
    void MetricsUpdate();

    void Serialize( const FBuildOptions & options );

    // Items processed during the build
    class Event
    {
    public:
        Event( int32_t machineId, uint32_t threadId, int64_t startTime, int64_t endTime, const char * stepName, const char * targetName )
            : m_MachineId( machineId )
            , m_ThreadId( threadId )
            , m_StartTime( startTime )
            , m_EndTime( endTime )
            , m_StepName( stepName )
            , m_TargetName( targetName )
        {
        }

        inline static const int32_t kLocalMachineId = -1;

        int32_t m_MachineId; // Local or remote machine identifier
        uint32_t m_ThreadId; // Thread identifier
        int64_t m_StartTime;
        int64_t m_EndTime;
        const char * m_StepName;
        const char * m_TargetName;
    };

    // System wide metrics, gathered periodically
    class Metrics
    {
    public:
        int64_t m_Time = 0;

        // Memory
        uint32_t m_JobMemoryMiB = 0;
        uint32_t m_ProcessMiB = 0;
        uint32_t m_SystemMiB = 0;

        // Network
        uint16_t m_NumConnections = 0;
    };

    // Track information about workers which performed useful work
    class WorkerInfo
    {
    public:
        uint32_t m_MaxThreadId = 0;
        AString m_WorkerName;
    };

    Mutex m_Mutex;
    Atomic<bool> m_ThreadExit{ false };
    Semaphore m_ThreadSignalSemaphore;
    Thread m_Thread;
    Array<Event> m_Events;
    Array<Metrics> m_Metrics;
    Array<WorkerInfo> m_WorkerInfo;

    // Static data for use after teardown
    static int64_t s_CaptureStart;
    static int64_t s_CaptureEnd;
    static AString s_Buffer; // Final JSON to write to disk
};

// BuildProfilerScope
//------------------------------------------------------------------------------
class BuildProfilerScope
{
public:
    // A "main" thread task
    BuildProfilerScope( const char * type );

    // A local "worker" thread task
    BuildProfilerScope( Job & job, uint32_t threadId, const char * type );
    ~BuildProfilerScope();

    void SetStepName( const char * stepName ) { m_StepName = stepName; }

protected:
    BuildProfilerScope & operator=( BuildProfilerScope & other ) = delete;

    bool m_Active;
    uint32_t m_ThreadId;
    const char * m_StepName;
    const char * m_TargetName;
    int64_t m_StartTime;
    Job * m_Job;
};

//------------------------------------------------------------------------------
