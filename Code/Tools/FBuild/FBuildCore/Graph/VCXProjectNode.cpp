// UnityNode.cpp
//------------------------------------------------------------------------------

// Includes
//------------------------------------------------------------------------------
#include "Tools/FBuild/FBuildCore/PrecompiledHeader.h"

#include "VCXProjectNode.h"

#include "Tools/FBuild/FBuildCore/Error.h"
#include "Tools/FBuild/FBuildCore/FBuild.h"
#include "Tools/FBuild/FBuildCore/FLog.h"
#include "Tools/FBuild/FBuildCore/BFF/Functions/Function.h"
#include "Tools/FBuild/FBuildCore/Graph/NodeGraph.h"
#include "Tools/FBuild/FBuildCore/Graph/DirectoryListNode.h"
#include "Tools/FBuild/FBuildCore/Helpers/ProjectGeneratorBase.h"
#include "Tools/FBuild/FBuildCore/Helpers/VSProjectGenerator.h"

// Core
#include "Core/Containers/AutoPtr.h"
#include "Core/Env/Env.h"
#include "Core/FileIO/FileIO.h"
#include "Core/FileIO/FileStream.h"
#include "Core/FileIO/PathUtils.h"
#include "Core/Strings/AStackString.h"

// system
#include <string.h> // for memcmp

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

REFLECT_NODE_BEGIN( VCXProjectNode, Node, MetaName( "ProjectOutput" ) + MetaFile() )
    REFLECT_ARRAY(  m_ProjectInputPaths,            "ProjectInputPaths",            MetaOptional() + MetaPath() )
    REFLECT_ARRAY(  m_ProjectInputPathsExclude,     "ProjectInputPathsExclude",     MetaOptional() + MetaPath() )
    REFLECT_ARRAY(  m_ProjectFiles,                 "ProjectFiles",                 MetaOptional() + MetaFile() )
    REFLECT_ARRAY(  m_ProjectFilesToExclude,        "ProjectFilesToExclude",        MetaOptional() + MetaFile() )
    REFLECT_ARRAY(  m_ProjectPatternToExclude,      "ProjectPatternToExclude",      MetaOptional() + MetaFile() )
    REFLECT_ARRAY(  m_ProjectBasePaths,             "ProjectBasePath",              MetaOptional() + MetaPath() ) // NOTE: Exposed as "ProjectBasePath" for backwards compat
    REFLECT_ARRAY(  m_ProjectAllowedFileExtensions, "ProjectAllowedFileExtensions", MetaOptional() )
    REFLECT_ARRAY_OF_STRUCT(    m_ProjectConfigs,   "ProjectConfigs",               VSProjectConfig,    MetaOptional() )
    REFLECT_ARRAY_OF_STRUCT(    m_ProjectFileTypes, "ProjectFileTypes",             VSProjectFileType,  MetaOptional() )

    REFLECT(        m_RootNamespace,                "RootNamespace",                MetaOptional() )
    REFLECT(        m_ProjectGuid,                  "ProjectGuid",                  MetaOptional() )
    REFLECT(        m_DefaultLanguage,              "DefaultLanguage",              MetaOptional() )
    REFLECT(        m_ApplicationEnvironment,       "ApplicationEnvironment",       MetaOptional() )
    REFLECT(        m_ProjectSccEntrySAK,           "ProjectSccEntrySAK",           MetaOptional() )

    REFLECT_ARRAY(  m_ProjectReferences,            "ProjectReferences",            MetaOptional() )
    REFLECT_ARRAY(  m_ProjectProjectReferences,     "ProjectProjectReferences",     MetaOptional() )

    // Base Project Config settings
    REFLECT_STRUCT( m_BaseProjectConfig,            "BaseProjectConfig",            VSProjectConfigBase,    MetaEmbedMembers() );
REFLECT_END( VCXProjectNode )

// VSProjectConfig::ResolveTargets
//------------------------------------------------------------------------------
/*static*/ bool VSProjectConfig::ResolveTargets( NodeGraph & nodeGraph,
                                                 Array< VSProjectConfig > & configs,
                                                 const BFFIterator * iter,
                                                 const Function * function )
{
    // Must provide iter and function, or neither
    ASSERT( ( ( iter == nullptr ) && ( function == nullptr ) ) ||
            ( iter && function ) );

    for ( auto & config : configs )
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
                Error::Error_1104_TargetNotDefined( *iter, function, ".Target", config.m_Target );
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
    : FileNode( AString::GetEmpty(), Node::FLAG_NONE )
    , m_ProjectSccEntrySAK( false )
{
    m_Type = Node::VCXPROJECT_NODE;
    m_LastBuildTimeMs = 100; // higher default than a file node

    ProjectGeneratorBase::GetDefaultAllowedFileExtensions( m_ProjectAllowedFileExtensions );
}

// Initialize
//------------------------------------------------------------------------------
/*virtual*/ bool VCXProjectNode::Initialize( NodeGraph & nodeGraph, const BFFIterator & iter, const Function * function )
{
    ProjectGeneratorBase::FixupAllowedFileExtensions( m_ProjectAllowedFileExtensions );

    Dependencies dirNodes( m_ProjectInputPaths.GetSize() );
    if ( !Function::GetDirectoryListNodeList( nodeGraph, iter, function, m_ProjectInputPaths, m_ProjectInputPathsExclude, m_ProjectFilesToExclude, m_ProjectPatternToExclude, true, &m_ProjectAllowedFileExtensions, "ProjectInputPaths", dirNodes ) )
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
    if ( VSProjectConfig::ResolveTargets( nodeGraph, m_ProjectConfigs, &iter, function ) == false )
    {
        return false; // Initialize will have emitted an error
    }

    // Store all dependencies
    m_StaticDependencies.SetCapacity( dirNodes.GetSize() );
    m_StaticDependencies.Append( dirNodes );

    return true;
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

    // Globals
    pg.SetRootNamespace( m_RootNamespace );
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
        DirectoryListNode * dirNode = staticDep.GetNode()->CastTo< DirectoryListNode >();
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
    const AString & project = pg.GenerateVCXProj( m_Name, m_ProjectConfigs, m_ProjectFileTypes );
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

// PostLoad
//------------------------------------------------------------------------------
/*virtual*/ void VCXProjectNode::PostLoad( NodeGraph & nodeGraph )
{
    VSProjectConfig::ResolveTargets( nodeGraph, m_ProjectConfigs );
}

//------------------------------------------------------------------------------
