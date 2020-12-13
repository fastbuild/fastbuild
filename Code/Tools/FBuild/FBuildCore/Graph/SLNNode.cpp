// SLNNode.cpp
//------------------------------------------------------------------------------

// Includes
//------------------------------------------------------------------------------
#include "SLNNode.h"

#include "Tools/FBuild/FBuildCore/Error.h"
#include "Tools/FBuild/FBuildCore/FBuild.h"
#include "Tools/FBuild/FBuildCore/FLog.h"
#include "Tools/FBuild/FBuildCore/Graph/NodeGraph.h"
#include "Tools/FBuild/FBuildCore/Graph/VCXProjectNode.h"
#include "Tools/FBuild/FBuildCore/Graph/VSProjectBaseNode.h"
#include "Tools/FBuild/FBuildCore/Graph/VSProjectExternalNode.h"
#include "Tools/FBuild/FBuildCore/Helpers/SLNGenerator.h"
#include "Tools/FBuild/FBuildCore/Helpers/VSProjectGenerator.h"

// Core
#include "Core/Containers/UniquePtr.h"
#include "Core/Env/ErrorFormat.h"
#include "Core/FileIO/FileStream.h"
#include "Core/FileIO/PathUtils.h"
#include "Core/Strings/AStackString.h"

// system
#include <string.h> // for memcmp

// Reflection
//------------------------------------------------------------------------------
REFLECT_STRUCT_BEGIN_BASE( SolutionConfigBase )
    REFLECT_ARRAY(  m_SolutionBuildProjects,                "SolutionBuildProject",                     MetaInheritFromOwner() + MetaOptional() + MetaFile() ) // "SolutionBuildProject" for backwards compat
    REFLECT_ARRAY(  m_SolutionDeployProjects,               "SolutionDeployProjects",                   MetaInheritFromOwner() + MetaOptional() + MetaFile() )
REFLECT_END( SolutionConfigBase )

REFLECT_STRUCT_BEGIN( SolutionConfig, SolutionConfigBase, MetaNone() )
    REFLECT(        m_SolutionPlatform,                     "SolutionPlatform",                         MetaOptional() )
    REFLECT(        m_SolutionConfig,                       "SolutionConfig",                           MetaOptional() )
    REFLECT(        m_Platform,                             "Platform",                                 MetaNone() )
    REFLECT(        m_Config,                               "Config",                                   MetaNone() )
REFLECT_END( SolutionConfig )

REFLECT_STRUCT_BEGIN_BASE( SolutionFolder )
    REFLECT(        m_Path,                                 "Path",                                     MetaNone() )
    REFLECT_ARRAY(  m_Projects,                             "Projects",                                 MetaOptional() + MetaFile() )
    REFLECT_ARRAY(  m_Items,                                "Items",                                    MetaOptional() + MetaFile() )
REFLECT_END( SolutionFolder )

REFLECT_STRUCT_BEGIN_BASE( SolutionDependency )
    REFLECT_ARRAY(  m_Projects,                             "Projects",                                 MetaFile() )
    REFLECT_ARRAY(  m_Dependencies,                         "Dependencies",                             MetaFile() )
REFLECT_END( SolutionDependency )

REFLECT_NODE_BEGIN( SLNNode, Node, MetaName( "SolutionOutput" ) + MetaFile() )
    REFLECT_ARRAY(  m_SolutionProjects,                     "SolutionProjects",                         MetaOptional() + MetaFile() )
    REFLECT(        m_SolutionVisualStudioVersion,          "SolutionVisualStudioVersion",              MetaOptional() )
    REFLECT(        m_SolutionMinimumVisualStudioVersion,   "SolutionMinimumVisualStudioVersion",       MetaOptional() )
    REFLECT_ARRAY_OF_STRUCT( m_SolutionConfigs,             "SolutionConfigs",      SolutionConfig,     MetaOptional() )
    REFLECT_ARRAY_OF_STRUCT( m_SolutionFolders,             "SolutionFolders",      SolutionFolder,     MetaOptional() )
    REFLECT_ARRAY_OF_STRUCT( m_SolutionDependencies,        "SolutionDependencies", SolutionDependency, MetaOptional() )

    // Base Project Config settings
    REFLECT_STRUCT( m_BaseSolutionConfig,                   "BaseSolutionConfig",   SolutionConfigBase, MetaEmbedMembers() )
REFLECT_END( SLNNode )

// VCXProjectNodeComp
//------------------------------------------------------------------------------
struct VCXProjectNodeComp
{
    bool operator ()( const VSProjectBaseNode * a, const VSProjectBaseNode * b ) const
    {
        return ( a->GetName() < b->GetName() );
    }
};

// CONSTRUCTOR
//------------------------------------------------------------------------------
SLNNode::SLNNode()
    : FileNode( AString::GetEmpty(), Node::FLAG_ALWAYS_BUILD )
{
    m_LastBuildTimeMs = 100; // higher default than a file node
    m_Type = Node::SLN_NODE;
}

