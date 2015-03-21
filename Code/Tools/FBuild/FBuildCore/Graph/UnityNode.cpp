// UnityNode.cpp
//------------------------------------------------------------------------------

// Includes
//------------------------------------------------------------------------------
#include "Tools/FBuild/FBuildCore/PrecompiledHeader.h"

#include "UnityNode.h"
#include "DirectoryListNode.h"

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

// CONSTRUCTOR
//------------------------------------------------------------------------------
UnityNode::UnityNode( const AString & unityName,
						const Dependencies & dirNodes,
						const Array< AString > & files,
						const AString & outputPath,
						const AString & outputPattern,
						uint32_t numUnityFilesToCreate,
						const AString & precompiledHeader,
						const Array< AString > & pathsToExclude,
						const Array< AString > & filesToExclude,
						bool isolateWritableFiles,
						uint32_t maxIsolatedFiles,
						const Array< AString > & excludePatterns )
: Node( unityName, Node::UNITY_NODE, Node::FLAG_NONE )
, m_Files( files )
, m_OutputPath( outputPath )
, m_OutputPattern( outputPattern )
, m_NumUnityFilesToCreate( numUnityFilesToCreate )
, m_UnityFileNames( numUnityFilesToCreate, false )
, m_PrecompiledHeader( precompiledHeader )
, m_PathsToExclude( pathsToExclude )
, m_FilesToExclude( filesToExclude )
, m_IsolateWritableFiles( isolateWritableFiles )
, m_MaxIsolatedFiles( maxIsolatedFiles )
, m_ExcludePatterns( excludePatterns )
, m_IsolatedFiles( 0, true )
{
	m_LastBuildTimeMs = 100; // higher default than a file node

	// depend on the input nodes
	m_StaticDependencies.Append( dirNodes );

	ASSERT( m_NumUnityFilesToCreate > 0 );

	// ensure path is properly formatted
	ASSERT( m_OutputPath.EndsWith( NATIVE_SLASH ) );

	// generate the destination unity file names
	// TODO:C move this processing to the FunctionUnity
	AStackString<> tmp;
	for ( size_t i=0; i< m_NumUnityFilesToCreate; ++i )
	{
		tmp.Format( "%u", i + 1 ); // number from 1

		AStackString<> unityFileName( m_OutputPath );
		unityFileName += m_OutputPattern;
		unityFileName.Replace( "*", tmp.Get() );

		m_UnityFileNames.Append( unityFileName );
	}
}

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
		return NODE_RESULT_FAILED;
	}

	// get the files
	Array< FileIO::FileInfo * > files( 4096, true );
	GetFiles( files );

    // TODO:A Sort files for consistent ordering across file systems/platforms
    
	// how many files should go in each unity file?
	const size_t numFiles = files.GetSize();
	float numFilesPerUnity = (float)numFiles / m_NumUnityFilesToCreate;
	float remainingInThisUnity( 0.0 );

	uint32_t numFilesWritten( 0 );

	size_t index = 0;


	// create each unity file
	for ( size_t i=0; i<m_NumUnityFilesToCreate; ++i )
	{
		// add allocation to this unity
		remainingInThisUnity += numFilesPerUnity;

		// header
		AStackString<4096> output( "// Auto-generated Unity file - do not modify\r\n\r\n" );
		
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
		Array< FileIO::FileInfo * > filesInThisUnity( 256, true );
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
				if ( files[ index ]->IsReadOnly() == false )
				{
					numIsolated++;
				}
			}

			// count the file, whether we wrote it or not, to keep unity files stable
			index++;
			numFilesWritten++;		
		}

		// write allocation of includes for this unity file
		const FileIO::FileInfo * const * end = filesInThisUnity.End();
		for ( FileIO::FileInfo ** it = filesInThisUnity.Begin(); it != end; ++it )
		{
			const FileIO::FileInfo * file = *it;

			// write pragma showing cpp file being compiled to assist resolving compilation errors
			AStackString<> buffer( file->m_Name.Get() );
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
					m_IsolatedFiles.Append( file->m_Name );
				}
			}

			// write include
			output += "#include \"";
			output += file->m_Name;
			output += "\"\r\n\r\n";
		}
		output += "\r\n";

		// generate the destination unity file name
		const AString & unityName = m_UnityFileNames[ i ];

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
	NODE_LOAD( AStackString<>,	name );
	NODE_LOAD( AStackString<>,	outputPath );
	NODE_LOAD( AStackString<>,	outputPattern );
	NODE_LOAD( uint32_t,		numFiles );
	NODE_LOAD_DEPS( 1,			staticDeps );
	NODE_LOAD( Array< AString >, files )
	NODE_LOAD( AStackString<>,	precompiledHeader );
	NODE_LOAD( Array< AString >, pathsToExclude );
	NODE_LOAD( Array< AString >, filesToExclude );
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
								 pathsToExclude,
								 filesToExclude,
								 isolateWritableFiles,
								 maxIsolatedFiles,
								 excludePatterns );
	return n;
}

// Save
//------------------------------------------------------------------------------
/*virtual*/ void UnityNode::Save( IOStream & stream ) const
{
	NODE_SAVE( m_Name );
	NODE_SAVE( m_OutputPath );
	NODE_SAVE( m_OutputPattern );
	NODE_SAVE( m_NumUnityFilesToCreate );
	NODE_SAVE_DEPS( m_StaticDependencies );
	NODE_SAVE( m_Files );
	NODE_SAVE( m_PrecompiledHeader );
	NODE_SAVE( m_PathsToExclude );
	NODE_SAVE( m_FilesToExclude );
	NODE_SAVE( m_IsolateWritableFiles );
	NODE_SAVE( m_MaxIsolatedFiles );
	NODE_SAVE( m_ExcludePatterns );
}

// GetFiles
//------------------------------------------------------------------------------
void UnityNode::GetFiles( Array< FileIO::FileInfo * > & files )
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

			// filter excluded files
			const AString * fit = m_FilesToExclude.Begin();
			const AString * const fend = m_FilesToExclude.End();
			for ( ; fit != fend; ++fit )
			{
				if ( filesIt->m_Name.EndsWithI( *fit ) )
				{
					keep = false;
					break;
				}
			}

			// filter excluded directories
			if ( keep )
			{
				const AString * pit = m_PathsToExclude.Begin();
				const AString * const pend = m_PathsToExclude.End();
				for ( ; pit != pend; ++pit )
				{
					if ( filesIt->m_Name.BeginsWithI( *pit ) )
					{
						keep = false;
						break;
					}
				}
			}

			// filter patterns
			if ( keep )
			{
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
			}

			if ( keep )
			{
				files.Append( filesIt );
			}
		}
	}

	// explicitly listed files
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
				files.Append( &m_FilesInfo[ i ] );
			}
		}
	}
}

//------------------------------------------------------------------------------
