// SLNGenerator - Generate a Visual Studio solution
//------------------------------------------------------------------------------
#pragma once

// Includes
//------------------------------------------------------------------------------
#include "Core/Containers/Array.h"
#include "Core/Env/MSVCStaticAnalysis.h"
#include "Core/Strings/AString.h"

// Forward Declarations
//------------------------------------------------------------------------------
class IOStream;
class SolutionConfig;
class SolutionDependency;
class SolutionFolder;
class VSProjectConfig;
class VSProjectBaseNode;

// SLNGenerator
//------------------------------------------------------------------------------
class SLNGenerator
{
public:
    SLNGenerator();
    ~SLNGenerator();

    const AString & GenerateSLN( const AString & solutionFile,
                                 const AString & solutionVisualStudioVersion,
                                 const AString & solutionMinimumVisualStudioVersion,
                                 const Array< SolutionConfig > & solutionConfigs,
                                 const Array< VSProjectBaseNode * > & projects,
                                 const Array< SolutionDependency > & solutionDependencies,
                                 const Array< SolutionFolder > & solutionFolders );

private:
    void WriteHeader( const AString & solutionVisualStudioVersion,
                      const AString & solutionMinimumVisualStudioVersion );
    void WriteProjectListings( const AString& solutionBasePath,
                               const Array< VSProjectBaseNode * > & projects,
                               const Array< SolutionFolder > & solutionFolders,
                               const Array< SolutionDependency > & solutionDependencies,
                               Array< AString > & solutionProjectsToFolder );
    void WriteSolutionFolderListings( const AString & solutionBasePath,
                                      const Array< SolutionFolder > & solutionFolders,
                                      Array< AString > & solutionFolderPaths );
    void WriteSolutionConfigurationPlatforms( const Array< SolutionConfig > & solutionConfigs );
    void WriteProjectConfigurationPlatforms( const Array< SolutionConfig > & solutionConfigs,
                                             const Array< VSProjectBaseNode * > & projects );
    void WriteNestedProjects( const Array< AString > & solutionProjectsToFolder,
                              const Array< AString > & solutionFolderPaths );
    void WriteFooter();

    // Helper to format some text
    void Write( MSVC_SAL_PRINTF const char * fmtString, ... ) FORMAT_STRING( 2, 3 );

    // working buffer
    AString m_Output;
};

//------------------------------------------------------------------------------
