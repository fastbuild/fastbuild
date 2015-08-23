// LinkerNode.cpp
//------------------------------------------------------------------------------

// Includes
//------------------------------------------------------------------------------
#include "Tools/FBuild/FBuildCore/PrecompiledHeader.h"

#include "ExeNode.h"

#include "Tools/FBuild/FBuildCore/FBuild.h"
#include "Tools/FBuild/FBuildCore/Graph/NodeGraph.h"

#include "Core/FileIO/IOStream.h"
#include "Core/Strings/AStackString.h"

// CONSTRUCTOR
//------------------------------------------------------------------------------
ExeNode::ExeNode( const AString & linkerOutputName,
				  const Dependencies & inputLibraries,
				  const Dependencies & otherLibraries,
				  const AString & linker,
				  const AString & linkerArgs,
				  uint32_t flags,
				  const Dependencies & assemblyResources,
				  const AString & importLibName,
				  Node * linkerStampExe,
				  const AString & linkerStampExeArgs )
: LinkerNode( linkerOutputName, inputLibraries, otherLibraries, linker, linkerArgs, flags, assemblyResources, importLibName, linkerStampExe, linkerStampExeArgs )
{
	m_Type = EXE_NODE;
}

// DESTRUCTOR
//------------------------------------------------------------------------------
ExeNode::~ExeNode()
{
}

// Load
//------------------------------------------------------------------------------
/*static*/ Node * ExeNode::Load( IOStream & stream )
{
    // common Linker properties
	NODE_LOAD( AStackString<>,	name );
	NODE_LOAD( AStackString<>,	linker );
	NODE_LOAD( AStackString<>,	linkerArgs );
	NODE_LOAD_DEPS( 0,			inputLibs);
	NODE_LOAD( uint32_t,		flags );
	NODE_LOAD_DEPS( 0,			assemblyResources );
	NODE_LOAD_DEPS( 0,			otherLibs );
	NODE_LOAD( AStackString<>,	importLibName );
    NODE_LOAD_NODE( Node,		linkerStampExe );
    NODE_LOAD( AStackString<>,  linkerStampExeArgs );

	NodeGraph & ng = FBuild::Get().GetDependencyGraph();
	ExeNode * en = ng.CreateExeNode( name, inputLibs, otherLibs, linker, linkerArgs, flags, assemblyResources, importLibName, linkerStampExe, linkerStampExeArgs );
	return en;
}

//------------------------------------------------------------------------------
