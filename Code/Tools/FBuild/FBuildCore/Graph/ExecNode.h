// ExecNode
//------------------------------------------------------------------------------
#pragma once
#ifndef FBUILD_GRAPH_EXECNODE_H
#define FBUILD_GRAPH_EXECNODE_H

// Includes
//------------------------------------------------------------------------------
#include "FileNode.h"
#include "Core/Containers/Array.h"

// Forward Declarations
//------------------------------------------------------------------------------

// ExecNode
//------------------------------------------------------------------------------
class ExecNode : public FileNode
{
public:
	explicit ExecNode( const AString & dstFileName,
					    FileNode * sourceFile,
						FileNode * executable,
						const AString & arguments,
						const AString & workingDir,
						int32_t expectedReturnCode,
						const Dependencies & preBuildDependencies );
	virtual ~ExecNode();

	static inline Node::Type GetTypeS() { return Node::EXEC_NODE; }

	static Node * Load( IOStream & stream );
	virtual void Save( IOStream & stream ) const;
private:
	virtual BuildResult DoBuild( Job * job );

	void EmitCompilationMessage( const AString & args ) const;

	FileNode * m_SourceFile;
	FileNode * m_Executable;
	AString		m_Arguments;
	AString		m_WorkingDir;
	int32_t		m_ExpectedReturnCode;
};

//------------------------------------------------------------------------------
#endif // FBUILD_GRAPH_EXECNODE_H
