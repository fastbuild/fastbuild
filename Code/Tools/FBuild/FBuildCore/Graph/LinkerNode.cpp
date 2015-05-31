// LinkerNode.cpp
//------------------------------------------------------------------------------

// Includes
//------------------------------------------------------------------------------
#include "Tools/FBuild/FBuildCore/PrecompiledHeader.h"

#include "LinkerNode.h"

#include "Tools/FBuild/FBuildCore/FBuild.h"
#include "Tools/FBuild/FBuildCore/FLog.h"
#include "Tools/FBuild/FBuildCore/Graph/CopyNode.h"
#include "Tools/FBuild/FBuildCore/Graph/DLLNode.h"
#include "Tools/FBuild/FBuildCore/Graph/FileNode.h"
#include "Tools/FBuild/FBuildCore/Graph/LibraryNode.h"
#include "Tools/FBuild/FBuildCore/Graph/ObjectListNode.h"
#include "Tools/FBuild/FBuildCore/Graph/ObjectNode.h"
#include "Tools/FBuild/FBuildCore/Graph/NodeGraph.h"
#include "Tools/FBuild/FBuildCore/Helpers/ResponseFile.h"

#include "Core/FileIO/FileIO.h"
#include "Core/FileIO/FileStream.h"
#include "Core/Process/Process.h"
#include "Core/Strings/AStackString.h"

// CONSTRUCTOR
//------------------------------------------------------------------------------
LinkerNode::LinkerNode( const AString & linkerOutputName,
						 const Dependencies & inputLibraries,
						 const Dependencies & otherLibraries,
						 const AString & linker,
						 const AString & linkerArgs,
						 uint32_t flags,
						 const Dependencies & assemblyResources,
						 Node * linkerStampExe, 
						 const AString & linkerStampExeArgs )
: FileNode( linkerOutputName, Node::FLAG_NONE )
, m_Flags( flags )
, m_AssemblyResources( assemblyResources )
, m_OtherLibraries( otherLibraries )
, m_LinkerStampExe( linkerStampExe )
, m_LinkerStampExeArgs( linkerStampExeArgs )
{
	m_LastBuildTimeMs = 20000;

	// presize vector
	size_t numStaticDeps = inputLibraries.GetSize() + assemblyResources.GetSize() + otherLibraries.GetSize();
	if ( linkerStampExe )
	{
		numStaticDeps++;
	}
	m_StaticDependencies.SetCapacity( numStaticDeps );

	// depend on everything we'll link together
	m_StaticDependencies.Append( inputLibraries );
	m_StaticDependencies.Append( assemblyResources );
	m_StaticDependencies.Append( otherLibraries );	

	// manage optional LinkerStampExe
	if ( linkerStampExe )
	{
		m_StaticDependencies.Append( Dependency( linkerStampExe ) );
	}

	// store options we'll need to use dynamically
	m_Linker = linker; // TODO:C This should be a node
	m_LinkerArgs = linkerArgs;
}

// DESTRUCTOR
//------------------------------------------------------------------------------
LinkerNode::~LinkerNode()
{
}

