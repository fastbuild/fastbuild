// NodeGraph.h - interface to the dependency graph
//------------------------------------------------------------------------------
#pragma once

// Includes
//------------------------------------------------------------------------------
// FBuildCore
#include "Tools/FBuild/FBuildCore/BFF/BFFFileExists.h"
#include "Tools/FBuild/FBuildCore/Graph/Node.h"
#include "Tools/FBuild/FBuildCore/Helpers/SLNGenerator.h"
#include "Tools/FBuild/FBuildCore/Helpers/VSProjectGenerator.h"

// Core
#include "Core/Containers/Array.h"
#include "Core/Strings/AString.h"
#include "Core/Time/Timer.h"

// Forward Declaration
//------------------------------------------------------------------------------
class AliasNode;
class AString;
class ChainedMemoryStream;
class CompilerNode;
class ConstMemoryStream;
class CopyDirNode;
class CopyFileNode;
class CSNode;
class Dependencies;
class DirectoryListNode;
class DLLNode;
class ExeNode;
class ExecNode;
class FileNode;
class IOStream;
class LibraryNode;
class LinkerNode;
class ListDependenciesNode;
class Node;
class ObjectListNode;
class ObjectNode;
class ReflectionInfo;
class ReflectedProperty;
class RemoveDirNode;
class SettingsNode;
class SLNNode;
class TestNode;
class TextFileNode;
class UnityNode;
class VCXProjectNode;
class VSProjectExternalNode;
class XCodeProjectNode;

// NodeGraphHeader
//------------------------------------------------------------------------------
class NodeGraphHeader
{
public:
    explicit NodeGraphHeader()
    {
        m_Identifier[ 0 ] = 'N';
        m_Identifier[ 1 ] = 'G';
        m_Identifier[ 2 ] = 'D';
        m_Version = kCurrentVersion;
        m_Padding = 0;
        m_ContentHash = 0;
    }
    ~NodeGraphHeader() = default;

    inline static const uint8_t kCurrentVersion = 182;

    bool IsValid() const;
    bool IsCompatibleVersion() const { return m_Version == kCurrentVersion; }

    uint64_t GetContentHash() const { return m_ContentHash; }
    void SetContentHash( uint64_t hash ) { m_ContentHash = hash; }

private:
    char m_Identifier[ 3 ];
    uint8_t m_Version;
    uint32_t m_Padding; // Unused
    uint64_t m_ContentHash; // Hash of data excluding this header
};

// NodeGraph
//------------------------------------------------------------------------------
class NodeGraph
{
public:
    explicit NodeGraph( unsigned nodeMapHashBits = 16 );
    ~NodeGraph();

    static NodeGraph * Initialize( const char * bffFile, const char * nodeGraphDBFile, bool forceMigration );

    enum class LoadResult
    {
        MISSING_OR_INCOMPATIBLE,
        LOAD_ERROR,
        LOAD_ERROR_MOVED,
        OK_BFF_NEEDS_REPARSING,
        OK
    };
    NodeGraph::LoadResult Load( const char * nodeGraphDBFile );

    LoadResult Load( ConstMemoryStream & stream, const char * nodeGraphDBFile );
    void Save( ChainedMemoryStream & stream, const char * nodeGraphDBFile ) const;
    void SerializeToText( const Dependencies & dependencies, AString & outBuffer ) const;
    void SerializeToDotFormat( const Dependencies & deps, const bool fullGraph, AString & outBuffer ) const;

    // access existing nodes
    Node * FindNode( const AString & nodeName ) const;
    Node * FindNodeExact( const AString & nodeName ) const;
    Node * GetNodeByIndex( size_t index ) const;
    size_t GetNodeCount() const;

    void SetSettings( const SettingsNode & settings );
    const SettingsNode * GetSettings() const { return m_Settings; }

    void RegisterNode( Node * n, const BFFToken * sourceToken );

    // create new nodes
    Node * CreateNode( Node::Type type,
                       AString && name,
                       uint32_t nameHash );
    Node * CreateNode( Node::Type type,
                       const AString & name,
                       const BFFToken * sourceToken = nullptr );
    template <class T>
    T * CreateNode( const AString & name,
                    const BFFToken * sourceToken = nullptr )
    {
        return CreateNode( T::GetTypeS(), name, sourceToken )->template CastTo<T>();
    }

    void DoBuildPass( Node * nodeToBuild );

    // Non-build operations that use the BuildPassTag can set it to a known value
    void SetBuildPassTagForAllNodes( uint32_t value ) const;

