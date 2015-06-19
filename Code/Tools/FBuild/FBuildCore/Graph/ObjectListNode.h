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
                             CompilerNode * preprocessor,
                             const AString & preprocessorArgs );
	virtual ~ObjectListNode();

	static inline Node::Type GetTypeS() { return Node::OBJECT_LIST_NODE; }

	virtual bool IsAFile() const override;

	static Node * Load( IOStream & stream );
	virtual void Save( IOStream & stream ) const override;

	const char * GetObjExtension() const;

	void GetInputFiles( AString & fullArgs, const AString & pre, const AString & post ) const;
	void GetInputFiles( Array< AString > & files ) const;
protected:
	friend class FunctionObjectList;

    virtual bool GatherDynamicDependencies( bool forceClean );
	virtual bool DoDynamicDependencies( bool forceClean ) override;
	virtual BuildResult DoBuild( Job * job ) override;

	// internal helpers
	bool CreateDynamicObjectNode( Node * inputFile, const DirectoryListNode * dirNode, bool isUnityNode = false, bool isIsolatedFromUnityNode = false );

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
	CompilerNode *	m_Preprocessor;
	AString			m_PreprocessorArgs;
};

//------------------------------------------------------------------------------
#endif // FBUILD_GRAPH_OBJECTNODE_H
