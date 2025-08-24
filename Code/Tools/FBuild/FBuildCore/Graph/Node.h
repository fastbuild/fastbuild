// Node.h - base interface for dependency graph nodes
//------------------------------------------------------------------------------
#pragma once

// Includes
//------------------------------------------------------------------------------
// FBuild
#include "Tools/FBuild/FBuildCore/Graph/Dependencies.h"

// Core
#include "Core/Containers/Array.h"
#include "Core/Reflection/ReflectionMacros.h"
#include "Core/Reflection/Struct.h"
#include "Core/Strings/AString.h"

// Forward Declarations
//------------------------------------------------------------------------------
class BFFToken;
class CompilerNode;
class ConstMemoryStream;
class FileNode;
class Function;
class IMetaData;
class IOStream;
class Job;
class NodeGraph;
class ObjectListNode;

// Defines
//------------------------------------------------------------------------------
#define INVALID_NODE_INDEX ( (uint32_t)0xFFFFFFFF )

// Custom Reflection Macros
//------------------------------------------------------------------------------
#define REFLECT_NODE_DECLARE( nodeName )                                \
    REFLECT_STRUCT_DECLARE( nodeName )                                  \
    virtual const ReflectionInfo * GetReflectionInfoV() const override  \
    {                                                                   \
        return nodeName::GetReflectionInfoS();                          \
    }

#define REFLECT_NODE_BEGIN( nodeName, baseNodeName, metaData )          \
    REFLECT_STRUCT_BEGIN( nodeName, baseNodeName, metaData )

#define REFLECT_NODE_BEGIN_ABSTRACT( nodeName, baseNodeName, metaData ) \
    REFLECT_STRUCT_BEGIN_ABSTRACT( nodeName, baseNodeName, metaData )

inline constexpr PropertyType GetPropertyType( ObjectListNode * const * )
{
    return PT_CUSTOM_1;
}

// FBuild
//------------------------------------------------------------------------------
class Node : public Struct
{
    REFLECT_STRUCT_DECLARE( Node )
    virtual const ReflectionInfo * GetReflectionInfoV() const
    {
        return nullptr; // TODO:B Make pure virtual when everything is using reflection
    }

public:
    enum Type : uint8_t
    {
        PROXY_NODE = 0,
        COPY_FILE_NODE = 1,
        DIRECTORY_LIST_NODE = 2,
        EXEC_NODE = 3,
        FILE_NODE = 4,
        LIBRARY_NODE = 5,
        OBJECT_NODE = 6,
        ALIAS_NODE = 7,
        EXE_NODE = 8,
        UNITY_NODE = 9,
        CS_NODE = 10,
        TEST_NODE = 11,
        COMPILER_NODE = 12,
        DLL_NODE = 13,
        VCXPROJECT_NODE = 14,
        OBJECT_LIST_NODE = 15,
        COPY_DIR_NODE = 16,
        SLN_NODE = 17,
        REMOVE_DIR_NODE = 18,
        XCODEPROJECT_NODE = 19,
        SETTINGS_NODE = 20,
        VSPROJEXTERNAL_NODE = 21,
        TEXT_FILE_NODE = 22,
        LIST_DEPENDENCIES_NODE = 23,
        // Make sure you update 's_NodeTypeNames' in the cpp
        NUM_NODE_TYPES      // leave this last
    };

    enum ControlFlag : uint8_t
    {
        FLAG_NONE = 0x00,
        FLAG_ALWAYS_BUILD = 0x01, // DoBuild is always performed (for e.g. directory listings)
    };

    enum StatsFlag : uint16_t
    {
        STATS_PROCESSED = 0x01, // node was processed during the build
        STATS_BUILT = 0x02, // node needed building, and was built
        STATS_CACHE_HIT = 0x04, // needed building, was cacheable & was retrieved from the cache
        STATS_CACHE_MISS = 0x08, // needed building, was cacheable, but wasn't in cache
        STATS_CACHE_STORE = 0x10, // needed building, was cacheable & was stored to the cache
        STATS_LIGHT_CACHE = 0x20, // used the LightCache
        STATS_BUILT_REMOTE = 0x40, // node was built remotely
        STATS_FAILED = 0x80, // node needed building, but failed
        STATS_FIRST_BUILD = 0x100,// node has never been built before
    };

    enum class BuildResult
    {
        eFailed,            // something went wrong building
        eAborted,           // interrupted due to build termination
        eNeedSecondPass,    // needs build called again
        eOk,                // built ok
    };

