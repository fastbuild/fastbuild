// SLNGenerator
//------------------------------------------------------------------------------

// Includes
//------------------------------------------------------------------------------
#include "SLNGenerator.h"

#include "Tools/FBuild/FBuildCore/Graph/SLNNode.h"
#include "Tools/FBuild/FBuildCore/Graph/VSProjectBaseNode.h"
#include "Tools/FBuild/FBuildCore/Helpers/ProjectGeneratorBase.h"
#include "Tools/FBuild/FBuildCore/Helpers/VSProjectGenerator.h"

// Core
#include "Core/FileIO/IOStream.h"
#include "Core/FileIO/PathUtils.h"
#include "Core/Strings/AStackString.h"

// system
#include <stdarg.h> // for va_args
#include <stdio.h>

// CONSTRUCTOR
//------------------------------------------------------------------------------
SLNGenerator::SLNGenerator() = default;

// DESTRUCTOR
//------------------------------------------------------------------------------
SLNGenerator::~SLNGenerator() = default;

// GenerateSLN
//------------------------------------------------------------------------------
const AString & SLNGenerator::GenerateSLN( const AString & solutionFile,
                                           const AString & solutionVisualStudioVersion,
                                           const AString & solutionMinimumVisualStudioVersion,
                                           const Array< SolutionConfig > & solutionConfigs,
                                           const Array< VSProjectBaseNode * > & projects,
                                           const Array< SolutionDependency > & solutionDependencies,
                                           const Array< SolutionFolder > & solutionFolders )
{
    // preallocate to avoid re-allocations
    m_Output.SetReserved( MEGABYTE );
    m_Output.SetLength( 0 );

    // determine folder for project
    const char * lastSlash = solutionFile.FindLast( NATIVE_SLASH );
    AStackString<> solutionBasePath( solutionFile.Get(), lastSlash ? lastSlash + 1 : solutionFile.Get() );

    Array< AString > solutionProjectsToFolder( projects.GetSize(), true );
    Array< AString > solutionFolderPaths( solutionFolders.GetSize(), true );

    // construct sln file
    WriteHeader( solutionVisualStudioVersion, solutionMinimumVisualStudioVersion );
    WriteProjectListings( solutionBasePath, projects, solutionFolders, solutionDependencies, solutionProjectsToFolder );
    WriteSolutionFolderListings( solutionBasePath, solutionFolders, solutionFolderPaths );
    Write( "Global\r\n" );
    WriteSolutionConfigurationPlatforms( solutionConfigs );
    WriteProjectConfigurationPlatforms( solutionConfigs, projects );
    WriteNestedProjects( solutionProjectsToFolder, solutionFolderPaths );
    WriteFooter();

    return m_Output;
}

// WriteHeader
//------------------------------------------------------------------------------
void SLNGenerator::WriteHeader( const AString & solutionVisualStudioVersion,
                                const AString & solutionMinimumVisualStudioVersion )
{
    const char * defaultVersion         = "14.0.22823.1"; // Visual Studio 2015 RC
    const char * defaultMinimumVersion  = "10.0.40219.1"; // Visual Studio Express 2010

    const char * version = ( solutionVisualStudioVersion.GetLength() > 0 )
                         ? solutionVisualStudioVersion.Get()
                         : defaultVersion ;

    const char * minimumVersion = ( solutionMinimumVisualStudioVersion.GetLength() > 0 )
                                ? solutionMinimumVisualStudioVersion.Get()
                                : defaultMinimumVersion ;

    const char * shortVersionStart = version;
    const char * shortVersionEnd = version;
    for ( ; *shortVersionEnd && *shortVersionEnd != '.' ; ++shortVersionEnd );

    AStackString<> shortVersion( shortVersionStart, shortVersionEnd );

    // Extract primary version as an int
    uint32_t shortVersionInt = 0;
    PRAGMA_DISABLE_PUSH_MSVC( 4996 ) // This function or variable may be unsafe...
    PRAGMA_DISABLE_PUSH_CLANG_WINDOWS( "-Wdeprecated-declarations" ) // 'sscanf' is deprecated: This function or variable may be unsafe...
    VERIFY( sscanf( shortVersion.Get(), "%u", &shortVersionInt ) == 1 );
    PRAGMA_DISABLE_POP_CLANG_WINDOWS // -Wdeprecated-declarations
    PRAGMA_DISABLE_POP_MSVC // 4996

    // header
    Write( "Microsoft Visual Studio Solution File, Format Version 12.00\r\n" );
    if ( shortVersionInt >= 16 )
    {
        Write( "# Visual Studio Version %s\r\n", shortVersion.Get() );
    }
    else
    {
        Write( "# Visual Studio %s\r\n", shortVersion.Get() );
    }
    Write( "VisualStudioVersion = %s\r\n", version );
    Write( "MinimumVisualStudioVersion = %s\r\n", minimumVersion );
}

