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
class SolutionConfig;
class SolutionDependency;
class SolutionFolder;
class VSProjectConfig;
class VCXProjectNode;

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
                                 const Array< SolutionConfig > & solutionConfigs,
                                 const Array< VCXProjectNode * > & projects,
                                 const Array< SolutionDependency > & solutionDependencies,
                                 const Array< SolutionFolder > & solutionFolders );

private:
    void WriteHeader( const AString & solutionVisualStudioVersion,
                      const AString & solutionMinimumVisualStudioVersion );
    void WriteProjectListings( const AString& solutionBasePath,
                               const AString & solutionBuildProject,
                               const Array< VCXProjectNode * > & projects,
                               const Array< SolutionFolder > & solutionFolders,
                               const Array< SolutionDependency > & solutionDependencies,
                               AString & solutionBuildProjectGuid,
                               Array< AString > & projectGuids,
                               Array< AString > & solutionProjectsToFolder );
    void WriteSolutionFolderListings( const Array< SolutionFolder > & solutionFolders,
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
