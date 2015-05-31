// UnityNode.cpp
//------------------------------------------------------------------------------

// Includes
//------------------------------------------------------------------------------
#include "Tools/FBuild/FBuildCore/PrecompiledHeader.h"

#include "UnityNode.h"
#include "DirectoryListNode.h"

#include "Tools/FBuild/FBuildCore/BFF/Functions/Function.h" // TODO:C Remove this
#include "Tools/FBuild/FBuildCore/FBuild.h"
#include "Tools/FBuild/FBuildCore/FLog.h"
#include "Tools/FBuild/FBuildCore/Graph/NodeGraph.h"
#include "Tools/FBuild/FBuildCore/Graph/ObjectNode.h"

// Core
#include "Core/FileIO/FileIO.h"
#include "Core/FileIO/FileStream.h"
#include "Core/FileIO/PathUtils.h"
#include "Core/Process/Process.h"
#include "Core/Strings/AStackString.h"

// Reflection
//------------------------------------------------------------------------------
#ifdef USE_NODE_REFLECTION
REFLECT_BEGIN( UnityNode, Node, MetaNone() )
	REFLECT_ARRAY( m_InputPaths,		"UnityInputPath",						MetaOptional() + MetaPath() )
	REFLECT_ARRAY( m_PathsToExclude,	"UnityInputExcludePath",				MetaOptional() + MetaPath() )
	REFLECT( m_InputPathRecurse,		"UnityInputPathRecurse",				MetaOptional() )
	REFLECT( m_InputPattern,			"UnityInputPattern",					MetaOptional() )
	REFLECT_ARRAY( m_Files,				"UnityInputFiles",						MetaOptional() + MetaFile() )
	REFLECT_ARRAY( m_FilesToExclude,	"UnityInputExcludedFiles",				MetaOptional() + MetaFile( true ) ) // relative
	REFLECT_ARRAY( m_ExcludePatterns,	"UnityInputExcludePattern",				MetaOptional() + MetaFile( true ) ) // relative
	REFLECT( m_OutputPath,				"UnityOutputPath",						MetaPath() )
	REFLECT( m_OutputPattern,			"UnityOutputPattern",					MetaOptional() )
	REFLECT( m_NumUnityFilesToCreate,	"UnityNumFiles",						MetaOptional() + MetaRange( 1, 1048576 ) )
	REFLECT( m_MaxIsolatedFiles,		"UnityInputIsolateWritableFilesLimit",	MetaOptional() + MetaRange( 0, 1048576 ) )
	REFLECT( m_IsolateWritableFiles,	"UnityInputIsolateWritableFiles",		MetaOptional() )
	REFLECT( m_PrecompiledHeader,		"UnityPCH",								MetaOptional() + MetaFile( true ) ) // relative
REFLECT_END( UnityNode )
#endif

// CONSTRUCTOR
//------------------------------------------------------------------------------
#ifdef USE_NODE_REFLECTION
UnityNode::UnityNode()
: Node( AString::GetEmpty(), Node::UNITY_NODE, Node::FLAG_NONE )
, m_InputPattern( "*.cpp" )
, m_InputPathRecurse( true )
, m_Files( 0, true )
, m_OutputPath()
, m_OutputPattern( "Unity*.cpp" )
, m_NumUnityFilesToCreate( 1 )
, m_UnityFileNames( 0, true )
, m_PrecompiledHeader()
, m_PathsToExclude( 0, true )
, m_FilesToExclude( 0, true )
, m_IsolateWritableFiles( false )
, m_MaxIsolatedFiles( 0 )
, m_ExcludePatterns( 0, true )
, m_IsolatedFiles( 0, true )
{
	m_LastBuildTimeMs = 100; // higher default than a file node
}
#endif

// CONSTRUCTOR
//------------------------------------------------------------------------------
#ifndef USE_NODE_REFLECTION
UnityNode::UnityNode( const AString & unityName,
						const Dependencies & dirNodes,
						const Array< AString > & files,
						const AString & outputPath,
						const AString & outputPattern,
						uint32_t numUnityFilesToCreate,
						const AString & precompiledHeader,
						bool isolateWritableFiles,
						uint32_t maxIsolatedFiles,
						const Array< AString > & excludePatterns )
