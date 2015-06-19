// CSNode.cpp
//------------------------------------------------------------------------------

// Includes
//------------------------------------------------------------------------------
#include "Tools/FBuild/FBuildCore/PrecompiledHeader.h"

#include "CSNode.h"
#include "Tools/FBuild/FBuildCore/Graph/DirectoryListNode.h"
#include "Tools/FBuild/FBuildCore/FBuild.h"
#include "Tools/FBuild/FBuildCore/FLog.h"
#include "Tools/FBuild/FBuildCore/Graph/NodeGraph.h"
#include "Tools/FBuild/FBuildCore/Helpers/CIncludeParser.h"
#include "Tools/FBuild/FBuildCore/Helpers/ResponseFile.h"

#include "Core/FileIO/FileIO.h"
#include "Core/FileIO/FileStream.h"
#include "Core/Math/CRC32.h"
#include "Core/Process/Process.h"
#include "Core/Tracing/Tracing.h"
#include "Core/Strings/AStackString.h"

// CONSTRUCTOR
//------------------------------------------------------------------------------
CSNode::CSNode( const AString & compilerOutput,
				const Dependencies & inputNodes,
				const AString & compiler,
				const AString & compilerArgs,
				const Dependencies & extraRefs )
: FileNode( compilerOutput, Node::FLAG_NONE )
, m_ExtraRefs( extraRefs )
{
	ASSERT( !inputNodes.IsEmpty() );

	m_StaticDependencies.SetCapacity( inputNodes.GetSize() + extraRefs.GetSize() );
	m_StaticDependencies.Append( inputNodes );
	m_StaticDependencies.Append( extraRefs );

	// store options we'll need to use when building
	m_CompilerPath = compiler; // TODO:C This should be a node we statically depend on
	m_CompilerArgs = compilerArgs;

	m_Type = CS_NODE;
	m_LastBuildTimeMs = 5000; // higher default than a file node
}

// DESTRUCTOR
//------------------------------------------------------------------------------
CSNode::~CSNode()
{
}

// DoDynamicDependencies
//------------------------------------------------------------------------------
/*virtual*/ bool CSNode::DoDynamicDependencies( bool UNUSED( forceClean ) )
{
	ASSERT( m_DynamicDependencies.GetSize() == 0 );

	NodeGraph & ng = FBuild::Get().GetDependencyGraph();

	// preallocate a reasonable amount of space
	m_DynamicDependencies.SetCapacity( m_StaticDependencies.GetSize() );

	// convert static deps to dynamic deps
	// (ignore the extra refs here)
	size_t numDeps = m_StaticDependencies.GetSize() - m_ExtraRefs.GetSize();
	for ( size_t i=0; i<numDeps; ++i ) 
	{
		Node * n = m_StaticDependencies[ i ].GetNode();

		if ( n->IsAFile() )
		{
			m_DynamicDependencies.Append( Dependency( n ) );
			continue;
		}

		if ( n->GetType() == Node::DIRECTORY_LIST_NODE )
		{
			// get the list of files
			DirectoryListNode * dln = n->CastTo< DirectoryListNode >();
			const Array< FileIO::FileInfo > & files = dln->GetFiles();
			m_DynamicDependencies.SetCapacity( m_DynamicDependencies.GetSize() + files.GetSize() );
			for ( Array< FileIO::FileInfo >::Iter fIt = files.Begin();
					fIt != files.End();
					fIt++ )
			{
				// Create the file node (or find an existing one)
				Node * sn = ng.FindNode( fIt->m_Name );
				if ( sn == nullptr )
				{
					sn = ng.CreateFileNode( fIt->m_Name );
				}
				else if ( sn->IsAFile() == false )
				{
					FLOG_ERROR( "CSAssembly() .CompilerInputFile '%s' is not a FileNode (type: %s)", n->GetName().Get(), n->GetTypeName() );
					return false;
				}

				m_DynamicDependencies.Append( Dependency( sn ) );
			}
			continue;
		}

		FLOG_ERROR( "'%s' is not a supported node type (type: %s)", n->GetName().Get(), n->GetTypeName() );
		return false;
	}

	return true;
}

