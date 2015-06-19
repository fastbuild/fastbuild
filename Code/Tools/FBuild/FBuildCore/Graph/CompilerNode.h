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
class BFFIterator;
class Function;

// CompilerNode
//------------------------------------------------------------------------------
class CompilerNode : public FileNode
{
#ifdef USE_NODE_REFLECTION
	REFLECT_DECLARE( CompilerNode )
#endif
public:
#ifdef USE_NODE_REFLECTION
	explicit CompilerNode();
	bool Initialize( const BFFIterator & iter, const Function * function );
#else
	explicit CompilerNode( const AString & exe,
						   const Dependencies & extraFiles,
						   bool allowDistribution );
#endif
	virtual ~CompilerNode();

	static inline Node::Type GetTypeS() { return Node::COMPILER_NODE; }

	static Node * Load( IOStream & stream );
	virtual void Save( IOStream & stream ) const;

	inline const ToolManifest & GetManifest() const { return m_Manifest; }

	inline bool CanBeDistributed() const { return m_AllowDistribution; }
private:
	virtual BuildResult DoBuild( Job * job );

	// Exposed params
#ifdef USE_NODE_REFLECTION
	Array< AString >	m_ExtraFiles;
#endif

	bool			m_AllowDistribution;
	ToolManifest	m_Manifest;
};

//------------------------------------------------------------------------------
#endif // FBUILD_GRAPH_COMPILERNODE_H
