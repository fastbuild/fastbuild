// BuildProfiler
//------------------------------------------------------------------------------

// Includes
//------------------------------------------------------------------------------
#include "BuildProfiler.h"

// FBuildCore
#include "Tools/FBuild/FBuildCore/FBuild.h"
#include "Tools/FBuild/FBuildCore/FBuildOptions.h"
#include "Tools/FBuild/FBuildCore/Graph/Node.h"
#include "Tools/FBuild/FBuildCore/Helpers/JSON.h"
#include "Tools/FBuild/FBuildCore/WorkerPool/Job.h"

// Core
#include "Core/FileIO/FileStream.h"
#include "Core/FileIO/MemoryStream.h"
#include "Core/Math/Conversions.h"
#include "Core/Mem/MemInfo.h"
#include "Core/Profile/Profile.h"
#include "Core/Strings/AStackString.h"
#include "Core/Strings/AString.h"

// CONSTRUCTOR (BuildProfiler)
//------------------------------------------------------------------------------
BuildProfiler::BuildProfiler() = default;

// DESTRUCTOR (BuildProfiler)
//------------------------------------------------------------------------------
BuildProfiler::~BuildProfiler() = default;

// StartMetricsGathering
//------------------------------------------------------------------------------
void BuildProfiler::StartMetricsGathering()
{
    PROFILE_FUNCTION;
    m_ThreadExit.Store( false );
    m_Thread.Start( MetricsThreadWrapper, "BuildProfileMetrics" );
}

// StopMetricsGathering
//------------------------------------------------------------------------------
void BuildProfiler::StopMetricsGathering()
{
    PROFILE_FUNCTION;
    m_ThreadExit.Store( true );
    m_ThreadSignalSemaphore.Signal();
    m_Thread.Join();
}

// RecordLocal
//------------------------------------------------------------------------------
void BuildProfiler::RecordLocal( uint32_t threadId,
                                 int64_t startTime,
                                 int64_t endTime,
                                 const char * stepName,
                                 const char * targetName )
{
    const int32_t machineId = Event::kLocalMachineId;

    MutexHolder mh( m_Mutex );
    m_Events.EmplaceBack( machineId, threadId, startTime, endTime, stepName, targetName );
}

// RecordRemote
//------------------------------------------------------------------------------
void BuildProfiler::RecordRemote( uint32_t workerId,
                                  const AString & workerName,
                                  uint32_t remoteThreadId,
                                  int64_t startTime,
                                  int64_t endTime,
                                  const char * stepName,
                                  const char * targetName )
{
    MutexHolder mh( m_Mutex );

    // Record details of worker the first time we see one
    if ( workerId >= m_WorkerInfo.GetSize() )
    {
        // Extend the array so it encompasses the new index
        m_WorkerInfo.SetSize( (size_t)workerId + 1 );
        WorkerInfo & workerInfo = m_WorkerInfo.Top();

        // Take note of the highest seen thread index
        workerInfo.m_MaxThreadId = remoteThreadId;

        // Record the name the first time
        workerInfo.m_WorkerName = workerName;
    }
    else
    {
        // Update the highest seen thread index
        WorkerInfo & workerInfo = m_WorkerInfo[ workerId ];
        workerInfo.m_MaxThreadId = Math::Max( workerInfo.m_MaxThreadId, remoteThreadId );
    }

    // Note the remote compilation event
    m_Events.EmplaceBack( static_cast<int32_t>( workerId ), remoteThreadId, startTime, endTime, stepName, targetName );
}