// DoBuild
//------------------------------------------------------------------------------
/*virtual*/ Node::BuildResult CSNode::DoBuild( Job * job )
{
	// Format compiler args string
	AStackString< 4 * KILOBYTE > fullArgs;
	GetFullArgs( fullArgs );

	// use the exe launch dir as the working dir
	const char * workingDir = nullptr;

	const char * environment = FBuild::Get().GetEnvironmentString();

	EmitCompilationMessage( fullArgs );

	// write args to response file
	ResponseFile rf;
	AStackString<> responseFileArgs;
	if ( !rf.Create( fullArgs ) )
	{
		return NODE_RESULT_FAILED; // Create will have emitted error
	}

    const bool useResponseFile = ( fullArgs.GetLength() > 32767 );
    if ( useResponseFile )
    {
	    // override args to use response file
	    responseFileArgs.Format( "@\"%s\"", rf.GetResponseFilePath().Get() );
    }

	// spawn the process
	Process p;
	if ( p.Spawn( m_CompilerPath.Get(), useResponseFile ? responseFileArgs.Get() : fullArgs.Get(),
				  workingDir, environment ) == false )
	{
		FLOG_ERROR( "Failed to spawn process to build '%s'", GetName().Get() );
		return NODE_RESULT_FAILED;
	}

	// capture all of the stdout and stderr
	AutoPtr< char > memOut;
	AutoPtr< char > memErr;
	uint32_t memOutSize = 0;
	uint32_t memErrSize = 0;
	p.ReadAllData( memOut, &memOutSize, memErr, &memErrSize );

	// Get result
	ASSERT( !p.IsRunning() );
	int result = p.WaitForExit();
	bool ok = ( result == 0 );

	if ( !ok )
	{
		// something went wrong, print details
		Node::DumpOutput( job, memOut.Get(), memOutSize );
		Node::DumpOutput( job, memErr.Get(), memErrSize );
		goto failed;
	}

	if ( !FileIO::FileExists( m_Name.Get() ) )
	{
		FLOG_ERROR( "Object missing despite success for '%s'", GetName().Get() );
		return NODE_RESULT_FAILED;
	}

	// record new file time
	m_Stamp = FileIO::GetFileLastWriteTime( m_Name );

	return NODE_RESULT_OK;

failed:
	FLOG_ERROR( "Failed to build Object (error %i) '%s'", result, GetName().Get() );

	return NODE_RESULT_FAILED;
}

// Load
//------------------------------------------------------------------------------
/*static*/ Node * CSNode::Load( IOStream & stream )
{
	NODE_LOAD( AStackString<>,	name );
	NODE_LOAD_DEPS( 2,			staticDeps );
	NODE_LOAD( AStackString<>,	compilerPath );
	NODE_LOAD( AStackString<>,	compilerArgs );
	NODE_LOAD_DEPS( 0,			extraRefs );

	ASSERT( staticDeps.GetSize() >= 1 );

	NodeGraph & ng = FBuild::Get().GetDependencyGraph();
	Node * on = ng.CreateCSNode( name, staticDeps, compilerPath, compilerArgs, extraRefs );
	CSNode * csNode = on->CastTo< CSNode >();
	return csNode;
}

// Save
//------------------------------------------------------------------------------
/*virtual*/ void CSNode::Save( IOStream & stream ) const
{
	NODE_SAVE( m_Name );

	// Only save the original static deps here (remove the extra refs)
	size_t numBaseDeps = m_StaticDependencies.GetSize() - m_ExtraRefs.GetSize();
	Dependencies staticDeps( numBaseDeps, false );
	for ( size_t i=0; i<numBaseDeps; ++i )
	{
		staticDeps.Append( Dependency( m_StaticDependencies[ i ].GetNode() ) );
	}
	NODE_SAVE_DEPS( staticDeps );

	NODE_SAVE( m_CompilerPath );
	NODE_SAVE( m_CompilerArgs );
	NODE_SAVE_DEPS( m_ExtraRefs );
}

