// FunctionVCXProject
//------------------------------------------------------------------------------

// Includes
//------------------------------------------------------------------------------
#include "Tools/FBuild/FBuildCore/PrecompiledHeader.h"

// FBuild
#include "FunctionVCXProject.h"
#include "Tools/FBuild/FBuildCore/FBuild.h"
#include "Tools/FBuild/FBuildCore/BFF/BFFStackFrame.h"
#include "Tools/FBuild/FBuildCore/BFF/BFFVariable.h"
#include "Tools/FBuild/FBuildCore/Graph/NodeGraph.h"
#include "Tools/FBuild/FBuildCore/Graph/DirectoryListNode.h"
#include "Tools/FBuild/FBuildCore/Graph/VCXProjectNode.h"
#include "Tools/FBuild/FBuildCore/Helpers/ProjectGeneratorBase.h"

// Core
#include "Core/Strings/AStackString.h"

// CONSTRUCTOR
//------------------------------------------------------------------------------
FunctionVCXProject::FunctionVCXProject()
: Function( "VCXProject" )
{
}

// AcceptsHeader
//------------------------------------------------------------------------------
/*virtual*/ bool FunctionVCXProject::AcceptsHeader() const
{
	return true;
}

// Commit
//------------------------------------------------------------------------------
/*virtual*/ bool FunctionVCXProject::Commit( NodeGraph & nodeGraph, const BFFIterator & funcStartIter ) const
{
	// required
	AStackString<> projectOutput;
	AStackString<> rootNamespace;
	AStackString<> projectGuid;
	AStackString<> defaultLanguage;
	AStackString<> applicationEnvironment;
	if ( !GetString( funcStartIter, projectOutput,		".ProjectOutput", true ) ||
		 !GetString( funcStartIter, rootNamespace,		".RootNamespace", false ) ||
		 !GetString( funcStartIter, projectGuid,		".ProjectGuid", false ) ||
		 !GetString( funcStartIter, defaultLanguage,	".DefaultLanguage", false ) ||
		 !GetString( funcStartIter, applicationEnvironment,	".ApplicationEnvironment", false ) )
	{
		return false;
	}

	// optional inputs
	Array< AString > inputPaths;
	Array< AString > inputPathsExclude;
	if ( !GetStrings( funcStartIter, inputPaths,		".ProjectInputPaths", false ) ||
		 !GetStrings( funcStartIter, inputPathsExclude,	".ProjectInputPathsExclude", false ) )
	{
		return false;
	}

	// project base
	Array< AString > basePaths;
	if ( !GetStrings( funcStartIter, basePaths,	".ProjectBasePath", false ) )
	{
		return false;
	}
	CleanFolderPaths( basePaths );

	// references
	Array< AString > references;
	Array< AString > projectReferences;
	if ( !GetStrings( funcStartIter, references,		".ProjectReferences", false ) ||
		 !GetStrings( funcStartIter, projectReferences,	".ProjectProjectReferences", false ) )
	{
		return false;
	}

	// permitted file extensions
	Array< AString > allowedFileExtensions( 0, true );
	if ( !GetStrings( funcStartIter, allowedFileExtensions, ".ProjectAllowedFileExtensions", false ) )
	{
		return true;
	}
	if ( allowedFileExtensions.IsEmpty() )
	{
		ProjectGeneratorBase::GetDefaultAllowedFileExtensions( allowedFileExtensions );
	}
	else
	{
		ProjectGeneratorBase::FixupAllowedFileExtensions( allowedFileExtensions );
	}

	// files and filesToExclude
	Array< AString > files( 8, true );
	Array< AString > filesToExclude( 8, true );	
	Array< AString > patternToExclude(8, true);
	if ( !GetStrings( funcStartIter, files,				".ProjectFiles", false ) ||
 		 !GetStrings( funcStartIter, filesToExclude,	".ProjectFilesToExclude", false ) ||
		 !GetStrings( funcStartIter, patternToExclude,  ".ProjectPatternToExclude", false) )
	{
		return false;
	}

	// filetypes
	Array< VSProjectFileType > fileTypes;
	const BFFVariable * projectFileTypes = BFFStackFrame::GetVar( ".ProjectFileTypes" );
	if ( projectFileTypes )
	{
		if ( projectFileTypes->IsArrayOfStructs() == false )
		{
			Error::Error_1050_PropertyMustBeOfType( funcStartIter, this, ".ProjectFileTypes", projectFileTypes->GetType(), BFFVariable::VAR_ARRAY_OF_STRUCTS );
			return false;
		}

		const Array< const BFFVariable * > & structs = projectFileTypes->GetArrayOfStructs();
		const BFFVariable * const * end = structs.End();
		for ( const BFFVariable ** it = structs.Begin(); it != end; ++it )
		{
			const BFFVariable * s = *it;

			VSProjectFileType ft;

			// .FileType must be provided
			if ( !GetStringFromStruct( s, ".FileType",	ft.m_FileType ) )
			{
				// TODO:B custom error
				Error::Error_1101_MissingProperty( funcStartIter, this, AStackString<>( ".FileType" ) );
				return false;
			}

			// .Pattern must be provided
			if ( !GetStringFromStruct( s, ".Pattern",	ft.m_Pattern ) )
			{
				// TODO:B custom error
				Error::Error_1101_MissingProperty( funcStartIter, this, AStackString<>( ".Pattern" ) );
				return false;
			}

			fileTypes.Append( ft );
		}
	}

	// path cleaning
	CleanFolderPaths( inputPaths );			// input paths
	CleanFolderPaths( inputPathsExclude );	// exclude paths
	CleanFilePaths( files );				// explicit files
	CleanFilePaths( patternToExclude );		// explicit files

	// per-config options
	VSProjectConfig baseConfig;

	// various options
	if ( !GetString( funcStartIter, baseConfig.m_BuildCommand,	".ProjectBuildCommand", false ) ||
		 !GetString( funcStartIter, baseConfig.m_RebuildCommand,".ProjectRebuildCommand", false ) ||
		 !GetString( funcStartIter, baseConfig.m_CleanCommand,	".ProjectCleanCommand", false ) ||
		 !GetString( funcStartIter, baseConfig.m_Output,		".Output", false ) ||
		 !GetString( funcStartIter, baseConfig.m_PreprocessorDefinitions,	".PreprocessorDefinitions", false ) ||
		 !GetString( funcStartIter, baseConfig.m_IncludeSearchPath,		".IncludeSearchPath", false ) ||
		 !GetString( funcStartIter, baseConfig.m_ForcedIncludes,		".ForcedIncludes", false ) ||
		 !GetString( funcStartIter, baseConfig.m_AssemblySearchPath,	".AssemblySearchPath", false ) ||
		 !GetString( funcStartIter, baseConfig.m_ForcedUsingAssemblies,	".ForcedUsingAssemblies", false ) ||
		 !GetString( funcStartIter, baseConfig.m_AdditionalOptions,		".AdditionalOptions", false ) ||
		 !GetString( funcStartIter, baseConfig.m_OutputDirectory,		".OutputDirectory", false ) ||
		 !GetString( funcStartIter, baseConfig.m_IntermediateDirectory,	".IntermediateDirectory", false ) ||
		 !GetString( funcStartIter, baseConfig.m_Xbox360DebuggerCommand,".Xbox360DebuggerCommand", false ) ||
		 !GetString( funcStartIter, baseConfig.m_LayoutDir,				".LayoutDir", false ) ||
		 !GetString( funcStartIter, baseConfig.m_LayoutExtensionFilter,	".LayoutExtensionFilter", false ) ||
		 !GetString( funcStartIter, baseConfig.m_DebuggerFlavor,		".DebuggerFlavor", false ) ||
		 !GetString( funcStartIter, baseConfig.m_AumidOverride,			".AumidOverride", false ) ||
		 !GetString( funcStartIter, baseConfig.m_PlatformToolset,		".PlatformToolset", false ) ||
		 !GetString( funcStartIter, baseConfig.m_DeploymentType,		".DeploymentType", false ) ||
		 !GetString( funcStartIter, baseConfig.m_DeploymentFiles,		".DeploymentFiles", false ) ||
		 !GetString( funcStartIter, baseConfig.m_LocalDebuggerCommandArguments,	".LocalDebuggerCommandArguments", false ) ||
		 !GetString( funcStartIter, baseConfig.m_LocalDebuggerWorkingDirectory,	".LocalDebuggerWorkingDirectory", false ) ||
		 !GetString( funcStartIter, baseConfig.m_LocalDebuggerCommand,			".LocalDebuggerCommand", false ) ||
		 !GetString( funcStartIter, baseConfig.m_LocalDebuggerEnvironment,		".LocalDebuggerEnvironment", false ) )
	{
		return false;
	}

	// create configs
	Array< VSProjectConfig > configs( 16, true );

	const BFFVariable * projectConfigs = BFFStackFrame::GetVar( ".ProjectConfigs" );
	if ( projectConfigs )
	{
		if ( projectConfigs->IsArrayOfStructs() == false )
		{
			Error::Error_1050_PropertyMustBeOfType( funcStartIter, this, ".ProjectConfigs", projectConfigs->GetType(), BFFVariable::VAR_ARRAY_OF_STRUCTS );
			return false;
		}

		const Array< const BFFVariable * > & structs = projectConfigs->GetArrayOfStructs();
		const BFFVariable * const * end = structs.End();
		for ( const BFFVariable ** it = structs.Begin(); it != end; ++it )
		{
			const BFFVariable * s = *it;

			// start with the base configuration
			VSProjectConfig newConfig( baseConfig );

			// .Platform must be provided
			if ( !GetStringFromStruct( s, ".Platform",	newConfig.m_Platform ) )
			{
				// TODO:B custom error
				Error::Error_1101_MissingProperty( funcStartIter, this, AStackString<>( ".Platform" ) );
				return false;
			}

			// .Config must be provided
			if ( !GetStringFromStruct( s, ".Config",	newConfig.m_Config ) )
			{
				// TODO:B custom error
				Error::Error_1101_MissingProperty( funcStartIter, this, AStackString<>( ".Config" ) );
				return false;
			}

			// .Target is optional
			AStackString<> target;
			if ( GetStringFromStruct( s, ".Target", target ) )
			{
				if ( target.IsEmpty() == false )
				{
					newConfig.m_Target = nodeGraph.FindNode( target );
					if ( !newConfig.m_Target )
					{
						Error::Error_1104_TargetNotDefined( funcStartIter, this, ".Target", target );
						return false;
					}
				}
			}

			GetStringFromStruct( s, ".ProjectBuildCommand",		newConfig.m_BuildCommand );
			GetStringFromStruct( s, ".ProjectRebuildCommand",	newConfig.m_RebuildCommand );
			GetStringFromStruct( s, ".ProjectCleanCommand",		newConfig.m_CleanCommand );
			GetStringFromStruct( s, ".Output",					newConfig.m_Output );
			GetStringFromStruct( s, ".PreprocessorDefinitions",	newConfig.m_PreprocessorDefinitions );
			GetStringFromStruct( s, ".IncludeSearchPath",		newConfig.m_IncludeSearchPath );
			GetStringFromStruct( s, ".ForcedIncludes",			newConfig.m_ForcedIncludes );
			GetStringFromStruct( s, ".AssemblySearchPath",		newConfig.m_AssemblySearchPath );
			GetStringFromStruct( s, ".ForcedUsingAssemblies",	newConfig.m_ForcedUsingAssemblies );
			GetStringFromStruct( s, ".AdditionalOptions",		newConfig.m_AdditionalOptions );
			GetStringFromStruct( s, ".OutputDirectory",			newConfig.m_OutputDirectory );
			GetStringFromStruct( s, ".IntermediateDirectory",	newConfig.m_IntermediateDirectory );
		 	GetStringFromStruct( s, ".LayoutDir",				newConfig.m_LayoutDir );
			GetStringFromStruct( s, ".LayoutExtensionFilter",	newConfig.m_LayoutExtensionFilter );
			GetStringFromStruct( s, ".Xbox360DebuggerCommand",	newConfig.m_Xbox360DebuggerCommand );
			GetStringFromStruct( s, ".DebuggerFlavor",			newConfig.m_DebuggerFlavor );
			GetStringFromStruct( s, ".AumidOverride",			newConfig.m_AumidOverride );
			GetStringFromStruct( s, ".PlatformToolset",			newConfig.m_PlatformToolset );
			GetStringFromStruct( s, ".DeploymentType",			newConfig.m_DeploymentType );
			GetStringFromStruct( s, ".DeploymentFiles",			newConfig.m_DeploymentFiles );
			GetStringFromStruct( s, ".LocalDebuggerCommandArguments",	newConfig.m_LocalDebuggerCommandArguments );
			GetStringFromStruct( s, ".LocalDebuggerWorkingDirectory",	newConfig.m_LocalDebuggerWorkingDirectory );
			GetStringFromStruct( s, ".LocalDebuggerCommand",			newConfig.m_LocalDebuggerCommand );
			GetStringFromStruct( s, ".LocalDebuggerEnvironment",		newConfig.m_LocalDebuggerEnvironment );

			configs.Append( newConfig );
		}
	}
	else
	{
		// no user specified configs, make some defaults

		// start from the default
		VSProjectConfig config( baseConfig );

		// make the configs
		config.m_Platform	= "Win32";
		config.m_Config		= "Debug";
		configs.Append( config );
		config.m_Config		= "Release";
		configs.Append( config );
		config.m_Platform	= "x64";
		configs.Append( config );
		config.m_Config		= "Debug";
		configs.Append( config );
	}

	// create all of the DirectoryListNodes we need
	Dependencies dirNodes( inputPaths.GetSize() );
	if ( !GetDirectoryListNodeList( nodeGraph, funcStartIter, inputPaths, Array< AString >(), Array< AString >(), true, &allowedFileExtensions, "ProjectInputPaths", dirNodes ) )
	{
		return false; // GetDirectoryListNodeList will have emitted an error
	}

	// Check for existing node
	if ( nodeGraph.FindNode( projectOutput ) )
	{
		Error::Error_1100_AlreadyDefined( funcStartIter, this, projectOutput );
		return false;
	}

	VCXProjectNode * pn = nodeGraph.CreateVCXProjectNode( projectOutput,
												   basePaths,
												   dirNodes,
												   inputPathsExclude, // TODO:B Remove this (handled by DirectoryListNode now)
												   patternToExclude,
												   files,
												   filesToExclude,
												   rootNamespace,
												   projectGuid,
												   defaultLanguage,
												   applicationEnvironment,
												   configs,
												   fileTypes,
												   references,
												   projectReferences );

	ASSERT( pn );

	return ProcessAlias( nodeGraph, funcStartIter, pn );
}

// GetStringFromStruct
//------------------------------------------------------------------------------
bool FunctionVCXProject::GetStringFromStruct( const BFFVariable * s, const char * name, AString & result ) const
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
