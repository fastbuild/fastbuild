// ObjectNode.h - a node that builds a single object from a source file
//------------------------------------------------------------------------------
#pragma once
#ifndef FBUILD_GRAPH_OBJECTNODE_H
#define FBUILD_GRAPH_OBJECTNODE_H

// Includes
//------------------------------------------------------------------------------
#include "FileNode.h"
#include "Core/Containers/Array.h"
#include "Core/Containers/AutoPtr.h"
#include "Core/Env/Assert.h"
#include "Core/Process/Process.h"

// Forward Declarations
//------------------------------------------------------------------------------
class NodeProxy;

// ObjectNode
//------------------------------------------------------------------------------
class ObjectNode : public FileNode
{
public:
	explicit ObjectNode( const AString & objectName,
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
                         uint32_t preprocessorFlags);
	// simplified remote constructor
	explicit ObjectNode( const AString & objectName,
						 NodeProxy * srcFile,
						 const AString & compilerArgs,
						 uint32_t flags );
	virtual ~ObjectNode();

	static inline Node::Type GetTypeS() { return Node::OBJECT_NODE; }


	enum Flags
	{
		FLAG_CAN_BE_CACHED		=	0x01,
		FLAG_CAN_BE_DISTRIBUTED	=	0x02,
		FLAG_USING_PCH			=	0x04,
		FLAG_GCC				=	0x10,
		FLAG_MSVC				=	0x20,
		FLAG_CREATING_PCH		=   0x40,
		FLAG_SNC				=	0x80,
		FLAG_USING_CLR			=   0x100,
		FLAG_CLANG				=	0x200,
		FLAG_UNITY				=   0x400,
		FLAG_ISOLATED_FROM_UNITY=	0x800,
		FLAG_USING_PDB			=   0x1000,
		CODEWARRIOR_WII			=	0x2000,
		GREENHILLS_WIIU			=	0x4000,
		FLAG_CUDA_NVCC			=   0x10000,
	};
	static uint32_t DetermineFlags( const Node * compilerNode, const AString & args );

	inline bool IsCreatingPCH() const { return GetFlag( FLAG_CREATING_PCH ); }
    inline bool IsUsingPCH() const { return GetFlag( FLAG_USING_PCH ); }
	inline bool IsMSVC() const { return GetFlag( FLAG_MSVC ); }
	inline bool IsUsingPDB() const { return GetFlag( FLAG_USING_PDB ); }

	virtual void Save( IOStream & stream ) const override;
	static Node * Load( IOStream & stream );

	virtual void SaveRemote( IOStream & stream ) const override;
	static Node * LoadRemote( IOStream & stream );

	inline Node * GetCompiler() const { return m_StaticDependencies[ 0 ].GetNode(); }
	inline Node * GetSourceFile() const { return m_StaticDependencies[ 1 ].GetNode(); }
    inline Node * GetDedicatedPreprocessor() const { return m_PreprocessorNode; }
	inline Node * GetPrecompiledHeaderCPPFile() const { ASSERT( GetFlag( FLAG_CREATING_PCH ) ); return m_StaticDependencies[ 1 ].GetNode(); }

	void GetPDBName( AString & pdbName ) const;

	const char * GetObjExtension() const;
private:
	virtual BuildResult DoBuild( Job * job ) override;
	virtual BuildResult DoBuild2( Job * job, bool racingRemoteJob ) override;
	virtual bool Finalize() override;

	BuildResult DoBuildMSCL_NoCache( Job * job, bool useDeoptimization );
	BuildResult DoBuildWithPreProcessor( Job * job, bool useDeoptimization, bool useCache );
	BuildResult DoBuildWithPreProcessor2( Job * job, bool useDeoptimization, bool stealingRemoteJob, bool racingRemoteJob );
	BuildResult DoBuildOther( Job * job, bool useDeoptimization );

	bool ProcessIncludesMSCL( const char * output, uint32_t outputSize );
	bool ProcessIncludesWithPreProcessor( Job * job );

	const AString & GetCacheName( Job * job ) const;
	bool RetrieveFromCache( Job * job );
	void WriteToCache( Job * job );

	static void DumpOutput( Job * job, const char * data, uint32_t dataSize, const AString & name, bool treatAsWarnings = false );

	void EmitCompilationMessage( const AString & fullArgs, bool useDeoptimization, bool stealingRemoteJob = false, bool racingRemoteJob = false, bool useDedicatedPreprocessor = false ) const;

	enum Pass
	{
		PASS_PREPROCESSOR_ONLY,
		PASS_COMPILE_PREPROCESSED,
		PASS_COMPILE
	};
	static bool StripTokenWithArg( const char * tokenToCheckFor, const AString & token, size_t & index );
	static bool StripToken( const char * tokenToCheckFor, const AString & token, bool allowStartsWith = false );
	void BuildFullArgs( const Job * job, AString & fullArgs, Pass pass, bool useDeoptimization ) const;

	void ExpandTokenList( const Dependencies & nodes, AString & fullArgs, const AString & pre, const AString & post ) const;
	bool BuildPreprocessedOutput( const AString & fullArgs, Job * job, bool useDeoptimization ) const;
	bool WriteTmpFile( Job * job, AString & tmpFileName ) const;
	bool BuildFinalOutput( Job * job, const AString & fullArgs ) const;

	inline bool GetFlag( uint32_t flag ) const { return ( ( m_Flags & flag ) != 0 ); }
	inline bool GetPreprocessorFlag( uint32_t flag ) const { return ( ( m_PreprocessorFlags & flag ) != 0 ); }

	static void HandleSystemFailures( Job * job, int result, const char * stdOut, const char * stdErr );
	bool ShouldUseDeoptimization() const;
	friend class Client;
	bool ShouldUseCache() const;

	class CompileHelper
	{
	public:
		CompileHelper( bool handleOutput = true );
		~CompileHelper();

		// start compilation
		bool SpawnCompiler( Job * job, const AString & name, const AString & compiler, const AString & fullArgs, bool useResponseFile, const char * workingDir = nullptr );

		// determine overall result
		inline int						GetResult() const { return m_Result; }

		// access output/error
		inline const AutoPtr< char > &	GetOut() const { return m_Out; }
		inline uint32_t					GetOutSize() const { return m_OutSize; }
		inline const AutoPtr< char > &	GetErr() const { return m_Err; }
		inline uint32_t					GetErrSize() const { return m_ErrSize; }

	private:
		bool			m_HandleOutput;
		Process			m_Process;
		AutoPtr< char > m_Out;
		uint32_t		m_OutSize;
		AutoPtr< char > m_Err;
		uint32_t		m_ErrSize;
		int				m_Result;
	};

	Array< AString > m_Includes;
	uint32_t m_Flags;
	AString m_CompilerArgs;
	AString m_CompilerArgsDeoptimized;
	AString m_ObjExtensionOverride;
	Dependencies m_CompilerForceUsing;
	bool m_DeoptimizeWritableFiles;
	bool m_DeoptimizeWritableFilesWithToken;
	bool m_Remote;
    Node* m_PCHNode;
    Node* m_PreprocessorNode;
    AString m_PreprocessorArgs;
	uint32_t m_PreprocessorFlags;
};

//------------------------------------------------------------------------------
#endif // FBUILD_GRAPH_OBJECTNODE_H
