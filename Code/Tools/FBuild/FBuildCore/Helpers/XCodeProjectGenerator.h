// XCodeProjectGenerator
//------------------------------------------------------------------------------
#pragma once

// Includes
//------------------------------------------------------------------------------
#include "ProjectGeneratorBase.h"

// Forward Declarations
//------------------------------------------------------------------------------

// XCodeProjectGenerator
//------------------------------------------------------------------------------
class XCodeProjectGenerator : public ProjectGeneratorBase
{
public:
    XCodeProjectGenerator();
    ~XCodeProjectGenerator();

    inline void SetProjectName( const AString & projectName ) { m_ProjectName = projectName; }
    inline void SetXCodeOrganizationName( const AString& organizationName ) { m_XCodeOrganizationName = organizationName; }
    inline void SetXCodeBuildToolPath( const AString& buildToolPath ) { m_XCodeBuildToolPath = buildToolPath; }
    inline void SetXCodeBuildToolArgs( const AString& buildToolArgs ) { m_XCodeBuildToolArgs = buildToolArgs; }
    inline void SetXCodeBuildWorkingDir( const AString& buildWorkingDir ){ m_XCodeBuildWorkingDir = buildWorkingDir; }

    // NOTE: These return pointers to shared storage. User must not hold
    // refrences simultaneously.
    const AString & GeneratePBXProj();
    const AString & GenerateUserSchemeMangementPList();

private:
    void WriteHeader();
    void WriteFiles();
    void WriteFolders();
    void WriteBuildCommand();
    void WriteGeneralSettings();
    void WritePBXSourcesBuildPhase();
    void WriteBuildConfiguration();
    void WriteConfigurationList();
    void WriteFooter();

    // Additional Input Data
    AString             m_ProjectName;
    AString             m_XCodeOrganizationName;
    AString             m_XCodeBuildToolPath;
    AString             m_XCodeBuildToolArgs;
    AString             m_XCodeBuildWorkingDir;
};

//------------------------------------------------------------------------------