// EmitCompilationMessage
//------------------------------------------------------------------------------
void CSNode::EmitCompilationMessage( const AString & fullArgs ) const
{
	// print basic or detailed output, depending on options
	// we combine everything into one string to ensure it is contiguous in
	// the output
	AStackString<> output;
	output += "C#: ";
	output += GetName();
	output += '\n';
	if ( FLog::ShowInfo() || FBuild::Get().GetOptions().m_ShowCommandLines )
	{
		output += m_CompilerPath;
		output += ' ';
		output += fullArgs;
		output += '\n';
	}
    FLOG_BUILD_DIRECT( output.Get() );
}

// GetFullArgs
//------------------------------------------------------------------------------
void CSNode::GetFullArgs( AString & fullArgs ) const
{
	// split into tokens
	Array< AString > tokens( 1024, true );
	m_CompilerArgs.Tokenize( tokens );

	AStackString<> quote( "\"" );

	const AString * const end = tokens.End();
	for ( const AString * it = tokens.Begin(); it!=end; ++it )
	{
		const AString & token = *it;
		if ( token.EndsWith( "%1" ) )
		{
			// handle /Option:%1 -> /Option:A /Option:B /Option:C
			AStackString<> pre;
			if ( token.GetLength() > 2 )
			{
				pre.Assign( token.Get(), token.GetEnd() - 2 );
			}

			// concatenate files, unquoted
			GetInputFiles( fullArgs, pre, AString::GetEmpty() );
		}
		else if ( token.EndsWith( "\"%1\"" ) )
		{
			// handle /Option:"%1" -> /Option:"A" /Option:"B" /Option:"C"
			AStackString<> pre( token.Get(), token.GetEnd() - 3 ); // 3 instead of 4 to include quote

			// concatenate files, quoted
			GetInputFiles( fullArgs, pre, quote );
		}
		else if ( token.EndsWith( "%2" ) )
		{
			// handle /Option:%2 -> /Option:A
			if ( token.GetLength() > 2 )
			{
				fullArgs += AStackString<>( token.Get(), token.GetEnd() - 2 );
			}
			fullArgs += m_Name;
		}
		else if ( token.EndsWith( "\"%2\"" ) )
		{
			// handle /Option:"%2" -> /Option:"A"
			AStackString<> pre( token.Get(), token.GetEnd() - 3 ); // 3 instead of 4 to include quote
			fullArgs += pre;
			fullArgs += m_Name;
			fullArgs += '"'; // post
		}
		else if ( token.EndsWith( "%3" ) )
		{
			// handle /Option:%3 -> /Option:A,B,C
			AStackString<> pre( token.Get(), token.GetEnd() - 2 );
			fullArgs += pre;

			// concatenate files, unquoted
			GetExtraRefs( fullArgs, AString::GetEmpty(), AString::GetEmpty() );
		}
		else if ( token.EndsWith( "\"%3\"" ) )
		{
			// handle /Option:"%3" -> /Option:"A","B","C"
			AStackString<> pre( token.Get(), token.GetEnd() - 4 );
			fullArgs += pre;

			// concatenate files, quoted
			GetExtraRefs( fullArgs, quote, quote );
		}
		else
		{
			fullArgs += token;
		}

		fullArgs += ' ';
	}
}

// GetInputFiles
//------------------------------------------------------------------------------
void CSNode::GetInputFiles( AString & fullArgs, const AString & pre, const AString & post ) const
{
	bool first = true;
	const Dependency * const end = m_DynamicDependencies.End();
	for ( const Dependency * it = m_DynamicDependencies.Begin();
		  it != end;
		  ++it )
	{
		if ( !first )
		{
			fullArgs += ' ';
		}
		fullArgs += pre;
		fullArgs += it->GetNode()->GetName();
		fullArgs += post;
		first = false;
	}
}

// GetExtraRefs
//------------------------------------------------------------------------------
void CSNode::GetExtraRefs( AString & fullArgs, const AString & pre, const AString & post ) const
{
	bool first = true;
	const Dependency * const end = m_ExtraRefs.End();
	for ( const Dependency * it = m_ExtraRefs.Begin(); it!=end; ++it )
	{
		if ( !first )
		{
			fullArgs += ',';
		}
		fullArgs += pre;
		fullArgs += it->GetNode()->GetName();
		fullArgs += post;
		first = false;
	}
}

//------------------------------------------------------------------------------
