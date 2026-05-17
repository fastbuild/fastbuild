// UnityNode.cpp
//------------------------------------------------------------------------------

// Includes
//------------------------------------------------------------------------------
#include "VCXProjectNode.h"

// FBuildCore
#include "Tools/FBuild/FBuildCore/BFF/Functions/Function.h"
#include "Tools/FBuild/FBuildCore/Error.h"
#include "Tools/FBuild/FBuildCore/FBuild.h"
#include "Tools/FBuild/FBuildCore/FLog.h"
#include "Tools/FBuild/FBuildCore/Graph/DirectoryListNode.h"
#include "Tools/FBuild/FBuildCore/Graph/NodeGraph.h"
#include "Tools/FBuild/FBuildCore/Helpers/ProjectGeneratorBase.h"
#include "Tools/FBuild/FBuildCore/Helpers/VSProjectGenerator.h"

// Core
#include "Core/Containers/UniquePtr.h"
#include "Core/Env/ErrorFormat.h"
#include "Core/FileIO/FileIO.h"
#include "Core/FileIO/FileStream.h"
#include "Core/FileIO/PathUtils.h"
#include "Core/Math/xxHash.h"
#include "Core/Strings/AStackString.h"

// system
#include <string.h> // for memcmp

// Globals
//------------------------------------------------------------------------------
static const AString g_DefaultProjectTypeGuid( "{8BC9CEB8-8B4A-11D0-8D11-00A0C91BC942}" );

// Reflection
//------------------------------------------------------------------------------
REFLECT_STRUCT_BEGIN_BASE( VSProjectConfigBase )
    REFLECT( m_ProjectBuildCommand, MetaInheritFromOwner() )
    REFLECT( m_ProjectRebuildCommand, MetaInheritFromOwner() )
    REFLECT( m_ProjectCleanCommand, MetaInheritFromOwner() )
    REFLECT( m_CompileFileCommand, MetaInheritFromOwner() )
    REFLECT( m_Output, MetaInheritFromOwner() )
    REFLECT( m_PreprocessorDefinitions, MetaInheritFromOwner() )
    REFLECT( m_IncludeSearchPath, MetaInheritFromOwner() )
    REFLECT( m_ForcedIncludes, MetaInheritFromOwner() )
    REFLECT( m_AssemblySearchPath, MetaInheritFromOwner() )
    REFLECT( m_ForcedUsingAssemblies, MetaInheritFromOwner() )
    REFLECT( m_AdditionalOptions, MetaInheritFromOwner() )
    REFLECT( m_OutputDirectory, MetaInheritFromOwner() )
    REFLECT( m_IntermediateDirectory, MetaInheritFromOwner() )
    REFLECT( m_BuildLogFile, MetaInheritFromOwner() )
    REFLECT( m_LayoutDir, MetaInheritFromOwner() )
    REFLECT( m_LayoutExtensionFilter, MetaInheritFromOwner() )
    REFLECT( m_Xbox360DebuggerCommand, MetaInheritFromOwner() )
    REFLECT( m_DebuggerFlavor, MetaInheritFromOwner() )
    REFLECT( m_AumidOverride, MetaInheritFromOwner() )
    REFLECT( m_PlatformToolset, MetaInheritFromOwner() )
    REFLECT( m_DeploymentType, MetaInheritFromOwner() )
    REFLECT( m_DeploymentFiles, MetaInheritFromOwner() )
    REFLECT( m_LocalDebuggerCommandArguments, MetaInheritFromOwner() )
    REFLECT( m_LocalDebuggerWorkingDirectory, MetaInheritFromOwner() )
    REFLECT( m_LocalDebuggerCommand, MetaInheritFromOwner() )
    REFLECT( m_LocalDebuggerEnvironment, MetaInheritFromOwner() )
    REFLECT( m_RemoteDebuggerCommand, MetaInheritFromOwner() )
    REFLECT( m_RemoteDebuggerCommandArguments, MetaInheritFromOwner() )
    REFLECT( m_RemoteDebuggerWorkingDirectory, MetaInheritFromOwner() )
    REFLECT( m_Keyword, MetaInheritFromOwner() )
    REFLECT( m_RootNamespace,  MetaInheritFromOwner() )
    REFLECT( m_ApplicationType,  MetaInheritFromOwner() )
    REFLECT( m_ApplicationTypeRevision,  MetaInheritFromOwner() )
    REFLECT( m_TargetLinuxPlatform, MetaInheritFromOwner() )
    REFLECT( m_LinuxProjectType, MetaInheritFromOwner() )
    REFLECT( m_PackagePath, MetaInheritFromOwner() )
    REFLECT( m_AdditionalSymbolSearchPaths, MetaInheritFromOwner() )
    REFLECT( m_AndroidApkLocation, MetaInheritFromOwner() )
    REFLECT( m_AndroidDebugComponent, MetaInheritFromOwner() )
    REFLECT( m_AndroidDebugTarget, MetaInheritFromOwner() )
    REFLECT( m_AndroidJdb, MetaInheritFromOwner() )
    REFLECT( m_AndroidLldbPostAttachCommands, MetaInheritFromOwner() )
    REFLECT( m_AndroidLldbStartupCommands, MetaInheritFromOwner() )
    REFLECT( m_AndroidPostApkInstallCommands, MetaInheritFromOwner() )
    REFLECT( m_AndroidPreApkInstallCommands, MetaInheritFromOwner() )
    REFLECT( m_AndroidSymbolDirectories, MetaInheritFromOwner() )
    REFLECT( m_AndroidWaitForDebugger, MetaInheritFromOwner() )
    REFLECT( m_LaunchFlags, MetaInheritFromOwner() )