// SaveJSON
//------------------------------------------------------------------------------
bool BuildProfiler::SaveJSON( const FBuildOptions & options, const char * fileName )
{
    // Thread must be stopped
    ASSERT( m_Thread.IsRunning() == false );

    // Record time taken to save (can't use regular macros since we're process those)
    const int64_t saveStart = Timer::GetNow();

    AString buffer;
    buffer.SetReserved( 1024 * 1024 );

    // Open JSON
    buffer += '[';

    // Section headings
    // - Global metrics
    buffer += "{\"name\":\"process_name\",\"ph\":\"M\",\"pid\":-2,\"tid\":0,\"args\":{\"name\":\"Memory Usage\"}},";
    buffer += "{\"name\":\"process_name\",\"ph\":\"M\",\"pid\":-3,\"tid\":0,\"args\":{\"name\":\"Network Usage\"}},";

    // - Local Processing
    AStackString args( options.GetArgs() );
    JSON::Escape( args );
    AStackString programName( options.m_ProgramName );
    JSON::Escape( programName );
    buffer.AppendFormat( "{\"name\":\"process_name\",\"ph\":\"M\",\"pid\":-1,\"tid\":0,\"args\":{\"name\":\"%s %s\"}},", programName.Get(), args.Get() );
    buffer += "{\"name\":\"thread_name\",\"ph\":\"M\",\"pid\":-1,\"tid\":0,\"args\":{\"name\":\"Phase\"}},";
    const uint32_t numThreads = options.m_NumWorkerThreads;
    for ( uint32_t i = 1; i <= numThreads; ++i )
    {
        buffer.AppendFormat( "{\"name\":\"thread_name\",\"ph\":\"M\",\"pid\":-1,\"tid\":%u,\"args\":{\"name\":\"Thread %02u\"}},", i, i );
    }

    // Remote Processing
    for ( const WorkerInfo & workerInfo : m_WorkerInfo )
    {
        const uint32_t workedPid = static_cast<uint32_t>( m_WorkerInfo.GetIndexOf( &workerInfo ) );
        buffer.AppendFormat( "{\"name\":\"process_name\",\"ph\":\"M\",\"pid\":%u,\"tid\":0,\"args\":{\"name\":\"Worker: %s\"}},", workedPid, workerInfo.m_WorkerName.Get() );
        for ( uint32_t i = 1000; i <= workerInfo.m_MaxThreadId; ++i )
        {
            buffer.AppendFormat( "{\"name\":\"thread_name\",\"ph\":\"M\",\"pid\":%u,\"tid\":%u,\"args\":{\"name\":\"Thread %02u\"}},", workedPid, i, i - 1000 );
        }
    }

    // Serialize events
    const double freqMul = ( static_cast<double>( Timer::GetFrequencyInvFloatMS() ) * 1000.0 );
    for ( const Event & event : m_Events )
    {
        // Emit event with duration
        buffer.AppendFormat( "{\"name\":\"%s\",\"ph\":\"X\",\"ts\":%" PRIu64
                             ",\"dur\":%" PRIu64 ",\"pid\":%i,\"tid\":%u",
                             event.m_StepName,
                             (uint64_t)( (double)event.m_StartTime * freqMul ),
                             (uint64_t)( (double)( event.m_EndTime - event.m_StartTime ) * freqMul ),
                             event.m_MachineId,
                             event.m_ThreadId );

        // Optional additional "target name"
        if ( event.m_TargetName )
        {
            buffer += ",\"args\":{\"name\":\"";
            JSON::AppendEscaped( event.m_TargetName, buffer );
            buffer += "\"}";
        }

        buffer += ( "}," );
    }

    // Serialize metrics
    Metrics lastValues;
    for ( const Metrics & metrics : m_Metrics )
    {
        const uint64_t currentTimeStamp = static_cast<uint64_t>( static_cast<double>( metrics.m_Time ) * freqMul );
        const bool isLast = ( &metrics == &m_Metrics.Top() );

        // Macro to output a stat, but only if it changed (to reduce output size)
#define OUTPUT_STAT( stat, pid, name, unit, type, fmt, div ) \
        if ( ( lastValues.stat != metrics.stat ) || isLast ) \
        { \
            buffer.AppendFormat( "{\"name\":\"" name "\",\"ph\":\"C\",\"ts\":%" PRIu64 ",\"pid\":" pid ",\"args\":{\"" unit "\":" fmt "}},", \
                                 currentTimeStamp, \
                                 static_cast<type>( metrics.stat ) / div ); \
            lastValues.stat = metrics.stat; \
        }

        // Memory
        OUTPUT_STAT( m_JobMemoryMiB, "-2", "Process - Job (MiB)", "MiB", uint32_t, "%u", 1u )
        OUTPUT_STAT( m_ProcessMiB, "-2", "Process - Total (MiB)", "MiB", uint32_t, "%u", 1u )
        OUTPUT_STAT( m_SystemMiB, "-2", "System - Total (GiB)", "GiB", double, "%2.1f", 1024.0 )

        // Network Connections (if using distributed compilation)
        if ( options.m_AllowDistributed )
        {
            OUTPUT_STAT( m_NumConnections, "-3", "Connections", "Num", uint32_t, "%u", 1u )
        }

#undef OUTPUT_STAT
    }

    // Open output file and write the majority of the profiling info
    FileStream f;
    if ( ( f.Open( fileName, FileStream::OPEN_OR_CREATE_READ_WRITE ) == false ) ||
         ( f.WriteBuffer( buffer.Get(), buffer.GetLength() ) != buffer.GetLength() ) )
    {
        return false;
    }

    // Note the time taken to generate the file so far
    const int64_t saveEnd = Timer::GetNow();

    // Write a final event recording the time we took to generate the file
    buffer.Format( "{\"name\":\"%s\",\"ph\":\"X\",\"ts\":%" PRIu64 ",\"dur\":%" PRIu64 ",\"pid\":%i,\"tid\":%u}]",
                   "WriteProfileJSON",
                   (uint64_t)( (double)saveStart * freqMul ),
                   (uint64_t)( (double)( saveEnd - saveStart ) * freqMul ),
                   Event::kLocalMachineId,
                   0u );
    return ( ( f.WriteBuffer( buffer.Get(), buffer.GetLength() ) == buffer.GetLength() ) &&
             f.Truncate() );
}

