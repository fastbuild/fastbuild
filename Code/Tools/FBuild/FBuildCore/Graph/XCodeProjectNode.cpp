// XCodeProjectNode.cpp
//------------------------------------------------------------------------------

// Includes
//------------------------------------------------------------------------------
#include "XCodeProjectNode.h"

#include "Tools/FBuild/FBuildCore/FBuild.h"
#include "Tools/FBuild/FBuildCore/FLog.h"
#include "Tools/FBuild/FBuildCore/BFF/Functions/Function.h"
#include "Tools/FBuild/FBuildCore/Graph/NodeGraph.h"
#include "Tools/FBuild/FBuildCore/Graph/DirectoryListNode.h"
#include "Tools/FBuild/FBuildCore/Helpers/ProjectGeneratorBase.h"
#include "Tools/FBuild/FBuildCore/Helpers/XCodeProjectGenerator.h"

#include "Core/Env/Env.h"
#include "Core/FileIO/IOStream.h"
#include "Core/FileIO/PathUtils.h"
#include "Core/Strings/AStackString.h"

// Reflection
//------------------------------------------------------------------------------
REFLECT_STRUCT_BEGIN_BASE( XCodeProjectConfig )
    REFLECT( m_Config,  "Config",   MetaNone() )
    REFLECT( m_Target,  "Target",   MetaOptional() )
    REFLECT( m_XCodeBaseSDK,            "XCodeBaseSDK",         MetaOptional() )
    REFLECT( m_XCodeDebugWorkingDir,    "XCodeDebugWorkingDir", MetaOptional() )
    REFLECT( m_XCodeIphoneOSDeploymentTarget, "XCodeIphoneOSDeploymentTarget", MetaOptional() )
REFLECT_END( XCodeProjectConfig )

REFLECT_NODE_BEGIN( XCodeProjectNode, Node, MetaName( "ProjectOutput" ) + MetaFile() )
    REFLECT_ARRAY( m_ProjectInputPaths,             "ProjectInputPaths",            MetaOptional() + MetaPath() )
    REFLECT_ARRAY( m_ProjectInputPathsExclude,      "ProjectInputPathsExclude",     MetaOptional() + MetaPath() )
    REFLECT_ARRAY( m_ProjectFiles,                  "ProjectFiles",                 MetaOptional() + MetaFile() )
    REFLECT_ARRAY( m_ProjectFilesToExclude,         "ProjectFilesToExclude",        MetaOptional() + MetaFile() )
    REFLECT_ARRAY( m_PatternToExclude,              "ProjectPatternToExclude",      MetaOptional() + MetaFile())
    REFLECT_ARRAY( m_ProjectBasePath,               "ProjectBasePath",              MetaOptional() + MetaPath() )
    REFLECT_ARRAY( m_ProjectAllowedFileExtensions,  "ProjectAllowedFileExtensions", MetaOptional() )
    REFLECT_ARRAY_OF_STRUCT( m_ProjectConfigs,      "ProjectConfigs",   XCodeProjectConfig,     MetaNone() )
    REFLECT( m_XCodeOrganizationName,               "XCodeOrganizationName",        MetaOptional() )
    REFLECT( m_XCodeBuildToolPath,                  "XCodeBuildToolPath",           MetaOptional() )
    REFLECT( m_XCodeBuildToolArgs,                  "XCodeBuildToolArgs",           MetaOptional() )
    REFLECT( m_XCodeBuildWorkingDir,                "XCodeBuildWorkingDir",         MetaOptional() )
    REFLECT( m_XCodeDocumentVersioning,             "XCodeDocumentVersioning",      MetaOptional() )
    REFLECT_ARRAY( m_XCodeCommandLineArguments,         "XCodeCommandLineArguments",            MetaOptional() )
    REFLECT_ARRAY( m_XCodeCommandLineArgumentsDisabled, "XCodeCommandLineArgumentsDisabled",    MetaOptional() )
REFLECT_END( XCodeProjectNode )

