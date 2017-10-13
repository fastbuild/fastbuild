// NodeGraph.h - interface to the dependency graph
//------------------------------------------------------------------------------
#pragma once

// Includes
//------------------------------------------------------------------------------
#include "Tools/FBuild/FBuildCore/Helpers/SLNGenerator.h"
#include "Tools/FBuild/FBuildCore/Helpers/VSProjectGenerator.h"

#include "Core/Containers/Array.h"
#include "Core/Strings/AString.h"
#include "Core/Time/Timer.h"

// Forward Declaration
//------------------------------------------------------------------------------
class AliasNode;
class AString;
class CompilerNode;
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
class Node;
class ObjectListNode;
class ObjectNode;
class RemoveDirNode;
class SettingsNode;
class SLNNode;
class TestNode;
class UnityNode;
class VCXProjectNode;
class XCodeProjectNode;

// NodeGraphHeader
//------------------------------------------------------------------------------
class NodeGraphHeader
{
public:
    inline explicit NodeGraphHeader()
    {
        m_Identifier[ 0 ] = 'N';
        m_Identifier[ 1 ] = 'G';
        m_Identifier[ 2 ] = 'D';
        m_Version = NODE_GRAPH_CURRENT_VERSION;
    }
    inline ~NodeGraphHeader() = default;

    enum { NODE_GRAPH_CURRENT_VERSION = 105 };

    bool IsValid() const
    {
        return ( ( m_Identifier[ 0 ] == 'N' ) &&
                 ( m_Identifier[ 1 ] == 'G' ) &&
                 ( m_Identifier[ 2 ] == 'D' ) );
    }
    bool IsCompatibleVersion() const { return m_Version == NODE_GRAPH_CURRENT_VERSION; }
private:
    char        m_Identifier[ 3 ];
    uint8_t     m_Version;
};

// NodeGraph
//------------------------------------------------------------------------------
class NodeGraph
{
public:
    explicit NodeGraph();
    ~NodeGraph();

    static NodeGraph * Initialize( const char * bffFile, const char * nodeGraphDBFile );

    enum class LoadResult
    {
        MISSING,
        LOAD_ERROR,
        OK_BFF_CHANGED,
        OK
    };
    NodeGraph::LoadResult Load( const char * nodeGraphDBFile );

    LoadResult Load( IOStream & stream, const char * nodeGraphDBFile );
    void Save( IOStream & stream, const char * nodeGraphDBFile ) const;
    void Display( const Dependencies & dependencies ) const;

    // access existing nodes
    Node * FindNode( const AString & nodeName ) const;
    Node * GetNodeByIndex( size_t index ) const;
    size_t GetNodeCount() const;

    // create new nodes
    CopyFileNode * CreateCopyFileNode( const AString & dstFileName );
    CopyDirNode * CreateCopyDirNode( const AString & nodeName );
    RemoveDirNode * CreateRemoveDirNode( const AString & nodeName );
    ExecNode * CreateExecNode( const AString & dstFileName );
    FileNode * CreateFileNode( const AString & fileName, bool cleanPath = true );
    DirectoryListNode * CreateDirectoryListNode( const AString & name );
    LibraryNode *   CreateLibraryNode( const AString & libraryName );
    ObjectNode *    CreateObjectNode( const AString & objectName );
    AliasNode *     CreateAliasNode( const AString & aliasName );
    DLLNode *       CreateDLLNode( const AString & dllName );
    ExeNode *       CreateExeNode( const AString & exeName );
    UnityNode * CreateUnityNode( const AString & unityName );
    CSNode * CreateCSNode( const AString & csAssemblyName );
    TestNode * CreateTestNode( const AString & testOutput );
    CompilerNode * CreateCompilerNode( const AString & executable );
    VCXProjectNode * CreateVCXProjectNode( const AString & projectOutput,
                                           const Array< AString > & projectBasePaths,
                                           const Dependencies & paths,
                                           const Array< AString > & pathsToExclude,
                                           const Array< AString > & files,
                                           const Array< AString > & filesToExclude,
                                           const Array< AString > & patternToExclude,
                                           const AString & rootNamespace,
                                           const AString & projectGuid,
                                           const AString & defaultLanguage,
                                           const AString & applicationEnvironment,
                                           const bool projectSccEntrySAK,
                                           const Array< VSProjectConfig > & configs,
                                           const Array< VSProjectFileType > & fileTypes,
                                           const Array< AString > & references,
                                           const Array< AString > & projectReferences );
    SLNNode * CreateSLNNode(    const AString & solutionOutput,
                                const AString & solutionBuildProject,
                                const AString & solutionVisualStudioVersion,
                                const AString & solutionMinimumVisualStudioVersion,
                                const Array< VSProjectConfig > & configs,
                                const Array< VCXProjectNode * > & projects,
                                const Array< SLNDependency > & slnDeps,
                                const Array< SLNSolutionFolder > & folders );
    ObjectListNode * CreateObjectListNode( const AString & listName );
    XCodeProjectNode * CreateXCodeProjectNode( const AString & name );
    SettingsNode * CreateSettingsNode( const AString & name );

