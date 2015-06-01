// NodeGraph.h - interface to the dependency graph
//------------------------------------------------------------------------------
#pragma once
#ifndef FBUILD_GRAPH_NODEGRAPH_H
#define FBUILD_GRAPH_NODEGRAPH_H

// Includes
//------------------------------------------------------------------------------
#include "Tools/FBuild/FBuildCore/Helpers/VSProjectGenerator.h"
#include "Tools/FBuild/FBuildCore/Graph/Node.h" // TODO:C remove when USE_NODE_REFLECTION is removed

#include "Core/Containers/Array.h"
#include "Core/Strings/AString.h"
#include "Core/Time/Timer.h"

// Forward Declaration
//------------------------------------------------------------------------------
class AliasNode;
class AString;
class CompilerNode;
class CopyDirNode;
class CopyNode;
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
class TestNode;
class UnityNode;
class VCXProjectNode;

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
	inline ~NodeGraphHeader() {}

	enum { NODE_GRAPH_CURRENT_VERSION = 56 };

	bool IsValid() const
	{
		return ( ( m_Identifier[ 0 ] == 'N' ) &&
				 ( m_Identifier[ 1 ] == 'G' ) &&
				 ( m_Identifier[ 2 ] == 'D' ) );
	}
	bool IsCompatibleVersion() const { return m_Version == NODE_GRAPH_CURRENT_VERSION; }
private:
	char		m_Identifier[ 3 ];
	uint8_t		m_Version;
};

// NodeGraph
//------------------------------------------------------------------------------
class NodeGraph
{
public:
	explicit NodeGraph();
	~NodeGraph();

	bool Initialize( const char * bffFile, const char * nodeGraphDBFile );

	bool Load( const char * nodeGraphDBFile, bool & needReparsing );
	bool Load( IOStream & stream, bool & needReparsing );
	void Save( IOStream & stream ) const;

	// access existing nodes
	Node * FindNode( const AString & nodeName ) const;
	Node * GetNodeByIndex( uint32_t index ) const;

	// create new nodes
	CopyNode * CreateCopyNode( const AString & dstFileName, 
							   Node * sourceFile,
							   const Dependencies & preBuildDependencies );
	CopyDirNode * CreateCopyDirNode( const AString & nodeName, 
									 Dependencies & staticDeps,
									 const AString & destPath,
									 const Dependencies & preBuildDependencies );
	ExecNode * CreateExecNode( const AString & dstFileName, 
							   FileNode * sourceFile, 
							   FileNode * executable, 
							   const AString & arguments, 
							   const AString & workingDir,
							   int32_t expectedReturnCode,
							   const Dependencies & preBuildDependencies );
	FileNode * CreateFileNode( const AString & fileName, bool cleanPath = true );
	DirectoryListNode * CreateDirectoryListNode( const AString & name,
												 const AString & path,
												 const AString & wildCard,
												 bool recursive,
                                                 const Array< AString > & excludePaths,
                                                 const Array< AString > & filesToExclude
                                                 );
	LibraryNode *	CreateLibraryNode( const AString & libraryName,
									   const Dependencies & inputNodes,
									   CompilerNode * compilerNode,
									   const AString & compilerArgs,
									   const AString & compilerArgsDeoptimized,
									   const AString & compilerOutputPath,
									   const AString & linker,
									   const AString & linkerArgs,
									   uint32_t flags,
									   ObjectNode * precompiledHeader,
									   const Dependencies & compilerForceUsing,
									   const Dependencies & preBuildDependencies,
									   const Dependencies & additionalInputs,
									   bool deoptimizeWritableFiles,
									   bool deoptimizeWritableFilesWithToken,
                                       CompilerNode * preprocessor,
                                       const AString & preprocessorArgs );
	ObjectNode *	CreateObjectNode( const AString & objectName,
									  Node * inputNode,
									  Node * compilerNode,
									  const AString & compilerArgs,
									  const AString & compilerArgsDeoptimized,
									  Node * precompiledHeader,
									  uint32_t flags,
									  const Dependencies & compilerForceUsing,
									  bool deoptimizeWritableFiles,
									  bool deoptimizeWritableFilesWithToken,
                                      Node * preprocessorNode,
                                      const AString & preprocessorArgs,
                                      uint32_t preprocessorFlags );
#ifdef USE_NODE_REFLECTION
	AliasNode *		CreateAliasNode( const AString & aliasName );
#else
	AliasNode *		CreateAliasNode( const AString & aliasName,
									 const Dependencies & targets );
#endif
	DLLNode *		CreateDLLNode( const AString & linkerOutputName,
								   const Dependencies & inputLibraries,
								   const Dependencies & otherLibraries,
								   const AString & linker,
								   const AString & linkerArgs,
								   uint32_t flags,
								   const Dependencies & assemblyResources,
								   const AString & importLibName,
								   Node * linkerStampExe,
								   const AString & linkerStampExeArgs );
	ExeNode *		CreateExeNode( const AString & linkerOutputName,
								   const Dependencies & inputLibraries,
								   const Dependencies & otherLibraries,
								   const AString & linker,
								   const AString & linkerArgs,
								   uint32_t flags,
								   const Dependencies & assemblyResources,
								   Node * linkerStampExe,
								   const AString & linkerStampExeArgs );
#ifdef USE_NODE_REFLECTION
	UnityNode *	CreateUnityNode( const AString & unityName );
#else
	UnityNode *	CreateUnityNode( const AString & unityName,
								 const Dependencies & dirNodes,
								 const Array< AString > & files,
								 const AString & outputPath,
								 const AString & outputPattern,
								 uint32_t numUnityFilesToCreate,
								 const AString & precompiledHeader,
								 bool isolateWritableFiles,
								 uint32_t maxIsolatedFiles,
								 const Array< AString > & excludePatterns );
#endif
	CSNode * CreateCSNode( const AString & compilerOutput,
						   const Dependencies & inputNodes,
						   const AString & compiler,
						   const AString & compilerOptions,
						   const Dependencies & extraRefs );
	TestNode * CreateTestNode( const AString & testOutput,
							   FileNode * testExecutable,
							   const AString & arguments,
							   const AString & workingDir );
#ifdef USE_NODE_REFLECTION
	CompilerNode * CreateCompilerNode( const AString & executable );
#else
	CompilerNode * CreateCompilerNode( const AString & executable,
									   const Dependencies & extraFiles,
									   bool allowDistribution );
#endif
	VCXProjectNode * CreateVCXProjectNode( const AString & projectOutput,
										   const Array< AString > & projectBasePaths,
										   const Dependencies & paths,
										   const Array< AString > & pathsToExclude,
										   const Array< AString > & allowedFileExtensions,
										   const Array< AString > & files,
										   const Array< AString > & filesToExclude,
										   const AString & rootNamespace,
										   const AString & projectGuid,
										   const AString & defaultLanguage,
										   const AString & applicationEnvironment,
										   const Array< VSProjectConfig > & configs,
										   const Array< VSProjectFileType > & fileTypes,
										   const Array< AString > & references,
										   const Array< AString > & projectReferences );
	ObjectListNode * CreateObjectListNode( const AString & listName,
							 const Dependencies & inputNodes,
							 CompilerNode * compiler,
							 const AString & compilerArgs,
							 const AString & compilerArgsDeoptimized,
							 const AString & compilerOutputPath,
							 ObjectNode * precompiledHeader,
							 const Dependencies & compilerForceUsing,
							 const Dependencies & preBuildDependencies,
							 bool deoptimizeWritableFiles,
							 bool deoptimizeWritableFilesWithToken,
                             CompilerNode * preprocessor,
                             const AString & preprocessorArgs );

