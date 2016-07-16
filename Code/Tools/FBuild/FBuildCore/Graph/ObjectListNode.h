// ObjectListNode.h - manages a list of ObjectNodes
//------------------------------------------------------------------------------
#pragma once
#ifndef FBUILD_GRAPH_OBJECTLISTNODE_H
#define FBUILD_GRAPH_OBJECTLISTNODE_H

// Includes
//------------------------------------------------------------------------------
#include "Node.h"
#include "Core/Containers/Array.h"

// Forward Declarations
//------------------------------------------------------------------------------
class Args;
class DirectoryListNode;
class CompilerNode;
class ObjectNode;

// ObjectListNode
//------------------------------------------------------------------------------
class ObjectListNode : public Node
{
public:
	explicit ObjectListNode( const AString & listName,
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
							 bool allowDistribution,
							 bool allowCaching,
							 CompilerNode * preprocessor,
							 const AString & preprocessorArgs,
							 const AString & baseDirectory );
	virtual ~ObjectListNode();

	static inline Node::Type GetTypeS() { return Node::OBJECT_LIST_NODE; }

	virtual bool IsAFile() const override;

	static Node * Load( NodeGraph & nodeGraph, IOStream & stream );
	virtual void Save( IOStream & stream ) const override;

	const char * GetObjExtension() const;

	void GetInputFiles( Args & fullArgs, const AString & pre, const AString & post ) const;
	void GetInputFiles( Array< AString > & files ) const;

	inline const AString & GetCompilerArgs() const { return m_CompilerArgs; }
protected:
	friend class FunctionObjectList;

    virtual bool GatherDynamicDependencies( NodeGraph & nodeGraph, bool forceClean );
	virtual bool DoDynamicDependencies( NodeGraph & nodeGraph, bool forceClean ) override;
	virtual BuildResult DoBuild( Job * job ) override;

	// internal helpers
	bool CreateDynamicObjectNode( NodeGraph & nodeGraph, Node * inputFile, const AString & baseDir, bool isUnityNode = false, bool isIsolatedFromUnityNode = false );

	CompilerNode *	m_Compiler;
	AString			m_CompilerArgs;
	AString			m_CompilerArgsDeoptimized;
	AString			m_CompilerOutputPath;
	Dependencies	m_CompilerForceUsing;
	ObjectNode *	m_PrecompiledHeader;
	AString			m_ObjExtensionOverride;
    AString         m_CompilerOutputPrefix;
	bool			m_DeoptimizeWritableFiles;
	bool			m_DeoptimizeWritableFilesWithToken;
	bool			m_AllowDistribution;
	bool			m_AllowCaching;
	CompilerNode *	m_Preprocessor;
	AString			m_PreprocessorArgs;
	AString			m_BaseDirectory;
	AString			m_ExtraPDBPath;
	AString			m_ExtraASMPath;
};

//------------------------------------------------------------------------------
#endif // FBUILD_GRAPH_OBJECTNODE_H