: Node( unityName, Node::UNITY_NODE, Node::FLAG_NONE )
, m_Files( files )
, m_OutputPath( outputPath )
, m_OutputPattern( outputPattern )
, m_NumUnityFilesToCreate( numUnityFilesToCreate )
, m_PrecompiledHeader( precompiledHeader )
, m_IsolateWritableFiles( isolateWritableFiles )
, m_MaxIsolatedFiles( maxIsolatedFiles )
, m_ExcludePatterns( excludePatterns )
, m_IsolatedFiles( 0, true )
, m_UnityFileNames( 0, true )
{
	m_LastBuildTimeMs = 100; // higher default than a file node

	// depend on the input nodes
	m_StaticDependencies.Append( dirNodes );

	ASSERT( m_NumUnityFilesToCreate > 0 );

	// ensure path is properly formatted
	ASSERT( m_OutputPath.EndsWith( NATIVE_SLASH ) );
}
#endif

// Initialize
//------------------------------------------------------------------------------
#ifdef USE_NODE_REFLECTION
bool UnityNode::Initialize( const BFFIterator & iter, const Function * function )
{
	if ( m_PrecompiledHeader.IsEmpty() == false )
	{
		// automatically exclude the associated CPP file for a PCH (if there is one)
		if ( m_PrecompiledHeader.EndsWithI( ".h" ) )
		{
			AStackString<> pchCPP( m_PrecompiledHeader.Get(), 
								   m_PrecompiledHeader.Get() + m_PrecompiledHeader.GetLength() - 2 );
			pchCPP += ".cpp";
			m_FilesToExclude.Append( pchCPP );
		}
	}

	Dependencies dirNodes( m_InputPaths.GetSize() );
	if ( !function->GetDirectoryListNodeList( iter, m_InputPaths, m_PathsToExclude, m_FilesToExclude, m_InputPathRecurse, m_InputPattern, "UnityInputPath", dirNodes ) )
	{
		return false; // GetDirectoryListNodeList will have emitted an error
	}
	ASSERT( m_StaticDependencies.IsEmpty() );
	m_StaticDependencies.Append( dirNodes );

	return true;
}
#endif

// DESTRUCTOR
//------------------------------------------------------------------------------
UnityNode::~UnityNode()
{
}

