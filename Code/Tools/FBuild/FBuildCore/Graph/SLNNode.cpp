// SLNNode.cpp
//------------------------------------------------------------------------------

// Includes
//------------------------------------------------------------------------------
#include "Tools/FBuild/FBuildCore/PrecompiledHeader.h"

#include "SLNNode.h"

#include "Tools/FBuild/FBuildCore/Error.h"
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

// Reflection
//------------------------------------------------------------------------------
REFLECT_STRUCT_BEGIN_BASE( SolutionConfig )
    REFLECT(        m_SolutionPlatform,                     "SolutionPlatform",                         MetaOptional() )
    REFLECT(        m_SolutionConfig,                       "SolutionConfig",                           MetaOptional() )
    REFLECT(        m_Platform,                             "Platform",                                 MetaNone() )
    REFLECT(        m_Config,                               "Config",                                   MetaNone() )
REFLECT_END( SolutionConfig )

REFLECT_STRUCT_BEGIN_BASE( SolutionFolder )
    REFLECT(        m_Path,                                 "Path",                                     MetaNone() )
    REFLECT_ARRAY(  m_Projects,                             "Projects",                                 MetaFile() )
REFLECT_END( SolutionFolder )

REFLECT_STRUCT_BEGIN_BASE( SolutionDependency )
    REFLECT_ARRAY(  m_Projects,                             "Projects",                                 MetaFile() )
    REFLECT_ARRAY(  m_Dependencies,                         "Dependencies",                             MetaFile() )
REFLECT_END( SolutionDependency )

REFLECT_NODE_BEGIN( SLNNode, Node, MetaName( "SolutionOutput" ) + MetaFile() )
    REFLECT_ARRAY(  m_SolutionProjects,                     "SolutionProjects",                         MetaOptional() + MetaFile() )
    REFLECT_ARRAY(  m_SolutionBuildProjects,                "SolutionBuildProject",                     MetaOptional() + MetaFile() ) // "SolutionBuildProject" for backwards compat
    REFLECT(        m_SolutionVisualStudioVersion,          "SolutionVisualStudioVersion",              MetaOptional() )
    REFLECT(        m_SolutionMinimumVisualStudioVersion,   "SolutionMinimumVisualStudioVersion",       MetaOptional() )
    REFLECT_ARRAY_OF_STRUCT( m_SolutionConfigs,             "SolutionConfigs",      SolutionConfig,     MetaOptional() )
    REFLECT_ARRAY_OF_STRUCT( m_SolutionFolders,             "SolutionFolders",      SolutionFolder,     MetaOptional() )
    REFLECT_ARRAY_OF_STRUCT( m_SolutionDependencies,        "SolutionDependencies", SolutionDependency, MetaOptional() )
REFLECT_END( SLNNode )

// VCXProjectNodeComp
//------------------------------------------------------------------------------
struct VCXProjectNodeComp
{
    bool operator ()( const VCXProjectNode * a, const VCXProjectNode * b ) const
    {
        return ( a->GetName() < b->GetName() );
    }
};

// CONSTRUCTOR
//------------------------------------------------------------------------------
SLNNode::SLNNode()
    : FileNode( AString::GetEmpty(), Node::FLAG_NONE )
{
    m_LastBuildTimeMs = 100; // higher default than a file node
    m_Type = Node::SLN_NODE;
}

