// CopyDirNode.h - a node that copies one or more directories
//------------------------------------------------------------------------------
#pragma once
#ifndef FBUILD_GRAPH_COPYDIRNODE_H
#define FBUILD_GRAPH_COPYDIRNODE_H

// Includes
//------------------------------------------------------------------------------
#include "Node.h"

// Forward Declarations
//------------------------------------------------------------------------------

// CopyDirNode
//------------------------------------------------------------------------------
class CopyDirNode : public Node
{
public:
	explicit CopyDirNode( const AString & name,
						  Dependencies & staticDeps,
						  const AString & destPath,
						  const Dependencies & preBuildDeps );
	virtual ~CopyDirNode();

	static inline Node::Type GetTypeS() { return Node::COPY_DIR_NODE; }
	virtual bool IsAFile() const override;

	static Node * Load( NodeGraph & nodeGraph, IOStream & stream );
	virtual void Save( IOStream & stream ) const override;

private:
	virtual bool DoDynamicDependencies( NodeGraph & nodeGraph, bool forceClean ) override;
	virtual BuildResult DoBuild( Job * job ) override;

	AString m_DestPath;
};

//------------------------------------------------------------------------------
#endif // FBUILD_GRAPH_COPYDIRNODE_H
