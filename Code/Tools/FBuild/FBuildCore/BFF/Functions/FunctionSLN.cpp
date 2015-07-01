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
#include "Core/Strings/AStackString.h"

// CONSTRUCTOR
//------------------------------------------------------------------------------
FunctionSLN::FunctionSLN()
: Function( "SLN" )
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
        return ( a.m_Config == b.m_Config )
            ? a.m_Platform < b.m_Platform
            : a.m_Config   < b.m_Config ;
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
    AString solutionVisualStudioVersion;
    AString solutionMinimumVisualStudioVersion;
    if ( !GetString( funcStartIter, solutionVisualStudioVersion,          ".SolutionVisualStudioVersion", false ) ||
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

            VCXProjectNode * const project = ResolveVCXProjectRecurse( node );

            if ( project == nullptr )
            {
                // don't know how to handle this type of node
                Error::Error_1005_UnsupportedNodeType( funcStartIter, this, "SolutionProjects", node->GetName(), node->GetType() );
                return false;
            }

            // check that this project contains all .SolutionConfigs
            const Array< VSProjectConfig > & projectConfigs = project->GetConfigs();

            for ( size_t i = 0 ; i < configs.GetSize() ; ++i )
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

    SLNNode * sln = ng.CreateSLNNode(   solutionOutput,
                                        solutionVisualStudioVersion,
                                        solutionMinimumVisualStudioVersion,
                                        configs,
                                        projects );

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

//------------------------------------------------------------------------------
