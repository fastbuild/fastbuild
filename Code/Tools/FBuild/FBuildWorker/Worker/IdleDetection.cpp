// Worker
//------------------------------------------------------------------------------

// Includes
//------------------------------------------------------------------------------
#include "IdleDetection.h"

// Core
#include "Core/Containers/Array.h"
#include "Core/FileIO/FileStream.h"
#include "Core/Math/Conversions.h"
#include "Core/Process/Process.h"
#include "Core/Strings/AStackString.h"

// system
#if defined( __WINDOWS__ )
    #include "Core/Env/WindowsHeader.h"
    #include <TlHelp32.h>
#endif
#if defined( __LINUX__ )
    #include <dirent.h>
    #include <stdlib.h>
    #include <sys/stat.h>
#endif
#if defined( __OSX__ )
    #include <mach/mach_host.h>
#endif

// Defines
//------------------------------------------------------------------------------
#define IDLE_CHECK_DELAY_SECONDS ( 0.1f )

// CONSTRUCTOR
//------------------------------------------------------------------------------
IdleDetection::IdleDetection()
    : m_CPUUsageFASTBuild( 0.0f )
    , m_CPUUsageTotal( 0.0f )
    , m_IsIdle( false )
    , m_IsIdleFloat( 0.0f )
    , m_IsIdleCurrent( 0.0f )
    , m_IdleSmoother( 0 )
    , m_IdleFloatSmoother( 0 )
    , m_ProcessesInOurHierarchy( 32, true )
    , m_LastTimeIdle( 0 )
    , m_LastTimeBusy( 0 )
{
    ProcessInfo self;
    self.m_PID = Process::GetCurrentId();
    self.m_AliveValue = 0;
    #if defined( __WINDOWS__ )
        self.m_ProcessHandle = ::GetCurrentProcess();
    #endif
    self.m_LastTime = 0;
    m_ProcessesInOurHierarchy.Append( self );
}

// DESTRUCTOR
//------------------------------------------------------------------------------
IdleDetection::~IdleDetection() = default;

// Update
//------------------------------------------------------------------------------
void IdleDetection::Update( uint32_t idleThresholdPercent )
{
    // apply smoothing based on current "idle" state
    if ( IsIdleInternal( idleThresholdPercent, m_IsIdleCurrent ) )
    {
        ++m_IdleSmoother;
    }
    else
    {
        m_IdleSmoother -= 2; // become non-idle more quickly than we become idle
    }
    m_IdleSmoother = Math::Clamp( m_IdleSmoother, 0, 10 );

    // change state only when at extreme of either end of scale
    if ( m_IdleSmoother == 10 ) // 5 secs (called every ~500ms)
    {
        m_IsIdle = true;
    }
    if ( m_IdleSmoother == 0 )
    {
        m_IsIdle = false;
    }

    // separate smoothing for idle float values. They behave differently.
    if ( m_IsIdleCurrent >= m_IsIdleFloat )
    {
        ++m_IdleFloatSmoother;
    }
    else
    {
        m_IdleFloatSmoother -= 2; // become non-idle more quickly than we become idle
    }
    m_IdleFloatSmoother = Math::Clamp( m_IdleFloatSmoother, 0, 10 );

    // change state only when at extreme of either end of scale
    if ( ( m_IdleFloatSmoother == 10 ) || ( m_IdleFloatSmoother == 0 ) ) // 5 secs (called every ~500ms)
    {
        m_IsIdleFloat = m_IsIdleCurrent;
    }
}