    const BFFToken * FindNodeSourceToken( const Node * node ) const;

    static void CleanPath( AString & name, bool makeFullPath = true );
    static void CleanPath( const AString & name, AString & cleanPath, bool makeFullPath = true );
#if defined( ASSERTS_ENABLED )
    static bool IsCleanPath( const AString & path );
#endif

    static void UpdateBuildStatus( const Node * node,
                                   uint32_t & nodesBuiltTime,
                                   uint32_t & totalNodeTime );

private:
    friend class FBuild;

    bool ParseFromRoot( const char * bffFile );

    void AddNode( Node * node );

    void BuildRecurse( Node * nodeToBuild, uint32_t cost );
    bool CheckDependencies( Node * nodeToBuild, const Dependencies & dependencies, uint32_t cost );
    static void UpdateBuildStatusRecurse( const Node * node,
                                          uint32_t & nodesBuiltTime,
                                          uint32_t & totalNodeTime );
    static void UpdateBuildStatusRecurse( const Dependencies & dependencies,
                                          uint32_t & nodesBuiltTime,
                                          uint32_t & totalNodeTime );

    static bool CheckForCyclicDependencies( const Node * node );
    static bool CheckForCyclicDependenciesRecurse( const Node * node, Array<const Node *> & dependencyStack );
    static bool CheckForCyclicDependenciesRecurse( const Dependencies & dependencies,
                                                   Array<const Node *> & dependencyStack );

    Node * FindNodeInternal( const AString & name, uint32_t nameHashHint ) const;

    struct NodeWithDistance
    {
        NodeWithDistance() = default;
        NodeWithDistance( Node * n, uint32_t dist )
            : m_Node( n )
            , m_Distance( dist )
        {
        }

        Node * m_Node;
        uint32_t m_Distance;
    };
    void FindNearestNodesInternal( const AString & fullPath, Array<NodeWithDistance> & nodes, const uint32_t maxDistance = 5 ) const;

    struct UsedFile;
    bool ReadHeaderAndUsedFiles( ConstMemoryStream & nodeGraphStream,
                                 const char * nodeGraphDBFile,
                                 Array<UsedFile> & files,
                                 bool & compatibleDB,
                                 bool & movedDB ) const;
    uint32_t GetLibEnvVarHash() const;

    void RegisterSourceToken( const Node * node, const BFFToken * sourceToken );

    // load/save helpers
    static void SerializeToText( Node * node, uint32_t depth, AString & outBuffer );
    static void SerializeToText( const char * title, const Dependencies & dependencies, uint32_t depth, AString & outBuffer );
    static void SerializeToDot( Node * node,
                                const bool fullGraph,
                                AString & outBuffer );
    static void SerializeToDot( const char * dependencyType,
                                const char * style,
                                const Node * node,
                                const Dependencies & dependencies,
                                const bool fullGraph,
                                AString & outBuffer );
    static void SerializeToDot( const Dependencies & dependencies,
                                const bool fullGraph,
                                AString & outBuffer );

    // DB Migration
    void Migrate( const NodeGraph & oldNodeGraph );
    void MigrateNode( const NodeGraph & oldNodeGraph, Node & newNode, const Node * oldNode );
    void MigrateProperties( const void * oldBase, void * newBase, const ReflectionInfo * ri );
    void MigrateProperty( const void * oldBase, void * newBase, const ReflectedProperty & property );
    static bool AreNodesTheSame( const void * baseA, const void * baseB, const ReflectionInfo * ri );
    static bool AreNodesTheSame( const void * baseA, const void * baseB, const ReflectedProperty & property );
    static bool DoDependenciesMatch( const Dependencies & depsA, const Dependencies & depsB );

    Node ** m_NodeMap;
    uint32_t m_NodeMapMaxKey; // Always equals to some power of 2 minus 1, can be used as mask.
    Array<Node *> m_AllNodes;

    Timer m_Timer;

    // each file used in the generation of the node graph is tracked
    struct UsedFile
    {
        explicit UsedFile( const AString & fileName, uint64_t timeStamp, uint64_t dataHash )
            : m_FileName( fileName )
            , m_TimeStamp( timeStamp )
            , m_DataHash( dataHash )
        {
        }

        AString m_FileName;
        uint64_t m_TimeStamp;
        uint64_t m_DataHash;
    };
    Array<UsedFile> m_UsedFiles;

    Array<const BFFToken *> m_NodeSourceTokens;

    const SettingsNode * m_Settings;

    static uint32_t s_BuildPassTag;
};

//------------------------------------------------------------------------------
