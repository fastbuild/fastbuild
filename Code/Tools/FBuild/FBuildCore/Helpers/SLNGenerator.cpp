// SLNGenerator
//------------------------------------------------------------------------------

// Includes
//------------------------------------------------------------------------------
#include "Tools/FBuild/FBuildCore/PrecompiledHeader.h"

#include "SLNGenerator.h"

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
SLNGenerator::SLNGenerator()
{
}

// DESTRUCTOR
//------------------------------------------------------------------------------
SLNGenerator::~SLNGenerator()
{
}

// GenerateVCXProj
//------------------------------------------------------------------------------
const AString & SLNGenerator::GenerateSLN(  const AString & solutionFile,
                                            const AString & solutionBuildProject,
                                            const AString & solutionVisualStudioVersion,
                                            const AString & solutionMinimumVisualStudioVersion,
                                            const Array< VSProjectConfig > & configs,
                                            const Array< VCXProjectNode * > & projects,
                                            const Array< SLNSolutionFolder > & folders )
{
    // preallocate to avoid re-allocations
    m_Output.SetReserved( MEGABYTE );
    m_Output.SetLength( 0 );

    // determine folder for project
    const char * lastSlash = solutionFile.FindLast( NATIVE_SLASH );
    AStackString<> solutionBasePath( solutionFile.Get(), lastSlash ? lastSlash + 1 : solutionFile.Get() );

    AStackString<> solutionBuildProjectGuid;
    Array< AString > projectGuids( projects.GetSize(), false );
    Array< AString > solutionProjectsToFolder( projects.GetSize(), true );
    Array< AString > solutionFolderPaths( folders.GetSize(), true );

    // Create solution configs (solves Visual Studio weirdness)
    const size_t configCount = configs.GetSize();
    Array< SolutionConfig > solutionConfigs( configCount, false );
    solutionConfigs.SetSize( configCount );
    for ( size_t i = 0 ; i < configCount ;  ++i )
    {
        const VSProjectConfig & projectConfig = configs[ i ];
        SolutionConfig & solutionConfig = solutionConfigs[ i ];

        solutionConfig.m_Config = projectConfig.m_Config;
        solutionConfig.m_Platform = projectConfig.m_Platform;

        if ( projectConfig.m_Platform.MatchesI( "Win32" ) )
        {
            solutionConfig.m_SolutionPlatform = "x86";
        }
        else
        {
            solutionConfig.m_SolutionPlatform = projectConfig.m_Platform;
        }
    }

    // Sort again with substituted solution platforms
    solutionConfigs.Sort();

    // construct sln file
    WriteHeader( solutionVisualStudioVersion, solutionMinimumVisualStudioVersion );
    WriteProjectListings( solutionBasePath, solutionBuildProject, projects, folders, solutionBuildProjectGuid, projectGuids, solutionProjectsToFolder );
    WriteSolutionFolderListings( folders, solutionFolderPaths );
    Write( "Global\r\n" );
    WriteSolutionConfigurationPlatforms( solutionConfigs );
    WriteProjectConfigurationPlatforms( solutionBuildProjectGuid, solutionConfigs, projectGuids );
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
	Write( "\r\n" ); // Deliberate blank line
    Write( "Microsoft Visual Studio Solution File, Format Version 12.00\r\n" );
    Write( "# Visual Studio %s\r\n", shortVersion.Get() );
    Write( "VisualStudioVersion = %s\r\n", version );
    Write( "MinimumVisualStudioVersion = %s\r\n", minimumVersion );
}

