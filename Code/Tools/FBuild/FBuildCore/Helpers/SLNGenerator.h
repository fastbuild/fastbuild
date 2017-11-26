// SLNGenerator - Generate a Visual Studio solution
//------------------------------------------------------------------------------
#pragma once

// Includes
//------------------------------------------------------------------------------
#include "Core/Containers/Array.h"
#include "Core/Strings/AString.h"

// Forward Declarations
//------------------------------------------------------------------------------
class IOStream;
class VSProjectConfig;
class VCXProjectNode;

// SLNSolutionFolder
//------------------------------------------------------------------------------
class SLNSolutionFolder
{
public:
    AString m_Path;
    Array< AString > m_ProjectNames;

    static bool Load( IOStream & stream, Array< SLNSolutionFolder > & solutionFolders );
    static void Save( IOStream & stream, const Array< SLNSolutionFolder > & solutionFolders );
};

// SolutionConfig
//------------------------------------------------------------------------------
struct SolutionConfig
{
    AString m_Config;
    AString m_Platform;
    AString m_SolutionPlatform;
    AString m_SolutionConfig;

    bool operator < ( const SolutionConfig& other ) const
    {
        int32_t cmpConfig = m_Config.CompareI( other.m_Config );
        return ( cmpConfig == 0 ) ? m_SolutionPlatform < other.m_SolutionPlatform
                                  : cmpConfig < 0 ;
    }
};

// SLNDependency
//------------------------------------------------------------------------------
class SLNDependency
{
public:
    Array< AString > m_Projects;
    Array< AString > m_Dependencies;

    static bool Load( IOStream & stream, Array< SLNDependency > & slnDeps );
    static void Save( IOStream & stream, const Array< SLNDependency > & slnDeps );
};

// SLNGenerator
//------------------------------------------------------------------------------
class SLNGenerator
{
public:
    SLNGenerator();
    ~SLNGenerator();

    const AString & GenerateSLN( const AString & solutionFile,
                                 const AString & solutionBuildProject,
                                 const AString & solutionVisualStudioVersion,
                                 const AString & solutionMinimumVisualStudioVersion,
                                 const Array< VSProjectConfig > & configs,
                                 const Array< VCXProjectNode * > & projects,
                                 const Array< SLNDependency > & slnDeps,
                                 const Array< SLNSolutionFolder > & folders );

private:
    void WriteHeader( const AString & solutionVisualStudioVersion,
                      const AString & solutionMinimumVisualStudioVersion );
    void WriteProjectListings( const AString& solutionBasePath,
                               const AString & solutionBuildProject,
                               const Array< VCXProjectNode * > & projects,
                               const Array< SLNSolutionFolder > & folders,
                               const Array< SLNDependency > & slnDeps,
                               AString & solutionBuildProjectGuid,
                               Array< AString > & projectGuids,
                               Array< AString > & solutionProjectsToFolder );
    void WriteSolutionFolderListings( const Array< SLNSolutionFolder > & folders,
                                      Array< AString > & solutionFolderPaths );
    void WriteSolutionConfigurationPlatforms( const Array< SolutionConfig > & solutionConfigs );
    void WriteProjectConfigurationPlatforms( const AString & solutionBuildProjectGuid,
                                             const Array< SolutionConfig > & solutionConfigs,
                                             const Array< AString > & projectGuids );
    void WriteNestedProjects( const Array< AString > & solutionProjectsToFolder,
                              const Array< AString > & solutionFolderPaths );
    void WriteFooter();

    // Helper to format some text
    void Write( const char * fmtString, ... ) FORMAT_STRING( 2, 3 );

    // working buffer
    AString m_Output;
};

//------------------------------------------------------------------------------
