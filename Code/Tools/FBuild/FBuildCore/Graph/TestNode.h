// TestNode.h - Run a Test
//------------------------------------------------------------------------------
#pragma once
#ifndef FBUILD_GRAPH_TESTNODE_H
#define FBUILD_GRAPH_TESTNODE_H

// Includes
//------------------------------------------------------------------------------
#include "ExecNode.h"
#include "Core/Containers/Array.h"

// Forward Declarations
//------------------------------------------------------------------------------

// TestNode
//------------------------------------------------------------------------------
class TestNode : public FileNode
{
public:
	explicit TestNode( const AString & testOutput,
					   FileNode * testExecutable,
					   const AString & m_Arguments,
					   const AString & m_WorkingDir );
	virtual ~TestNode();

	static inline Node::Type GetTypeS() { return Node::TEST_NODE; }

	virtual void Save( IOStream & stream ) const;
	static Node * Load( IOStream & stream );
private:
	virtual BuildResult DoBuild( Job * job );

	void EmitCompilationMessage( const char * workingDir ) const;

	FileNode * m_Executable;
	AString		m_Arguments;
	AString		m_WorkingDir;
};

//------------------------------------------------------------------------------
#endif // FBUILD_GRAPH_TESTNODE_H
