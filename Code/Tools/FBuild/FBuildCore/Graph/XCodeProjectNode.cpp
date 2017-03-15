// XCodeProjectNode.cpp
//------------------------------------------------------------------------------

// Includes
//------------------------------------------------------------------------------
#include "Tools/FBuild/FBuildCore/PrecompiledHeader.h"

#include "XCodeProjectNode.h"

#include "Tools/FBuild/FBuildCore/FBuild.h"
#include "Tools/FBuild/FBuildCore/FLog.h"
#include "Tools/FBuild/FBuildCore/BFF/Functions/Function.h"
#include "Tools/FBuild/FBuildCore/Graph/NodeGraph.h"
#include "Tools/FBuild/FBuildCore/Graph/DirectoryListNode.h"
#include "Tools/FBuild/FBuildCore/Helpers/ProjectGeneratorBase.h"
#include "Tools/FBuild/FBuildCore/Helpers/XCodeProjectGenerator.h"

#include "Core/FileIO/IOStream.h"
#include "Core/FileIO/PathUtils.h"
#include "Core/Strings/AStackString.h"

// Reflection
//------------------------------------------------------------------------------
REFLECT_STRUCT_BEGIN_BASE( XCodeProjectConfig )
    REFLECT( m_Config,  "Config",   MetaNone() )
    REFLECT( m_Target,  "Target",   MetaOptional() )
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
REFLECT_END( XCodeProjectNode )

// XCodeProjectConfig::ResolveTagets
//------------------------------------------------------------------------------
/*static*/ bool XCodeProjectConfig::ResolveTagets( NodeGraph & nodeGraph,
                                                   Array< XCodeProjectConfig > & configs,
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
            return true;
        }

        // Find the node
        const Node * node = nodeGraph.FindNode( config.m_Target );
        if ( node == nullptr )
        {
            if ( iter && function )
            {
                Error::Error_1104_TargetNotDefined( *iter, function, ".Target", config.m_Target );
            }
            return false;
        }

        config.m_TargetNode = node;
    }
    return true;
}

// CONSTRUCTOR
//------------------------------------------------------------------------------
XCodeProjectNode::XCodeProjectNode()
    : FileNode( AString::GetEmpty(), Node::FLAG_NONE )
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
bool XCodeProjectNode::Initialize( NodeGraph & nodeGraph, const BFFIterator & iter, const Function * function )
{
    ProjectGeneratorBase::FixupAllowedFileExtensions( m_ProjectAllowedFileExtensions );

    Dependencies dirNodes( m_ProjectInputPaths.GetSize() );
    if ( !function->GetDirectoryListNodeList( nodeGraph, iter, m_ProjectInputPaths, m_ProjectInputPathsExclude, m_ProjectFilesToExclude, m_PatternToExclude, true, &m_ProjectAllowedFileExtensions, "ProjectInputPaths", dirNodes ) )
    {
        return false; // GetDirectoryListNodeList will have emitted an error
    }

    // TODO:B use m_ProjectFiles instead of finding it again
    Dependencies fileNodes( m_ProjectFiles.GetSize() );
    if ( !function->GetNodeList( nodeGraph, iter, ".ProjectFiles", fileNodes ) )
    {
        return false; // GetNodeList will have emitted an error
    }

    ASSERT( m_StaticDependencies.IsEmpty() );
    m_StaticDependencies.Append( dirNodes );
    m_StaticDependencies.Append( fileNodes );

    // Resolve Target names to Node pointers for layer use
    if ( XCodeProjectConfig::ResolveTagets( nodeGraph, m_ProjectConfigs, &iter, function ) == false )
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

    // Add files
    for ( const Dependency & dep : m_StaticDependencies )
    {
        const Node * n = dep.GetNode();
        if ( n->GetType() == Node::DIRECTORY_LIST_NODE )
        {
            const DirectoryListNode * dln = n->CastTo< DirectoryListNode >();
            const auto & files = dln->GetFiles();
            for ( const auto & file : files )
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
    for ( const auto& cfg : m_ProjectConfigs )
    {
        g.AddConfig( cfg.m_Config, cfg.m_TargetNode );
    }

    // Generate project.pbxproj file
    const AString & output = g.Generate();
    if ( ProjectGeneratorBase::WriteIfDifferent( "XCodeProj", output, m_Name ) == false )
    {
        return Node::NODE_RESULT_FAILED; // WriteIfDifferent will have emitted an error
    }

    return Node::NODE_RESULT_OK;
}

// Load
//------------------------------------------------------------------------------
/*static*/ Node * XCodeProjectNode::Load( NodeGraph & nodeGraph, IOStream & stream )
{
    NODE_LOAD( AStackString<>, name );

    auto * n = nodeGraph.CreateXCodeProjectNode( name );

    if ( n->Deserialize( nodeGraph, stream ) == false )
    {
        return nullptr;
    }

    // Resolve Target names to Node pointers for layer use
    if ( XCodeProjectConfig::ResolveTagets( nodeGraph, n->m_ProjectConfigs ) == false )
    {
        ASSERT( false ); // Should be impossible to be loading a DB where the Target cannot be resolved
        return nullptr;
    }

    return n;
}

// Save
//------------------------------------------------------------------------------
/*virtual*/ void XCodeProjectNode::Save( IOStream & stream ) const
{
    NODE_SAVE( m_Name );
    Node::Serialize( stream );
}

//------------------------------------------------------------------------------
