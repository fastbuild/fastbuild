// FBuild.cpp - The main FBuild interface class
//------------------------------------------------------------------------------
#pragma once

// Includes
//------------------------------------------------------------------------------
#include "Tools/FBuild/FBuildCore/FBuildOptions.h"

#include "Helpers/FBuildStats.h"
#include "WorkerPool/WorkerBrokerage.h"

#include "Core/Containers/Array.h"
#include "Core/Containers/Singleton.h"
#include "Core/Strings/AString.h"
#include "Core/Time/Timer.h"

// Forward Declarations
//------------------------------------------------------------------------------
class BFFMacros;
class Client;
class Dependencies;
class FileStream;
class ICache;
class IOStream;
class JobQueue;
class Node;
class NodeGraph;
class SettingsNode;

// FBuild
//------------------------------------------------------------------------------
class FBuild : public Singleton< FBuild >
{
public:
    explicit FBuild( const FBuildOptions & options = FBuildOptions() );
    ~FBuild();

    // initialize the dependency graph, using the BFF config file
    // OR a previously saved NodeGraph DB (if available/matching the BFF)
    bool Initialize( const char * nodeGraphDBFile = nullptr );

    // build a target
    bool Build( const AString & target );
    bool Build( const Array< AString > & targets );
    bool Build( Node * nodeToBuild );

    // after a build we can store progress/parsed rules for next time
    bool SaveDependencyGraph( const char * nodeGraphDBFile ) const;
    void SaveDependencyGraph( IOStream & memorySteam, const char* nodeGraphDBFile ) const;

    const FBuildOptions & GetOptions() const { return m_Options; }

    const AString & GetWorkingDir() const { return m_Options.GetWorkingDir(); }

    static const char * GetDefaultBFFFileName();

    inline const SettingsNode * GetSettings() const { return m_Settings; }

    void GetCacheFileName( uint64_t keyA, uint32_t keyB, uint64_t keyC, uint64_t keyD,
                           AString & path ) const;

    void SetEnvironmentString( const char * envString, uint32_t size, const AString & libEnvVar );
    inline const char * GetEnvironmentString() const            { return m_EnvironmentString; }
    inline uint32_t     GetEnvironmentStringSize() const        { return m_EnvironmentStringSize; }

    const AString& GetRootPath() const { return m_RootPath; }
    void SetRootPath( const AString& rootPath ) { m_RootPath = rootPath; }

    void DisplayTargetList() const;
    bool DisplayDependencyDB( const Array< AString > & targets ) const;

    class EnvironmentVarAndHash
    {
    public:
        EnvironmentVarAndHash( const char * name, uint32_t hash )
         : m_Name( name )
         , m_Hash( hash )
        {}

        inline const AString &              GetName() const             { return m_Name; }
        inline uint32_t                     GetHash() const             { return m_Hash; }

    protected:
        AString     m_Name;
        uint32_t    m_Hash;
    };

    bool ImportEnvironmentVar( const char * name, bool optional, AString & value, uint32_t & hash );
    const Array< EnvironmentVarAndHash > & GetImportedEnvironmentVars() const { return m_ImportedEnvironmentVars; }

    void GetLibEnvVar( AString & libEnvVar ) const;

    // stats - read access
    const FBuildStats & GetStats() const    { return m_BuildStats; }
    // stats - write access
    FBuildStats & GetStatsMutable()         { return m_BuildStats; }

    // attempt to cleanly stop the build
    static        void AbortBuild();
    static        void OnBuildError();
    static inline bool GetStopBuild() { return s_StopBuild; }
    static inline volatile bool * GetAbortBuildPointer() { return &s_AbortBuild; }

    inline ICache * GetCache() const { return m_Cache; }

    static bool GetTempDir( AString & outTempDir );

    bool CacheOutputInfo() const;
    bool CacheTrim() const;

    // root path agnostic hash functions
    static uint32_t Hash32( const void * buffer, size_t len );
    static uint64_t Hash64( const void * buffer, size_t len );

    inline static uint32_t Hash32( const AString & string ) { return Hash32( string.Get(), string.GetLength() ); }
    inline static uint64_t Hash64( const AString & string ) { return Hash64( string.Get(), string.GetLength() ); }

private:
    bool GetTargets( const Array< AString > & targets, Dependencies & outDeps ) const;

    void UpdateBuildStatus( const Node * node );

    static bool s_StopBuild;
    static volatile bool s_AbortBuild;  // -fastcancel - TODO:C merge with StopBuild

    BFFMacros * m_Macros;

    NodeGraph * m_DependencyGraph;
    JobQueue * m_JobQueue;
    Client * m_Client; // manage connections to worker servers

    AString m_DependencyGraphFile;
    ICache * m_Cache;

    AString m_RootPath;

    SettingsNode * m_Settings;

    Timer m_Timer;
    float m_LastProgressOutputTime;
    float m_LastProgressCalcTime;
    float m_SmoothedProgressCurrent;
    float m_SmoothedProgressTarget;

    FBuildStats m_BuildStats;

    FBuildOptions m_Options;

    WorkerBrokerage m_WorkerBrokerage;

    AString m_OldWorkingDir;

    // a double-null terminated string
    char *      m_EnvironmentString;
    uint32_t    m_EnvironmentStringSize; // size excluding last null
    AString     m_LibEnvVar; // LIB= value

    Array< EnvironmentVarAndHash > m_ImportedEnvironmentVars;
};

//------------------------------------------------------------------------------