// DoBuild
//------------------------------------------------------------------------------
/*virtual*/ Node::BuildResult LinkerNode::DoBuild( Job * UNUSED( job ) )
{
	DoPreLinkCleanup();

	// Format compiler args string
	AStackString< 4 * KILOBYTE > fullArgs;
	GetFullArgs( fullArgs );

	// use the exe launch dir as the working dir
	const char * workingDir = nullptr;

	const char * environment = FBuild::Get().GetEnvironmentString();

	EmitCompilationMessage( fullArgs );

	// handle response file
	ResponseFile rf;
	AStackString<> responseFileArgs;
	const bool useResponseFile = GetFlag( LINK_FLAG_MSVC ) || GetFlag( LINK_FLAG_GCC ) || GetFlag( LINK_FLAG_SNC ) || GetFlag( LINK_FLAG_ORBIS_LD ) || GetFlag( LINK_FLAG_GREENHILLS_ELXR ) || GetFlag( LINK_FLAG_CODEWARRIOR_LD );
	if ( useResponseFile )
	{
		// orbis-ld.exe requires escaped slashes inside response file
		if ( GetFlag( LINK_FLAG_ORBIS_LD ) )
		{
			rf.SetEscapeSlashes();
		}

		// write args to response file
		if ( !rf.Create( fullArgs ) )
		{
			return NODE_RESULT_FAILED; // Create will have emitted error
		}

		// override args to use response file
		responseFileArgs.Format( "@\"%s\"", rf.GetResponseFilePath().Get() );
	}

	// we retry if linker crashes
	uint32_t attempt( 0 );

	for (;;)
	{
		++attempt;

		// spawn the process
		Process p;
		bool spawnOK = p.Spawn( m_Linker.Get(),
								useResponseFile ? responseFileArgs.Get() : fullArgs.Get(),
								workingDir,
								environment );

		if ( !spawnOK )
		{
			FLOG_ERROR( "Failed to spawn process '%s' for %s creation for '%s'", m_Linker.Get(), GetDLLOrExe(), GetName().Get() );
			return NODE_RESULT_FAILED;
		}

		// capture all of the stdout and stderr
		AutoPtr< char > memOut;
		AutoPtr< char > memErr;
		uint32_t memOutSize = 0;
		uint32_t memErrSize = 0;
		p.ReadAllData( memOut, &memOutSize, memErr, &memErrSize );

		ASSERT( !p.IsRunning() );
		// Get result
		int result = p.WaitForExit();

		// did the executable fail?
		if ( result != 0 )
		{
			// did the linker have an ICE (LNK1000)?
			if ( GetFlag( LINK_FLAG_MSVC ) && ( result == 1000 ) && ( attempt == 1 ) )
			{
				FLOG_WARN( "FBuild: Warning: Linker crashed (LNK1000), retrying '%s'", GetName().Get() );
				continue; // try again
			}

			if ( memOut.Get() ) { FLOG_ERROR_DIRECT( memOut.Get() ); }
			if ( memErr.Get() ) { FLOG_ERROR_DIRECT( memErr.Get() ); }

			// some other (genuine) linker failure
			FLOG_ERROR( "Failed to build %s (error %i) '%s'", GetDLLOrExe(), result, GetName().Get() );
			return NODE_RESULT_FAILED;
		}
		else
		{	
			break; // success!
		}
	}

	// post-link stamp step
	if ( m_LinkerStampExe )
	{
		EmitStampMessage();

		Process stampProcess;
		bool spawnOk = stampProcess.Spawn( m_LinkerStampExe->GetName().Get(),
										   m_LinkerStampExeArgs.Get(),
										   nullptr,		// working dir
										   nullptr );	// env
		if ( spawnOk == false )
		{
			FLOG_ERROR( "Failed to spawn process '%s' for '%s' stamping of '%s'", m_LinkerStampExe->GetName().Get(), GetDLLOrExe(), GetName().Get() );
			return NODE_RESULT_FAILED;
		}

		// capture all of the stdout and stderr
		AutoPtr< char > memOut;
		AutoPtr< char > memErr;
		uint32_t memOutSize = 0;
		uint32_t memErrSize = 0;
		stampProcess.ReadAllData( memOut, &memOutSize, memErr, &memErrSize );
		ASSERT( !stampProcess.IsRunning() );

		// Get result
		int result = stampProcess.WaitForExit();

		// did the executable fail?
		if ( result != 0 )
		{
			if ( memOut.Get() ) { FLOG_ERROR_DIRECT( memOut.Get() ); }
			if ( memErr.Get() ) { FLOG_ERROR_DIRECT( memErr.Get() ); }
			FLOG_ERROR( "Failed to stamp %s '%s' (error %i - '%s')", GetDLLOrExe(), GetName().Get(), result, m_LinkerStampExe->GetName().Get() );
			return NODE_RESULT_FAILED;
		}

		// success!
	}

	// record time stamp for next time
	m_Stamp = FileIO::GetFileLastWriteTime( m_Name );
	ASSERT( m_Stamp );

	return NODE_RESULT_OK;
}

