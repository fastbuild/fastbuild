// ExeNode.h - builds an exe
//------------------------------------------------------------------------------
#pragma once
#ifndef FBUILD_GRAPH_EXENODE_H
#define FBUILD_GRAPH_EXENODE_H

// Includes
//------------------------------------------------------------------------------
#include "LinkerNode.h"

// Forward Declarations
//------------------------------------------------------------------------------

// ExeNode
//------------------------------------------------------------------------------
class ExeNode : public LinkerNode
{
public:
	explicit ExeNode( const AString & linkerOutputName,
					  const Dependencies & inputLibraries,
					  const Dependencies & otherLibraries,
					  const AString & linker,
					  const AString & linkerArgs,
					  uint32_t flags,
					  const Dependencies & assemblyResources,
					  Node * linkerStampExe,
					  const AString & linkerStampExeArgs );
	virtual ~ExeNode();

	static inline Node::Type GetTypeS() { return Node::EXE_NODE; }

	static Node * Load( IOStream & stream );
private:
};

//------------------------------------------------------------------------------
#endif // FBUILD_GRAPH_EXENODE_H
