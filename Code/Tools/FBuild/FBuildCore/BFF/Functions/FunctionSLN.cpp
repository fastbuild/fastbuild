// FunctionSLN
//------------------------------------------------------------------------------

// Includes
//------------------------------------------------------------------------------
#include "Tools/FBuild/FBuildCore/PrecompiledHeader.h"

// FBuild
#include "FunctionSLN.h"
#include "Tools/FBuild/FBuildCore/FBuild.h"
#include "Tools/FBuild/FBuildCore/BFF/BFFStackFrame.h"
#include "Tools/FBuild/FBuildCore/BFF/BFFVariable.h"
#include "Tools/FBuild/FBuildCore/Graph/AliasNode.h"
#include "Tools/FBuild/FBuildCore/Graph/NodeGraph.h"
#include "Tools/FBuild/FBuildCore/Graph/SLNNode.h"
#include "Tools/FBuild/FBuildCore/Graph/VCXProjectNode.h"

// Core
#include "Core/FileIO/PathUtils.h"
#include "Core/Strings/AStackString.h"

// CONSTRUCTOR
//------------------------------------------------------------------------------
FunctionSLN::FunctionSLN()
: Function( "VSSolution" )
{
}

// AcceptsHeader
//------------------------------------------------------------------------------
/*virtual*/ bool FunctionSLN::AcceptsHeader() const
{
    return true;
}

// ResolveVCXProjectRecurse
//------------------------------------------------------------------------------
static VCXProjectNode * ResolveVCXProjectRecurse( Node * node )
{
    if ( node == nullptr )
    {
        return nullptr;
    }

    // search inside targets if this is an alias
    if ( node->GetType() == Node::ALIAS_NODE )
    {
        AliasNode * const alias = node->CastTo< AliasNode >();
        const Dependencies & targets = alias->GetAliasedNodes();

        const Dependency * const end = targets.End();
        for ( const Dependency * it = targets.Begin() ; it != end ; ++it )
        {
            VCXProjectNode *result = ResolveVCXProjectRecurse( it->GetNode() );
            if ( result != nullptr )
            {
                return result;
            }
        }
    }

    // check that this a vcxproject
    if ( node->GetType() != Node::VCXPROJECT_NODE )
    {
        // don't know how to handle this type of node
        return nullptr;
    }

    return node->CastTo< VCXProjectNode >();
}

// VSProjectConfigComp
//------------------------------------------------------------------------------
struct VSProjectConfigComp
{
    bool operator ()( const VSProjectConfig & a, const VSProjectConfig & b ) const
    {
        int32_t cmpConfig = a.m_Config.CompareI( b.m_Config );
        return ( cmpConfig == 0 )
            ? a.m_Platform < b.m_Platform
            : cmpConfig < 0 ;
    }
};

// VCXProjectNodeComp
//------------------------------------------------------------------------------
struct VCXProjectNodeComp
{
    bool operator ()( const VCXProjectNode * a, const VCXProjectNode * b ) const
    {
        return ( a->GetName() < b->GetName() );
    }
};

