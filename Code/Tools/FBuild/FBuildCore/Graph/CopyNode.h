// CopyNode.h - a node that copies a single object
//------------------------------------------------------------------------------
#pragma once
#ifndef FBUILD_GRAPH_COPYNODE_H
#define FBUILD_GRAPH_COPYNODE_H

// Includes
//------------------------------------------------------------------------------
#include "FileNode.h"

// Forward Declarations
//------------------------------------------------------------------------------

// CopyNode
//------------------------------------------------------------------------------
class CopyNode : public FileNode
{
public:
	explicit CopyNode( const AString & dstFileName,
					    FileNode * sourceFile,
						const Dependencies & preBuildDependencies );
	virtual ~CopyNode();

	static inline Node::Type GetTypeS() { return Node::COPY_NODE; }

	static Node * Load( IOStream & stream );
	virtual void Save( IOStream & stream ) const;

	FileNode * GetSourceNode() const { return m_SourceFile; }

private:
	virtual BuildResult DoBuild( Job * job );

	void EmitCopyMessage() const;

	FileNode * m_SourceFile;
};

//------------------------------------------------------------------------------
#endif // FBUILD_GRAPH_COPYNODE_H
