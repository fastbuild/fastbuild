// Job - a work item to be processed
//------------------------------------------------------------------------------
#pragma once

// Includes
//------------------------------------------------------------------------------
#include "Core/Env/MSVCStaticAnalysis.h"
#include "Core/Env/Types.h"
#include "Core/Process/Atomic.h"
#include "Core/Strings/AString.h"

// Forward Declarations
//------------------------------------------------------------------------------
class BuildProfilerScope;
class IOStream;
class Node;
class ToolManifest;

// Job
//------------------------------------------------------------------------------
class Job
{
public:
    explicit Job( Node * node );
    explicit Job( IOStream & stream );
    ~Job();

    uint32_t GetJobId() const { return m_JobId; }
    bool operator==( uint32_t jobId ) const { return ( m_JobId == jobId ); }

    Node * GetNode() const { return m_Node; }
    const AString & GetRemoteName() const { return m_RemoteName; }
    const AString & GetRemoteSourceRoot() const { return m_RemoteSourceRoot; }

    void SetCacheName( const AString & cacheName ) { m_CacheName = cacheName; }
    const AString & GetCacheName() const { return m_CacheName; }

    const volatile bool * GetAbortFlagPointer() const { return &m_Abort; }
    void CancelDueToRemoteRaceWin();

    // associate some data with this object, and destroy it when freed
    void OwnData( void * data, size_t size, bool compressed = false );

    void * GetData() const { return m_Data; }
    size_t GetDataSize() const { return m_DataSize; }

    void SetUserData( void * data ) { m_UserData = data; }
    void * GetUserData() const { return m_UserData; }

    void SetToolManifest( ToolManifest * manifest ) { m_ToolManifest = manifest; }
    ToolManifest * GetToolManifest() const { return m_ToolManifest; }

    bool IsDataCompressed() const { return m_DataIsCompressed; }
    bool IsLocal() const { return m_IsLocal; }

    const Array<AString> & GetMessages() const { return m_Messages; }

    // logging interface
    void Error( MSVC_SAL_PRINTF const char * format, ... ) FORMAT_STRING( 2, 3 );
    void ErrorPreformatted( const char * message );
    void SetMessages( const Array<AString> & messages );

    // Flag "system failures" - i.e. not a compilation failure, but some other problem (typically a remote worker misbehaving)
    void OnSystemError() { ++m_SystemErrorCount; }
    uint8_t GetSystemErrorCount() const { return m_SystemErrorCount; }

    // serialization for remote distribution
    void Serialize( IOStream & stream );
    void Deserialize( IOStream & stream );

    void GetMessagesForLog( AString & buffer ) const;
    static void GetMessagesForLog( const Array<AString> & messages, AString & buffer );
    void GetMessagesForMonitorLog( AString & buffer ) const;
    static void GetMessagesForMonitorLog( const Array<AString> & messages, AString & outBuffer );

    void SetRemoteThreadIndex( uint16_t threadIndex ) { m_RemoteThreadIndex = threadIndex; }
    uint16_t GetRemoteThreadIndex() const { return m_RemoteThreadIndex; }

    void SetResultCompressionLevel( int16_t compressionLevel, bool allowZstdUse )
    {
        m_ResultCompressionLevel = compressionLevel;
        m_AllowZstdUse = allowZstdUse;
    }
    int16_t GetResultCompressionLevel() const { return m_ResultCompressionLevel; }
    bool GetAllowZstdUse() const { return m_AllowZstdUse; }

    enum DistributionState : uint8_t
    {
        DIST_NONE = 0, // All non-distributable jobs
        DIST_AVAILABLE = 1, // A distributable job, not in progress

        DIST_BUILDING_REMOTELY = 2, // Building remotely only
        DIST_COMPLETED_REMOTELY = 3, // Completed remotely, but not finalized (don't start racing)

        DIST_BUILDING_LOCALLY = 4, // Building locally only
        DIST_COMPLETED_LOCALLY = 5, // Completed locally, but not finalized

        DIST_RACING = 6, // Building locally AND remotely
        DIST_RACE_WON_LOCALLY = 7, // Completed locally, but still in flight remotely
        DIST_RACE_WON_REMOTELY_CANCEL_LOCAL = 8, // Completed remotely, waiting for local job to cancel
        DIST_RACE_WON_REMOTELY = 9, // Completed remotely, local job cancelled successfully
    };
    void SetDistributionState( DistributionState state ) { m_DistributionState = state; }
    DistributionState GetDistributionState() const { return m_DistributionState; }

    // Access total memory usage by job data
    static uint64_t GetTotalLocalDataMemoryUsage();

    void SetBuildProfilerScope( BuildProfilerScope * scope );
    BuildProfilerScope * GetBuildProfilerScope() const { return m_BuildProfilerScope; }

private:
    uint32_t m_JobId = 0;
    uint32_t m_DataSize = 0;
    Node * m_Node = nullptr;
    void * m_Data = nullptr;
    void * m_UserData = nullptr;
    volatile bool m_Abort = false;
    bool m_DataIsCompressed:1;
    bool m_IsLocal:1;
    bool m_AllowZstdUse:1; // Can client accept Zstd results?
    uint8_t m_SystemErrorCount = 0; // On client, the total error count, on the worker a flag for the current attempt
    DistributionState m_DistributionState = DIST_NONE;
    int16_t m_ResultCompressionLevel = 0; // Compression level of returned results
    uint16_t m_RemoteThreadIndex = 0; // On server, the thread index used to build
    AString m_RemoteName;
    AString m_RemoteSourceRoot;
    AString m_CacheName;
    BuildProfilerScope * m_BuildProfilerScope = nullptr; // Additional context when profiling a build
    ToolManifest * m_ToolManifest = nullptr;

    Array<AString> m_Messages;

    static Atomic<int64_t> s_TotalLocalDataMemoryUsage; // Total memory being managed by OwnData
};

//------------------------------------------------------------------------------
