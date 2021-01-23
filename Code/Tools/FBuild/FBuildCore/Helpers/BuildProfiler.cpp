// BuildProfiler
//------------------------------------------------------------------------------

// Includes
//------------------------------------------------------------------------------
#include "BuildProfiler.h"

// FBuildCore
#include "Tools/FBuild/FBuildCore/FBuildOptions.h"
#include "Tools/FBuild/FBuildCore/Graph/Node.h"
#include "Tools/FBuild/FBuildCore/Helpers/JSON.h"
#include "Tools/FBuild/FBuildCore/WorkerPool/Job.h"

// Core
#include "Core/FileIO/FileStream.h"
#include "Core/FileIO/MemoryStream.h"
#include "Core/Math/Conversions.h"
#include "Core/Profile/Profile.h"
#include "Core/Strings/AString.h"
#include "Core/Strings/AStackString.h"
#if defined( __WINDOWS__ )
    #include "Core/Env/WindowsHeader.h"
    #include "Psapi.h"
#endif

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

    ASSERT( m_Thread == INVALID_THREAD_HANDLE );
    m_ThreadExit = false;
    m_Thread = Thread::CreateThread( MetricsThreadWrapper, "BuildProfileMetrics" );
}

// StopMetricsGathering
//------------------------------------------------------------------------------
/*static*/ void BuildProfiler::StopMetricsGathering()
{
    PROFILE_FUNCTION;

    ASSERT( m_Thread != INVALID_THREAD_HANDLE );
    m_ThreadExit = true;
    m_ThreadSignalSemaphore.Signal();
    Thread::WaitForThread( m_Thread );
    Thread::CloseHandle( m_Thread );
    m_Thread = INVALID_THREAD_HANDLE;
}

// RecordLocal
//------------------------------------------------------------------------------
void BuildProfiler::RecordLocal( uint32_t threadId,
                      int64_t startTime,
                      int64_t endTime,
                      const char * stepName,
                      const char * targetName )
{
    const int32_t machineId = Event::LOCAL_MACHINE_ID;

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
    //RecordWorkerInfo( workerId, remoteThreadId, workerName );
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
    m_Events.EmplaceBack( static_cast<int32_t>(workerId), remoteThreadId, startTime, endTime, stepName, targetName );
}

