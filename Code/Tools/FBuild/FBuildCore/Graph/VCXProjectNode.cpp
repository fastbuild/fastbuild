// UnityNode.cpp
//------------------------------------------------------------------------------

// Includes
//------------------------------------------------------------------------------
#include "Tools/FBuild/FBuildCore/PrecompiledHeader.h"

#include "VCXProjectNode.h"

#include "Tools/FBuild/FBuildCore/FBuild.h"
#include "Tools/FBuild/FBuildCore/FLog.h"
#include "Tools/FBuild/FBuildCore/Graph/NodeGraph.h"
#include "Tools/FBuild/FBuildCore/Graph/DirectoryListNode.h"
#include "Tools/FBuild/FBuildCore/Helpers/VSProjectGenerator.h"

// Core
#include "Core/Env/Env.h"
#include "Core/FileIO/FileIO.h"
#include "Core/FileIO/FileStream.h"
#include "Core/FileIO/PathUtils.h"
#include "Core/Process/Process.h"
#include "Core/Strings/AStackString.h"

// system
#include <string.h> // for memcmp

// CONSTRUCTOR
//------------------------------------------------------------------------------
VCXProjectNode::VCXProjectNode( const AString & projectOutput,
                                const Array< AString > & projectBasePaths,
                                const Dependencies & paths,
                                const Array< AString > & pathsToExclude,
                                const Array< AString > & patternToExclude,
                                const Array< AString > & files,
                                const Array< AString > & filesToExclude,
                                const AString & rootNamespace,
                                const AString & projectGuid,
                                const AString & defaultLanguage,
                                const AString & applicationEnvironment,
                                const bool projectSccEntrySAK,
                                const Array< VSProjectConfig > & configs,
                                const Array< VSProjectFileType > & fileTypes,
                                const Array< AString > & references,
                                const Array< AString > & projectReferences )
: FileNode( projectOutput, Node::FLAG_NONE )
, m_ProjectBasePaths( projectBasePaths )
, m_PathsToExclude( pathsToExclude )
, m_PatternToExclude( patternToExclude )
, m_Files( files )
, m_FilesToExclude( filesToExclude )
, m_RootNamespace( rootNamespace )
, m_ProjectGuid( projectGuid )
, m_DefaultLanguage( defaultLanguage )
, m_ApplicationEnvironment( applicationEnvironment )
, m_ProjectSccEntrySAK( projectSccEntrySAK )
, m_Configs( configs )
, m_FileTypes( fileTypes )
, m_References( references )
, m_ProjectReferences( projectReferences )
{
    m_LastBuildTimeMs = 100; // higher default than a file node
    m_Type = Node::VCXPROJECT_NODE;

    // depend on the input nodes
    m_StaticDependencies.Append( paths );
}

// DESTRUCTOR
//------------------------------------------------------------------------------
VCXProjectNode::~VCXProjectNode() = default;

// DoBuild
//------------------------------------------------------------------------------
/*virtual*/ Node::BuildResult VCXProjectNode::DoBuild( Job * UNUSED( job ) )
{
    VSProjectGenerator pg;
    pg.SetBasePaths( m_ProjectBasePaths );

    // get project file name only
    const char * p1 = m_Name.FindLast( NATIVE_SLASH );
    p1 = p1 ? p1 : m_Name.Get();
    AStackString<> projectName( p1 );
    pg.SetProjectName( projectName );

    // Globals
    pg.SetRootNamespace( m_RootNamespace );
    pg.SetProjectGuid( m_ProjectGuid );
    pg.SetDefaultLanguage( m_DefaultLanguage );
    pg.SetApplicationEnvironment( m_ApplicationEnvironment );
    pg.SetProjectSccEntrySAK( m_ProjectSccEntrySAK );

    // References
    pg.SetReferences( m_References );
    pg.SetProjectReferences( m_ProjectReferences );

    // files from directory listings
    Array< FileIO::FileInfo * > files( 1024, true );
    GetFiles( files );
    for ( FileIO::FileInfo ** it=files.Begin(); it!=files.End(); ++it )
    {
        const AString & fileName = ( *it )->m_Name;
        AddFile( pg, fileName );
    }

    // files explicitly listed
    for ( const AString * it=m_Files.Begin(); it!=m_Files.End(); ++it )
    {
        const AString & fileName = ( *it );
        pg.AddFile( fileName );
    }

    // .vcxproj
    const AString & project = pg.GenerateVCXProj( m_Name, m_Configs, m_FileTypes );
    if ( Save( project, m_Name ) == false )
    {
        return NODE_RESULT_FAILED; // Save will have emitted an error
    }

    // .vcxproj.filters
    const AString & filters = pg.GenerateVCXProjFilters( m_Name );
    AStackString<> filterFile( m_Name );
    filterFile += ".filters";
    if ( Save( filters, filterFile ) == false )
    {
        return NODE_RESULT_FAILED; // Save will have emitted an error
    }

    return NODE_RESULT_OK;
}

// AddFile
//------------------------------------------------------------------------------
void VCXProjectNode::AddFile( VSProjectGenerator & pg, const AString & fileName ) const
{
    const AString * const end = m_FilesToExclude.End();
    for( const AString * it=m_FilesToExclude.Begin(); it!=end; ++it )
    {
        if ( fileName.EndsWithI( *it ) )
        {
            return; // file is ignored
        }
    }

    pg.AddFile( fileName );
}

