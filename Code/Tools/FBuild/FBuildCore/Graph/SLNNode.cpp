// SLNNode.cpp
//------------------------------------------------------------------------------

// Includes
//------------------------------------------------------------------------------
#include "Tools/FBuild/FBuildCore/PrecompiledHeader.h"

#include "SLNNode.h"

#include "Tools/FBuild/FBuildCore/FBuild.h"
#include "Tools/FBuild/FBuildCore/FLog.h"
#include "Tools/FBuild/FBuildCore/Graph/NodeGraph.h"
#include "Tools/FBuild/FBuildCore/Graph/VCXProjectNode.h"
#include "Tools/FBuild/FBuildCore/Helpers/SLNGenerator.h"
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
SLNNode::SLNNode( const AString & solutionOuput,
                  const AString & solutionBuildProject,
                  const AString & solutionVisualStudioVersion,
                  const AString & solutionMinimumVisualStudioVersion,
                  const Array< VSProjectConfig > & configs,
                  const Array< VCXProjectNode * > & projects,
                  const Array< SLNDependency > & slnDeps,
                  const Array< SLNSolutionFolder > & folders )
: FileNode( solutionOuput, Node::FLAG_NONE )
, m_SolutionBuildProject( solutionBuildProject )
, m_SolutionVisualStudioVersion( solutionVisualStudioVersion )
, m_SolutionMinimumVisualStudioVersion( solutionMinimumVisualStudioVersion )
, m_Configs( configs )
, m_SolutionDeps( slnDeps )
, m_Folders( folders )
{
    m_LastBuildTimeMs = 100; // higher default than a file node
    m_Type = Node::SLN_NODE;

    // depend on the input nodes
    VCXProjectNode * const * projectsEnd = projects.End();
    for( VCXProjectNode ** it = projects.Begin() ; it != projectsEnd ; ++it )
    {
        m_StaticDependencies.Append( Dependency( *it ) );
    }
}

// DESTRUCTOR
//------------------------------------------------------------------------------
SLNNode::~SLNNode() = default;

// DoBuild
//------------------------------------------------------------------------------
/*virtual*/ Node::BuildResult SLNNode::DoBuild( Job * UNUSED( job ) )
{
    SLNGenerator sg;

    // projects
    Array< VCXProjectNode * > projects( m_StaticDependencies.GetSize(), false );
    const Dependency * const end = m_StaticDependencies.End();
    for ( const Dependency * it = m_StaticDependencies.Begin() ; it != end ; ++it )
    {
        projects.Append( it->GetNode()->CastTo< VCXProjectNode >() );
    }

    // .sln solution file
    const AString & sln = sg.GenerateSLN(   m_Name,
                                            m_SolutionBuildProject,
                                            m_SolutionVisualStudioVersion,
                                            m_SolutionMinimumVisualStudioVersion,
                                            m_Configs,
                                            projects,
                                            m_SolutionDeps,
                                            m_Folders );
    if ( Save( sln, m_Name ) == false )
    {
        return NODE_RESULT_FAILED; // Save will have emitted an error
    }

    return NODE_RESULT_OK;
}

// Save
//------------------------------------------------------------------------------
bool SLNNode::Save( const AString & content, const AString & fileName ) const
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
                FLOG_ERROR( "SLN - Failed to read '%s'", fileName.Get() );
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

    FLOG_BUILD( "SLN: %s\n", fileName.Get() );

    // actually write
    FileStream f;
    if ( !f.Open( fileName.Get(), FileStream::WRITE_ONLY ) )
    {
        FLOG_ERROR( "SLN - Failed to open '%s' for write (error: %u)", fileName.Get(), Env::GetLastErr() );
        return false;
    }
    if ( f.Write( content.Get(), content.GetLength() ) != content.GetLength() )
    {
        FLOG_ERROR( "SLN - Error writing to '%s' (error: %u)", fileName.Get(), Env::GetLastErr() );
        return false;
    }
    f.Close();

    return true;
}

// Load
//------------------------------------------------------------------------------
/*static*/ Node * SLNNode::Load( NodeGraph & nodeGraph, IOStream & stream )
{
    NODE_LOAD( AStackString<>,  name );
    NODE_LOAD( AStackString<>,  buildProject );
    NODE_LOAD( AStackString<>,  visualStudioVersion );
    NODE_LOAD( AStackString<>,  minimumVisualStudioVersion );
    NODE_LOAD_DEPS( 1,          staticDeps );

    Array< VSProjectConfig > configs;
    VSProjectConfig::Load( nodeGraph, stream, configs );

    Array< SLNSolutionFolder > folders;
    SLNSolutionFolder::Load( stream, folders );

    Array< SLNDependency > slnDeps;
    SLNDependency::Load( stream, slnDeps );

    Array< VCXProjectNode * > projects( staticDeps.GetSize(), false );
    const Dependency * const end = staticDeps.End();
    for ( const Dependency * it = staticDeps.Begin() ; it != end ; ++it )
    {
        projects.Append( it->GetNode()->CastTo< VCXProjectNode >() );
    }

    SLNNode * n = nodeGraph.CreateSLNNode( name,
                                           buildProject,
                                           visualStudioVersion,
                                           minimumVisualStudioVersion,
                                           configs,
                                           projects,
                                           slnDeps,
                                           folders );
    return n;
}

// Save
//------------------------------------------------------------------------------
/*virtual*/ void SLNNode::Save( IOStream & stream ) const
{
    NODE_SAVE( m_Name );
    NODE_SAVE( m_SolutionBuildProject );
    NODE_SAVE( m_SolutionVisualStudioVersion );
    NODE_SAVE( m_SolutionMinimumVisualStudioVersion );
    NODE_SAVE_DEPS( m_StaticDependencies );
    VSProjectConfig::Save( stream, m_Configs );
    SLNSolutionFolder::Save( stream, m_Folders );
    SLNDependency::Save( stream, m_SolutionDeps );
}

//------------------------------------------------------------------------------