    enum State : uint8_t
    {
        NOT_PROCESSED,      // no work done (either not part of this build, or not yet seen)
        STATIC_DEPS,        // pre-build deps processed and checking static deps
        DYNAMIC_DEPS,       // dynamic deps regenerated and being checked
        BUILDING,           // in the queue for building
        FAILED,             // failed to build
        UP_TO_DATE,         // built, or confirmed as not needing building
    };

    explicit Node( Type type );
    virtual bool Initialize( NodeGraph & nodeGraph, const BFFToken * funcStartIter, const Function * function ) = 0;
    virtual ~Node();

    uint32_t GetNameHash() const { return m_NameHash; }
    Type GetType() const { return m_Type; }
    const char * GetTypeName() const { return s_NodeTypeNames[ m_Type ]; }
    static const char * GetTypeName( Type t ) { return s_NodeTypeNames[ t ]; }
    template <class T>
    T * CastTo() const;

    // each node must specify if it outputs a file
    virtual bool IsAFile() const = 0;

    State GetState() const { return m_State; }

    [[nodiscard]] bool GetStatFlag( StatsFlag flag ) const { return ( ( m_StatsFlags & flag ) != 0 ); }
    void SetStatFlag( StatsFlag flag ) const { m_StatsFlags |= flag; }

    uint32_t GetLastBuildTime() const;
    uint32_t GetProcessingTime() const { return m_ProcessingTime; }
    uint32_t GetCachingTime() const { return m_CachingTime; }
    uint32_t GetRecursiveCost() const { return m_RecursiveCost; }

    uint32_t GetProgressAccumulator() const { return m_ProgressAccumulator; }
    void SetProgressAccumulator( uint32_t p ) const { m_ProgressAccumulator = p; }

    static void Load( NodeGraph & nodeGraph, ConstMemoryStream & stream );
    static void LoadExtended( NodeGraph & nodeGraph, Node * node, ConstMemoryStream & stream );
    static void Save( IOStream & stream, const Node * node );
    static void SaveExtended( IOStream & stream, const Node * node );
    virtual void PostLoad( NodeGraph & nodeGraph ); // TODO:C Eliminate the need for this function

    static Node * LoadRemote( IOStream & stream );
    static void SaveRemote( IOStream & stream, const Node * node );

    static bool EnsurePathExistsForFile( const AString & name );
    static bool DoPreBuildFileDeletion( const AString & fileName );

    uint64_t GetStamp() const { return m_Stamp; }

    static void DumpOutput( Job * job,
                            const AString & output,
                            const Array<AString> * exclusions = nullptr );

    void SetBuildPassTag( uint32_t pass ) const { m_BuildPassTag = pass; }
    uint32_t GetBuildPassTag() const { return m_BuildPassTag; }

    const AString & GetName() const { return m_Name; }

    virtual const AString & GetPrettyName() const { return GetName(); }

    bool IsHidden() const { return m_Hidden; }
    uint8_t GetConcurrencyGroupIndex() const { return m_ConcurrencyGroupIndex; }

    const Dependencies & GetPreBuildDependencies() const { return m_PreBuildDependencies; }
    const Dependencies & GetStaticDependencies() const { return m_StaticDependencies; }
    const Dependencies & GetDynamicDependencies() const { return m_DynamicDependencies; }

    static uint32_t CalcNameHash( const AString & name );

    static void CleanMessageToPreventMSBuildFailure( const AString & msg, AString & outMsg );

protected:
    friend class FBuild;
    friend struct FBuildStats;
    friend class Function;
    friend class JobQueue;
    friend class JobQueueRemote;
    friend class NodeGraph;
    friend class ProjectGeneratorBase; // TODO:C Remove this
    friend class Report;
    friend class VSProjectConfig; // TODO:C Remove this
    friend class WorkerThread;
    friend class CompilationDatabase;

    void SetName( AString && name, uint32_t nameHashHint = 0 );

    void ReplaceDummyName( const AString & newName );

    virtual void SaveRemote( IOStream & stream ) const;

    uint8_t GetControlFlags() const { return m_ControlFlags; }

    void SetState( State state ) { m_State = state; }

