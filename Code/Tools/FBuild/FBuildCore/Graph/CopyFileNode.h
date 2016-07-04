// CopyFileNode.h - a node that copies a single object
//------------------------------------------------------------------------------
#pragma once

// Includes
//------------------------------------------------------------------------------
#include "FileNode.h"

// Forward Declarations
//------------------------------------------------------------------------------

// CopyFileNode
//------------------------------------------------------------------------------
class CopyFileNode : public FileNode
{
public:
	explicit CopyFileNode( const AString & dstFileName,
					    FileNode * sourceFile,
						const Dependencies & preBuildDependencies );
	virtual ~CopyFileNode();

	static inline Node::Type GetTypeS() { return Node::COPY_FILE_NODE; }

	static Node * Load( IOStream & stream );
	virtual void Save( IOStream & stream ) const;

	FileNode * GetSourceNode() const { return m_SourceFile; }

private:
	virtual BuildResult DoBuild( Job * job );

	void EmitCopyMessage() const;

	FileNode * m_SourceFile;
};

//------------------------------------------------------------------------------
