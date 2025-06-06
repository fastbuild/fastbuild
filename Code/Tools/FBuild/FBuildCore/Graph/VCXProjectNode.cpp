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
static const AString g_DefaultProjectTypeGuid( "{8BC9CEB8-8B4A-11D0-8D11-00A0C91BC942}");

// Reflection
//------------------------------------------------------------------------------
REFLECT_STRUCT_BEGIN_BASE( VSProjectConfigBase )
    REFLECT(        m_ProjectBuildCommand,          "ProjectBuildCommand",          MetaInheritFromOwner() + MetaOptional() )
    REFLECT(        m_ProjectRebuildCommand,        "ProjectRebuildCommand",        MetaInheritFromOwner() + MetaOptional() )
    REFLECT(        m_ProjectCleanCommand,          "ProjectCleanCommand",          MetaInheritFromOwner() + MetaOptional() )
    REFLECT(        m_Output,                       "Output",                       MetaInheritFromOwner() + MetaOptional() )
    REFLECT(        m_PreprocessorDefinitions,      "PreprocessorDefinitions",      MetaInheritFromOwner() + MetaOptional() )
    REFLECT(        m_IncludeSearchPath,            "IncludeSearchPath",            MetaInheritFromOwner() + MetaOptional() )
    REFLECT(        m_ForcedIncludes,               "ForcedIncludes",               MetaInheritFromOwner() + MetaOptional() )
    REFLECT(        m_AssemblySearchPath,           "AssemblySearchPath",           MetaInheritFromOwner() + MetaOptional() )
    REFLECT(        m_ForcedUsingAssemblies,        "ForcedUsingAssemblies",        MetaInheritFromOwner() + MetaOptional() )
    REFLECT(        m_AdditionalOptions,            "AdditionalOptions",            MetaInheritFromOwner() + MetaOptional() )
    REFLECT(        m_OutputDirectory,              "OutputDirectory",              MetaInheritFromOwner() + MetaOptional() )
    REFLECT(        m_IntermediateDirectory,        "IntermediateDirectory",        MetaInheritFromOwner() + MetaOptional() )
    REFLECT(        m_BuildLogFile,                 "BuildLogFile",                 MetaInheritFromOwner() + MetaOptional() )
    REFLECT(        m_LayoutDir,                    "LayoutDir",                    MetaInheritFromOwner() + MetaOptional() )
    REFLECT(        m_LayoutExtensionFilter,        "LayoutExtensionFilter",        MetaInheritFromOwner() + MetaOptional() )
    REFLECT(        m_Xbox360DebuggerCommand,       "Xbox360DebuggerCommand",       MetaInheritFromOwner() + MetaOptional() )
    REFLECT(        m_DebuggerFlavor,               "DebuggerFlavor",               MetaInheritFromOwner() + MetaOptional() )
    REFLECT(        m_AumidOverride,                "AumidOverride",                MetaInheritFromOwner() + MetaOptional() )
    REFLECT(        m_PlatformToolset,              "PlatformToolset",              MetaInheritFromOwner() + MetaOptional() )
    REFLECT(        m_DeploymentType,               "DeploymentType",               MetaInheritFromOwner() + MetaOptional() )
    REFLECT(        m_DeploymentFiles,              "DeploymentFiles",              MetaInheritFromOwner() + MetaOptional() )
    REFLECT(        m_LocalDebuggerCommandArguments,"LocalDebuggerCommandArguments",MetaInheritFromOwner() + MetaOptional() )
    REFLECT(        m_LocalDebuggerWorkingDirectory,"LocalDebuggerWorkingDirectory",MetaInheritFromOwner() + MetaOptional() )
    REFLECT(        m_LocalDebuggerCommand,         "LocalDebuggerCommand",         MetaInheritFromOwner() + MetaOptional() )
    REFLECT(        m_LocalDebuggerEnvironment,     "LocalDebuggerEnvironment",     MetaInheritFromOwner() + MetaOptional() )
    REFLECT(        m_RemoteDebuggerCommand,            "RemoteDebuggerCommand",            MetaInheritFromOwner() + MetaOptional() )
    REFLECT(        m_RemoteDebuggerCommandArguments,   "RemoteDebuggerCommandArguments",   MetaInheritFromOwner() + MetaOptional() )
    REFLECT(        m_RemoteDebuggerWorkingDirectory,   "RemoteDebuggerWorkingDirectory",   MetaInheritFromOwner() + MetaOptional() )
    REFLECT(        m_Keyword,                      "Keyword",                      MetaInheritFromOwner() + MetaOptional() )
    REFLECT(        m_RootNamespace,                "RootNamespace",                MetaInheritFromOwner() + MetaOptional() )
    REFLECT(        m_ApplicationType,              "ApplicationType",              MetaInheritFromOwner() + MetaOptional() )
    REFLECT(        m_ApplicationTypeRevision,      "ApplicationTypeRevision",      MetaInheritFromOwner() + MetaOptional() )
    REFLECT(        m_TargetLinuxPlatform,          "TargetLinuxPlatform",          MetaInheritFromOwner() + MetaOptional() )
    REFLECT(        m_LinuxProjectType,             "LinuxProjectType",             MetaInheritFromOwner() + MetaOptional() )
    REFLECT(        m_PackagePath,                  "PackagePath",                  MetaInheritFromOwner() + MetaOptional() )
    REFLECT(        m_AdditionalSymbolSearchPaths,  "AdditionalSymbolSearchPaths",  MetaInheritFromOwner() + MetaOptional() )
    REFLECT(        m_AndroidApkLocation,           "AndroidApkLocation",           MetaInheritFromOwner() + MetaOptional() )
    REFLECT(        m_AndroidDebugComponent,        "AndroidDebugComponent",        MetaInheritFromOwner() + MetaOptional() )
    REFLECT(        m_AndroidDebugTarget,           "AndroidDebugTarget",           MetaInheritFromOwner() + MetaOptional() )
    REFLECT(        m_AndroidJdb,                   "AndroidJdb",                   MetaInheritFromOwner() + MetaOptional() )
    REFLECT(        m_AndroidLldbPostAttachCommands,    "AndroidLldbPostAttachCommands",    MetaInheritFromOwner() + MetaOptional() )
    REFLECT(        m_AndroidLldbStartupCommands,   "AndroidLldbStartupCommands",   MetaInheritFromOwner() + MetaOptional() )
    REFLECT(        m_AndroidPostApkInstallCommands,    "AndroidPostApkInstallCommands",    MetaInheritFromOwner() + MetaOptional() )
    REFLECT(        m_AndroidPreApkInstallCommands, "AndroidPreApkInstallCommands", MetaInheritFromOwner() + MetaOptional() )
    REFLECT(        m_AndroidSymbolDirectories,     "AndroidSymbolDirectories",     MetaInheritFromOwner() + MetaOptional() )
    REFLECT(        m_AndroidWaitForDebugger,       "AndroidWaitForDebugger",       MetaInheritFromOwner() + MetaOptional() )
    REFLECT(        m_LaunchFlags,                  "LaunchFlags",                  MetaInheritFromOwner() + MetaOptional() )
