// Node.h - base interface for dependency graph nodes
//------------------------------------------------------------------------------
#pragma once

// Includes
//------------------------------------------------------------------------------
// FBuild
#include "Tools/FBuild/FBuildCore/Graph/Dependencies.h"

// Core
#include "Core/Containers/Array.h"
#include "Core/Reflection/Object.h"
#include "Core/Strings/AString.h"

// Forward Declarations
//------------------------------------------------------------------------------
class BFFIterator;
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

// Load/SaveMacros
//------------------------------------------------------------------------------
#define NODE_SAVE( member ) stream.Write( member );
#define NODE_SAVE_DEPS( depsArray ) depsArray.Save( stream );
#define NODE_SAVE_NODE_LINK( node ) Node::SaveNodeLink( stream, node );

#define NODE_LOAD( type, member ) type member; if ( stream.Read( member ) == false ) { return nullptr; }
#define NODE_LOAD_DEPS( initialCapacity, depsArray ) \
    Dependencies depsArray( initialCapacity, true ); \
    if ( depsArray.Load( nodeGraph, stream ) == false ) { return nullptr; }
#define NODE_LOAD_NODE_LINK( type, node ) \
    type * node = nullptr; \
    if ( Node::LoadNodeLink( nodeGraph, stream, node ) == false ) { return nullptr; }

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
    enum Type
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
        // Make sure you update 's_NodeTypeNames' in the cpp
        NUM_NODE_TYPES      // leave this last
    };

    enum ControlFlag
    {
        FLAG_NONE                   = 0x00,
        FLAG_TRIVIAL_BUILD          = 0x01, // DoBuild is performed locally in main thread
        FLAG_NO_DELETE_ON_FAIL      = 0x02, // Don't delete output file on failure (for Test etc)
    };

    enum StatsFlag
    {
        STATS_PROCESSED     = 0x01, // node was processed during the build
        STATS_BUILT         = 0x02, // node needed building, and was built
        STATS_CACHE_HIT     = 0x04, // needed building, was cacheable & was retrieved from the cache
        STATS_CACHE_MISS    = 0x08, // needed building, was cacheable, but wasn't in cache
        STATS_CACHE_STORE   = 0x10, // needed building, was cacheable & was stored to the cache
        STATS_BUILT_REMOTE  = 0x20, // node was built remotely
        STATS_FAILED        = 0x40, // node needed building, but failed
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

    enum State
    {
        NOT_PROCESSED,      // no work done (either not part of this build, or waiting on static dependencies )
        PRE_DEPS_READY,     // pre-build deps processed
        STATIC_DEPS_READY,  // static dependencies are uptodate - we are ready to DoDynamicDeps
        DYNAMIC_DEPS_DONE,  // dynamic deps updated, waiting for dynamic deps to be ready
        BUILDING,           // in the queue for building
        FAILED,             // failed to build
        UP_TO_DATE,         // built, or confirmed as not needing building
    };

    explicit Node( const AString & name, Type type, uint32_t controlFlags );
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

    inline uint32_t GetLastBuildTime() const    { return m_LastBuildTimeMs; }
    inline uint32_t GetProcessingTime() const   { return m_ProcessingTime; }
    inline uint32_t GetRecursiveCost() const    { return m_RecursiveCost; }

    inline uint32_t GetProgressAccumulator() const { return m_ProgressAccumulator; }
    inline void     SetProgressAccumulator( uint32_t p ) const { m_ProgressAccumulator = p; }

    static Node *   Load( NodeGraph & nodeGraph, IOStream & stream );
    static void     Save( IOStream & stream, const Node * node );

    static Node *   LoadRemote( IOStream & stream );
    static void     SaveRemote( IOStream & stream, const Node * node );

    void Serialize( IOStream & stream ) const;
    bool Deserialize( NodeGraph & nodeGraph, IOStream & stream );

    static bool EnsurePathExistsForFile( const AString & name );

    inline uint64_t GetStamp() const { return m_Stamp; }

    inline uint32_t GetIndex() const { return m_Index; }

    static void DumpOutput( Job * job,
                            const char * data,
                            uint32_t dataSize,
                            const Array< AString > * exclusions = nullptr );

    inline void     SetBuildPassTag( uint32_t pass ) const { m_BuildPassTag = pass; }
    inline uint32_t GetBuildPassTag() const             { return m_BuildPassTag; }

    const AString & GetName() const { return m_Name; }

    #if defined( DEBUG )
        // Help catch serialization errors
        inline bool IsSaved() const     { return m_IsSaved; }
        inline void MarkAsSaved() const { m_IsSaved = true; }
    #endif

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

    inline const Dependencies & GetPreBuildDependencies() const { return m_PreBuildDependencies; }
    inline const Dependencies & GetStaticDependencies() const { return m_StaticDependencies; }
    inline const Dependencies & GetDynamicDependencies() const { return m_DynamicDependencies; }

    void SetName( const AString & name );

    void ReplaceDummyName( const AString & newName );

    virtual void Save( IOStream & stream ) const = 0;
    virtual void SaveRemote( IOStream & stream ) const;

    inline uint32_t GetControlFlags() const { return m_ControlFlags; }

    inline void SetState( State state ) { m_State = state; }

    inline void SetIndex( uint32_t index ) { m_Index = index; }

    // each node must implement these core functions
    virtual bool DoDynamicDependencies( NodeGraph & nodeGraph, bool forceClean );
    virtual bool DetermineNeedToBuild( bool forceClean ) const;
    virtual BuildResult DoBuild( Job * job );
    virtual BuildResult DoBuild2( Job * job, bool racingRemoteJob );
    virtual bool Finalize( NodeGraph & nodeGraph );

    inline void     SetLastBuildTime( uint32_t ms ) { m_LastBuildTimeMs = ms; }
    inline void     AddProcessingTime( uint32_t ms ){ m_ProcessingTime += ms; }

    static void SaveNodeLink( IOStream & stream, const Node * node );
    static bool LoadNodeLink( NodeGraph & nodeGraph, IOStream & stream, Node * & node );
    static bool LoadNodeLink( NodeGraph & nodeGraph, IOStream & stream, CompilerNode * & compilerNode );
    static bool LoadNodeLink( NodeGraph & nodeGraph, IOStream & stream, FileNode * & node );

    static void FixupPathForVSIntegration( AString & line );
    static void FixupPathForVSIntegration_GCC( AString & line, const char * tag );
    static void FixupPathForVSIntegration_SNC( AString & line, const char * tag );
    static void FixupPathForVSIntegration_VBCC( AString & line, const char * tag );

    static void Serialize( IOStream & stream, const void * base, const ReflectionInfo & ri );
    static void Serialize( IOStream & stream, const void * base, const ReflectedProperty & property );
    static bool Deserialize( IOStream & stream, void * base, const ReflectionInfo & ri );
    static bool Deserialize( IOStream & stream, void * base, const ReflectedProperty & property );

    bool            InitializePreBuildDependencies( NodeGraph & nodeGraph,
                                                    const BFFIterator & iter,
                                                    const Function * function,
                                                    const Array< AString > & preBuildDependencyNames );

    AString m_Name;

    State m_State;
    mutable uint32_t m_BuildPassTag; // prevent multiple recursions into the same node
    uint32_t        m_ControlFlags;
    mutable uint32_t        m_StatsFlags;
    uint64_t        m_Stamp;
    uint32_t        m_RecursiveCost;
    Type m_Type;
    Node *          m_Next; // node map linked list pointer
    uint32_t        m_NameCRC;
    uint32_t m_LastBuildTimeMs; // time it took to do last known full build of this node
    uint32_t m_ProcessingTime;  // time spent on this node
    mutable uint32_t m_ProgressAccumulator;
    uint32_t        m_Index;

    Dependencies m_PreBuildDependencies;
    Dependencies m_StaticDependencies;
    Dependencies m_DynamicDependencies;

    #if defined( DEBUG )
        mutable bool    m_IsSaved = false; // Help catch serialization errors
    #endif

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

//------------------------------------------------------------------------------
