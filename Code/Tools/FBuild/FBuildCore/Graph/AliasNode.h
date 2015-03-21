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

// AliasNode
//------------------------------------------------------------------------------
class AliasNode : public Node
{
public:
	explicit AliasNode( const AString & groupName,
						const Dependencies & targets );
	virtual ~AliasNode();

	static inline Node::Type GetType() { return Node::ALIAS_NODE; }

	virtual bool IsAFile() const { return false; }

	inline const Dependencies & GetAliasedNodes() const { return m_StaticDependencies; }

	static Node * Load( IOStream & stream );
	virtual void Save( IOStream & stream ) const;
private:
	virtual bool DetermineNeedToBuild( bool forceClean ) const;
	virtual BuildResult DoBuild( Job * job );
};

//------------------------------------------------------------------------------
#endif // FBUILD_GRAPH_GROUPNODE_H