// Initialize
//------------------------------------------------------------------------------
bool SLNNode::Initialize( NodeGraph & nodeGraph, const BFFIterator & iter, const Function * function )
{
    // Solution Configs
    //------------------------------------------------------------------------------
    // Create default SolutionConfigs if not specified
    if ( m_SolutionConfigs.IsEmpty() )
    {
        m_SolutionConfigs.SetSize( 4 );
        m_SolutionConfigs[ 0 ].m_Platform = "Win32";    m_SolutionConfigs[ 0 ].m_Config = "Debug";
        m_SolutionConfigs[ 1 ].m_Platform = "Win32";    m_SolutionConfigs[ 1 ].m_Config = "Release";
        m_SolutionConfigs[ 2 ].m_Platform = "x64";      m_SolutionConfigs[ 2 ].m_Config = "Debug";
        m_SolutionConfigs[ 3 ].m_Platform = "x64";      m_SolutionConfigs[ 3 ].m_Config = "Release";
    }

    // Auto-populate SolutionPlatform and SolutionConfig
    for ( SolutionConfig & solutionConfig : m_SolutionConfigs )
    {
        // SolutionConfig is the same as Config if not specified
        if ( solutionConfig.m_SolutionConfig.IsEmpty() )
        {
            solutionConfig.m_SolutionConfig = solutionConfig.m_Config;
        }
        // SolutionPlatform is the same as Platform if not specified
        if ( solutionConfig.m_SolutionPlatform.IsEmpty() )
        {
            solutionConfig.m_SolutionPlatform = solutionConfig.m_Platform;
        }

        // Handle inconsistency between "Win32" and "x86" for SolutionPlatform
        //  - i.e. for 64bits, Platform and SolutionPlatform are both "x64"
        //         but for 32 bits, Platform is "Win32" while SolutionPlatform is "x86"
        if ( solutionConfig.m_SolutionPlatform.MatchesI( "Win32" ) )
        {
             solutionConfig.m_SolutionPlatform = "x86";
        }
    }

    // sort project configs by config and by platform (like Visual Studio)
    m_SolutionConfigs.Sort();

    // Canonicalize SolutionFolder paths
    for ( SolutionFolder & solutionFolder : m_SolutionFolders )
    {
        solutionFolder.m_Path.Replace( FORWARD_SLASH, BACK_SLASH ); // Always use Windows-style
        if ( solutionFolder.m_Path.EndsWith( BACK_SLASH ) )
        {
            solutionFolder.m_Path.Trim( 0, 1 ); // Remove trailing slash
        }
    }

    // Collapse duplicate SolutionFolders
    Array< SolutionFolder > collapsedFolders;
    for ( SolutionFolder & folder : m_SolutionFolders )
    {
        // Have we already seen this folder?
        SolutionFolder * found = nullptr;
        for ( SolutionFolder & collapsedFolder : collapsedFolders )
        {
            if ( collapsedFolder.m_Path == folder.m_Path )
            {
                found = &collapsedFolder;
                break;
            }
        }

        if ( found )
        {
            // Merge list of projects
            found->m_Projects.Append( folder.m_Projects );
        }
        else
        {
            // Add new entry
            collapsedFolders.Append( SolutionFolder( folder ) );
        }
    }
    m_SolutionFolders.Swap( collapsedFolders );

    // Gather all Project references and canonicalize project names
    //------------------------------------------------------------------------------
    Array< VCXProjectNode * > projects( m_SolutionProjects.GetSize(), true );
    // SolutionProjects
    if ( !GatherProjects( nodeGraph, function, iter, ".SolutionProjects", m_SolutionProjects, projects ) )
    {
        return false; // MergeProjects will have emitted an error
    }
    // SolutionFolders
    for ( SolutionFolder & solutionFolder : m_SolutionFolders )
    {
        if ( !GatherProjects( nodeGraph, function, iter, "SolutionFolders", solutionFolder.m_Projects, projects ) )
        {
            return false; // MergeProjects will have emitted an error
        }
    }
    // SolutionDependencies
    for ( SolutionDependency & solutionDependency : m_SolutionDependencies )
    {
        if ( !GatherProjects( nodeGraph, function, iter, "SolutionDependencies", solutionDependency.m_Projects, projects ) )
        {
            return false; // MergeProjects will have emitted an error
        }
        if ( !GatherProjects( nodeGraph, function, iter, "SolutionDependencies", solutionDependency.m_Dependencies, projects ) )
        {
            return false; // MergeProjects will have emitted an error
        }
    }
    // SolutionBuildProjects
    if ( !GatherProjects( nodeGraph, function, iter, "SolutionBuildProject", m_SolutionBuildProjects, projects ) )
    {
        return false; // MergeProjects will have emitted an error
    }

    // Sort projects by name (like Visual Studio)
    projects.Sort( VCXProjectNodeComp() );

    // Check Project Configurations
    //------------------------------------------------------------------------------
    for ( const VCXProjectNode * project : projects )
    {
        // check that this Project contains all .SolutionConfigs
        for ( const SolutionConfig & solutionConfig : m_SolutionConfigs )
        {
            bool containsConfig = false;
            for ( const VSProjectConfig & projectConfig : project->GetConfigs() )
            {
                if ( ( projectConfig.m_Platform == solutionConfig.m_Platform ) &&
                     ( projectConfig.m_Config == solutionConfig.m_Config ) )
                {
                    containsConfig = true;
                    break;
                }
            }

            if ( containsConfig == false )
            {
                // TODO: specific error message "ProjectConfigNotFound"
                AStackString<> configName;
                configName.Format( "%s|%s", solutionConfig.m_Platform.Get(), solutionConfig.m_Config.Get() );
                Error::Error_1104_TargetNotDefined( iter, function, configName.Get(), project->GetName() );
                return false;
            }
        }
    }

    // Manage dependencies
    //------------------------------------------------------------------------------
    m_StaticDependencies.SetCapacity( projects.GetSize() );
    for ( VCXProjectNode * project : projects )
    {
        m_StaticDependencies.Append( Dependency( project ) );
    }

    return true;
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
                                            m_SolutionBuildProjects,
                                            m_SolutionVisualStudioVersion,
                                            m_SolutionMinimumVisualStudioVersion,
                                            m_SolutionConfigs,
                                            projects,
                                            m_SolutionDependencies,
                                            m_SolutionFolders );
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
    NODE_LOAD( AStackString<>, name );

    SLNNode * sn = nodeGraph.CreateSLNNode( name );

    if ( sn->Deserialize( nodeGraph, stream ) == false )
    {
        return nullptr;
    }
    return sn;
}

