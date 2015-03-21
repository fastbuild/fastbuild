// CompilerNode.h - a node that manages a compiler executable 
//------------------------------------------------------------------------------
#pragma once
#ifndef FBUILD_GRAPH_COMPILERNODE_H
#define FBUILD_GRAPH_COMPILERNODE_H

// Includes
//------------------------------------------------------------------------------
#include "FileNode.h"
#include "Tools/FBuild/FBuildCore/Helpers/ToolManifest.h"

// Forward Declarations
//------------------------------------------------------------------------------

// CompilerNode
//------------------------------------------------------------------------------
class CompilerNode : public FileNode
{
public:
	explicit CompilerNode( const AString & exe,
						   const Dependencies & extraFiles,
						   bool allowDistribution );
	virtual ~CompilerNode();

	static inline Node::Type GetType() { return Node::COMPILER_NODE; }

	static Node * Load( IOStream & stream );
	virtual void Save( IOStream & stream ) const;

	inline const ToolManifest & GetManifest() const { return m_Manifest; }

	inline bool CanBeDistributed() const { return m_AllowDistribution; }
private:
	virtual BuildResult DoBuild( Job * job );

	bool			m_AllowDistribution;
	ToolManifest	m_Manifest;
};

//------------------------------------------------------------------------------
#endif // FBUILD_GRAPH_COMPILERNODE_H