// DoPreLinkCleanup
//------------------------------------------------------------------------------
void LinkerNode::DoPreLinkCleanup() const
{
	// only for Microsoft compilers
	if ( GetFlag( LINK_FLAG_MSVC ) == false )
	{
		return;
	}

	bool deleteFiles = false;
	if ( GetFlag( LINK_FLAG_INCREMENTAL ) )
	{
		if ( FBuild::Get().GetOptions().m_ForceCleanBuild )
		{
			deleteFiles = true; // force a full re-link
		}
	}
	else
	{
		// cleanup old ILK files and force PDB to be rebuilt
		// (old PDBs get large and dramatically slow link times)
		deleteFiles = true;
	}

	if ( deleteFiles )
	{
		// output file
		FileIO::FileDelete( GetName().Get() );

		// .ilk
		const char * lastDot = GetName().FindLast( '.' );
		AStackString<> ilkName( GetName().Get(), lastDot ? lastDot : GetName().GetEnd() );
		ilkName += ".ilk";
		FileIO::FileDelete( ilkName.Get() );

		// .pdb - TODO: Handle manually specified /PDB
		AStackString<> pdbName( GetName().Get(), lastDot ? lastDot : GetName().GetEnd() );
		pdbName += ".pdb";
		FileIO::FileDelete( pdbName.Get() );
	}
}

// GetFullArgs
//------------------------------------------------------------------------------
void LinkerNode::GetFullArgs( AString & fullArgs ) const
{
	// split into tokens
	Array< AString > tokens( 1024, true );
	m_LinkerArgs.Tokenize( tokens );

	const AString * const end = tokens.End();
	for ( const AString * it = tokens.Begin(); it!=end; ++it )
	{
		const AString & token = *it;

		// %1 -> InputFiles
		const char * found = token.Find( "%1" );
		if ( found )
		{
			AStackString<> pre( token.Get(), found );
			AStackString<> post( found + 2, token.GetEnd() );
			GetInputFiles( fullArgs, pre, post );
			fullArgs += ' ';
			continue;
		}

		// %2 -> OutputFile
		found = token.Find( "%2" );
		if ( found )
		{
			fullArgs += AStackString<>( token.Get(), found );
			fullArgs += m_Name;
			fullArgs += AStackString<>( found + 2, token.GetEnd() );
			fullArgs += ' ';
			continue;
		}

		// %3 -> AssemblyResources
		if ( GetFlag( LINK_FLAG_MSVC ) )
		{
			found = token.Find( "%3" );
			if ( found )
			{
				AStackString<> pre( token.Get(), found );
				AStackString<> post( found + 2, token.GetEnd() );
				GetAssemblyResourceFiles( fullArgs, pre, post );
				fullArgs += ' ';
				continue;
			}
		}

		// untouched token
		fullArgs += token;
		fullArgs += ' ';
	}
}

// GetInputFiles
//------------------------------------------------------------------------------
void LinkerNode::GetInputFiles( AString & fullArgs, const AString & pre, const AString & post ) const
{
	// (exlude assembly resources from inputs)
	const Dependency * end = m_StaticDependencies.End() - ( m_AssemblyResources.GetSize() + m_OtherLibraries.GetSize() );
	if ( m_LinkerStampExe )
	{
		--end; // LinkerStampExe is not an input used for linking
	}
	for ( Dependencies::Iter i = m_StaticDependencies.Begin();
		  i != end;
		  i++ )
	{
		Node * n( i->GetNode() );
		GetInputFiles( n, fullArgs, pre, post );
	}
}