// Save
//------------------------------------------------------------------------------
/*virtual*/ void SLNNode::Save( IOStream & stream ) const
{
    NODE_SAVE( m_Name );
    Node::Serialize( stream );
}

// GatherProject
//------------------------------------------------------------------------------
bool SLNNode::GatherProject( NodeGraph & nodeGraph,
                             const Function * function,
                             const BFFIterator & iter,
                             const char * propertyName,
                             const AString & projectName,
                             Array< VCXProjectNode * > & inOutProjects ) const
{
    // Get associated project file
    Node * node = nodeGraph.FindNode( projectName );
    if ( node == nullptr )
    {
        Error::Error_1104_TargetNotDefined( iter, function, propertyName, projectName );
        return nullptr;
    }
    if ( node->GetType() != Node::VCXPROJECT_NODE )
    {
        // don't know how to handle this type of node
        Error::Error_1005_UnsupportedNodeType( iter, function, propertyName, node->GetName(), node->GetType() );
        return nullptr;
    }
    VCXProjectNode * projectNode = node->CastTo< VCXProjectNode >();

    // Add to project list if not already there
    if ( inOutProjects.Find( projectNode ) == nullptr )
    {
        inOutProjects.Append( projectNode );
    }

    return true;
}

// GatherProjects
//------------------------------------------------------------------------------
bool SLNNode::GatherProjects( NodeGraph & nodeGraph,
                              const Function * function,
                              const BFFIterator & iter,
                              const char * propertyName,
                              const Array< AString > & projectNames,
                              Array< VCXProjectNode * > & inOutProjects ) const
{
    for ( const AString & projectName : projectNames )
    {
        if ( GatherProject( nodeGraph, function, iter, propertyName, projectName, inOutProjects ) == false )
        {
            return false;
        }
    }
    return true;
}

//------------------------------------------------------------------------------