// SaveJSON
//------------------------------------------------------------------------------
bool BuildProfiler::SaveJSON( const FBuildOptions & options,  const char * fileName )
{
    // Thread must be stopped
    ASSERT( m_Thread == INVALID_THREAD_HANDLE );

    // Record time taken to save (can't use regular macros since we're process those)
    const int64_t saveStart = Timer::GetNow();

    AString buffer;
    buffer.SetReserved( 1024 * 1024 );

    // Open JSON
    buffer += '[';

    // Section headings
    // - Global metrics
    buffer += "{\"name\":\"process_name\",\"ph\":\"M\",\"pid\":-2,\"tid\":0,\"args\":{\"name\":\"Memory Usage\"}},";

    // - Local Processing
    AStackString<> args( options.GetArgs() );
    JSON::Escape( args );
    AStackString<> programName( options.m_ProgramName );
    JSON::Escape( programName );
    buffer.AppendFormat( "{\"name\":\"process_name\",\"ph\":\"M\",\"pid\":-1,\"tid\":0,\"args\":{\"name\":\"%s %s\"}},", programName.Get(), args.Get() );
    buffer += "{\"name\":\"thread_name\",\"ph\":\"M\",\"pid\":-1,\"tid\":0,\"args\":{\"name\":\"Phase\"}},";
    const uint32_t numThreads = options.m_NumWorkerThreads;
    for ( uint32_t i = 1; i <= numThreads; ++i )
    {
        buffer.AppendFormat("{\"name\":\"thread_name\",\"ph\":\"M\",\"pid\":-1,\"tid\":%u,\"args\":{\"name\":\"Thread %02u\"}},", i, i);
    }

    // Remote Processing
    for ( const WorkerInfo & workerInfo : m_WorkerInfo )
    {
        const uint32_t workedPid = (uint32_t)( &workerInfo - m_WorkerInfo.Begin() );
        buffer.AppendFormat( "{\"name\":\"process_name\",\"ph\":\"M\",\"pid\":%u,\"tid\":0,\"args\":{\"name\":\"Worker: %s\"}},", workedPid, workerInfo.m_WorkerName.Get() );
        for ( uint32_t i = 1000; i <= workerInfo.m_MaxThreadId; ++i )
        {
            buffer.AppendFormat("{\"name\":\"thread_name\",\"ph\":\"M\",\"pid\":%u,\"tid\":%u,\"args\":{\"name\":\"Thread %02u\"}},", workedPid, i, i - 1000);
        }
    }

    // Serialize events
    AStackString<> nameBuffer;
    const double freqMul = ( static_cast<double>( Timer::GetFrequencyInvFloatMS()) * 1000.0 );
    for ( Event & event : m_Events )
    {
        // Emit event with duration
        buffer.AppendFormat( "{\"name\":\"%s\",\"ph\":\"X\",\"ts\":%" PRIu64 ",\"dur\":%" PRIu64 ",\"pid\":%i,\"tid\":%u",
                                event.m_StepName,
                                (uint64_t)( (double)event.m_StartTime * freqMul ),
                                (uint64_t)( (double)(event.m_EndTime - event.m_StartTime) * freqMul ),
                                event.m_MachineId,
                                event.m_ThreadId );

        // Optional additional "target name"
        if ( event.m_TargetName )
        {
            nameBuffer = event.m_TargetName;
            JSON::Escape( nameBuffer );
            buffer.AppendFormat( ",\"args\":{\"name\":\"%s\"}", nameBuffer.Get());
        }

        buffer += ( "}," );
    }

    // Serialize metrics
    for ( const Metrics & metrics : m_Metrics )
    {
        // Total Memory
        buffer.AppendFormat( "{\"name\":\"Total Mem\",\"ph\":\"C\",\"ts\":%" PRIu64 ",\"pid\":-2,\"args\":{\"MiB\":%u}},",
                             (uint64_t)( (double)metrics.m_Time * freqMul ),
                             metrics.m_TotalMemoryMiB );
        // Distributed Memory
        if ( metrics.m_DistributedMemoryMiB > 0 )
        {
            buffer.AppendFormat( "{\"name\":\"Distributed Mem\",\"ph\":\"C\",\"ts\":%" PRIu64 ",\"pid\":-2,\"args\":{\"MiB\":%u}},",
                                 (uint64_t)( (double)metrics.m_Time * freqMul ),
                                 metrics.m_DistributedMemoryMiB );
        }
    }

    // Open output file and write the majority of the profiling info
    FileStream f;
    if ( ( f.Open( fileName, FileStream::WRITE_ONLY ) == false ) ||
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
                   (uint64_t)( (double)(saveEnd - saveStart) * freqMul ),
                   Event::LOCAL_MACHINE_ID,
                   0 );
    return( f.WriteBuffer( buffer.Get(), buffer.GetLength() ) == buffer.GetLength() );
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
    // Preiodically record interesting metrics
    const uint32_t updateIntervalMS = 100;
    for ( ;; )
    {
        Metrics & metrics = m_Metrics.EmplaceBack();
        metrics.m_Time = Timer::GetNow();

        // FASTBuild memory usage (this process)
#if defined( __WINDOWS__ )
        PROCESS_MEMORY_COUNTERS_EX counters;
        VERIFY( GetProcessMemoryInfo( GetCurrentProcess(),
                                      (PROCESS_MEMORY_COUNTERS*)&counters,
                                      sizeof(PROCESS_MEMORY_COUNTERS_EX) ) );
        metrics.m_TotalMemoryMiB = (uint32_t)( counters.WorkingSetSize / ( 1024 * 1024 ) );
#else
        metrics.m_TotalMemoryMiB = 0; // TODO:LINUX TODO:OSX Implement total memory usage stat
#endif

        // Memory used by distributed jobs
        metrics.m_DistributedMemoryMiB = (uint32_t)( Job::GetTotalLocalDataMemoryUsage() / ( 1024 * 1024 ) );

        // Exit if we're finished. We check the exit condition here to ensure
        // we always do one final metrics gathering operation before exiting
        if ( m_ThreadExit )
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
    // Only record info if the BuildProfiler is active
    if ( BuildProfiler::IsValid() )
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
BuildProfilerScope::BuildProfilerScope( Job * job, uint32_t threadId, const char * stepName )
{
    // Only record info if the BuildProfiler is active
    if ( BuildProfiler::IsValid() )
    {
        m_ThreadId = threadId;
        m_StepName = stepName;
        m_TargetName = job->GetNode()->GetName().Get();
        m_StartTime = Timer::GetNow();
    }

    // Hook into associated job
    m_Job = job;
    if ( m_Job )
    {
        m_Job->SetBuildProfilerScope( this );
    }
}

// DESTRUCTOR (BuildProfilerScope)
//------------------------------------------------------------------------------
BuildProfilerScope::~BuildProfilerScope()
{
    // Commit profiling info
    if ( BuildProfiler::IsValid() )
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
