// SLNGenerator
//------------------------------------------------------------------------------

// Includes
//------------------------------------------------------------------------------
#include "Tools/FBuild/FBuildCore/PrecompiledHeader.h"

#include "SLNGenerator.h"

#include "Tools/FBuild/FBuildCore/Graph/SLNNode.h"
#include "Tools/FBuild/FBuildCore/Graph/VCXProjectNode.h"
#include "Tools/FBuild/FBuildCore/Helpers/VSProjectGenerator.h"

// Core
#include "Core/FileIO/IOStream.h"
#include "Core/FileIO/PathUtils.h"
#include "Core/Math/CRC32.h"
#include "Core/Strings/AStackString.h"

// system
#include <stdarg.h> // for va_args

// CONSTRUCTOR
//------------------------------------------------------------------------------
SLNGenerator::SLNGenerator() = default;

// DESTRUCTOR
//------------------------------------------------------------------------------
SLNGenerator::~SLNGenerator() = default;

// GenerateVCXProj
//------------------------------------------------------------------------------
const AString & SLNGenerator::GenerateSLN( const AString & solutionFile,
                                           const Array< AString > & solutionBuildProjects,
                                           const AString & solutionVisualStudioVersion,
                                           const AString & solutionMinimumVisualStudioVersion,
                                           const Array< SolutionConfig > & solutionConfigs,
                                           const Array< VCXProjectNode * > & projects,
                                           const Array< SolutionDependency > & solutionDependencies,
                                           const Array< SolutionFolder > & solutionFolders )
{
    // preallocate to avoid re-allocations
    m_Output.SetReserved( MEGABYTE );
    m_Output.SetLength( 0 );

    // determine folder for project
    const char * lastSlash = solutionFile.FindLast( NATIVE_SLASH );
    AStackString<> solutionBasePath( solutionFile.Get(), lastSlash ? lastSlash + 1 : solutionFile.Get() );

    Array< AString > solutionBuildProjectGuids;
    Array< AString > projectGuids( projects.GetSize(), false );
    Array< AString > solutionProjectsToFolder( projects.GetSize(), true );
    Array< AString > solutionFolderPaths( solutionFolders.GetSize(), true );

    // construct sln file
    WriteHeader( solutionVisualStudioVersion, solutionMinimumVisualStudioVersion );
    WriteProjectListings( solutionBasePath, solutionBuildProjects, projects, solutionFolders, solutionDependencies, solutionBuildProjectGuids, projectGuids, solutionProjectsToFolder );
    WriteSolutionFolderListings( solutionFolders, solutionFolderPaths );
    Write( "Global\r\n" );
    WriteSolutionConfigurationPlatforms( solutionConfigs );
    WriteProjectConfigurationPlatforms( solutionBuildProjectGuids, solutionConfigs, projectGuids );
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

    // header
    Write( "Microsoft Visual Studio Solution File, Format Version 12.00\r\n" );
    Write( "# Visual Studio %s\r\n", shortVersion.Get() );
    Write( "VisualStudioVersion = %s\r\n", version );
    Write( "MinimumVisualStudioVersion = %s\r\n", minimumVersion );
}

// WriteProjectListings
//------------------------------------------------------------------------------
void SLNGenerator::WriteProjectListings( const AString& solutionBasePath,
                                         const Array< AString > & solutionBuildProjects,
                                         const Array< VCXProjectNode * > & projects,
                                         const Array< SolutionFolder > & solutionFolders,
                                         const Array< SolutionDependency > & solutionDependencies,
                                         Array< AString > & solutionBuildProjectGuids,
                                         Array< AString > & projectGuids,
                                         Array< AString > & solutionProjectsToFolder )
{
    // Project Listings

    VCXProjectNode ** const projectsEnd = projects.End();
    for( VCXProjectNode ** it = projects.Begin() ; it != projectsEnd ; ++it )
    {
        // Is project active in solution build?
        bool projectIsActive = false;
        for ( const AString & solutionBuildProject : solutionBuildProjects )
        {
            if ( solutionBuildProject.EqualsI( (*it)->GetName() ) )
            {
                projectIsActive = true;
                break;
            }
        }

        AStackString<> projectPath( (*it)->GetName() );

        // get project base name only
        const char * lastSlash  = projectPath.FindLast( NATIVE_SLASH );
        const char * lastPeriod = projectPath.FindLast( '.' );
        AStackString<> projectName( lastSlash  ? lastSlash + 1  : projectPath.Get(),
                                    lastPeriod ? lastPeriod     : projectPath.GetEnd() );

        // make project path relative
        projectPath.Replace( solutionBasePath.Get(), "" );

        // retrieve projectGuid
        AStackString<> projectGuid( (*it)->GetProjectGuid() );

        // projectGuid must be uppercase (visual does that, it changes the .sln otherwise)
        projectGuid.ToUpper();

        if ( projectIsActive )
        {
            solutionBuildProjectGuids.Append( projectGuid );
        }

        Write( "Project(\"{8BC9CEB8-8B4A-11D0-8D11-00A0C91BC942}\") = \"%s\", \"%s\", \"%s\"\r\n",
               projectName.Get(), projectPath.Get(), projectGuid.Get() );

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
                for ( const VCXProjectNode* dependencyProject : projects )
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

        projectGuids.Append( projectGuid );

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

// WriteSolutionConfigs
//------------------------------------------------------------------------------
void SLNGenerator::WriteSolutionFolderListings( const Array< SolutionFolder > & solutionFolders,
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
void SLNGenerator::WriteProjectConfigurationPlatforms( const Array< AString > & solutionBuildProjectGuids,
                                                       const Array< SolutionConfig > & solutionConfigs,
                                                       const Array< AString > & projectGuids )
{
    Write( "\tGlobalSection(ProjectConfigurationPlatforms) = postSolution\r\n" );

    // Solution Configuration Mappings to Projects
    const AString * const projectGuidsEnd = projectGuids.End();
    for( const AString * it = projectGuids.Begin() ; it != projectGuidsEnd ; ++it )
    {
        // only one project active in the solution build
        const bool projectIsActive = solutionBuildProjectGuids.Find( *it );

        const SolutionConfig * const solutionConfigsEnd = solutionConfigs.End();
        for( const SolutionConfig * it2 = solutionConfigs.Begin() ; it2 != solutionConfigsEnd ; ++it2 )
        {
            Write( "\t\t%s.%s|%s.ActiveCfg = %s|%s\r\n",
                   it->Get(),
                   it2->m_SolutionConfig.Get(), it2->m_SolutionPlatform.Get(),
                   it2->m_Config.Get(), it2->m_Platform.Get() );

            if ( projectIsActive )
            {
                Write(  "\t\t%s.%s|%s.Build.0 = %s|%s\r\n",
                        it->Get(),
                        it2->m_SolutionConfig.Get(), it2->m_SolutionPlatform.Get(),
                        it2->m_Config.Get(), it2->m_Platform.Get() );
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
void SLNGenerator::Write( const char * fmtString, ... )
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
