// LibraryNode.h - a node that builds a single object from a source file
//------------------------------------------------------------------------------
#pragma once
#ifndef FBUILD_GRAPH_LIBRARYNODE_H
#define FBUILD_GRAPH_LIBRARYNODE_H

// Includes
//------------------------------------------------------------------------------
#include "ObjectListNode.h"
#include "Core/Containers/Array.h"

// Forward Declarations
//------------------------------------------------------------------------------
class Args;
class CompilerNode;
class DirectoryListNode;
class ObjectNode;

// LibraryNode
//------------------------------------------------------------------------------
class LibraryNode : public ObjectListNode
{
public:
	explicit LibraryNode( const AString & libraryName,
						  const Dependencies & inputNodes,
						  CompilerNode * compiler,
						  const AString & compilerArgs,
						  const AString & compilerArgsDeoptimized,
						  const AString & compilerOutputPath,
						  const AString & librarian,
						  const AString & librarianArgs,
						  uint32_t flags,
						  ObjectNode * precompiledHeader,
						  const Dependencies & compilerForceUsing,
						  const Dependencies & preBuildDependencies,
						  const Dependencies & additionalInputs,
						  bool deoptimizeWritableFiles,
						  bool deoptimizeWritableFilesWithToken,
						  bool allowDistribution,
						  bool allowCaching,
                          CompilerNode * preprocessor,
                          const AString & preprocessorArgs,
						  const AString & baseDirectory );
	virtual ~LibraryNode();

	static inline Node::Type GetTypeS() { return Node::LIBRARY_NODE; }

	virtual bool IsAFile() const override;

	static Node * Load( NodeGraph & nodeGraph, IOStream & stream );
	virtual void Save( IOStream & stream ) const override;

	enum Flag
	{
		LIB_FLAG_LIB	= 0x01,	// MSVC style lib.exe
		LIB_FLAG_AR		= 0x02,	// gcc/clang style ar.exe
		LIB_FLAG_ORBIS_AR=0x04, // Orbis ar.exe
		LIB_FLAG_GREENHILLS_AX=0x08, // Greenhills (WiiU) ax.exe
	};
	static uint32_t DetermineFlags( const AString & librarianName );
private:
	friend class FunctionLibrary;

    virtual bool GatherDynamicDependencies( NodeGraph & nodeGraph, bool forceClean ) override;
	virtual BuildResult DoBuild( Job * job ) override;

	// internal helpers
	bool BuildArgs( Args & fullArgs ) const;
	void EmitCompilationMessage( const Args & fullArgs ) const;

	inline bool GetFlag( Flag flag ) const { return ( ( m_Flags & (uint32_t)flag ) != 0 ); }

	bool CanUseResponseFile() const;

	AString m_LibrarianPath;
	AString m_LibrarianArgs;
	uint32_t m_Flags;
	Dependencies m_AdditionalInputs;
};

//------------------------------------------------------------------------------
#endif // FBUILD_GRAPH_LIBRARYNODE_H