REFLECT_END( VSProjectConfigBase )

REFLECT_STRUCT_BEGIN( VSProjectConfig, VSProjectConfigBase )
    REFLECT( m_Platform, MetaRequired() )
    REFLECT( m_Config, MetaRequired() )
    REFLECT( m_Target )
REFLECT_END( VSProjectConfig )

REFLECT_STRUCT_BEGIN_BASE( VSProjectFileType )
    REFLECT( m_FileType, MetaRequired() )
    REFLECT( m_Pattern, MetaRequired() )
REFLECT_END( VSProjectFileType )

REFLECT_STRUCT_BEGIN_BASE( VSProjectImport )
    REFLECT( m_Condition, MetaRequired() )
    REFLECT( m_Project, MetaRequired() )
REFLECT_END( VSProjectImport )

REFLECT_NODE_BEGIN( VCXProjectNode, VSProjectBaseNode, MetaName( "ProjectOutput" ) + MetaFile() )
    REFLECT( m_ProjectInputPaths, MetaPath() )
    REFLECT( m_ProjectInputPathsExclude, MetaPath() )
    REFLECT( m_ProjectInputPathsRecurse )
    REFLECT( m_ProjectFiles, MetaFile() )
    REFLECT( m_ProjectFilesToExclude, MetaFile() )
    REFLECT( m_ProjectPatternToExclude, MetaFile() )
    REFLECT_RENAME( m_ProjectBasePaths, "ProjectBasePath", MetaPath() ) // NOTE: Exposed as "ProjectBasePath" for backwards compat
    REFLECT( m_ProjectAllowedFileExtensions )
    REFLECT( m_ProjectConfigs )
    REFLECT( m_ProjectFileTypes )

    REFLECT( m_DefaultLanguage )
    REFLECT( m_ApplicationEnvironment )
    REFLECT( m_ProjectSccEntrySAK )

    REFLECT( m_ProjectReferences )
    REFLECT( m_ProjectProjectReferences )

    REFLECT( m_ProjectProjectImports )

    // Base Project Config settings
    REFLECT( m_BaseProjectConfig, MetaEmbedMembers() )
REFLECT_END( VCXProjectNode )