// DoBuild
//------------------------------------------------------------------------------
/*virtual*/ Node::BuildResult UnityNode::DoBuild( Job * UNUSED( job ) )
{
	bool hasOutputMessage = false; // print msg first time we actually save a file

	// Ensure dest path exists
	// NOTE: Normally a node doesn't need to worry about this, but because
	// UnityNode outputs files that do not match the node-name (and doesn't
	// inherit from FileNoe), we have to handle it ourselves
	// TODO:C Would be good to refactor things to avoid this special case
	if ( EnsurePathExistsForFile( m_OutputPath ) == false )
	{
		return NODE_RESULT_FAILED; // EnsurePathExistsForFile will have emitted error
	}

	m_UnityFileNames.SetCapacity( m_NumUnityFilesToCreate );

	// get the files
	Array< FileAndOrigin > files( 4096, true );

	if ( !GetFiles( files ) )
    {
        return NODE_RESULT_FAILED; // GetFiles will have emitted an error
    }

    // TODO:A Sort files for consistent ordering across file systems/platforms
    
	// how many files should go in each unity file?
	const size_t numFiles = files.GetSize();
	float numFilesPerUnity = (float)numFiles / m_NumUnityFilesToCreate;
	float remainingInThisUnity( 0.0 );

	uint32_t numFilesWritten( 0 );

	size_t index = 0;

    AString output;
    output.SetReserved( 32 * 1024 );

	// create each unity file
	for ( size_t i=0; i<m_NumUnityFilesToCreate; ++i )
	{
		// add allocation to this unity
		remainingInThisUnity += numFilesPerUnity;

		// header
		output = "// Auto-generated Unity file - do not modify\r\n\r\n";
		
		// precompiled header
		if ( !m_PrecompiledHeader.IsEmpty() )
		{
			output += "#include \"";
			output += m_PrecompiledHeader;
			output += "\"\r\n\r\n";
		}

		// make sure any remaining files are added to the last unity to account
		// for floating point imprecision

		// determine allocation of includes for this unity file
		Array< FileAndOrigin > filesInThisUnity( 256, true );
		uint32_t numIsolated( 0 );
		const bool lastUnity = ( i == ( m_NumUnityFilesToCreate - 1 ) );
		while ( ( remainingInThisUnity > 0.0f ) || lastUnity )
		{
			remainingInThisUnity -= 1.0f; // reduce allocation, but leave rounding

			// handle cases where there's more unity files than source files
			if ( index >= numFiles )
			{
				break;
			}

			filesInThisUnity.Append( files[index ] );

			// files which are modified (writable) can optionally be excluded from the unity
			if ( m_IsolateWritableFiles )
			{
				// is the file writable?
				if ( files[ index ].IsReadOnly() == false )
				{
					numIsolated++;
				}
			}

			// count the file, whether we wrote it or not, to keep unity files stable
			index++;
			numFilesWritten++;		
		}

		// write allocation of includes for this unity file
		const FileAndOrigin * const end = filesInThisUnity.End();
        size_t numFilesActuallyIsolatedInThisUnity( 0 );
		for ( const FileAndOrigin * file = filesInThisUnity.Begin(); file != end; ++file )
		{
			// write pragma showing cpp file being compiled to assist resolving compilation errors
			AStackString<> buffer( file->GetName().Get() );
			buffer.Replace( BACK_SLASH, FORWARD_SLASH ); // avoid problems with slashes in generated code
            #if defined( __LINUX__ )
                output += "//"; // TODO:LINUX - Find how to avoid GCC spamming "note:" about use of pragma
            #endif
			output += "#pragma message( \"";
			output += buffer;
			output += "\" )\r\n";

			// files which are modified (writable) can optionally be excluded from the unity
			if ( m_IsolateWritableFiles && ( ( m_MaxIsolatedFiles == 0 ) || ( numIsolated <= m_MaxIsolatedFiles ) ) )
			{
				// is the file writable?
				if ( file->IsReadOnly() == false )
				{
					// disable compilation of this file (comment it out)
					output += "//";
					m_IsolatedFiles.Append( *file );
                    numFilesActuallyIsolatedInThisUnity++;
				}
			}

			// write include
			output += "#include \"";
			output += file->GetName();
			output += "\"\r\n\r\n";
		}
		output += "\r\n";

		// generate the destination unity file name
		AStackString<> unityName( m_OutputPath );
		unityName += m_OutputPattern;
		{
			AStackString<> tmp;
			tmp.Format( "%u", i + 1 ); // number from 1
			unityName.Replace( "*", tmp.Get() );
		}

		// only keep track of non-empty unity files (to avoid link errors with empty objects)
        if ( filesInThisUnity.GetSize() != numFilesActuallyIsolatedInThisUnity )
        {
			m_UnityFileNames.Append( unityName );
        }

		// need to write the unity file?
		bool needToWrite = false;
		FileStream f;
		if ( FBuild::Get().GetOptions().m_ForceCleanBuild )
		{
			needToWrite = true; // clean build forces regeneration
		}
		else
		{
			if ( f.Open( unityName.Get(), FileStream::READ_ONLY ) )
			{
				const size_t fileSize( (size_t)f.GetFileSize() );
				if ( output.GetLength() != fileSize )
				{
					// output not the same size as the file on disc
					needToWrite = true;
				}
				else
				{
					// files the same size - are the contents the same?
					AutoPtr< char > mem( (char *)ALLOC( fileSize ) );
					if ( f.Read( mem.Get(), fileSize ) != fileSize )
					{
						// problem reading file - try to write it again
						needToWrite = true;
					}
					else
					{
						if ( AString::StrNCmp( mem.Get(), output.Get(), fileSize ) != 0 )
						{
							// contents differ
							needToWrite = true;
						}
					}
				}
				f.Close();
			}
			else
			{
				// file missing - must create
				needToWrite = true;
			}
		}

		// needs updating?
		if ( needToWrite )
		{
			if ( hasOutputMessage == false )
			{
				FLOG_BUILD( "Uni: %s\n", GetName().Get() );
				hasOutputMessage = true;
			}

			if ( f.Open( unityName.Get(), FileStream::WRITE_ONLY ) == false )
			{
				FLOG_ERROR( "Failed to create Unity file '%s'", unityName.Get() );
				return NODE_RESULT_FAILED;
			}

			if ( f.Write( output.Get(), output.GetLength() ) != output.GetLength() )
			{
				FLOG_ERROR( "Error writing Unity file '%s'", unityName.Get() );
				return NODE_RESULT_FAILED;
			}

			f.Close();
		}
	}

	// Sanity check that all files were written
	ASSERT( numFilesWritten == numFiles );

	return NODE_RESULT_OK;
}