// WriteProjectListings
//------------------------------------------------------------------------------
void SLNGenerator::WriteProjectListings( const AString& solutionBasePath,
                                         const Array< VSProjectBaseNode * > & projects,
                                         const Array< SolutionFolder > & solutionFolders,
                                         const Array< SolutionDependency > & solutionDependencies,
                                         Array< AString > & solutionProjectsToFolder )
{
    // Project Listings

    const VSProjectBaseNode * const * projectsEnd = projects.End();
    for( VSProjectBaseNode ** it = projects.Begin() ; it != projectsEnd ; ++it )
    {
        AStackString<> projectPath( (*it)->GetName() );

        // get project base name only
        const char * lastSlash  = projectPath.FindLast( NATIVE_SLASH );
        const char * lastPeriod = projectPath.FindLast( '.' );
        AStackString<> projectName( lastSlash  ? lastSlash + 1  : projectPath.Get(),
                                    lastPeriod ? lastPeriod     : projectPath.GetEnd() );

        // make project path relative
        AStackString<> solutionRelativePath;
        ProjectGeneratorBase::GetRelativePath( solutionBasePath, projectPath, solutionRelativePath );
        #if !defined( __WINDOWS__ )
            solutionRelativePath.Replace( '/', '\\' ); // Convert to Windows-style slashes
        #endif

        // retrieve projectGuid
        AStackString<> projectGuid( (*it)->GetProjectGuid() );

        // Visual Studio expects the GUID to be uppercase
        projectGuid.ToUpper();

        // retrieve projectTypeGuid
        AStackString<> projectTypeGuid( (*it)->GetProjectTypeGuid() );

        // Visual Studio expects the GUID to be uppercase
        projectTypeGuid.ToUpper();

        Write( "Project(\"%s\") = \"%s\", \"%s\", \"%s\"\r\n",
               projectTypeGuid.Get(), projectName.Get(), solutionRelativePath.Get(), projectGuid.Get() );

        // Manage dependencies
        Array< AString > dependencyGUIDs( 64, true );
        const AString & fullProjectPath = (*it)->GetName();
        for ( const SolutionDependency & deps : solutionDependencies )
        {
            // is the set of deps relevant to this project?
            if ( !deps.m_Projects.Find( fullProjectPath ) )
            {
                continue;
            }

            // get all the projects this project depends on
            for ( const AString & dependency : deps.m_Dependencies )
            {
                for ( const VSProjectBaseNode * dependencyProject : projects )
                {
                    if ( dependencyProject->GetName() == dependency )
                    {
                        dependencyGUIDs.Append( dependencyProject->GetProjectGuid() );
                        break;
                    }
                }
            }
        }
        if ( !dependencyGUIDs.IsEmpty() )
        {
            Write( "\tProjectSection(ProjectDependencies) = postProject\r\n" );
            for ( const AString & guid : dependencyGUIDs )
            {
                Write( "\t\t%s = %s\r\n", guid.Get(), guid.Get() );
            }
            Write( "\tEndProjectSection\r\n" );
        }

        Write( "EndProject\r\n" );

        // check if this project is in a solution folder
        for ( const SolutionFolder & solutionFolder : solutionFolders )
        {
            // this has to be done here to have the same order of declaration (like visual)
            if ( solutionFolder.m_Projects.Find( (*it)->GetName() ) )
            {
                // generate a guid for the solution folder
                AStackString<> solutionFolderGuid;
                VSProjectGenerator::FormatDeterministicProjectGUID( solutionFolderGuid, solutionFolder.m_Path );

                solutionFolderGuid.ToUpper();

                AStackString<> projectToFolder;
                projectToFolder.Format( "\t\t%s = %s\r\n", projectGuid.Get(), solutionFolderGuid.Get() );

                solutionProjectsToFolder.Append( projectToFolder );
            }
        }
    }
}