// VSProjectConfig::ResolveTargets
//------------------------------------------------------------------------------
/*static*/ bool VSProjectConfig::ResolveTargets( NodeGraph & nodeGraph,
                                                 Array<VSProjectConfig> & configs,
                                                 const BFFToken * iter,
                                                 const Function * function )
{
    // Must provide iter and function, or neither
    ASSERT( ( ( iter == nullptr ) && ( function == nullptr ) ) ||
            ( iter && function ) );

    for ( VSProjectConfig & config : configs )
    {
        // Target is allowed to be empty (perhaps this project represents
        // something that cannot be built, like header browsing information
        // for a 3rd party library for example)
        if ( config.m_Target.IsEmpty() )
        {
            continue;
        }

        // Find the node
        const Node * node = nodeGraph.FindNode( config.m_Target );
        if ( node == nullptr )
        {
            if ( iter && function )
            {
                Error::Error_1104_TargetNotDefined( iter, function, ".Target", config.m_Target );
                return false;
            }
            ASSERT( false ); // Should not be possible to fail when restoring from serialized DB
            continue;
        }

        config.m_TargetNode = node;
    }
    return true;
}

// CONSTRUCTOR
//------------------------------------------------------------------------------
VCXProjectNode::VCXProjectNode()
    : VSProjectBaseNode()
    , m_ProjectSccEntrySAK( false )
{
    m_Type = Node::VCXPROJECT_NODE;

    ProjectGeneratorBase::GetDefaultAllowedFileExtensions( m_ProjectAllowedFileExtensions );

    // Additional default imports to allow debugging on some target platforms
    m_ProjectProjectImports.SetSize( 2 );
    // PS4
    m_ProjectProjectImports[ 0 ].m_Condition = "'$(ConfigurationType)' == 'Makefile' and Exists('$(VCTargetsPath)\\Platforms\\$(Platform)\\SCE.Makefile.$(Platform).targets')";
    m_ProjectProjectImports[ 0 ].m_Project = "$(VCTargetsPath)\\Platforms\\$(Platform)\\SCE.Makefile.$(Platform).targets";
    // Android
    m_ProjectProjectImports[ 1 ].m_Condition = "'$(ConfigurationType)' == 'Makefile' and '$(AndroidAPILevel)' != '' and Exists('$(VCTargetsPath)\\Application Type\\$(ApplicationType)\\$(ApplicationTypeRevision)\\Android.Common.targets')";
    m_ProjectProjectImports[ 1 ].m_Project = "$(VCTargetsPath)\\Application Type\\$(ApplicationType)\\$(ApplicationTypeRevision)\\Android.Common.targets";
}

// Initialize
//------------------------------------------------------------------------------
/*virtual*/ bool VCXProjectNode::Initialize( NodeGraph & nodeGraph, const BFFToken * iter, const Function * function )
{
    ProjectGeneratorBase::FixupAllowedFileExtensions( m_ProjectAllowedFileExtensions );

    Dependencies dirNodes( m_ProjectInputPaths.GetSize() );
    if ( !Function::GetDirectoryListNodeList( nodeGraph,
                                              iter,
                                              function,
                                              m_ProjectInputPaths,
                                              m_ProjectInputPathsExclude,
                                              m_ProjectFilesToExclude,
                                              m_ProjectPatternToExclude,
                                              m_ProjectInputPathsRecurse,
                                              false, // Don't include read-only status in hash
                                              false, // Don't include directories
                                              &m_ProjectAllowedFileExtensions,
                                              "ProjectInputPaths",
                                              dirNodes ) )
    {
        return false; // GetDirectoryListNodeList will have emitted an error
    }

    // generate GUID if not specified
    if ( m_ProjectGuid.IsEmpty() )
    {
        AStackString relativePath;
        if ( m_Name.BeginsWith( FBuild::Get().GetWorkingDir() ) )
        {
            relativePath = m_Name.Get() + FBuild::Get().GetWorkingDir().GetLength() + 1;
        }
        else
        {
            relativePath = m_Name;
        }
        relativePath.Replace( '\\', '/' );
        VSProjectGenerator::FormatDeterministicProjectGUID( m_ProjectGuid, relativePath );
    }

    // Generate default configs if not specified
    if ( m_ProjectConfigs.IsEmpty() )
    {
        // Generated configs will take any properties we've
        // set at the project level as a default
        VSProjectConfig config( m_BaseProjectConfig );

        // make the configs
        m_ProjectConfigs.SetCapacity( 4 );
        config.m_Platform = "Win32";
        config.m_Config = "Debug";
        m_ProjectConfigs.Append( config );
        config.m_Config = "Release";
        m_ProjectConfigs.Append( config );
        config.m_Platform = "x64";
        m_ProjectConfigs.Append( config );
        config.m_Config = "Debug";
        m_ProjectConfigs.Append( config );
    }

    // Resolve Target names to Node pointers for later use
    if ( VSProjectConfig::ResolveTargets( nodeGraph, m_ProjectConfigs, iter, function ) == false )
    {
        return false; // Initialize will have emitted an error
    }

    // copy to base class platform config tuples array
    if ( m_ProjectPlatformConfigTuples.IsEmpty() )
    {
        VSProjectPlatformConfigTuple platCfgTuple;
        m_ProjectPlatformConfigTuples.SetCapacity( m_ProjectConfigs.GetSize() );
        for ( const VSProjectConfig & config : m_ProjectConfigs )
        {
            platCfgTuple.m_Config = config.m_Config;
            platCfgTuple.m_Platform = config.m_Platform;
            m_ProjectPlatformConfigTuples.Append( platCfgTuple );
        }
    }

    // Store all dependencies
    m_StaticDependencies.SetCapacity( dirNodes.GetSize() );
    m_StaticDependencies.Add( dirNodes );

    return true;
}

