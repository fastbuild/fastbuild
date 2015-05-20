// DirectoryListNode.cpp
//------------------------------------------------------------------------------

// Includes
//------------------------------------------------------------------------------
#include "Tools/FBuild/FBuildCore/PrecompiledHeader.h"

#include "DirectoryListNode.h"

#include "Tools/FBuild/FBuildCore/FBuild.h"
#include "Tools/FBuild/FBuildCore/FLog.h"
#include "Tools/FBuild/FBuildCore/Graph/NodeGraph.h"

// Core
#include "Core/FileIO/FileIO.h"
#include "Core/FileIO/FileStream.h"
#include "Core/FileIO/PathUtils.h"
#include "Core/Strings/AStackString.h"

// CONSTRUCTOR
//------------------------------------------------------------------------------
DirectoryListNode::DirectoryListNode( const AString & name,
									  const AString & path,
									  const AString & wildcard,
								      bool recursive,
									  const Array< AString > & excludePaths,
                                      const Array< AString > & filesToExclude )
: Node( name, Node::DIRECTORY_LIST_NODE, Node::FLAG_NONE )
	, m_Path( path )
	, m_WildCard( wildcard )
	, m_ExcludePaths( excludePaths )
    , m_FilesToExclude( filesToExclude )
	, m_Recursive( recursive )
	, m_Files( 4096, true )
{
	// ensure name is correctly formatted
	//   path|wildcard|recursive|[excludePath]
	ASSERT( name.BeginsWith( path ) );
	ASSERT( name[ path.GetLength() ] == '|' );
	ASSERT( name.Find( wildcard.Get() ) == name.Get() + path.GetLength() + 1 );
	ASSERT( ( recursive && name.Find( "|true|" ) ) || 
			( !recursive && name.Find( "|false|" ) ) );

	// paths must have trailing slash
	ASSERT( path.EndsWith( NATIVE_SLASH ) );

	// make sure exclusion path has trailing slash if provided
	#ifdef DEBUG
		const AString * const end = excludePaths.End();
		for ( const AString * it=excludePaths.Begin(); it != end; ++it )
		{
			ASSERT( ( *it ).EndsWith( NATIVE_SLASH ) );
		}
	#endif
}

// DESTRUCTOR
//------------------------------------------------------------------------------
DirectoryListNode::~DirectoryListNode()
{
}

// FormatName
//------------------------------------------------------------------------------
/*static*/ void DirectoryListNode::FormatName( const AString & path,
											   const AString & pattern,
											   bool recursive,
											   const Array< AString > & excludePaths,
                                               const Array< AString > & excludeFiles,
											   AString & result )
{
	ASSERT( path.EndsWith( NATIVE_SLASH ) );
	ASSERT( pattern.IsEmpty() == false );

	result.Format( "%s|%s|%s|", path.Get(),
								  pattern.Get(),
								  recursive ? "true" : "false" );

	const AString * const end = excludePaths.End();
	for ( const AString * it = excludePaths.Begin(); it!=end; ++it )
	{
		const AString & excludePath = *it;
		ASSERT( excludePath.EndsWith( NATIVE_SLASH ) );
		result += excludePath;
		result += '<';
	}

    if ( !excludeFiles.IsEmpty() )
    {
        result += '|';
        const AString * const endFiles = excludeFiles.End();
        for ( const AString * itFiles = excludeFiles.Begin(); itFiles != endFiles; ++itFiles )
        {
            const AString & excludedFile = *itFiles;
            result += excludedFile;
            result += '<';
        }
    }
}

// DoBuild
//------------------------------------------------------------------------------
/*virtual*/ Node::BuildResult DirectoryListNode::DoBuild( Job * UNUSED( job ) )
{
	// NOTE: The DirectoryListNode makes no assumptions about whether no files
	// is an error or not.  That's up to the dependent nodes to decide.

	Array< FileIO::FileInfo > files( 4096, true );
	FileIO::GetFilesEx( m_Path, m_WildCard, m_Recursive, &files );

	m_Files.SetCapacity( files.GetSize() );

	// filter exclusions
	const FileIO::FileInfo * const end = files.End();
	for ( const FileIO::FileInfo * it = files.Begin(); it != end; it++ )
	{
		bool excluded = false;
        
		// filter excluded paths
		const AString * const eEnd = m_ExcludePaths.End();
		for ( const AString * eIt=m_ExcludePaths.Begin(); eIt != eEnd; ++eIt )
		{
			if ( PathUtils::PathBeginsWith( it->m_Name, *eIt ) )
			{
				excluded = true;
				break;
			}
		}

        // filter excluded files
		if ( !excluded )
		{
	        const AString * fit = m_FilesToExclude.Begin();
	        const AString * const fend = m_FilesToExclude.End();
	        for ( ; fit != fend; ++fit )
	        {
				if ( PathUtils::PathEndsWithFile( it->m_Name, *fit ) )
	            {
	                excluded = true;
	                break;
	            }
	        }
		}

		if ( !excluded )
		{
			m_Files.Append( *it );
		}
	}

	if ( FLog::ShowInfo() )
	{
		const size_t numFiles = m_Files.GetSize();
		FLOG_INFO( "Dir: '%s' (found %u files)\n", 
							m_Name.Get(), 
							(uint32_t)numFiles);
		for ( size_t i=0; i<numFiles; ++i )
		{
			FLOG_INFO( " - %s\n", m_Files[ i ].m_Name.Get() );
		}
	}

	return NODE_RESULT_OK;
}

// Load
//------------------------------------------------------------------------------
/*static*/ Node * DirectoryListNode::Load( IOStream & stream )
{
	NODE_LOAD( AStackString<>,	name );
	NODE_LOAD( AStackString<>,	path );
	NODE_LOAD( AStackString<>,	wildCard );
	NODE_LOAD( Array< AString >,excludePaths );
	NODE_LOAD( bool,			recursive );
    NODE_LOAD( Array< AString >, filesToExclude );


	NodeGraph & ng = FBuild::Get().GetDependencyGraph();
	Node * n = ng.CreateDirectoryListNode( name, path, wildCard, recursive, excludePaths, filesToExclude );
	ASSERT( n );
	return n;
}

// Save
//------------------------------------------------------------------------------
/*virtual*/ void DirectoryListNode::Save( IOStream & stream ) const
{
	NODE_SAVE( m_Name );
	NODE_SAVE( m_Path );
	NODE_SAVE( m_WildCard );
	NODE_SAVE( m_ExcludePaths );
	NODE_SAVE( m_Recursive );
    NODE_SAVE( m_FilesToExclude );
}

//------------------------------------------------------------------------------