	void DoBuildPass( Node * nodeToBuild );

	static void CleanPath( const AString & name, AString & fullPath );

	// as BFF files are encountered during parsing, we track them
	void AddUsedFile( const AString & fileName, uint64_t timeStamp );
	bool IsOneUseFile( const AString & fileName ) const;
	void SetCurrentFileAsOneUse();

	static void UpdateBuildStatus( const Node * node, 
								   uint32_t & nodesBuiltTime, 
								   uint32_t & totalNodeTime );
private:
	friend class FBuild;

	void AddNode( Node * node );

	static void BuildRecurse( Node * nodeToBuild );
	static bool CheckDependencies( Node * nodeToBuild, const Dependencies & dependencies );
	static void UpdateBuildStatusRecurse( const Node * node, 
										  uint32_t & nodesBuiltTime, 
										  uint32_t & totalNodeTime );
	static void UpdateBuildStatusRecurse( const Dependencies & dependencies, 
										  uint32_t & nodesBuiltTime, 
										  uint32_t & totalNodeTime );

	Node * FindNodeInternal( const AString & fullPath ) const;

	struct UsedFile;
	bool ReadHeaderAndUsedFiles( IOStream & nodeGraphStream, Array< UsedFile > & files, bool & compatibleDB ) const;
	uint32_t GetLibEnvVarHash() const;

	// load/save helpers
	static void SaveRecurse( IOStream & stream, Node * node, Array< bool > & savedNodeFlags );
	static void SaveRecurse( IOStream & stream, const Dependencies & dependencies, Array< bool > & savedNodeFlags );
	bool LoadNode( IOStream & stream );

	enum { NODEMAP_TABLE_SIZE = 65536 };
	Node *			m_NodeMap[ NODEMAP_TABLE_SIZE ];
	Array< Node * > m_AllNodes;
	uint32_t		m_NextNodeIndex;

	Timer m_Timer;

	// each file used in the generation of the node graph is tracked
	struct UsedFile
	{
		explicit UsedFile( const AString & fileName, uint64_t timeStamp ) : m_FileName( fileName ), m_TimeStamp( timeStamp ), m_Once( false ) {}
		AString		m_FileName;
		uint64_t	m_TimeStamp;
		bool		m_Once;
	};
	Array< UsedFile > m_UsedFiles;

	static uint32_t s_BuildPassTag;
};

//------------------------------------------------------------------------------
#endif // FBUILD_GRAPH_NODEGRAPH_H
