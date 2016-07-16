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

// ResolveVCXProject
//------------------------------------------------------------------------------
VCXProjectNode * FunctionSLN::ResolveVCXProject( NodeGraph & nodeGraph, const BFFIterator & iter, const AString & projectName ) const
{
	// Find the Node
    Node * node = nodeGraph.FindNode( projectName );
    if ( node == nullptr )
    {
        Error::Error_1104_TargetNotDefined( iter, this, ".Projects", projectName );
        return nullptr;
    }
	
    VCXProjectNode * project = ResolveVCXProjectRecurse( node );
    if ( project )
	{
		return project;
	}

    // don't know how to handle this type of node
    Error::Error_1005_UnsupportedNodeType( iter, this, ".Projects", node->GetName(), node->GetType() );
	return nullptr;
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
/*virtual*/ bool FunctionSLN::Commit( NodeGraph & nodeGraph, const BFFIterator & funcStartIter ) const
{
    AStackString<> solutionOutput;
    Array< AString > solutionProjects( 8, true );
    if ( !GetString( funcStartIter, solutionOutput,     ".SolutionOutput", true ) ||
         !GetStrings( funcStartIter, solutionProjects,  ".SolutionProjects", false ) )
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

			GetStringFromStruct( s, ".SolutionPlatform", newConfig.m_SolutionPlatform );
			GetStringFromStruct( s, ".SolutionConfig", newConfig.m_SolutionConfig );

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

			// Clean slashes
            newFolder.m_Path.Replace( OTHER_SLASH, NATIVE_SLASH );
			if (newFolder.m_Path.EndsWith(NATIVE_SLASH))
			{
				newFolder.m_Path.SetLength(newFolder.m_Path.GetLength() - 1); // Remove trailing slash
			}

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
            if ( !GetStringOrArrayOfStringsFromStruct( funcStartIter, s, ".Projects", newFolder.m_ProjectNames ) )
            {
                return false; // GetStringOrArrayOfStringsFromStruct has emitted an error
            }

            // check if this project is included in the solution
            for ( const AString & projectName : newFolder.m_ProjectNames )
            {
                if ( solutionProjects.Find( projectName ) == nullptr )
                {
					solutionProjects.Append( projectName );
                }
            }

            folders.Append( newFolder );
        }
    }

    // Check for existing node
    if ( nodeGraph.FindNode( solutionOutput ) )
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
            VCXProjectNode * project = ResolveVCXProject( nodeGraph, funcStartIter, *it );
            if ( project == nullptr )
            {
                return false; // ResolveVCXProject will have emitted error
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
				// Get associate project file
				VCXProjectNode * project = ResolveVCXProject( nodeGraph, funcStartIter, *it2 );
				if ( project == nullptr )
				{
					return false; // ResolveVCXProjectRecurse will have emitted error
				}

                ASSERT( projects.Find( project ) ); // Sanity check in global list

				// fixup name to be to final project
                *it2 = project->GetName();
            }
        }
    }

    // resolves VCXProject node referenced by solutionBuildProject
    if ( solutionBuildProject.GetLength() > 0 )
    {
		// Get associate project file
		const VCXProjectNode * project = ResolveVCXProject( nodeGraph, funcStartIter, solutionBuildProject );
		if ( project == nullptr )
		{
			return false; // ResolveVCXProject will have emitted error
		}

        if ( projects.Find( project ) == nullptr )
        {
            // project referenced in .SolutionBuildProject is not referenced in .SolutionProjects
            Error::Error_1104_TargetNotDefined( funcStartIter, this, ".SolutionBuildProject", project->GetName() );
            return false;
        }

        solutionBuildProject = project->GetName();
    }

    // Project Dependencies
    Array< SLNDependency > slnDeps( 0, true );
    const BFFVariable * projectDepsVar = BFFStackFrame::GetVar( ".SolutionDependencies" );
    if ( projectDepsVar )
    {
        if ( projectDepsVar->IsArrayOfStructs() == false )
        {
            Error::Error_1050_PropertyMustBeOfType( funcStartIter, this, ".SolutionDependencies", projectDepsVar->GetType(), BFFVariable::VAR_ARRAY_OF_STRUCTS );
            return false;
        }
	
		slnDeps.SetCapacity( projectDepsVar->GetArrayOfStructs().GetSize() );
		for ( const BFFVariable * s : projectDepsVar->GetArrayOfStructs() )
		{
            // .Projects must be provided
            // .Dependencies must be provided
			SLNDependency deps;
            if ( !GetStringOrArrayOfStringsFromStruct( funcStartIter, s, ".Projects", deps.m_Projects ) ||
				 !GetStringOrArrayOfStringsFromStruct( funcStartIter, s, ".Dependencies", deps.m_Dependencies ) )
            {
                return false; // GetStringOrArrayOfStringsFromStruct has emitted an error
            }

			// fixup
			for ( AString & projectName : deps.m_Projects )
			{
				// Get associated project file
				const VCXProjectNode * project = ResolveVCXProject( nodeGraph, funcStartIter, projectName );
				if ( project == nullptr )
				{
					return false; // ResolveVCXProject will have emitted error
				}
				projectName = project->GetName();
			}
			for ( AString & projectName : deps.m_Dependencies )
			{
				// Get associated project file
				const VCXProjectNode * project = ResolveVCXProject( nodeGraph, funcStartIter, projectName );
				if ( project == nullptr )
				{
					return false; // ResolveVCXProject will have emitted error
				}
				projectName = project->GetName();
			}

			slnDeps.Append( deps );
		}
	}

    SLNNode * sln = nodeGraph.CreateSLNNode( solutionOutput,
                                        solutionBuildProject,
                                        solutionVisualStudioVersion,
                                        solutionMinimumVisualStudioVersion,
                                        configs,
                                        projects,
										slnDeps,
                                        folders );

    ASSERT( sln );

    return ProcessAlias( nodeGraph, funcStartIter, sln );
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

// GetStringOrArrayOfStringsFromStruct
//------------------------------------------------------------------------------
bool FunctionSLN::GetStringOrArrayOfStringsFromStruct( const BFFIterator & iter, const BFFVariable * s, const char * name, Array< AString > & result ) const
{
    for ( const BFFVariable * v : s->GetStructMembers() )
    {
        if ( v->GetName() == name )
        {
			if ( ( v->IsArrayOfStrings() == false ) && ( v->IsString() == false ) )
			{
				Error::Error_1050_PropertyMustBeOfType( iter, this, name, v->GetType(), BFFVariable::VAR_ARRAY_OF_STRINGS, BFFVariable::VAR_STRING );
				return false;
			}

			if ( v->IsArrayOfStrings() )
			{
				result = v->GetArrayOfStrings();
			}
			else
			{
				result.Append( v->GetString() );
			}
			return true; // found
		}
    }

    // TODO:B custom error
    Error::Error_1101_MissingProperty( iter, this, AStackString<>( name ) );
    return false;
}

//------------------------------------------------------------------------------