// Save
//------------------------------------------------------------------------------
bool VCXProjectNode::Save( const AString & content, const AString & fileName ) const
{
    bool needToWrite = false;

    FileStream old;
    if ( FBuild::Get().GetOptions().m_ForceCleanBuild )
    {
        needToWrite = true;
    }
    else if ( old.Open( fileName.Get(), FileStream::READ_ONLY ) == false )
    {
        needToWrite = true;
    }
    else
    {
        // files differ in size?
        size_t oldFileSize = (size_t)old.GetFileSize();
        if ( oldFileSize != content.GetLength() )
        {
            needToWrite = true;
        }
        else
        {
            // check content
            AutoPtr< char > mem( ( char *)ALLOC( oldFileSize ) );
            if ( old.Read( mem.Get(), oldFileSize ) != oldFileSize )
            {
                FLOG_ERROR( "VCXProject - Failed to read '%s'", fileName.Get() );
                return false;
            }

            // compare content
            if ( memcmp( mem.Get(), content.Get(), oldFileSize ) != 0 )
            {
                needToWrite = true;
            }
        }

        // ensure we are closed, so we can open again for write if needed
        old.Close();
    }

    // only save if missing or ner
    if ( needToWrite == false )
    {
        return true; // nothing to do.
    }

    FLOG_BUILD( "VCXProj: %s\n", fileName.Get() );

    // ensure path exists (normally handled by framework, but VCXPorject
    // is not a "file" node)
    if ( EnsurePathExistsForFile( fileName ) == false )
    {
        FLOG_ERROR( "VCXProject - Invalid path for '%s' (error: %u)", fileName.Get(), Env::GetLastErr() );
        return false;
    }

    // actually write
    FileStream f;
    if ( !f.Open( fileName.Get(), FileStream::WRITE_ONLY ) )
    {
        FLOG_ERROR( "VCXProject - Failed to open '%s' for write (error: %u)", fileName.Get(), Env::GetLastErr() );
        return false;
    }
    if ( f.Write( content.Get(), content.GetLength() ) != content.GetLength() )
    {
        FLOG_ERROR( "VCXProject - Error writing to '%s' (error: %u)", fileName.Get(), Env::GetLastErr() );
        return false;
    }
    f.Close();

    return true;
}

// Load
//------------------------------------------------------------------------------
/*static*/ Node * VCXProjectNode::Load( NodeGraph & nodeGraph, IOStream & stream )
{
    NODE_LOAD( AStackString<>,  name );
    NODE_LOAD( Array< AString >, projectBasePaths );
    NODE_LOAD_DEPS( 1,          staticDeps );
    NODE_LOAD( Array< AString >, pathsToExclude );
    NODE_LOAD( Array< AString >, patternToExclude );
    NODE_LOAD( Array< AString >, files );
    NODE_LOAD( Array< AString >, filesToExclude );
    NODE_LOAD( AStackString<>, rootNamespace );
    NODE_LOAD( AStackString<>, projectGuid );
    NODE_LOAD( AStackString<>, defaultLanguage );
    NODE_LOAD( AStackString<>, applicationEnvironment );
    NODE_LOAD( bool, projectSccEntrySAK );
    NODE_LOAD( Array< AString >, references );
    NODE_LOAD( Array< AString >, projectReferences );

    Array< VSProjectConfig > configs;
    VSProjectConfig::Load( nodeGraph, stream, configs );

    Array< VSProjectFileType > fileTypes;
    VSProjectFileType::Load( stream, fileTypes );

    VCXProjectNode * n = nodeGraph.CreateVCXProjectNode( name,
                                 projectBasePaths,
                                 staticDeps, // all static deps are DirectoryListNode
                                 pathsToExclude,
                                 patternToExclude,
                                 files,
                                 filesToExclude,
                                 rootNamespace,
                                 projectGuid,
                                 defaultLanguage,
                                 applicationEnvironment,
                                 projectSccEntrySAK,
                                 configs,
                                 fileTypes,
                                 references,
                                 projectReferences );
    return n;
}

// Save
//------------------------------------------------------------------------------
/*virtual*/ void VCXProjectNode::Save( IOStream & stream ) const
{
    NODE_SAVE( m_Name );
    NODE_SAVE( m_ProjectBasePaths );
    NODE_SAVE_DEPS( m_StaticDependencies );
    NODE_SAVE( m_PathsToExclude );
    NODE_SAVE( m_PatternToExclude );
    NODE_SAVE( m_Files );
    NODE_SAVE( m_FilesToExclude );
    NODE_SAVE( m_RootNamespace );
    NODE_SAVE( m_ProjectGuid );
    NODE_SAVE( m_DefaultLanguage );
    NODE_SAVE( m_ApplicationEnvironment );
    NODE_SAVE( m_ProjectSccEntrySAK );
    NODE_SAVE( m_References );
    NODE_SAVE( m_ProjectReferences );
    VSProjectConfig::Save( stream, m_Configs );
    VSProjectFileType::Save( stream, m_FileTypes );
}

// GetFiles
//------------------------------------------------------------------------------
void VCXProjectNode::GetFiles( Array< FileIO::FileInfo * > & files ) const
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

            // filter excluded pattern
            if ( keep )
            {
                const AString * pit = m_PatternToExclude.Begin();
                const AString * const pend = m_PatternToExclude.End();
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
}

//------------------------------------------------------------------------------