// MetricsThreadWrapper
//------------------------------------------------------------------------------
/*static*/ uint32_t BuildProfiler::MetricsThreadWrapper( void * /*userData*/ )
{
    BuildProfiler::Get().MetricsUpdate();
    return 0;
}

// MetricsUpdate
//------------------------------------------------------------------------------
void BuildProfiler::MetricsUpdate()
{
    // Periodically record interesting metrics
    const uint32_t updateIntervalMS = 100;
    for ( ;; )
    {
        Metrics & metrics = m_Metrics.EmplaceBack();
        metrics.m_Time = Timer::GetNow();

        // FASTBuild memory usage (this process)
        metrics.m_ProcessMiB = MemInfo::GetProcessInfo();

        // Memory used by distributed jobs
        metrics.m_JobMemoryMiB = MemInfo::ConvertBytesToMiB( Job::GetTotalLocalDataMemoryUsage() );

        // System memory
        SystemMemInfo systemMemInfo;
        MemInfo::GetSystemInfo( systemMemInfo );
        metrics.m_SystemMiB = ( systemMemInfo.m_TotalPhysMiB - systemMemInfo.m_AvailPhysMiB );

        // Network connections
        metrics.m_NumConnections = (uint16_t)FBuild::Get().GetNumWorkerConnections();

        // Exit if we're finished. We check the exit condition here to ensure
        // we always do one final metrics gathering operation before exiting
        if ( m_ThreadExit.Load() )
        {
            return;
        }

        // Wait for exit signal
        m_ThreadSignalSemaphore.Wait( updateIntervalMS );
    }
}

// CONSTRUCTOR (BuildProfilerScope)
//------------------------------------------------------------------------------
BuildProfilerScope::BuildProfilerScope( const char * stepName )
{
    m_Active = BuildProfiler::IsValid();

    // Only record info if the BuildProfiler is active
    if ( m_Active )
    {
        m_ThreadId = 0;
        m_StepName = stepName;
        m_TargetName = nullptr; // Main thread tasks have no associated target
        m_StartTime = Timer::GetNow();
    }

    // No associated Job
    m_Job = nullptr;
}

// CONSTRUCTOR (BuildProfilerScope)
//------------------------------------------------------------------------------
BuildProfilerScope::BuildProfilerScope( Job & job, uint32_t threadId, const char * stepName )
{
    m_Active = ( BuildProfiler::IsValid() &&
                 job.IsLocal() ); // When testing, remote jobs can occur in the same process

    // Only record info if the BuildProfiler is active
    if ( m_Active )
    {
        m_ThreadId = threadId;
        m_StepName = stepName;
        m_TargetName = job.GetNode()->GetName().Get();
        m_StartTime = Timer::GetNow();
    }

    // Hook into associated job
    m_Job = &job;
    m_Job->SetBuildProfilerScope( this );
}

// DESTRUCTOR (BuildProfilerScope)
//------------------------------------------------------------------------------
BuildProfilerScope::~BuildProfilerScope()
{
    // Commit profiling info
    if ( m_Active )
    {
        BuildProfiler::Get().RecordLocal( m_ThreadId, m_StartTime, Timer::GetNow(), m_StepName, m_TargetName );
    }

    // Unhook from associated Job
    if ( m_Job )
    {
        m_Job->SetBuildProfilerScope( nullptr );
    }
}

//------------------------------------------------------------------------------