// IsIdleInternal
//------------------------------------------------------------------------------
bool IdleDetection::IsIdleInternal( uint32_t idleThresholdPercent, float & idleCurrent )
{
    // determine total cpu time (including idle)
    uint64_t systemTime = 0;
    {
        uint64_t idleTime = 0;
        uint64_t kernTime = 0;
        uint64_t userTime = 0;
        GetSystemTotalCPUUsage( idleTime, kernTime, userTime );

        if ( m_LastTimeBusy > 0 )
        {
            uint64_t idleTimeDelta = ( idleTime - m_LastTimeIdle );
            uint64_t usedTimeDelta = ( ( userTime + kernTime ) - m_LastTimeBusy );
            systemTime = ( idleTimeDelta + usedTimeDelta );
            m_CPUUsageTotal = (float)( (double)usedTimeDelta / (double)systemTime ) * 100.0f;
        }
        m_LastTimeIdle = ( idleTime );
        m_LastTimeBusy = ( userTime + kernTime );
    }

    // if the total CPU time is below the idle theshold, we don't need to
    // check to know acurately what the cpu use of FASTBuild is
    if ( m_CPUUsageTotal < (float)idleThresholdPercent )
    {
        idleCurrent = 1.0f;
        return true;
    }

    // reduce check frequency
    if ( m_Timer.GetElapsed() > IDLE_CHECK_DELAY_SECONDS )
    {
        // iterate all processes
        UpdateProcessList();

        // accumulate cpu usage for processes we care about
        if ( systemTime ) // skip first update
        {
            float totalPerc( 0.0f );

            for ( ProcessInfo & pi : m_ProcessesInOurHierarchy )
            {
                uint64_t kernTime = 0;
                uint64_t userTime = 0;
                GetProcessTime( pi, kernTime, userTime );

                const uint64_t totalTime = ( userTime + kernTime );
                const uint64_t lastTime = pi.m_LastTime;
                if ( lastTime != 0 ) // ignore first update
                {
                    const uint64_t timeSpent = ( totalTime - lastTime );
                    float perc = (float)( (double)timeSpent / (double)systemTime ) * 100.0f;
                    totalPerc += perc;
                }
                pi.m_LastTime = totalTime;
            }

            m_CPUUsageFASTBuild = totalPerc;
        }

        m_Timer.Start();
    }

    idleCurrent = ( 1.0f - ( ( m_CPUUsageTotal - m_CPUUsageFASTBuild ) * 0.01f ) );
    return ( ( m_CPUUsageTotal - m_CPUUsageFASTBuild ) < (float)idleThresholdPercent );
}

// GetSystemTotalCPUUsage
//------------------------------------------------------------------------------
/*static*/ void IdleDetection::GetSystemTotalCPUUsage( uint64_t & outIdleTime,
                                                       uint64_t & outKernTime,
                                                       uint64_t & outUserTime )
{
    #if defined( __WINDOWS__ )
        FILETIME ftIdle, ftKern, ftUser;
        VERIFY( ::GetSystemTimes( &ftIdle, &ftKern, &ftUser ) );
        outIdleTime = ( (uint64_t)ftIdle.dwHighDateTime << 32 ) | (uint64_t)ftIdle.dwLowDateTime;
        outKernTime = ( (uint64_t)ftKern.dwHighDateTime << 32 ) | (uint64_t)ftKern.dwLowDateTime;
        outUserTime = ( (uint64_t)ftUser.dwHighDateTime << 32 ) | (uint64_t)ftUser.dwLowDateTime;
        outKernTime -= outIdleTime; // Kern time includes Idle, but we don't want that
    #elif defined( __OSX__ )
        host_cpu_load_info_data_t cpuInfo;
        mach_msg_type_number_t count = HOST_CPU_LOAD_INFO_COUNT;
        VERIFY( host_statistics( mach_host_self(), HOST_CPU_LOAD_INFO, (host_info_t)&cpuInfo, &count ) == KERN_SUCCESS );
        outIdleTime = cpuInfo.cpu_ticks[ CPU_STATE_IDLE ];
        outKernTime = cpuInfo.cpu_ticks[ CPU_STATE_SYSTEM ];
        outUserTime = cpuInfo.cpu_ticks[ CPU_STATE_USER ];
    #elif defined( __LINUX__ )
        // Read first line of /proc/stat
        AStackString< 1024 > procStat;
        VERIFY( GetProcessInfoString( "/proc/stat", procStat ) ); // Should never fail

        // First line should be system totals
        if ( procStat.BeginsWithI( "cpu" ) )
        {
            Array< uint32_t > values( 10, true );
            const char * pos = procStat.Get() + 4; // skip "cpu "
            for ( ;; )
            {
                char * end;
                values.Append( strtoul( pos, &end, 10 ) );
                pos = end;
                if ( pos == procStat.GetEnd() )
                {
                    break;
                }
            }
            if ( values.GetSize() > 3 )
            {
                // 0+1 = user/nice time, 2 = kernel time
                outUserTime = values[ 0 ] + values[ 1 ];
                outKernTime = values[ 2 ];
                // idle is all times minus user/nice/kernel
                outIdleTime = 0;
                for ( const uint32_t v : values )
                {
                    outIdleTime += v;
                }
                outIdleTime -= outUserTime;
                outIdleTime -= outKernTime;
                return;
            }
        }
        ASSERT( false && "Unexpected /proc/stat format" );
    #endif
}