// WriteProjectListings
//------------------------------------------------------------------------------
void SLNGenerator::WriteProjectListings(    const AString& solutionBasePath,
                                            const AString& solutionBuildProject,
                                            const Array< VCXProjectNode * > & projects,
                                            const Array< SLNSolutionFolder > & folders,
                                            AString & solutionBuildProjectGuid,
                                            Array< AString > & projectGuids,
                                            Array< AString > & solutionProjectsToFolder )
{
    // Project Listings

    VCXProjectNode ** const projectsEnd = projects.End();
    for( VCXProjectNode ** it = projects.Begin() ; it != projectsEnd ; ++it )
    {
        // check if this project is the master project
        const bool projectIsActive = ( solutionBuildProject.CompareI( (*it)->GetName() ) == 0 );

        // project relative path
        AStackString<> projectPath( (*it)->GetName() );
        projectPath.Replace( solutionBasePath.Get(), "" );

        // get project base name only
        const char * p1 = projectPath.FindLast( NATIVE_SLASH );
        const char * p2 = projectPath.FindLast( '.' );
        AStackString<> projectName( p1 ? p1 + 1 : projectPath.Get(),
                                    p2 ? p2 : projectPath.GetEnd() );

        // retrieve projectGuid
        AStackString<> projectGuid;
        if ( (*it)->GetProjectGuid().GetLength() == 0 )
        {
            AStackString<> projectNameForGuid( p1 ? p1 : projectName.Get() );
            VSProjectGenerator::FormatDeterministicProjectGUID( projectGuid, projectNameForGuid );
        }
        else
        {
            projectGuid = (*it)->GetProjectGuid();
        }

        // projectGuid must be uppercase (visual does that, it changes the .sln otherwise)
        projectGuid.ToUpper();

        if ( projectIsActive )
        {
            ASSERT( solutionBuildProjectGuid.GetLength() == 0 );
            solutionBuildProjectGuid = projectGuid;
        }

        Write( "Project(\"{8BC9CEB8-8B4A-11D0-8D11-00A0C91BC942}\") = \"%s\", \"%s\", \"%s\"\r\n",
               projectName.Get(), projectPath.Get(), projectGuid.Get() );

        Write( "EndProject\r\n" );

        projectGuids.Append( projectGuid );

        // check if this project is in a solution folder
        const SLNSolutionFolder * const foldersEnd = folders.End();
        for ( const SLNSolutionFolder * it2 = folders.Begin() ; it2 != foldersEnd ; ++it2 )
        {
            // this has to be done here to have the same order of declaration (like visual)
            if ( it2->m_ProjectNames.Find( (*it)->GetName() ) )
            {
                // generate a guid for the solution folder
                AStackString<> solutionFolderGuid;
                VSProjectGenerator::FormatDeterministicProjectGUID( solutionFolderGuid, it2->m_Path );

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
void SLNGenerator::WriteSolutionFolderListings( const Array< SLNSolutionFolder > & folders,
                                                Array< AString > & solutionFolderPaths )
{
    // Create every intermediate path
    const SLNSolutionFolder * const foldersEnd = folders.End();
    for( const SLNSolutionFolder * it = folders.Begin() ; it != foldersEnd ; ++it )
    {
        if ( solutionFolderPaths.Find( it->m_Path ) == nullptr )
        {
            solutionFolderPaths.Append( it->m_Path );
        }

        const char * pathEnd = it->m_Path.Find( NATIVE_SLASH );
        while ( pathEnd )
        {
            AStackString<> solutionFolderPath( it->m_Path.Get(), pathEnd );
            if ( solutionFolderPaths.Find( solutionFolderPath ) == nullptr )
            {
                solutionFolderPaths.Append( solutionFolderPath );
            }

            pathEnd = it->m_Path.Find( NATIVE_SLASH, pathEnd + 1 );
        }
    }

    solutionFolderPaths.Sort();

    // Solution Folders Listings

    const AString * const solutionFolderPathsEnd = solutionFolderPaths.End();
    for( const AString * it = solutionFolderPaths.Begin() ; it != solutionFolderPathsEnd ; ++it )
    {
        // parse solution folder name
        const char * solutionFolderName = it->FindLast( NATIVE_SLASH );
        solutionFolderName = solutionFolderName ? solutionFolderName + 1 : it->Get();

        // generate a guid for the solution folder
        AStackString<> solutionFolderGuid;
        VSProjectGenerator::FormatDeterministicProjectGUID( solutionFolderGuid, *it );

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
        Write(  "\t\t%s|%s = %s|%s\r\n",
                it->m_Config.Get(), it->m_SolutionPlatform.Get(),
                it->m_Config.Get(), it->m_SolutionPlatform.Get() );
    }

    Write( "\tEndGlobalSection\r\n" );
}

// WriteProjectConfigurationPlatforms
//------------------------------------------------------------------------------
void SLNGenerator::WriteProjectConfigurationPlatforms(  const AString & solutionBuildProjectGuid,
                                                        const Array< SolutionConfig > & solutionConfigs,
                                                        const Array< AString > & projectGuids )
{
    Write( "\tGlobalSection(ProjectConfigurationPlatforms) = postSolution\r\n" );

    // Solution Configuration Mappings to Projects
    const AString * const projectGuidsEnd = projectGuids.End();
    for( const AString * it = projectGuids.Begin() ; it != projectGuidsEnd ; ++it )
    {
        // only one project active in the solution build
        const bool projectIsActive = ( solutionBuildProjectGuid == *it );

        const SolutionConfig * const solutionConfigsEnd = solutionConfigs.End();
        for( const SolutionConfig * it2 = solutionConfigs.Begin() ; it2 != solutionConfigsEnd ; ++it2 )
        {
            Write(  "\t\t%s.%s|%s.ActiveCfg = %s|%s\r\n",
                    it->Get(),
                    it2->m_Config.Get(), it2->m_SolutionPlatform.Get(),
                    it2->m_Config.Get(), it2->m_Platform.Get() );

            if ( projectIsActive )
            {
                Write(  "\t\t%s.%s|%s.Build.0 = %s|%s\r\n",
                        it->Get(),
                        it2->m_Config.Get(), it2->m_SolutionPlatform.Get(),
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
        Write( it->Get() );
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


// SLNSolutionFolder::Save
//------------------------------------------------------------------------------
/*static*/ void SLNSolutionFolder::Save( IOStream & stream, const Array< SLNSolutionFolder > & solutionFolders )
{
    uint32_t numSolutionFolders = (uint32_t)solutionFolders.GetSize();
    stream.Write( numSolutionFolders );
    for ( uint32_t i=0; i<numSolutionFolders; ++i )
    {
        const SLNSolutionFolder & sln = solutionFolders[ i ];

        stream.Write( sln.m_Path );
        stream.Write( sln.m_ProjectNames );
    }
}

// SLNSolutionFolder::Load
//------------------------------------------------------------------------------
/*static*/ bool SLNSolutionFolder::Load( IOStream & stream, Array< SLNSolutionFolder > & solutionFolders )
{
    ASSERT( solutionFolders.IsEmpty() );

    uint32_t numSolutionFolders( 0 );
    if ( !stream.Read( numSolutionFolders ) )
    {
        return false;
    }
    solutionFolders.SetSize( numSolutionFolders );
    for ( uint32_t i=0; i<numSolutionFolders; ++i )
    {
        SLNSolutionFolder & sln = solutionFolders[ i ];

        if ( stream.Read( sln.m_Path ) == false ) { return false; }
        if ( stream.Read( sln.m_ProjectNames ) == false ) { return false; }
    }
    return true;
}

//------------------------------------------------------------------------------
