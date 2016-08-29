// AliasNode.h - a node that groups together several targets under a new target
//------------------------------------------------------------------------------
#pragma once
#ifndef FBUILD_GRAPH_ALIASNODE_H
#define FBUILD_GRAPH_ALIASNODE_H

// Includes
//------------------------------------------------------------------------------
#include "Node.h"
#include "Core/Containers/Array.h"

// Forward Declarations
//------------------------------------------------------------------------------
class BFFIterator;
class Function;

// AliasNode
//------------------------------------------------------------------------------
class AliasNode : public Node
{
    REFLECT_DECLARE( AliasNode )
public:
	explicit AliasNode();
	bool Initialize( NodeGraph & nodeGraph, const BFFIterator & iter, const Function * function );
	virtual ~AliasNode();

	static inline Node::Type GetTypeS() { return Node::ALIAS_NODE; }

	virtual bool IsAFile() const override { return false; }

	inline const Dependencies & GetAliasedNodes() const { return m_StaticDependencies; }

	static Node * Load( NodeGraph & nodeGraph, IOStream & stream );
	virtual void Save( IOStream & stream ) const override;
private:
	virtual bool DetermineNeedToBuild( bool forceClean ) const override;
	virtual BuildResult DoBuild( Job * job ) override;

    Array< AString > m_Targets;
};

//------------------------------------------------------------------------------
#endif // FBUILD_GRAPH_GROUPNODE_H