// GetInputFiles
//------------------------------------------------------------------------------
void LinkerNode::GetInputFiles( Node * n, AString & fullArgs, const AString & pre, const AString & post ) const
{
	if ( n->GetType() == Node::LIBRARY_NODE )
	{
		bool linkObjectsInsteadOfLibs = GetFlag( LINK_OBJECTS );

		if ( linkObjectsInsteadOfLibs )
		{
			LibraryNode * ln = n->CastTo< LibraryNode >();
			ln->GetInputFiles( fullArgs, pre, post );
		}
		else
		{
			// not building a DLL, so link the lib directly
			fullArgs += pre;
			fullArgs += n->GetName();
			fullArgs += post;
		}
	}
	else if ( n->GetType() == Node::OBJECT_LIST_NODE )
	{
		ObjectListNode * ol = n->CastTo< ObjectListNode >();
		ol->GetInputFiles( fullArgs, pre, post );
	}
	else if ( n->GetType() == Node::DLL_NODE )
	{
		// for a DLL, link to the import library
		DLLNode * dllNode = n->CastTo< DLLNode >();
		AStackString<> importLibName;
		dllNode->GetImportLibName( importLibName );
		fullArgs += pre;
		fullArgs += importLibName;
		fullArgs += post;
	}
	else if ( n->GetType() == Node::COPY_NODE )
	{
		CopyNode * copyNode = n->CastTo< CopyNode >();
		Node * srcNode = copyNode->GetSourceNode();
		GetInputFiles( srcNode, fullArgs, pre, post );
	}
	else
	{
		// link anything else directly
		fullArgs += pre;
		fullArgs += n->GetName();
		fullArgs += post;
	}

	fullArgs += ' ';
}

// GetAssemblyResourceFiles
//------------------------------------------------------------------------------
void LinkerNode::GetAssemblyResourceFiles( AString & fullArgs, const AString & pre, const AString & post ) const
{
	const Dependency * const end = m_AssemblyResources.End();
	for ( Dependencies::Iter i = m_AssemblyResources.Begin();
		  i != end;
		  i++ )
	{
		Node * n( i->GetNode() );

		if ( n->GetType() == Node::OBJECT_LIST_NODE )
		{
			ObjectListNode * oln = n->CastTo< ObjectListNode >();
			oln->GetInputFiles( fullArgs, pre, post );
			continue;
		}

		if ( n->GetType() == Node::LIBRARY_NODE )
		{
			LibraryNode * ln = n->CastTo< LibraryNode >();
			ln->GetInputFiles( fullArgs, pre, post );
			continue;
		}

		fullArgs += pre;
		fullArgs += n->GetName();
		fullArgs += post;
		fullArgs += ' ';
	}
}

// DetermineFlags
//------------------------------------------------------------------------------
/*static*/ uint32_t LinkerNode::DetermineFlags( const AString & linkerName, const AString & args )
{
	uint32_t flags = 0;

	// Linker executable type
	if ( ( linkerName.EndsWithI( "link.exe" ) ) ||
		 ( linkerName.EndsWithI( "link" ) ) )
	{
		flags |= LinkerNode::LINK_FLAG_MSVC;
	}
	else if ( ( linkerName.EndsWithI( "gcc.exe" ) ) ||
			  ( linkerName.EndsWithI( "gcc" ) ) )
	{
		flags |= LinkerNode::LINK_FLAG_GCC;
	}
	else if ( ( linkerName.EndsWithI( "ps3ppuld.exe" ) ) ||
			  ( linkerName.EndsWithI( "ps3ppuld" ) ) )
	{
		flags |= LinkerNode::LINK_FLAG_SNC;
	}
	else if ( ( linkerName.EndsWithI( "orbis-ld.exe" ) ) ||
			  ( linkerName.EndsWithI( "orbis-ld" ) ) )
	{
		flags |= LinkerNode::LINK_FLAG_ORBIS_LD;
	}
	else if ( ( linkerName.EndsWithI( "elxr.exe" ) ) ||
			  ( linkerName.EndsWithI( "elxr" ) ) )
	{
		flags |= LinkerNode::LINK_FLAG_GREENHILLS_ELXR;
	}
	else if ( ( linkerName.EndsWithI( "mwldeppc.exe" ) ) ||
			  ( linkerName.EndsWithI( "mwldeppc." ) ) )
	{
		flags |= LinkerNode::LINK_FLAG_CODEWARRIOR_LD;
	}

	if ( flags & LINK_FLAG_MSVC )
	{
		// Parse args for some other flags
		Array< AString > tokens;
		args.Tokenize( tokens );

		bool debugFlag = false;
		bool incrementalFlag = false;
		bool incrementalNoFlag = false;
		bool optREFFlag = false;
		bool optICFFlag = false;
		bool optLBRFlag = false;
		bool orderFlag = false;

		const AString * const end = tokens.End();
		for ( const AString * it=tokens.Begin(); it!=end; ++it )
		{
			const AString & token = *it;
			if ( token == "/DLL" )
			{
				flags |= LinkerNode::LINK_FLAG_DLL;
				continue;
			}

			if ( token == "/DEBUG" )
			{
				debugFlag = true;
				continue;
			}

			if ( token == "/INCREMENTAL")
			{
				incrementalFlag = true;
				continue;
			}

			if ( token == "/INCREMENTAL:NO")
			{
				incrementalNoFlag = true;
				continue;
			}

			if ( token == "/OPT:REF")
			{
				optREFFlag = true;
				continue;
			}

			if ( token == "/OPT:ICF")
			{
				optICFFlag = true;
				continue;
			}

			if ( token == "/OPT:LBR")
			{
				optLBRFlag = true;
				continue;
			}

			if ( token.BeginsWith( "/ORDER" ) )
			{
				orderFlag = true;
				continue;
			}
		}

		// Determine incremental linking status
		bool usingIncrementalLinking = false; // false by default

		// these options enable incremental linking
		if ( debugFlag || incrementalFlag )
		{
			usingIncrementalLinking = true;
		}

		// these options disable incremental linking
		if ( incrementalNoFlag || optREFFlag || optICFFlag || optLBRFlag || orderFlag )
		{
			usingIncrementalLinking = false;
		}

		if ( usingIncrementalLinking )
		{
			flags |= LINK_FLAG_INCREMENTAL;
		}
	}
	else
	{
		// Parse args for some other flags
		Array< AString > tokens;
		args.Tokenize( tokens );

        const AString * const end = tokens.End();
		for ( const AString * it=tokens.Begin(); it!=end; ++it )
		{
			const AString & token = *it;
			if ( ( token == "-shared" ) || ( token == "-dynamiclib" ) )
			{
				flags |= LinkerNode::LINK_FLAG_DLL;
				continue;
			}
        }
	}

	return flags;
}