// DESTRUCTOR
//------------------------------------------------------------------------------
VCXProjectNode::~VCXProjectNode() = default;

// DoBuild
//------------------------------------------------------------------------------
/*virtual*/ Node::BuildResult VCXProjectNode::DoBuild( Job * /*job*/ )
{
    VSProjectGenerator pg;
    pg.SetBasePaths( m_ProjectBasePaths );

    // Globals
    pg.SetRootNamespace( m_BaseProjectConfig.m_RootNamespace );
    pg.SetProjectGuid( m_ProjectGuid );
    pg.SetDefaultLanguage( m_DefaultLanguage );
    pg.SetApplicationEnvironment( m_ApplicationEnvironment );
    pg.SetProjectSccEntrySAK( m_ProjectSccEntrySAK );

    // References
    pg.SetReferences( m_ProjectReferences );
    pg.SetProjectReferences( m_ProjectProjectReferences );

    // files from directory listings
    for ( const Dependency & staticDep : m_StaticDependencies )
    {
        const DirectoryListNode * dirNode = staticDep.GetNode()->CastTo<DirectoryListNode>();
        for ( const FileIO::FileInfo & fileInfo : dirNode->GetFiles() )
        {
            pg.AddFile( fileInfo.m_Name );
        }
    }

    // files explicitly listed
    for ( const AString & fileName : m_ProjectFiles )
    {
        pg.AddFile( fileName );
    }

    // .vcxproj
    const AString & project = pg.GenerateVCXProj( m_Name, m_ProjectConfigs, m_ProjectFileTypes, m_ProjectProjectImports );
    if ( ProjectGeneratorBase::WriteIfDifferent( "VCXProject", project, m_Name ) == false )
    {
        return BuildResult::eFailed; // Save will have emitted an error
    }

    // .vcxproj.filters
    const AString & filters = pg.GenerateVCXProjFilters( m_Name );
    AStackString filterFile( m_Name );
    filterFile += ".filters";
    if ( ProjectGeneratorBase::WriteIfDifferent( "VCXProject", filters, filterFile ) == false )
    {
        return BuildResult::eFailed; // Save will have emitted an error
    }

    // Record stamp representing the contents of the files
    m_Stamp = xxHash3::Calc64Big( project ) + xxHash3::Calc64Big( filters );

    return BuildResult::eOk;
}

// PostLoad
//------------------------------------------------------------------------------
/*virtual*/ void VCXProjectNode::PostLoad( NodeGraph & nodeGraph )
{
    VSProjectConfig::ResolveTargets( nodeGraph, m_ProjectConfigs );
}

// GetProjectTypeGuid
//------------------------------------------------------------------------------
/*virtual*/ const AString & VCXProjectNode::GetProjectTypeGuid() const
{
    return g_DefaultProjectTypeGuid;
}

//------------------------------------------------------------------------------
