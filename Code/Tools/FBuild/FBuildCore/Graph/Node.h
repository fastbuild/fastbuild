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
class FileNode;
class Function;
class IMetaData;
class IOStream;
class Job;
class NodeGraph;

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
        PROXY_NODE          = 0,
        COPY_FILE_NODE      = 1,
        DIRECTORY_LIST_NODE = 2,
        EXEC_NODE           = 3,
        FILE_NODE           = 4,
        LIBRARY_NODE        = 5,
        OBJECT_NODE         = 6,
        ALIAS_NODE          = 7,
        EXE_NODE            = 8,
        UNITY_NODE          = 9,
        CS_NODE             = 10,
        TEST_NODE           = 11,
        COMPILER_NODE       = 12,
        DLL_NODE            = 13,
        VCXPROJECT_NODE     = 14,
        OBJECT_LIST_NODE    = 15,
        COPY_DIR_NODE       = 16,
        SLN_NODE            = 17,
        REMOVE_DIR_NODE     = 18,
        XCODEPROJECT_NODE   = 19,
        SETTINGS_NODE       = 20,
        VSPROJEXTERNAL_NODE = 21,
        TEXT_FILE_NODE      = 22,
        LIST_DEPENDENCIES_NODE = 23,
        // Make sure you update 's_NodeTypeNames' in the cpp
        NUM_NODE_TYPES      // leave this last
    };

    enum ControlFlag : uint8_t
    {
        FLAG_NONE                   = 0x00,
        FLAG_ALWAYS_BUILD           = 0x01, // DoBuild is always performed (for e.g. directory listings)
    };

    enum StatsFlag : uint16_t
    {
        STATS_PROCESSED     = 0x01, // node was processed during the build
        STATS_BUILT         = 0x02, // node needed building, and was built
        STATS_CACHE_HIT     = 0x04, // needed building, was cacheable & was retrieved from the cache
        STATS_CACHE_MISS    = 0x08, // needed building, was cacheable, but wasn't in cache
        STATS_CACHE_STORE   = 0x10, // needed building, was cacheable & was stored to the cache
        STATS_LIGHT_CACHE   = 0x20, // used the LightCache
        STATS_BUILT_REMOTE  = 0x40, // node was built remotely
        STATS_FAILED        = 0x80, // node needed building, but failed
        STATS_FIRST_BUILD   = 0x100,// node has never been built before
        STATS_REPORT_PROCESSED  = 0x4000, // seen during report processing
        STATS_STATS_PROCESSED   = 0x8000 // mark during stats gathering (leave this last)
    };

    enum BuildResult
    {
        NODE_RESULT_FAILED      = 0,    // something went wrong building
        NODE_RESULT_NEED_SECOND_BUILD_PASS, // needs build called again
        NODE_RESULT_OK,                 // built ok
        NODE_RESULT_OK_CACHE            // retrieved from the cache
    };

    enum State : uint8_t
    {
        NOT_PROCESSED,      // no work done (either not part of this build, or waiting on static dependencies )
        PRE_DEPS_READY,     // pre-build deps processed
        STATIC_DEPS_READY,  // static dependencies are uptodate - we are ready to DoDynamicDeps
        DYNAMIC_DEPS_DONE,  // dynamic deps updated, waiting for dynamic deps to be ready
        BUILDING,           // in the queue for building
        FAILED,             // failed to build
        UP_TO_DATE,         // built, or confirmed as not needing building
    };

    explicit Node( const AString & name, Type type, uint8_t controlFlags );
    virtual bool Initialize( NodeGraph & nodeGraph, const BFFToken * funcStartIter, const Function * function ) = 0;
    virtual ~Node();

    inline uint32_t        GetNameCRC() const { return m_NameCRC; }
    inline Type GetType() const { return m_Type; }
    inline const char * GetTypeName() const { return s_NodeTypeNames[ m_Type ]; }
    inline static const char * GetTypeName( Type t ) { return s_NodeTypeNames[ t ]; }
    template < class T >
    inline T * CastTo() const;

    // each node must specify if it outputs a file
    virtual bool IsAFile() const = 0;

    inline State GetState() const { return m_State; }

    inline bool GetStatFlag( StatsFlag flag ) const { return ( ( m_StatsFlags & flag ) != 0 ); }
    inline void SetStatFlag( StatsFlag flag ) const { m_StatsFlags |= flag; }

    uint32_t GetLastBuildTime() const;
    inline uint32_t GetProcessingTime() const   { return m_ProcessingTime; }
    inline uint32_t GetCachingTime() const      { return m_CachingTime; }
    inline uint32_t GetRecursiveCost() const    { return m_RecursiveCost; }

    inline uint32_t GetProgressAccumulator() const { return m_ProgressAccumulator; }
    inline void     SetProgressAccumulator( uint32_t p ) const { m_ProgressAccumulator = p; }

    static Node *   CreateNode( NodeGraph & nodeGraph, Node::Type nodeType, const AString & name );
    static Node *   Load( NodeGraph & nodeGraph, IOStream & stream );
    static void     Save( IOStream & stream, const Node * node );
    virtual void    PostLoad( NodeGraph & nodeGraph ); // TODO:C Eliminate the need for this function

    static Node *   LoadRemote( IOStream & stream );
    static void     SaveRemote( IOStream & stream, const Node * node );

    static bool EnsurePathExistsForFile( const AString & name );
    static bool DoPreBuildFileDeletion( const AString & fileName );

    inline uint64_t GetStamp() const { return m_Stamp; }

    inline uint32_t GetIndex() const { return m_Index; }

    static void DumpOutput( Job * job,
                            const AString & output,
                            const Array< AString > * exclusions = nullptr );

    inline void     SetBuildPassTag( uint32_t pass ) const { m_BuildPassTag = pass; }
    inline uint32_t GetBuildPassTag() const             { return m_BuildPassTag; }

    const AString & GetName() const { return m_Name; }

    virtual const AString & GetPrettyName() const { return GetName(); }

    bool IsHidden() const { return m_Hidden; }

    #if defined( DEBUG )
        // Help catch serialization errors
        inline bool IsSaved() const     { return m_IsSaved; }
        inline void MarkAsSaved() const { m_IsSaved = true; }
    #endif

    inline const Dependencies & GetPreBuildDependencies() const { return m_PreBuildDependencies; }
    inline const Dependencies & GetStaticDependencies() const { return m_StaticDependencies; }
    inline const Dependencies & GetDynamicDependencies() const { return m_DynamicDependencies; }

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

    void SetName( const AString & name );

    void ReplaceDummyName( const AString & newName );

    virtual void SaveRemote( IOStream & stream ) const;

    inline uint8_t GetControlFlags() const { return m_ControlFlags; }

    inline void SetState( State state ) { m_State = state; }

    inline void SetIndex( uint32_t index ) { m_Index = index; }

    // each node must implement these core functions
    virtual bool DoDynamicDependencies( NodeGraph & nodeGraph, bool forceClean );
    virtual bool DetermineNeedToBuild( const Dependencies & deps ) const;
    virtual BuildResult DoBuild( Job * job );
    virtual BuildResult DoBuild2( Job * job, bool racingRemoteJob );
    virtual bool Finalize( NodeGraph & nodeGraph );

    void SetLastBuildTime( uint32_t ms );
    inline void     AddProcessingTime( uint32_t ms )  { m_ProcessingTime += ms; }
    inline void     AddCachingTime( uint32_t ms )     { m_CachingTime += ms; }

    static void FixupPathForVSIntegration( AString & line );
    static void FixupPathForVSIntegration_GCC( AString & line, const char * tag );
    static void FixupPathForVSIntegration_SNC( AString & line, const char * tag );
    static void FixupPathForVSIntegration_VBCC( AString & line, const char * tag );

    static void Serialize( IOStream & stream, const void * base, const ReflectionInfo & ri );
    static void Serialize( IOStream & stream, const void * base, const ReflectedProperty & property );
    static bool Deserialize( IOStream & stream, void * base, const ReflectionInfo & ri );
    static bool Deserialize( IOStream & stream, void * base, const ReflectedProperty & property );

    virtual void Migrate( const Node & oldNode );

    bool            InitializePreBuildDependencies( NodeGraph & nodeGraph,
                                                    const BFFToken * iter,
                                                    const Function * function,
                                                    const Array< AString > & preBuildDependencyNames );

    static const char * GetEnvironmentString( const Array< AString > & envVars,
                                              const char * & inoutCachedEnvString );

    void RecordStampFromBuiltFile();

    // Members are ordered to minimize wasted bytes due to padding.
    // Most frequently accessed members are favored for placement in the first cache line.
    AString             m_Name;                     // Full name. **Set by constructor**
    State               m_State = NOT_PROCESSED;    // State in the current build
    Type                m_Type;                     // Node type. **Set by constructor**
    mutable uint16_t    m_StatsFlags = 0;           // Stats recorded in the current build
    mutable uint32_t    m_BuildPassTag = 0;         // Prevent multiple recursions into the same node during a single sweep
    uint64_t            m_Stamp = 0;                // "Stamp" representing this node for dependency comparissons
    uint8_t             m_ControlFlags;             // Control build behavior special cases - Set by constructor
    bool                m_Hidden = false;           // Hidden from -showtargets?
    #if defined( DEBUG )
        mutable bool    m_IsSaved = false;          // Help catch serialization errors
    #endif
    // Note: Unused byte here
    uint32_t            m_RecursiveCost = 0;        // Recursive cost used during task ordering
    Node *              m_Next = nullptr;           // Node map in-place linked list pointer
    uint32_t            m_NameCRC;                  // Hash of mName. **Set by constructor**
    uint32_t            m_LastBuildTimeMs = 0;      // Time it took to do last known full build of this node
    uint32_t            m_ProcessingTime = 0;       // Time spent on this node during this build
    uint32_t            m_CachingTime = 0;          // Time spent caching this node
    mutable uint32_t    m_ProgressAccumulator = 0;  // Used to estimate build progress percentage
    uint32_t            m_Index = INVALID_NODE_INDEX;   // Index into flat array of all nodes

    Dependencies        m_PreBuildDependencies;
    Dependencies        m_StaticDependencies;
    Dependencies        m_DynamicDependencies;

    // Static Data
    static const char * const s_NodeTypeNames[];
};

//------------------------------------------------------------------------------
template < class T >
inline T * Node::CastTo() const
{
    ASSERT( T::GetTypeS() == GetType() );
    return (T *)this;
}

//------------------------------------------------------------------------------
template <>
inline FileNode * Node::CastTo< FileNode >() const
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