// XCodeProjectConfig::ResolveTargets
//------------------------------------------------------------------------------
/*static*/ bool XCodeProjectConfig::ResolveTargets( NodeGraph & nodeGraph,
                                                    Array< XCodeProjectConfig > & configs,
                                                    const BFFToken * iter,
                                                    const Function * function )
{
    // Must provide iter and function, or neither
    ASSERT( ( ( iter == nullptr ) && ( function == nullptr ) ) ||
            ( iter && function ) );

    for ( XCodeProjectConfig & config : configs )
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
XCodeProjectNode::XCodeProjectNode()
    : FileNode( AString::GetEmpty(), Node::FLAG_ALWAYS_BUILD )
    , m_XCodeOrganizationName( "Organization" )
    , m_XCodeBuildToolPath( "./FBuild" )
    , m_XCodeBuildToolArgs( "-ide $(FASTBUILD_TARGET)" )
    , m_XCodeBuildWorkingDir( "./" )
{
    m_Type = Node::XCODEPROJECT_NODE;

    ProjectGeneratorBase::GetDefaultAllowedFileExtensions( m_ProjectAllowedFileExtensions );
}

// Initialize
//------------------------------------------------------------------------------
/*virtual*/ bool XCodeProjectNode::Initialize( NodeGraph & nodeGraph, const BFFToken * iter, const Function * function )
{
    ProjectGeneratorBase::FixupAllowedFileExtensions( m_ProjectAllowedFileExtensions );

    Dependencies dirNodes( m_ProjectInputPaths.GetSize() );
    if ( !Function::GetDirectoryListNodeList( nodeGraph,
                                              iter,
                                              function,
                                              m_ProjectInputPaths,
                                              m_ProjectInputPathsExclude,
                                              m_ProjectFilesToExclude,
                                              m_PatternToExclude,
                                              true, // Resursive
                                              false, // Don't include read-only status in hash
                                              &m_ProjectAllowedFileExtensions,
                                              "ProjectInputPaths",
                                              dirNodes ) )
    {
        return false; // GetDirectoryListNodeList will have emitted an error
    }

    // .ProjectFiles
    Dependencies fileNodes( m_ProjectFiles.GetSize() );
    if ( !Function::GetNodeList( nodeGraph, iter, function, ".ProjectFiles", m_ProjectFiles, fileNodes ) )
    {
        return false; // GetNodeList will have emitted an error
    }

    ASSERT( m_StaticDependencies.IsEmpty() );
    m_StaticDependencies.Append( dirNodes );
    m_StaticDependencies.Append( fileNodes );

    // Resolve Target names to Node pointers for later use
    if ( XCodeProjectConfig::ResolveTargets( nodeGraph, m_ProjectConfigs, iter, function ) == false )
    {
        return false; // Initialize will have emitted an error
    }

    return true;
}

// DESTRUCTOR
//------------------------------------------------------------------------------
XCodeProjectNode::~XCodeProjectNode() = default;

// DoBuild
//------------------------------------------------------------------------------
/*virtual*/ Node::BuildResult XCodeProjectNode::DoBuild( Job * )
{
    // Generate project.pbxproj file
    XCodeProjectGenerator g;

    // Project Name
    AStackString<> tmp( m_Name );
    const char * lastSlash = tmp.FindLast( NATIVE_SLASH );
    if ( lastSlash )
    {
        --lastSlash;
        tmp.SetLength( (uint32_t)( lastSlash - tmp.Get() ) );
    }
    const char * projectNameStart = tmp.FindLast( NATIVE_SLASH );
    const char * projectNameEnd = tmp.FindLast( '.' );
    projectNameStart = projectNameStart ? projectNameStart + 1 : tmp.Get();
    projectNameEnd = projectNameEnd ? projectNameEnd : tmp.GetEnd();
    AStackString<> projectName( projectNameStart, projectNameEnd );
    g.SetProjectName( projectName );

    // Base Paths
    g.SetBasePaths( m_ProjectBasePath );

    // General Options
    g.SetXCodeOrganizationName( m_XCodeOrganizationName );
    g.SetXCodeBuildToolPath( m_XCodeBuildToolPath );
    g.SetXCodeBuildToolArgs( m_XCodeBuildToolArgs );
    g.SetXCodeBuildWorkingDir( m_XCodeBuildWorkingDir );
    g.SetXCodeDocumentVersioning( m_XCodeDocumentVersioning );
    g.SetXCodeCommandLineArguments( m_XCodeCommandLineArguments );
    g.SetXCodeCommandLineArgumentsDisabled( m_XCodeCommandLineArgumentsDisabled );

    // Add files
    for ( const Dependency & dep : m_StaticDependencies )
    {
        const Node * n = dep.GetNode();
        if ( n->GetType() == Node::DIRECTORY_LIST_NODE )
        {
            const DirectoryListNode * dln = n->CastTo< DirectoryListNode >();
            for ( const FileIO::FileInfo & file : dln->GetFiles() )
            {
                //filter the file by pattern
                const AString * pit = m_PatternToExclude.Begin();
                const AString * const pend = m_PatternToExclude.End();
                bool keep = true;
                for ( ; pit != pend; ++pit )
                {
                    if ( PathUtils::IsWildcardMatch( pit->Get(), file.m_Name.Get() ) )
                    {
                        keep = false;
                        break;
                    }
                }
                if ( keep )
                {
                    g.AddFile( file.m_Name );
                }
            }
        }
        else if ( n->IsAFile() )
        {
            // Is this an XCode project?
            if ( n->GetName().EndsWithI( ".pbxproj" ) )
            {
                // Is it the .pbxproj inside the main .xcodeproj folder?
                const char * ext = n->GetName().Find( ".xcodeproj/" );
                if ( ext )
                {
                    AStackString<> name( n->GetName().Get(), ext + 10 ); // include .xcodeproj
                    g.AddFile( name );
                    continue;
                }
            }

            // plain file
            g.AddFile( n->GetName() );
        }
        else
        {
            ASSERT( false ); // Something is terribly wrong
        }
    }

    // Add configs
    for ( const XCodeProjectConfig & cfg : m_ProjectConfigs )
    {
        g.AddConfig( cfg );
    }

    // Generate project.pbxproj file
    {
        const AString & output = g.GeneratePBXProj();
        if ( ProjectGeneratorBase::WriteIfDifferent( "XCodeProj", output, m_Name ) == false )
        {
            return Node::NODE_RESULT_FAILED; // WriteIfDifferent will have emitted an error
        }
    }

    // Get folder containing project.pbxproj
    const char * projectFolderSlash = m_Name.FindLast( NATIVE_SLASH );
    ASSERT( projectFolderSlash );
    const AStackString<> folder( m_Name.Get(), projectFolderSlash );

    // Generate user-specific xcschememanagement.plist
    {
        // Get the user name
        AStackString<> userName;
        if ( Env::GetLocalUserName( userName ) == false )
        {
            FLOG_ERROR( "Failed to determine username for '%s'", m_Name.Get() );
            return Node::NODE_RESULT_FAILED;
        }

        // Create the plist
        const AString & output = g.GenerateUserSchemeMangementPList();

        // Write to disk if missing (not written if different as this could stomp user settings)
        AStackString<> plist;
        #if defined( __WINDOWS__ )
            plist.Format( "%s\\xcuserdata\\%s.xcuserdatad\\xcschemes\\xcschememanagement.plist", folder.Get(), userName.Get() );
        #else
            plist.Format( "%s/xcuserdata/%s.xcuserdatad/xcschemes/xcschememanagement.plist", folder.Get(), userName.Get() );
        #endif
        if ( ProjectGeneratorBase::WriteIfMissing( "XCodeProj", output, plist ) == false )
        {
            return Node::NODE_RESULT_FAILED; // WriteIfMissing will have emitted an error
        }
    }

    // Generate .xcscheme file
    {
        // Create the plist
        const AString & output = g.GenerateXCScheme();

        // Write to disk if missing (not written if different as this could stomp user settings)
        AStackString<> xcscheme;
        #if defined( __WINDOWS__ )
            xcscheme.Format( "%s\\xcshareddata\\xcschemes\\%s.xcscheme", folder.Get(), g.GetProjectName().Get() );
        #else
            xcscheme.Format( "%s/xcshareddata/xcschemes/%s.xcscheme", folder.Get(), g.GetProjectName().Get() );
        #endif
        if ( ProjectGeneratorBase::WriteIfMissing( "XCodeProj", output, xcscheme ) == false )
        {
            return Node::NODE_RESULT_FAILED; // WriteIfMissing will have emitted an error
        }
    }

    return Node::NODE_RESULT_OK;
}

// PostLoad
//------------------------------------------------------------------------------
/*virtual*/ void XCodeProjectNode::PostLoad( NodeGraph & nodeGraph )
{
    XCodeProjectConfig::ResolveTargets( nodeGraph, m_ProjectConfigs );
}

//------------------------------------------------------------------------------