// Initialize
//------------------------------------------------------------------------------
/*virtual*/ bool SLNNode::Initialize( NodeGraph & nodeGraph, const BFFToken * iter, const Function * function )
{
    // Solution Configs
    //------------------------------------------------------------------------------
    // Create default SolutionConfigs if not specified
    if ( m_SolutionConfigs.IsEmpty() )
    {
        // Generated configs will take any properties we've
        // set at the solution level as a default
        SolutionConfig config( m_BaseSolutionConfig );

        m_SolutionConfigs.SetCapacity( 4 );
        config.m_Platform = "Win32";
        config.m_Config = "Debug";
        m_SolutionConfigs.Append( config );
        config.m_Config = "Release";
        m_SolutionConfigs.Append( config );
        config.m_Platform = "x64";
        config.m_Config = "Debug";
        m_SolutionConfigs.Append( config );
        config.m_Config = "Release";
        m_SolutionConfigs.Append( config );
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
            
            // Merge list of items
            found->m_Items.Append( folder.m_Items );
        }
        else
        {
            // Add new entry
            collapsedFolders.EmplaceBack( folder );
        }
    }
    m_SolutionFolders.Swap( collapsedFolders );

    // Gather all Project references and canonicalize project names
    //------------------------------------------------------------------------------
    Array< VSProjectBaseNode * > projects( m_SolutionProjects.GetSize(), true );
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
        if ( !GatherProjects( nodeGraph, function, iter, "Projects", solutionDependency.m_Projects, projects ) )
        {
            return false; // MergeProjects will have emitted an error
        }
        if ( !GatherProjects( nodeGraph, function, iter, "Dependencies", solutionDependency.m_Dependencies, projects ) )
        {
            return false; // MergeProjects will have emitted an error
        }
    }

    // SolutionConfigs
    for ( SolutionConfig & solutionConfig : m_SolutionConfigs )
    {
        // SolutionBuildProjects
        if ( !GatherProjects( nodeGraph, function, iter, "SolutionBuildProject", solutionConfig.m_SolutionBuildProjects, projects ) )
        {
            return false; // MergeProjects will have emitted an error
        }

        // SolutionDeployProjects
        if ( !GatherProjects( nodeGraph, function, iter, "SolutionDeployProjects", solutionConfig.m_SolutionDeployProjects, projects ) )
        {
            return false; // MergeProjects will have emitted an error
        }
    }

    // Sort projects by name (like Visual Studio)
    projects.Sort( VCXProjectNodeComp() );

    // Check Project Configurations
    //------------------------------------------------------------------------------
    for ( const VSProjectBaseNode * project : projects )
    {
        // check that this Project contains all .SolutionConfigs
        for ( const SolutionConfig & solutionConfig : m_SolutionConfigs )
        {
            bool containsConfig = false;
            for ( const VSProjectPlatformConfigTuple & projectConfig : project->GetPlatformConfigTuples() )
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
    for ( VSProjectBaseNode * project : projects )
    {
        m_StaticDependencies.EmplaceBack( project );
    }

    return true;
}

// DESTRUCTOR
//------------------------------------------------------------------------------
SLNNode::~SLNNode() = default;

// DoBuild
//------------------------------------------------------------------------------
/*virtual*/ Node::BuildResult SLNNode::DoBuild( Job * /*job*/ )
{
    SLNGenerator sg;

    // projects
    Array< VSProjectBaseNode * > projects( m_StaticDependencies.GetSize(), false );
    for ( Dependency & dep : m_StaticDependencies )
    {
        const Node * node = dep.GetNode();
        VSProjectBaseNode * projectNode = ( node->GetType() == Node::VCXPROJECT_NODE )
                                        ? static_cast< VSProjectBaseNode * >( node->CastTo< VCXProjectNode >() )
                                        : static_cast< VSProjectBaseNode * >( node->CastTo< VSProjectExternalNode >() );

        projects.Append( projectNode );
    }

    // .sln solution file
    const AString & sln = sg.GenerateSLN( m_Name,
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
            UniquePtr< char > mem( ( char *)ALLOC( oldFileSize ) );
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

    if ( FBuild::Get().GetOptions().m_ShowCommandSummary )
    {
        FLOG_OUTPUT( "SLN: %s\n", fileName.Get() );
    }

    // actually write
    FileStream f;
    if ( !f.Open( fileName.Get(), FileStream::WRITE_ONLY ) )
    {
        FLOG_ERROR( "SLN - Failed to open file for write. Error: %s Target: '%s'", LAST_ERROR_STR, fileName.Get() );
        return false;
    }
    if ( f.Write( content.Get(), content.GetLength() ) != content.GetLength() )
    {
        FLOG_ERROR( "SLN - Error writing file. Error: %s Target: '%s'", LAST_ERROR_STR, fileName.Get() );
        return false;
    }
    f.Close();

    return true;
}

// GatherProject
//------------------------------------------------------------------------------
bool SLNNode::GatherProject( NodeGraph & nodeGraph,
                             const Function * function,
                             const BFFToken * iter,
                             const char * propertyName,
                             const AString & projectName,
                             Array< VSProjectBaseNode * > & inOutProjects ) const
{
    // Get associated project file
    const Node * node = nodeGraph.FindNode( projectName );
    if ( node == nullptr )
    {
        Error::Error_1104_TargetNotDefined( iter, function, propertyName, projectName );
        return false;
    }
    if ( ( node->GetType() != Node::VCXPROJECT_NODE ) &&
         ( node->GetType() != Node::VSPROJEXTERNAL_NODE ) )
    {
        // don't know how to handle this type of node
        Error::Error_1005_UnsupportedNodeType( iter, function, propertyName, node->GetName(), node->GetType() );
        return false;
    }
    VSProjectBaseNode * projectNode = ( node->GetType() == Node::VCXPROJECT_NODE )
                                    ? static_cast< VSProjectBaseNode * >( node->CastTo< VCXProjectNode >() )
                                    : static_cast< VSProjectBaseNode * >( node->CastTo< VSProjectExternalNode >() );

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
                              const BFFToken * iter,
                              const char * propertyName,
                              const Array< AString > & projectNames,
                              Array< VSProjectBaseNode * > & inOutProjects ) const
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
