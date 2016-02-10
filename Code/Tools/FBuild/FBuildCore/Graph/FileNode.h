// FileNode.h - a node that tracks a single file
//------------------------------------------------------------------------------
#pragma once
#ifndef FBUILD_GRAPH_FILENODE_H
#define FBUILD_GRAPH_FILENODE_H

// Includes
//------------------------------------------------------------------------------
#include "Node.h"

// FileNode
//------------------------------------------------------------------------------
class FileNode : public Node
{
public:
	explicit FileNode( const AString & fileName, const char * baseDirectory = nullptr, uint32_t controlFlags = Node::FLAG_TRIVIAL_BUILD );
	virtual ~FileNode();

	const AString & GetBaseDirectory() const { return m_baseDirectory; }
	static inline Node::Type GetTypeS() { return Node::FILE_NODE; }

	virtual bool IsAFile() const { return true; }

	static Node * Load( IOStream & stream );
	virtual void Save( IOStream & stream ) const;
protected:
	virtual BuildResult DoBuild( Job * job );

	friend class Client;

	AString m_baseDirectory;
};

//------------------------------------------------------------------------------
#endif // FBUILD_GRAPH_FILENODE_H
