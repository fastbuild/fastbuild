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
                                            const AString & solutionVisualStudioVersion,
                                            const AString & solutionMinimumVisualStudioVersion,
                                            const Array< VSProjectConfig > & configs,
                                            const Array< VCXProjectNode * > & projects )
{
    // preallocate to avoid re-allocations
    m_Tmp.SetReserved( MEGABYTE );
    m_Tmp.SetLength( 0 );

    // determine folder for project
    const char * lastSlash = solutionFile.FindLast( NATIVE_SLASH );
    AStackString<> solutionBasePath( solutionFile.Get(), lastSlash ? lastSlash + 1 : solutionFile.Get() );

    Array< AString > projectGuids( projects.GetSize(), false );

    // construct sln file
    WriteHeader( solutionVisualStudioVersion, solutionMinimumVisualStudioVersion );
    WriteProjectListings( solutionBasePath, projects, projectGuids );
    Write( "Global\n" );
    WriteSolutionConfigs( configs );
    WriteSolutionMappings( configs, projectGuids );
    WriteFooter();

    m_OutputSLN = m_Tmp;
    return m_OutputSLN;
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
    Write( "Microsoft Visual Studio Solution File, Format Version 12.00\n" );
    Write( "# Visual Studio %s\n", shortVersion.Get() );
    Write( "VisualStudioVersion = %s\n", version );
    Write( "MinimumVisualStudioVersion = %s\n", minimumVersion );
}

// WriteProjectListings
//------------------------------------------------------------------------------
void SLNGenerator::WriteProjectListings(    const AString& solutionBasePath,
                                            const Array< VCXProjectNode * > & projects,
                                            Array< AString > & projectGuids )
{
    // Project Listings

    VCXProjectNode ** const projectsEnd = projects.End();
    for( VCXProjectNode ** it = projects.Begin() ; it != projectsEnd ; ++it )
    {
        // project relative path
        AStackString<> projectPath( (*it)->GetName() );
        projectPath.Replace( solutionBasePath.Get(), "" );

        // get project base name only
        const char * p1 = projectPath.FindLast( NATIVE_SLASH );
        const char * p2 = projectPath.FindLast( '.' );
        AStackString<> projectName( p1 ? p1 + 1 : projectPath.Get(),
                                    p2 ? p2 : projectPath.GetEnd() );

        // retrieve projectGuid
        AString projectGuid;
        if ( (*it)->GetProjectGuid().GetLength() == 0 )
        {
            AStackString<> projectNameForGuid( p1 ? p1 : projectName.Get() );
            VSProjectGenerator::FormatDeterministicProjectGUID( &projectGuid, projectNameForGuid );
        }
        else
        {
            projectGuid = (*it)->GetProjectGuid();
        }

        // projectGuid must be uppercase (visual does that, it change the .sln otherwise)
        projectGuid.ToUpper();

        Write( "Project(\"{8BC9CEB8-8B4A-11D0-8D11-00A0C91BC942}\") = \"%s\", \"%s\", \"%s\"\n",
               projectName.Get(), projectPath.Get(), projectGuid.Get() );

        Write( "EndProject\n" );

        projectGuids.Append( projectGuid );
    }
}

// WriteSolutionConfigs
//------------------------------------------------------------------------------
void SLNGenerator::WriteSolutionConfigs( const Array< VSProjectConfig > & configs )
{
    Write( "    GlobalSection(SolutionConfigurationPlatforms) = preSolution\n" );

    // Solution Configurations
    VSProjectConfig * const end = configs.End();
    for( VSProjectConfig * it = configs.Begin() ; it != end ; ++it )
    {
        Write(  "        %s|%s = %s|%s\n",
                it->m_Config.Get(), it->m_Platform.Get(),
                it->m_Config.Get(), it->m_Platform.Get() );
    }

    Write( "    EndGlobalSection\n" );
}

// WriteSolutionMappings
//------------------------------------------------------------------------------
void SLNGenerator::WriteSolutionMappings(   const Array< VSProjectConfig > & configs,
                                            const Array< AString > & projectGuids )
{
    Write( "    GlobalSection(ProjectConfigurationPlatforms) = postSolution\n" );

    // Solution Configuration Mappings to Projects
    const AString * const projectGuidsEnd = projectGuids.End();
    for( const AString * it = projectGuids.Begin() ; it != projectGuidsEnd ; ++it )
    {
        const VSProjectConfig * const configsEnd = configs.End();
        for( const VSProjectConfig * it2 = configs.Begin() ; it2 != configsEnd ; ++it2 )
        {
            Write(  "        %s.%s|%s.ActiveCfg = %s|%s\n",
                    it->Get(),
                    it2->m_Config.Get(), it2->m_Platform.Get(),
                    it2->m_Config.Get(), it2->m_Platform.Get() );

            Write(  "        %s.%s|%s.Build.0 = %s|%s\n",
                    it->Get(),
                    it2->m_Config.Get(), it2->m_Platform.Get(),
                    it2->m_Config.Get(), it2->m_Platform.Get() );
        }
    }

    Write( "    EndGlobalSection\n" );
}

// WriteFooter
//------------------------------------------------------------------------------
void SLNGenerator::WriteFooter()
{
    // footer
    Write( "    GlobalSection(SolutionProperties) = preSolution\n" );
    Write( "        HideSolutionNode = FALSE\n" );
    Write( "    EndGlobalSection\n" );
    Write( "EndGlobal\n" );
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
    if ( m_Tmp.GetLength() + tmp.GetLength() > m_Tmp.GetReserved() )
    {
        m_Tmp.SetReserved( m_Tmp.GetReserved() + MEGABYTE );
    }

    m_Tmp += tmp;
}

//------------------------------------------------------------------------------