// WriteSolutionFolderListings
//------------------------------------------------------------------------------
void SLNGenerator::WriteSolutionFolderListings( const AString & solutionBasePath,
                                                const Array< SolutionFolder > & solutionFolders,
                                                Array< AString > & solutionFolderPaths )
{
    // Create every intermediate path
    for ( const SolutionFolder & solutionFolder : solutionFolders )
    {
        if ( solutionFolderPaths.Find( solutionFolder.m_Path ) == nullptr )
        {
            solutionFolderPaths.Append( solutionFolder.m_Path );
        }

        const char * pathEnd = solutionFolder.m_Path.Find( BACK_SLASH ); // Always windows-style
        while ( pathEnd )
        {
            AStackString<> solutionFolderPath( solutionFolder.m_Path.Get(), pathEnd );
            if ( solutionFolderPaths.Find( solutionFolderPath ) == nullptr )
            {
                solutionFolderPaths.Append( solutionFolderPath );
            }

            pathEnd = solutionFolder.m_Path.Find( BACK_SLASH, pathEnd + 1 ); // Always windows-style
        }
    }

    solutionFolderPaths.Sort();

    // Solution Folders Listings

    for ( const AString & solutionFolderPath : solutionFolderPaths )
    {
        // parse solution folder name
        const char * solutionFolderName = solutionFolderPath.FindLast( BACK_SLASH ); // Always windows-style
        solutionFolderName = solutionFolderName ? solutionFolderName + 1 : solutionFolderPath.Get();

        // generate a guid for the solution folder
        AStackString<> solutionFolderGuid;
        VSProjectGenerator::FormatDeterministicProjectGUID( solutionFolderGuid, solutionFolderPath );

        // Guid must be uppercase (like visual)
        solutionFolderGuid.ToUpper();

        Write( "Project(\"{2150E333-8FDC-42A3-9474-1A3956D46DE8}\") = \"%s\", \"%s\", \"%s\"\r\n",
               solutionFolderName, solutionFolderName, solutionFolderGuid.Get() );

        // lookup solution folder to find out if it contains items
        for ( const SolutionFolder& solutionFolder : solutionFolders )
        {
            if ( solutionFolderPath.EqualsI( solutionFolder.m_Path ) )
            {
                if ( solutionFolder.m_Items.IsEmpty() == false )
                {
                    // make a local copy (to sort before writing to SLN, as Visual Studio will keep doing that after opening it):
                    Array< AString > items;
                    items.Append( solutionFolder.m_Items );
                    items.Sort();
                    Write( "\tProjectSection(SolutionItems) = preProject\r\n" );
                    for ( const AString & item : items )
                    {
                        // make item path relative
                        AStackString<> itemRelativePath;
                        ProjectGeneratorBase::GetRelativePath( solutionBasePath, item, itemRelativePath );
                        #if !defined( __WINDOWS__ )
                            itemRelativePath.Replace( '/', '\\' ); // Convert to Windows-style slashes
                        #endif
                        Write( "\t\t%s = %s\r\n", itemRelativePath.Get(), itemRelativePath.Get() );
                    }
                    Write( "\tEndProjectSection\r\n" );
                }
            }
        }

        Write( "EndProject\r\n" );
    }
}

// WriteSolutionConfigurationPlatforms
//------------------------------------------------------------------------------
void SLNGenerator::WriteSolutionConfigurationPlatforms( const Array< SolutionConfig > & solutionConfigs )
{
    Write( "\tGlobalSection(SolutionConfigurationPlatforms) = preSolution\r\n" );

    // Solution Configurations
    const SolutionConfig * const end = solutionConfigs.End();
    for( const SolutionConfig * it = solutionConfigs.Begin() ; it != end ; ++it )
    {
        Write( "\t\t%s|%s = %s|%s\r\n",
               it->m_SolutionConfig.Get(), it->m_SolutionPlatform.Get(),
               it->m_SolutionConfig.Get(), it->m_SolutionPlatform.Get() );
    }

    Write( "\tEndGlobalSection\r\n" );
}

