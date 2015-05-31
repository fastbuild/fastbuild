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

	static inline Node::Type GetType() { return Node::OBJECT_LIST_NODE; }

	virtual bool IsAFile() const;

	static Node * Load( IOStream & stream );
	virtual void Save( IOStream & stream ) const;

	const char * GetObjExtension() const;

	void GetInputFiles( AString & fullArgs, const AString & pre, const AString & post ) const;
private:
	friend class FunctionObjectList;

	virtual bool DoDynamicDependencies( bool forceClean );
	virtual BuildResult DoBuild( Job * job );

	// internal helpers
	bool CreateDynamicObjectNode( Node * inputFile, const DirectoryListNode * dirNode, bool isUnityNode = false, bool isIsolatedFromUnityNode = false );

	CompilerNode *	m_Compiler;
	AString			m_CompilerArgs;
	AString			m_CompilerArgsDeoptimized;
	AString			m_CompilerOutputPath;
	Dependencies	m_CompilerForceUsing;
	ObjectNode *	m_PrecompiledHeader;
	AString			m_ObjExtensionOverride;
	bool			m_DeoptimizeWritableFiles;
	bool			m_DeoptimizeWritableFilesWithToken;
	CompilerNode *	m_Preprocessor;
	AString			m_PreprocessorArgs;
};

//------------------------------------------------------------------------------
#endif // FBUILD_GRAPH_OBJECTNODE_H