REFLECT_END( VSProjectConfigBase )

REFLECT_STRUCT_BEGIN( VSProjectConfig, VSProjectConfigBase, MetaNone() )
    REFLECT(        m_Platform,                     "Platform",                     MetaNone() )
    REFLECT(        m_Config,                       "Config",                       MetaNone() )
    REFLECT(        m_Target,                       "Target",                       MetaOptional() )
REFLECT_END( VSProjectConfig )

REFLECT_STRUCT_BEGIN_BASE( VSProjectFileType )
    REFLECT(        m_FileType,                     "FileType",                     MetaNone() )
    REFLECT(        m_Pattern,                      "Pattern",                      MetaNone() )
REFLECT_END( VSProjectFileType )

REFLECT_STRUCT_BEGIN_BASE( VSProjectImport )
    REFLECT(        m_Condition,                    "Condition",                    MetaNone() )
    REFLECT(        m_Project,                      "Project",                      MetaNone() )
REFLECT_END( VSProjectImport )

REFLECT_NODE_BEGIN( VCXProjectNode, VSProjectBaseNode, MetaName( "ProjectOutput" ) + MetaFile() )
    REFLECT_ARRAY(  m_ProjectInputPaths,            "ProjectInputPaths",            MetaOptional() + MetaPath() )
    REFLECT_ARRAY(  m_ProjectInputPathsExclude,     "ProjectInputPathsExclude",     MetaOptional() + MetaPath() )
    REFLECT(        m_ProjectInputPathsRecurse,     "ProjectInputPathsRecurse",     MetaOptional() )
    REFLECT_ARRAY(  m_ProjectFiles,                 "ProjectFiles",                 MetaOptional() + MetaFile() )
    REFLECT_ARRAY(  m_ProjectFilesToExclude,        "ProjectFilesToExclude",        MetaOptional() + MetaFile() )
    REFLECT_ARRAY(  m_ProjectPatternToExclude,      "ProjectPatternToExclude",      MetaOptional() + MetaFile() )
    REFLECT_ARRAY(  m_ProjectBasePaths,             "ProjectBasePath",              MetaOptional() + MetaPath() ) // NOTE: Exposed as "ProjectBasePath" for backwards compat
    REFLECT_ARRAY(  m_ProjectAllowedFileExtensions, "ProjectAllowedFileExtensions", MetaOptional() )
    REFLECT_ARRAY_OF_STRUCT(    m_ProjectConfigs,   "ProjectConfigs",               VSProjectConfig,    MetaOptional() )
    REFLECT_ARRAY_OF_STRUCT(    m_ProjectFileTypes, "ProjectFileTypes",             VSProjectFileType,  MetaOptional() )

    REFLECT(        m_DefaultLanguage,              "DefaultLanguage",              MetaOptional() )
    REFLECT(        m_ApplicationEnvironment,       "ApplicationEnvironment",       MetaOptional() )
    REFLECT(        m_ProjectSccEntrySAK,           "ProjectSccEntrySAK",           MetaOptional() )

    REFLECT_ARRAY(  m_ProjectReferences,            "ProjectReferences",            MetaOptional() )
    REFLECT_ARRAY(  m_ProjectProjectReferences,     "ProjectProjectReferences",     MetaOptional() )

    REFLECT_ARRAY_OF_STRUCT(  m_ProjectProjectImports,  "ProjectProjectImports",    VSProjectImport,    MetaOptional() )

    // Base Project Config settings
    REFLECT_STRUCT( m_BaseProjectConfig,            "BaseProjectConfig",            VSProjectConfigBase,    MetaEmbedMembers() )
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
        AStackString<> relativePath;
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
        config.m_Platform   = "Win32";
        config.m_Config     = "Debug";
        m_ProjectConfigs.Append( config );
        config.m_Config     = "Release";
        m_ProjectConfigs.Append( config );
        config.m_Platform   = "x64";
        m_ProjectConfigs.Append( config );
        config.m_Config     = "Debug";
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
        const DirectoryListNode * dirNode = staticDep.GetNode()->CastTo< DirectoryListNode >();
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
    AStackString<> filterFile( m_Name );
    filterFile += ".filters";
    if ( ProjectGeneratorBase::WriteIfDifferent( "VCXProject", filters, filterFile ) == false )
    {
        return BuildResult::eFailed; // Save will have emitted an error
    }

    // Record stamp representing the contents of the files
    m_Stamp = xxHash3::Calc64( project ) + xxHash3::Calc64( filters );

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