// GetProcessTime
//------------------------------------------------------------------------------
/*static*/ void IdleDetection::GetProcessTime( const ProcessInfo & pi, uint64_t & outKernTime, uint64_t & outUserTime )
{
    #if defined( __WINDOWS__ )
        FILETIME ftProcKern, ftProcUser, ftUnused;
        if ( ::GetProcessTimes( pi.m_ProcessHandle,
                                &ftUnused,      // creation time
                                &ftUnused,      // exit time
                                &ftProcKern,    // kernel time
                                &ftProcUser ) ) // user time
        {
            outKernTime = ( (uint64_t)ftProcKern.dwHighDateTime << 32 ) | (uint64_t)ftProcKern.dwLowDateTime;
            outUserTime = ( (uint64_t)ftProcUser.dwHighDateTime << 32 ) | (uint64_t)ftProcUser.dwLowDateTime;
        }
        else
        {
            // Process no longer exists
            outKernTime = 0;
            outUserTime = 0;
        }
    #elif defined( __OSX__ )
        // TODO:OSX Implement GetProcecessTime
        (void)pi;
        outKernTime = 0;
        outUserTime = 0;
    #elif defined( __LINUX__ )
        // Read first line of /proc/<pid>/stat for the process
        AStackString< 1024 > processInfo;
        if ( GetProcessInfoString( AStackString<>().Format( "/proc/%u/stat", pi.m_PID ).Get(),
                                   processInfo ) )
        {
            Array< AString > tokens( 32, true );
            processInfo.Tokenize( tokens, ' ' );
            if ( tokens.GetSize() >= 15 )
            {
                // Item index 13 and 14 (0-based) are the utime and stime
                outUserTime = strtoul( tokens[ 13 ].Get(), nullptr, 10 );
                outKernTime = strtoul( tokens[ 14 ].Get(), nullptr, 10 );
                return;
            }
            else
            {
                // Something is terribly wrong
                ASSERT( false && "Unexpected '/proc/<pid>/stat' format" );
            }
        }

        // Process may have exited, so handle that gracefully
        outKernTime = 0;
        outUserTime = 0;
    #endif
}

