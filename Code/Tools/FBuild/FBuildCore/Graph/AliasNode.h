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
#ifdef USE_NODE_REFLECTION
    REFLECT_DECLARE( AliasNode )
#endif
public:
#ifdef USE_NODE_REFLECTION
	explicit AliasNode();
	bool Initialize( const BFFIterator & iter, const Function * function );
#else
	explicit AliasNode( const AString & groupName,
						const Dependencies & targets );
#endif
	virtual ~AliasNode();

	static inline Node::Type GetTypeS() { return Node::ALIAS_NODE; }

	virtual bool IsAFile() const { return false; }

	inline const Dependencies & GetAliasedNodes() const { return m_StaticDependencies; }

	static Node * Load( IOStream & stream );
	virtual void Save( IOStream & stream ) const;
private:
	virtual bool DetermineNeedToBuild( bool forceClean ) const;
	virtual BuildResult DoBuild( Job * job );

#ifdef USE_NODE_REFLECTION
    Array< AString > m_Targets;
#endif
};

//------------------------------------------------------------------------------
#endif // FBUILD_GRAPH_GROUPNODE_H