// Load
//------------------------------------------------------------------------------
/*static*/ Node * UnityNode::Load( IOStream & stream )
{
#ifdef USE_NODE_REFLECTION
	NODE_LOAD( AStackString<>,	name );

	NodeGraph & ng = FBuild::Get().GetDependencyGraph();
	UnityNode * un = ng.CreateUnityNode( name );

	if ( un->Deserialize( stream ) == false )
	{
		return nullptr;
	}

	return un;
#else
	NODE_LOAD( AStackString<>,	name );
	NODE_LOAD( AStackString<>,	outputPath );
	NODE_LOAD( AStackString<>,	outputPattern );
	NODE_LOAD( uint32_t,		numFiles );
	NODE_LOAD_DEPS( 1,			staticDeps );
	NODE_LOAD( Array< AString >, files )
	NODE_LOAD( AStackString<>,	precompiledHeader );
	NODE_LOAD( bool,			isolateWritableFiles );
	NODE_LOAD( uint32_t,		maxIsolatedFiles );
	NODE_LOAD( Array< AString >, excludePatterns );

	NodeGraph & ng = FBuild::Get().GetDependencyGraph();
	UnityNode * n = ng.CreateUnityNode( name, 
								 staticDeps, // all static deps are DirectoryListNode
								 files,
								 outputPath, 
								 outputPattern, 
								 numFiles,
								 precompiledHeader,
								 isolateWritableFiles,
								 maxIsolatedFiles,
								 excludePatterns );
	return n;
#endif
}

// Save
//------------------------------------------------------------------------------
/*virtual*/ void UnityNode::Save( IOStream & stream ) const
{
#ifdef USE_NODE_REFLECTION
	NODE_SAVE( m_Name );
	Node::Serialize( stream );
#else
	NODE_SAVE( m_Name );
	NODE_SAVE( m_OutputPath );
	NODE_SAVE( m_OutputPattern );
	NODE_SAVE( m_NumUnityFilesToCreate );
	NODE_SAVE_DEPS( m_StaticDependencies );
	NODE_SAVE( m_Files );
	NODE_SAVE( m_PrecompiledHeader );
	NODE_SAVE( m_IsolateWritableFiles );
	NODE_SAVE( m_MaxIsolatedFiles );
	NODE_SAVE( m_ExcludePatterns );
#endif
}

// GetFiles
//------------------------------------------------------------------------------
bool UnityNode::GetFiles( Array< FileAndOrigin > & files )
{
	// find all the directory lists
	const Dependency * const sEnd = m_StaticDependencies.End();
	for ( const Dependency * sIt = m_StaticDependencies.Begin(); sIt != sEnd; ++sIt )
	{
		DirectoryListNode * dirNode = sIt->GetNode()->CastTo< DirectoryListNode >();
		const FileIO::FileInfo * const filesEnd = dirNode->GetFiles().End();

		// filter files in the dir list
		for ( FileIO::FileInfo * filesIt = dirNode->GetFiles().Begin(); filesIt != filesEnd; ++filesIt )
		{
			bool keep = true;

			// filter patterns
			const AString * pit = m_ExcludePatterns.Begin();
			const AString * const pend = m_ExcludePatterns.End();
			for ( ; pit != pend; ++pit )
			{
				if ( PathUtils::IsWildcardMatch( pit->Get(), filesIt->m_Name.Get() ) )
				{
					keep = false;
					break;
				}
			}

			if ( keep )
			{
				files.Append( FileAndOrigin( filesIt, dirNode ) );
			}
		}
	}

	// explicitly listed files
    bool ok = true;
	size_t numFiles = m_Files.GetSize();
	if ( numFiles )
	{
		// presize to the worst case so we can safely keep 
		// pointers into this array
		m_FilesInfo.SetSize( numFiles );
		for ( size_t i=0; i<numFiles; ++i )
		{
			// obtain file info for this file so we can handle
			// writable files			
			if ( FileIO::GetFileInfo( m_Files[ i ], m_FilesInfo[ i ] ) )
			{
				// only add files that exist
				files.Append( FileAndOrigin( &m_FilesInfo[ i ], nullptr ) );
			}
            else
            {
                FLOG_ERROR( "FBuild: Error: Unity missing file: '%s'\n", m_Files[ i ].Get() );
                ok = false;
            }
		}
	}

    return ok;
}

//------------------------------------------------------------------------------