// UpdateProcessList
//------------------------------------------------------------------------------
void IdleDetection::UpdateProcessList()
{
    // Mark processes we've seen so we can prune them when they terminate
    static uint32_t sAliveValue = 0;
    sAliveValue++;

    #if defined( __WINDOWS__ )
        HANDLE hSnapShot = ::CreateToolhelp32Snapshot( TH32CS_SNAPPROCESS, 0 );
        if ( hSnapShot == INVALID_HANDLE_VALUE )
        {
            return;
        }

        PROCESSENTRY32 thProcessInfo;
        memset( &thProcessInfo, 0, sizeof(PROCESSENTRY32) );
        thProcessInfo.dwSize = sizeof(PROCESSENTRY32);
        while ( Process32Next( hSnapShot, &thProcessInfo ) != FALSE )
        {
            const uint32_t parentPID = thProcessInfo.th32ParentProcessID;

            // is process a child of one we care about?
            if ( m_ProcessesInOurHierarchy.Find( parentPID ) )
            {
                const uint32_t pid = thProcessInfo.th32ProcessID;
                ProcessInfo * info = m_ProcessesInOurHierarchy.Find( pid );
                if ( info )
                {
                    // an existing process that is still alive
                    info->m_AliveValue = sAliveValue; // still active
                }
                else
                {
                    // a new process
                    void * handle = OpenProcess( PROCESS_ALL_ACCESS, TRUE, pid );
                    if ( handle )
                    {
                        // track new process
                        ProcessInfo newProcess;
                        newProcess.m_PID = pid;
                        newProcess.m_ProcessHandle = handle;
                        newProcess.m_AliveValue = sAliveValue;
                        newProcess.m_LastTime = 0;
                        m_ProcessesInOurHierarchy.Append( newProcess );
                    }
                    else
                    {
                        // gracefully handle failure to open proces
                        // maybe it closed before we got to it
                    }
                }
            }
        }
        CloseHandle( hSnapShot );
    #elif defined( __OSX__ )
        // TODO:OSX Implement FindNewProcesses
    #elif defined( __LINUX__ )
        // Each process has a directory in /proc/
        // The name of the dir is the pid
        AStackString<> path( "/proc/" );
        DIR * dir = opendir( path.Get() );
        ASSERT( dir ); // This should never fail
        if ( dir )
        {
            for ( ;; )
            {
                dirent * entry = readdir( dir );
                if ( entry == nullptr )
                {
                    break; // no more entries
                }

                bool isDir = ( entry->d_type == DT_DIR );

                // Not all filesystems have support for returning the file type in
                // d_type and applications must properly handle a return of DT_UNKNOWN.
                if ( entry->d_type == DT_UNKNOWN )
                {
                    path.SetLength( 6 ); // truncate to /proc/
                    path += entry->d_name;

                    struct stat info;
                    VERIFY( stat( path.Get(), &info ) == 0 );
                    isDir = S_ISDIR( info.st_mode );
                }

                // Is this a directory?
                if ( isDir == false )
                {
                    continue;
                }

                // Determine if this is a PID
                const char * pos = entry->d_name;
                for (;;)
                {
                    const char c = *pos;
                    if ( ( c >= '0' ) && ( c <= '9' ) )
                    {
                        ++pos;
                    }
                    else
                    {
                        break;
                    }
                }
                if ( pos == entry->d_name )
                {
                    // Not a PID
                    continue;
                }

                // Filename is PID
                const uint32_t pid = strtoul( entry->d_name, nullptr, 10 );

                // Read the first line of /proc/<pid>/stat for the process
                AStackString< 1024 > processInfo;
                if ( GetProcessInfoString( AStackString<>().Format( "/proc/%u/stat", pid ).Get(),
                                           processInfo ) == false )
                {
                    continue; // Process might have exited
                }

                // Item index 3 (0-based) is the parent PID
                Array< AString > tokens( 32, true );
                processInfo.Tokenize( tokens, ' ' );
                const uint32_t parentPID = strtoul( tokens[ 3 ].Get(), nullptr, 10 );

                // is process a child of one we care about?
                if ( m_ProcessesInOurHierarchy.Find( parentPID ) )
                {
                    ASSERT( pid == strtoul( tokens[ 0 ].Get(), nullptr, 10 ) ); // Item index 0 (0-based) is the PID

                    // Are we already tracking this process?
                    ProcessInfo * info = m_ProcessesInOurHierarchy.Find( pid );
                    if ( info )
                    {
                        // an existing process that is still alive
                        info->m_AliveValue = sAliveValue; // still active
                    }
                    else
                    {
                        // track new process
                        ProcessInfo newProcess;
                        newProcess.m_PID = pid;
                        newProcess.m_AliveValue = sAliveValue;
                        newProcess.m_LastTime = 0;
                        m_ProcessesInOurHierarchy.Append( newProcess );
                    }
                }
            }
            closedir( dir );
        }
    #endif

    // prune dead processes
    {
        // never prune first process (this process)
        const size_t numProcesses = m_ProcessesInOurHierarchy.GetSize();
        for ( size_t i = ( numProcesses - 1 ); i > 0; --i )
        {
            if ( m_ProcessesInOurHierarchy[ i ].m_AliveValue != sAliveValue )
            {
                // dead process
                #if defined( __WINDOWS__ )
                    #if ( _MSC_VER <= 1900 ) // Avoid VS2015 false positive
                        PRAGMA_DISABLE_PUSH_MSVC( 6001 ) // Using uninitialized memory
                    #endif
                    CloseHandle( m_ProcessesInOurHierarchy[ i ].m_ProcessHandle );
                    #if ( _MSC_VER <= 1900 ) // Avoid VS2015 false positive
                        PRAGMA_DISABLE_POP_MSVC // 6001
                    #endif
                #endif
                m_ProcessesInOurHierarchy.EraseIndex( i );
            }
        }
    }
}

// GetProcessInfoString
//------------------------------------------------------------------------------
#if defined( __LINUX__ )
    /*static*/ bool IdleDetection::GetProcessInfoString( const char * fileName,
                                                         AStackString< 1024 > & outProcessInfoString )
    {
        // Open the file
        FileStream f;
        if ( f.Open( fileName, FileStream::READ_ONLY ) == false )
        {
            return false;
        }

        // Try to read 1KiB
        outProcessInfoString.SetLength( 1024 );
        const uint32_t len = f.ReadBuffer( outProcessInfoString.Get(), outProcessInfoString.GetLength() );
        outProcessInfoString.SetLength( len );

        // Truncate to the first line
        const char * lineEnd = outProcessInfoString.Find( '\n' );
        if ( lineEnd )
        {
            outProcessInfoString.SetLength( lineEnd - outProcessInfoString.Get() );
            return true;
        }

        // Line was too long or there was some other problem
        ASSERT( false && "Unexpected proc file size");
        outProcessInfoString.Clear();
        return false;
    }
#endif

//------------------------------------------------------------------------------