// EmitCompilationMessage
//------------------------------------------------------------------------------
void LinkerNode::EmitCompilationMessage( const AString & fullArgs ) const
{
	AStackString<> output;
	output += GetDLLOrExe();
	output += ": ";
	output += GetName();
	output += '\n';
	if ( FLog::ShowInfo() || FBuild::Get().GetOptions().m_ShowCommandLines )
	{
		output += m_Linker;
		output += ' ';
		output += fullArgs;
		output += '\n';
	}
	FLOG_BUILD( "%s", output.Get() );
}

// EmitStampMessage
//------------------------------------------------------------------------------
void LinkerNode::EmitStampMessage() const
{
	ASSERT( m_LinkerStampExe );

	AStackString<> output;
	output += "Stamp: ";
	output += GetName();
	output += '\n';
	if ( FLog::ShowInfo() || FBuild::Get().GetOptions().m_ShowCommandLines )
	{
		output += m_LinkerStampExe->GetName();
		output += ' ';
		output += m_LinkerStampExeArgs;
		output += '\n';
	}
	FLOG_BUILD( "%s", output.Get() );
}

// Save
//------------------------------------------------------------------------------
/*virtual*/ void LinkerNode::Save( IOStream & stream ) const
{
	// we want to save the static deps excluding the 
	size_t count = m_StaticDependencies.GetSize() - ( m_AssemblyResources.GetSize() + m_OtherLibraries.GetSize() );
	if ( m_LinkerStampExe )
	{
		count--;
	}
	Dependencies staticDeps( m_StaticDependencies.Begin(), m_StaticDependencies.Begin() + count );

	NODE_SAVE( m_Name );
	NODE_SAVE( m_Linker );
	NODE_SAVE( m_LinkerArgs );
	NODE_SAVE_DEPS( staticDeps );
	NODE_SAVE( m_Flags );
	NODE_SAVE_DEPS( m_AssemblyResources );
	NODE_SAVE_DEPS( m_OtherLibraries );
    NODE_SAVE_NODE( m_LinkerStampExe );
    NODE_SAVE( m_LinkerStampExeArgs );
}

//------------------------------------------------------------------------------
