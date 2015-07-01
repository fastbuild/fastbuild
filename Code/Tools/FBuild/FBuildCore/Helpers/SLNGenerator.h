// SLNGenerator - Generate a Visual Studio solution
//------------------------------------------------------------------------------
#pragma once
#ifndef FBUILD_HELPERS_SLNGENERATOR_H
#define FBUILD_HELPERS_SLNGENERATOR_H

// Includes
//------------------------------------------------------------------------------
#include "Core/Containers/Array.h"
#include "Core/Strings/AString.h"

// Forward Declarations
//------------------------------------------------------------------------------
class IOStream;
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
                                 const AString & solutionVisualStudioVersion,
                                 const AString & solutionMinimumVisualStudioVersion,
                                 const Array< VSProjectConfig > & configs,
                                 const Array< VCXProjectNode * > & projects );

private:
    void WriteHeader( const AString & solutionVisualStudioVersion,
                      const AString & solutionMinimumVisualStudioVersion );
    void WriteProjectListings(  const AString& solutionBasePath,
                                const Array< VCXProjectNode * > & projects,
                                Array< AString > & projectGuids );
    void WriteSolutionConfigs(  const Array< VSProjectConfig > & configs );
    void WriteSolutionMappings( const Array< VSProjectConfig > & configs,
                                const Array< AString > & projectGuids );
    void WriteFooter();

    // Helper to format some text
    void Write( const char * fmtString, ... );

    // working buffer
    AString m_Tmp;

    // final output
    AString m_OutputSLN;
};

//------------------------------------------------------------------------------
#endif // FBUILD_HELPERS_SLNGENERATOR_H
