// LibraryNode.h - a node that builds a single object from a source file
//------------------------------------------------------------------------------
#pragma once
#ifndef FBUILD_GRAPH_LIBRARYNODE_H
#define FBUILD_GRAPH_LIBRARYNODE_H

// Includes
//------------------------------------------------------------------------------
#include "FileNode.h"
#include "Core/Containers/Array.h"

// Forward Declarations
//------------------------------------------------------------------------------
class CompilerNode;
class DirectoryListNode;
class ObjectNode;

// LibraryNode
//------------------------------------------------------------------------------
class LibraryNode : public FileNode
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
                          CompilerNode * preprocessor,
                          const AString & preprocessorArgs );
	virtual ~LibraryNode();

	static inline Node::Type GetType() { return Node::LIBRARY_NODE; }

	static Node * Load( IOStream & stream );
	virtual void Save( IOStream & stream ) const;

	const char * GetObjExtension() const;

	void GetInputFiles( AString & fullArgs, const AString & pre, const AString & post ) const;

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

	virtual bool DoDynamicDependencies( bool forceClean );
	virtual BuildResult DoBuild( Job * job );

	// internal helpers
	void GetFullArgs( AString & fullArgs ) const;
	bool CreateDynamicObjectNode( Node * inputFile, const DirectoryListNode * dirList, bool isUnityNode = false, bool isIsolatedFromUnityNode = false );
	void EmitCompilationMessage( const AString & fullArgs ) const;

	virtual Priority GetPriority() const { return PRIORITY_HIGH; }

	inline bool GetFlag( Flag flag ) const { return ( ( m_Flags & (uint32_t)flag ) != 0 ); }

	CompilerNode * m_Compiler;
	AString m_CompilerArgs;
	AString m_CompilerArgsDeoptimized;
	AString m_CompilerOutputPath;
	Dependencies m_CompilerForceUsing;
	AString m_LibrarianPath;
	AString m_LibrarianArgs;
	uint32_t m_Flags;
	ObjectNode * m_PrecompiledHeader;
	AString m_ObjExtensionOverride;
	Dependencies m_AdditionalInputs;
	bool m_DeoptimizeWritableFiles;
	bool m_DeoptimizeWritableFilesWithToken;
	CompilerNode *	m_Preprocessor;
	AString			m_PreprocessorArgs;
};

//------------------------------------------------------------------------------
#endif // FBUILD_GRAPH_LIBRARYNODE_H