    // each node implements a subset of these as needed
    virtual bool DetermineNeedToBuildStatic() const;
    virtual bool DetermineNeedToBuildDynamic() const;
    virtual bool DoDynamicDependencies( NodeGraph & nodeGraph );
    virtual BuildResult DoBuild( Job * job );
    virtual BuildResult DoBuild2( Job * job, bool racingRemoteJob );
    virtual bool Finalize( NodeGraph & nodeGraph );

    bool DetermineNeedToBuild( const Dependencies & deps ) const;

    void SetLastBuildTime( uint32_t ms );
    void AddProcessingTime( uint32_t ms ) { m_ProcessingTime += ms; }
    void AddCachingTime( uint32_t ms ) { m_CachingTime += ms; }

    static void FixupPathForVSIntegration( AString & line );
    static void FixupPathForVSIntegration_GCC( AString & line, const char * tag );
    static void FixupPathForVSIntegration_SNC( AString & line, const char * tag );
    static void FixupPathForVSIntegration_VBCC( AString & line, const char * tag );
    static void CleanPathForVSIntegration( const AString & path, AString & outFixedPath );

    static void Serialize( IOStream & stream, const void * base, const ReflectionInfo & ri );
    static void Serialize( IOStream & stream, const void * base, const ReflectedProperty & property );
    static void Deserialize( NodeGraph & nodeGraph,
                             ConstMemoryStream & stream,
                             void * base,
                             const ReflectionInfo & ri );
    static void Deserialize( NodeGraph & nodeGraph,
                             ConstMemoryStream & stream,
                             void * base,
                             const ReflectedProperty & property );

    virtual void Migrate( const Node & oldNode );

    bool InitializePreBuildDependencies( NodeGraph & nodeGraph,
                                         const BFFToken * iter,
                                         const Function * function,
                                         const Array<AString> & preBuildDependencyNames );
    bool InitializeConcurrencyGroup( NodeGraph & nodeGraph,
                                     const BFFToken * iter,
                                     const Function * function,
                                     const AString & concurrencyGroupName );

    static const char * GetEnvironmentString( const Array<AString> & envVars,
                                              const char *& inoutCachedEnvString );

    void RecordStampFromBuiltFile();

    // Members are ordered to minimize wasted bytes due to padding.
    // Most frequently accessed members are favored for placement in the first cache line.
    AString m_Name; // Full name. **Set by constructor**
    State m_State = NOT_PROCESSED; // State in the current build
    Type m_Type; // Node type. **Set by constructor**
    mutable uint16_t m_StatsFlags = 0; // Stats recorded in the current build
    mutable uint32_t m_BuildPassTag = 0; // Prevent multiple recursions into the same node during a single sweep
    uint64_t m_Stamp = 0; // "Stamp" representing this node for dependency comparisons
    uint8_t m_ControlFlags = FLAG_NONE; // Control build behavior special cases - Set by constructor
    bool m_Hidden = false; // Hidden from -showtargets?
    uint8_t m_ConcurrencyGroupIndex = 0; // Concurrency group, or 0 if not set
    // Note: Unused 1 byte here
    uint32_t m_RecursiveCost = 0; // Recursive cost used during task ordering
    Node * m_Next = nullptr; // Node map in-place linked list pointer
    uint32_t m_NameHash; // Hash of mName
    uint32_t m_LastBuildTimeMs = 0; // Time it took to do last known full build of this node
    uint32_t m_ProcessingTime = 0; // Time spent on this node during this build
    uint32_t m_CachingTime = 0; // Time spent caching this node
    mutable uint32_t m_ProgressAccumulator = 0; // Used to estimate build progress percentage

    Dependencies m_PreBuildDependencies;
    Dependencies m_StaticDependencies;
    Dependencies m_DynamicDependencies;

    // Static Data
    static const char * const s_NodeTypeNames[];
};

//------------------------------------------------------------------------------
template <class T>
inline T * Node::CastTo() const
{
    ASSERT( T::GetTypeS() == GetType() );
    return (T *)this;
}

//------------------------------------------------------------------------------
template <>
inline FileNode * Node::CastTo<FileNode>() const
{
    ASSERT( IsAFile() );
    return (FileNode *)this;
}

// Custom MetaData
//------------------------------------------------------------------------------
IMetaData & MetaName( const char * name );
IMetaData & MetaAllowNonFile();
IMetaData & MetaAllowNonFile( const Node::Type limitToType );
IMetaData & MetaEmbedMembers();
IMetaData & MetaInheritFromOwner();
IMetaData & MetaIgnoreForComparison();

//------------------------------------------------------------------------------