// Commit
//------------------------------------------------------------------------------
/*virtual*/ bool FunctionSLN::Commit( const BFFIterator & funcStartIter ) const
{
    // required
    AStackString<> solutionOutput;
    Array< AString > solutionProjects( 8, true );
    if ( !GetString( funcStartIter, solutionOutput,     ".SolutionOutput", true ) ||
         !GetStrings( funcStartIter, solutionProjects,  ".SolutionProjects", true ) )
    {
        return false;
    }

    // optional inputs
    AString solutionBuildProject;
    AString solutionVisualStudioVersion;
    AString solutionMinimumVisualStudioVersion;
    if ( !GetString( funcStartIter, solutionBuildProject,                 ".SolutionBuildProject", false ) ||
         !GetString( funcStartIter, solutionVisualStudioVersion,          ".SolutionVisualStudioVersion", false ) ||
         !GetString( funcStartIter, solutionMinimumVisualStudioVersion,   ".SolutionMinimumVisualStudioVersion", false ) )
    {
        return false;
    }

    // base config
    VSProjectConfig baseConfig;

    // create configs
    Array< VSProjectConfig > configs( 16, true );

    const BFFVariable * solutionConfigs = BFFStackFrame::GetVar( ".SolutionConfigs" );
    if ( solutionConfigs )
    {
        if ( solutionConfigs->IsArrayOfStructs() == false )
        {
            Error::Error_1050_PropertyMustBeOfType( funcStartIter, this, ".SolutionConfigs", solutionConfigs->GetType(), BFFVariable::VAR_ARRAY_OF_STRUCTS );
            return false;
        }

        const Array< const BFFVariable * > & structs = solutionConfigs->GetArrayOfStructs();
        const BFFVariable * const * end = structs.End();
        for ( const BFFVariable ** it = structs.Begin(); it != end; ++it )
        {
            const BFFVariable * s = *it;

            // start with the base configuration
            VSProjectConfig newConfig( baseConfig );

            // .Platform must be provided
            if ( !GetStringFromStruct( s, ".Platform",  newConfig.m_Platform ) )
            {
                // TODO:B custom error
                Error::Error_1101_MissingProperty( funcStartIter, this, AStackString<>( ".Platform" ) );
                return false;
            }

            // .Config must be provided
            if ( !GetStringFromStruct( s, ".Config",    newConfig.m_Config ) )
            {
                // TODO:B custom error
                Error::Error_1101_MissingProperty( funcStartIter, this, AStackString<>( ".Config" ) );
                return false;
            }

            configs.Append( newConfig );
        }
    }
    else
    {
        // no user specified configs, make some defaults

        // start from the default
        VSProjectConfig config( baseConfig );

        // make the configs
        config.m_Platform   = "Win32";
        config.m_Config     = "Debug";
        configs.Append( config );
        config.m_Config     = "Release";
        configs.Append( config );
        config.m_Platform   = "x64";
        configs.Append( config );
        config.m_Config     = "Debug";
        configs.Append( config );
    }

    // sort project configs by config and by platform (like visual)
    configs.Sort( VSProjectConfigComp() );

    // create solution folders
    Array< SLNSolutionFolder > folders( 16, true );

    const BFFVariable * solutionFolders = BFFStackFrame::GetVar( ".SolutionFolders" );
    if ( solutionFolders )
    {
        if ( solutionFolders->IsArrayOfStructs() == false )
        {
            Error::Error_1050_PropertyMustBeOfType( funcStartIter, this, ".SolutionFolders", solutionFolders->GetType(), BFFVariable::VAR_ARRAY_OF_STRUCTS );
            return false;
        }

        const Array< const BFFVariable * > & structs = solutionFolders->GetArrayOfStructs();
        const BFFVariable * const * end = structs.End();
        for ( const BFFVariable ** it = structs.Begin(); it != end; ++it )
        {
            const BFFVariable * s = *it;

            // start with the base configuration
            SLNSolutionFolder newFolder;

            // .Path must be provided
            if ( !GetStringFromStruct( s, ".Path", newFolder.m_Path ) )
            {
                // TODO:B custom error
                Error::Error_1101_MissingProperty( funcStartIter, this, AStackString<>( ".Path" ) );
                return false;
            }

            newFolder.m_Path.Replace( OTHER_SLASH, NATIVE_SLASH );

            // check if this path was already defined
            {
                const SLNSolutionFolder * const end2 = folders.End();
                for ( const SLNSolutionFolder * it2 = folders.Begin() ; it2 != end2 ; ++it2 )
                {
                    if ( it2->m_Path == newFolder.m_Path  )
                    {
                        // TODO:B custom error
                        Error::Error_1100_AlreadyDefined( funcStartIter, this, it2->m_Path );
                        return false;
                    }
                }
            }

            // .Projects must be provided
            if ( !GetArrayOfStringsFromStruct( s, ".Projects", newFolder.m_ProjectNames ) )
            {
                // TODO:B custom error
                Error::Error_1101_MissingProperty( funcStartIter, this, AStackString<>( ".Projects" ) );
                return false;
            }
            // check if this project is included in the solution
            {
                const AString * const end2 = newFolder.m_ProjectNames.End();
                for ( const AString * it2 = newFolder.m_ProjectNames.Begin() ; it2 != end2 ; ++it2 )
                {
                    if ( solutionProjects.Find( *it2 ) == nullptr )
                    {
                        // TODO:B custom error
                        Error::Error_1104_TargetNotDefined( funcStartIter, this, ".Projects", *it2 );
                        return false;
                    }
                }
            }

            folders.Append( newFolder );
        }
    }

    NodeGraph & ng = FBuild::Get().GetDependencyGraph();

    // Check for existing node
    if ( ng.FindNode( solutionOutput ) )
    {
        Error::Error_1100_AlreadyDefined( funcStartIter, this, solutionOutput );
        return false;
    }

    // resolves VCXProject nodes associated to solutionProjects
    Array< VCXProjectNode * > projects( solutionProjects.GetSize(), false );
    {
        const AString * const end = solutionProjects.End();
        for ( const AString * it = solutionProjects.Begin(); it != end; ++it )
        {
            Node * const node = ng.FindNode( *it );

            if ( node == nullptr )
            {
                Error::Error_1104_TargetNotDefined( funcStartIter, this, ".SolutionProjects", *it );
                return false;
            }

            VCXProjectNode * const project = ResolveVCXProjectRecurse( node );

            if ( project == nullptr )
            {
                // don't know how to handle this type of node
                Error::Error_1005_UnsupportedNodeType( funcStartIter, this, ".SolutionProjects", node->GetName(), node->GetType() );
                return false;
            }

            // check that this project contains all .SolutionConfigs
            const Array< VSProjectConfig > & projectConfigs = project->GetConfigs();

            const size_t configsSize = configs.GetSize();
            for ( size_t i = 0 ; i < configsSize ; ++i )
            {
                bool containsConfig = false;

                const VSProjectConfig * const config = &configs[i];
                const VSProjectConfig * const end2 = projectConfigs.End();
                for ( const VSProjectConfig * it2 = projectConfigs.Begin(); it2 != end2; ++it2 )
                {
                    if ( it2->m_Platform == config->m_Platform &&
                         it2->m_Config   == config->m_Config )
                    {
                        containsConfig = true;
                        break;
                    }
                }

                if ( containsConfig == false )
                {
                    // TODO: specific error message "ProjectConfigNotFound"
                    AStackString<> configName;
                    configName.Format( "%s|%s", config->m_Platform.Get(), config->m_Config.Get() );
                    Error::Error_1104_TargetNotDefined( funcStartIter, this, configName.Get(), project->GetName() );
                    return false;
                }
            }

            // append vcxproject node to solution
            projects.Append( project );
        }
    }

    // sort projects by name (like visual)
    projects.Sort( VCXProjectNodeComp() );

    // resolves VCXProject nodes associated to solutionFolders
    {
        SLNSolutionFolder * const end = folders.End();
        for ( SLNSolutionFolder * it = folders.Begin(); it != end; ++it )
        {
            // retrieves full path of contained vcxprojects

            AString * const end2 = it->m_ProjectNames.End();
            for ( AString * it2 = it->m_ProjectNames.Begin(); it2 != end2; ++it2 )
            {
                Node * const node = ng.FindNode( *it2 );

                if ( node == nullptr )
                {
                    Error::Error_1104_TargetNotDefined( funcStartIter, this, ".Projects", *it2 );
                    return false;
                }

                VCXProjectNode * const project = ResolveVCXProjectRecurse( node );

                if ( project == nullptr )
                {
                    // don't know how to handle this type of node
                    Error::Error_1005_UnsupportedNodeType( funcStartIter, this, ".Projects", node->GetName(), node->GetType() );
                    return false;
                }

                if ( projects.Find( project ) == nullptr )
                {
                    // project referenced in a solution folder is not referenced in .SolutionProjects
                    Error::Error_1104_TargetNotDefined( funcStartIter, this, ".SolutionProjects", project->GetName() );
                    return false;
                }

                *it2 = project->GetName();
            }
        }
    }

    // resolves VCXProject node referenced by solutionBuildProject
    if ( solutionBuildProject.GetLength() > 0 )
    {
        Node * const node = ng.FindNode( solutionBuildProject );

        if ( node == nullptr )
        {
            Error::Error_1104_TargetNotDefined( funcStartIter, this, ".SolutionBuildProject", solutionBuildProject );
            return false;
        }

        VCXProjectNode * const project = ResolveVCXProjectRecurse( node );

        if ( project == nullptr )
        {
            // don't know how to handle this type of node
            Error::Error_1005_UnsupportedNodeType( funcStartIter, this, ".SolutionBuildProject", node->GetName(), node->GetType() );
            return false;
        }

        if ( projects.Find( project ) == nullptr )
        {
            // project referenced in .SolutionBuildProject is not referenced in .SolutionProjects
            Error::Error_1104_TargetNotDefined( funcStartIter, this, ".SolutionBuildProject", project->GetName() );
            return false;
        }

        solutionBuildProject = project->GetName();
    }

    SLNNode * sln = ng.CreateSLNNode(   solutionOutput,
                                        solutionBuildProject,
                                        solutionVisualStudioVersion,
                                        solutionMinimumVisualStudioVersion,
                                        configs,
                                        projects,
                                        folders );

    ASSERT( sln );

    return ProcessAlias( funcStartIter, sln );
}