    void DoBuildPass( Node * nodeToBuild );

    static void CleanPath( AString & name, bool makeFullPath = true );
    static void CleanPath(const AString & name, AString & cleanPath, bool makeFullPath = true );
    #if defined( ASSERTS_ENABLED )
        static bool IsCleanPath( const AString & path );
    #endif

    // as BFF files are encountered during parsing, we track them
    void AddUsedFile( const AString & fileName, uint64_t timeStamp, uint64_t dataHash );
    bool IsOneUseFile( const AString & fileName ) const;
    void SetCurrentFileAsOneUse();

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

    Node * FindNodeInternal( const AString & fullPath ) const;

    struct NodeWithDistance
    {
        inline NodeWithDistance() = default;
        NodeWithDistance( Node * n, uint32_t dist ) : m_Node( n ), m_Distance( dist ) {}
        Node *      m_Node;
        uint32_t    m_Distance;
    };
    void FindNearestNodesInternal( const AString & fullPath, Array< NodeWithDistance > & nodes, const uint32_t maxDistance = 5 ) const;

    struct UsedFile;
    bool ReadHeaderAndUsedFiles( IOStream & nodeGraphStream, const char* nodeGraphDBFile, Array< UsedFile > & files, bool & compatibleDB ) const;
    uint32_t GetLibEnvVarHash() const;

    // load/save helpers
    static void SaveRecurse( IOStream & stream, Node * node, Array< bool > & savedNodeFlags );
    static void SaveRecurse( IOStream & stream, const Dependencies & dependencies, Array< bool > & savedNodeFlags );
    bool LoadNode( IOStream & stream );
    static void DisplayRecurse( Node * node, Array< bool > & savedNodeFlags, uint32_t depth, AString & outBuffer );
    static void DisplayRecurse( const char * title, const Dependencies & dependencies, Array< bool > & savedNodeFlags, uint32_t depth, AString & outBuffer );

    enum { NODEMAP_TABLE_SIZE = 65536 };
    Node **         m_NodeMap;
    Array< Node * > m_AllNodes;
    uint32_t        m_NextNodeIndex;

    Timer m_Timer;

    // each file used in the generation of the node graph is tracked
    struct UsedFile
    {
        explicit UsedFile( const AString & fileName, uint64_t timeStamp, uint64_t dataHash ) : m_FileName( fileName ), m_TimeStamp( timeStamp ), m_DataHash( dataHash ) , m_Once( false ) {}
        AString     m_FileName;
        uint64_t    m_TimeStamp;
        uint64_t    m_DataHash;
        bool        m_Once;
    };
    Array< UsedFile > m_UsedFiles;

    static uint32_t s_BuildPassTag;
};

//------------------------------------------------------------------------------
