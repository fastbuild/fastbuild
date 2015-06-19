// LinkerNode.h - builds an exe or dll
//------------------------------------------------------------------------------
#pragma once
#ifndef FBUILD_GRAPH_LINKERNODE_H
#define FBUILD_GRAPH_LINKERNODE_H

// Includes
//------------------------------------------------------------------------------
#include "FileNode.h"
#include "Core/Containers/Array.h"

// Forward Declarations
//------------------------------------------------------------------------------

// LinkerNode
//------------------------------------------------------------------------------
class LinkerNode : public FileNode
{
public:
	explicit LinkerNode( const AString & linkerOutputName,
						 const Dependencies & inputLibraries,
						 const Dependencies & otherLibraries,
						 const AString & linker,
						 const AString & linkerArgs,
						 uint32_t flags,
						 const Dependencies & assemblyResources,
						 Node * linkerStampExe, 
						 const AString & linkerStampExeArgs );
	virtual ~LinkerNode();

	enum Flag
	{
		LINK_FLAG_MSVC		= 0x01,
		LINK_FLAG_DLL		= 0x02,
		LINK_OBJECTS		= 0x04,
		LINK_FLAG_GCC		= 0x08,
		LINK_FLAG_SNC		= 0x10,
		LINK_FLAG_ORBIS_LD	= 0x20,
		LINK_FLAG_INCREMENTAL = 0x40,
		LINK_FLAG_GREENHILLS_ELXR = 0x80,
		LINK_FLAG_CODEWARRIOR_LD=0x100
	};

	inline bool IsADLL() const { return GetFlag( LINK_FLAG_DLL ); }

	virtual void Save( IOStream & stream ) const;

	static uint32_t DetermineFlags( const AString & linkerName, const AString & args );
protected:
	virtual BuildResult DoBuild( Job * job );

	void DoPreLinkCleanup() const;

	void GetFullArgs( AString & fullArgs ) const;
	void GetInputFiles( AString & fullArgs, const AString & pre, const AString & post ) const;
	void GetInputFiles( Node * n, AString & fullArgs, const AString & pre, const AString & post ) const;
	void GetAssemblyResourceFiles( AString & fullArgs, const AString & pre, const AString & post ) const;
	void EmitCompilationMessage( const AString & fullArgs ) const;
	void EmitStampMessage() const;

	inline bool GetFlag( Flag flag ) const { return ( ( m_Flags & (uint32_t)flag ) != 0 ); }

	inline const char * GetDLLOrExe() const { return GetFlag( LINK_FLAG_DLL ) ? "DLL" : "Exe"; }

	AString m_Linker;
	AString m_LinkerArgs;
	uint32_t m_Flags;
	Dependencies m_AssemblyResources;
	Dependencies m_OtherLibraries;
    const Node * m_LinkerStampExe;
    AString m_LinkerStampExeArgs;
};

//------------------------------------------------------------------------------
#endif // FBUILD_GRAPH_LINKERNODE_H