// GetStringFromStruct
//------------------------------------------------------------------------------
bool FunctionSLN::GetStringFromStruct( const BFFVariable * s, const char * name, AString & result ) const
{
    const Array< const BFFVariable * > & members = s->GetStructMembers();
    const BFFVariable * const * end = members.End();
    for ( const BFFVariable ** it = members.Begin(); it != end; ++it )
    {
        const BFFVariable * v = *it;
        if ( v->IsString() == false )
        {
            continue; // ignore non-strings
        }
        if ( v->GetName() == name )
        {
            result = v->GetString();
            return true; // found
        }
    }
    return false; // not found - caller decides if this is an error
}

// GetArrayOfStringsFromStruct
//------------------------------------------------------------------------------
bool FunctionSLN::GetArrayOfStringsFromStruct( const BFFVariable * s, const char * name, Array< AString > & result ) const
{
    const Array< const BFFVariable * > & members = s->GetStructMembers();
    const BFFVariable * const * end = members.End();
    for ( const BFFVariable ** it = members.Begin(); it != end; ++it )
    {
        const BFFVariable * v = *it;
        if ( v->IsArrayOfStrings() == false )
        {
            continue; // ignore non-strings
        }
        if ( v->GetName() == name )
        {
            result = v->GetArrayOfStrings();
            return true; // found
        }
    }
    return false; // not found - caller decides if this is an error
}

//------------------------------------------------------------------------------