// WriteProjectConfigurationPlatforms
//------------------------------------------------------------------------------
void SLNGenerator::WriteProjectConfigurationPlatforms( const Array< SolutionConfig > & solutionConfigs,
                                                       const Array< VSProjectBaseNode * > & projects )
{
    Write( "\tGlobalSection(ProjectConfigurationPlatforms) = postSolution\r\n" );

    // Solution Configuration Mappings to Projects
    for( const VSProjectBaseNode * project : projects )
    {
        AStackString<> projectGuid( project->GetProjectGuid() );
        projectGuid.ToUpper();

        for( const SolutionConfig & solutionConfig : solutionConfigs )
        {
            Write( "\t\t%s.%s|%s.ActiveCfg = %s|%s\r\n",
                   projectGuid.Get(),
                   solutionConfig.m_SolutionConfig.Get(), solutionConfig.m_SolutionPlatform.Get(),
                   solutionConfig.m_Config.Get(), solutionConfig.m_Platform.Get() );

            // Is project active in solution build?
            bool projectIsActive = false;
            for ( const AString & solutionBuildProject : solutionConfig.m_SolutionBuildProjects )
            {
                if ( solutionBuildProject.EqualsI( project->GetName() ) )
                {
                    projectIsActive = true;
                    break;
                }
            }

            // Is project marked for deploy?
            bool projectDeployEnabled = false;
            for ( const AString & solutionDeployProject : solutionConfig.m_SolutionDeployProjects )
            {
                if ( solutionDeployProject.EqualsI( project->GetName() ) )
                {
                    projectDeployEnabled = true;
                    break;
                }
            }

            if ( projectIsActive )
            {
                Write( "\t\t%s.%s|%s.Build.0 = %s|%s\r\n",
                       projectGuid.Get(),
                       solutionConfig.m_SolutionConfig.Get(), solutionConfig.m_SolutionPlatform.Get(),
                       solutionConfig.m_Config.Get(), solutionConfig.m_Platform.Get() );
            }
            if ( projectDeployEnabled )
            {
                Write( "\t\t%s.%s|%s.Deploy.0 = %s|%s\r\n",
                       projectGuid.Get(),
                       solutionConfig.m_SolutionConfig.Get(), solutionConfig.m_SolutionPlatform.Get(),
                       solutionConfig.m_Config.Get(), solutionConfig.m_Platform.Get() );
            }

        }
    }

    Write( "\tEndGlobalSection\r\n" );
    Write( "\tGlobalSection(SolutionProperties) = preSolution\r\n" );
    Write( "\t\tHideSolutionNode = FALSE\r\n" );
    Write( "\tEndGlobalSection\r\n" );
}

// WriteNestedProjects
//------------------------------------------------------------------------------
void SLNGenerator::WriteNestedProjects( const Array< AString > & solutionProjectsToFolder,
                                        const Array< AString > & solutionFolderPaths )
{
    if ( solutionProjectsToFolder.GetSize() == 0 &&
         solutionFolderPaths.GetSize() == 0 )
    {
        return; // skip global section
    }

    Write( "\tGlobalSection(NestedProjects) = preSolution\r\n" );

    // Write every project to solution folder relationships
    const AString * const solutionProjectsToFolderEnd = solutionProjectsToFolder.End();
    for( const AString * it = solutionProjectsToFolder.Begin() ; it != solutionProjectsToFolderEnd ; ++it )
    {
        Write( "%s", it->Get() );
    }

    // Write every intermediate path
    const AString * const solutionFolderPathsEnd = solutionFolderPaths.End();
    for( const AString * it = solutionFolderPaths.Begin() ; it != solutionFolderPathsEnd ; ++it )
    {
        // parse solution folder parent path
        AStackString<> solutionFolderParentGuid;
        const char * lastSlash = it->FindLast( NATIVE_SLASH );
        if ( lastSlash )
        {
            AStackString<> solutionFolderParentPath( it->Get(), lastSlash );
            VSProjectGenerator::FormatDeterministicProjectGUID( solutionFolderParentGuid, solutionFolderParentPath );
        }

        if ( solutionFolderParentGuid.GetLength() > 0 )
        {
            // generate a guid for the solution folder
            AStackString<> solutionFolderGuid;
            VSProjectGenerator::FormatDeterministicProjectGUID( solutionFolderGuid, *it );

            solutionFolderGuid.ToUpper();
            solutionFolderParentGuid.ToUpper();

            // write parent solution folder relationship
            Write( "\t\t%s = %s\r\n", solutionFolderGuid.Get(), solutionFolderParentGuid.Get() );
        }
    }

    Write( "\tEndGlobalSection\r\n" );
}

// WriteFooter
//------------------------------------------------------------------------------
void SLNGenerator::WriteFooter()
{
    // footer
    Write( "EndGlobal\r\n" );
}

// Write
//------------------------------------------------------------------------------
void SLNGenerator::Write( MSVC_SAL_PRINTF const char * fmtString, ... )
{
    AStackString< 1024 > tmp;

    va_list args;
    va_start(args, fmtString);
    tmp.VFormat( fmtString, args );
    va_end( args );

    // resize output buffer in large chunks to prevent re-sizing
    if ( m_Output.GetLength() + tmp.GetLength() > m_Output.GetReserved() )
    {
        m_Output.SetReserved( m_Output.GetReserved() + MEGABYTE );
    }

    m_Output += tmp;
}

//------------------------------------------------------------------------------
