// XCodeProjectGenerator
//------------------------------------------------------------------------------
#pragma once

// Includes
//------------------------------------------------------------------------------
#include "ProjectGeneratorBase.h"

// Core
//------------------------------------------------------------------------------
#include <Core/Containers/Array.h>

// Forward Declarations
//------------------------------------------------------------------------------

// XCodeProjectGenerator
//------------------------------------------------------------------------------
class XCodeProjectGenerator : public ProjectGeneratorBase
{
public:
    XCodeProjectGenerator();
    ~XCodeProjectGenerator();

    void SetProjectName( const AString & projectName ) { m_ProjectName = projectName; }
    void SetXCodeOrganizationName( const AString & organizationName ) { m_XCodeOrganizationName = organizationName; }
    void SetXCodeBuildToolPath( const AString & buildToolPath ) { m_XCodeBuildToolPath = buildToolPath; }
    void SetXCodeBuildToolArgs( const AString & buildToolArgs ) { m_XCodeBuildToolArgs = buildToolArgs; }
    void SetXCodeBuildWorkingDir( const AString & buildWorkingDir ) { m_XCodeBuildWorkingDir = buildWorkingDir; }
    void SetXCodeDocumentVersioning( bool documentVersioning ) { m_XCodeDocumentVersioning = documentVersioning; }
    void SetXCodeCommandLineArguments( const Array<AString> & commandLineArgs ) { m_XCodeCommandLineArguments = commandLineArgs; }
    void SetXCodeCommandLineArgumentsDisabled( const Array<AString> & commandLineArgs ) { m_XCodeCommandLineArgumentsDisabled = commandLineArgs; }

    const AString & GetProjectName() const { return m_ProjectName; }

    // NOTE: These return pointers to shared storage. User must not hold
    // references simultaneously.
    const AString & GeneratePBXProj();
    const AString & GenerateUserSchemeManagementPList();
    const AString & GenerateXCScheme();

private:
    void GetGUID_PBXGroup( const uint32_t index, AString & outGUID );
    void GetGUID_PBXLegacyTarget( const uint32_t index, AString & outGUID );
    void GetGUID_PBXNativeTarget( const uint32_t index, AString & outGUID );
    void GetGUID_PBXProject( const uint32_t index, AString & outGUID );
    void GetGUID_XConfigurationList( const uint32_t index, AString & outGUID );
    void GetGUID_XCBuildConfiguration( const uint32_t index, AString & outGUID );
    void GetGUID_PBXSourcesBuildPhase( const uint32_t index, AString & outGUID );

    void WriteHeader();
    void WriteFiles();
    void WriteFolders();
    void WriteBuildCommand();
    void WriteGeneralSettings();
    void WritePBXSourcesBuildPhase();
    void WriteBuildConfiguration();
    void WriteConfigurationList();
    void WriteFooter();

    void WriteString( uint32_t indentDepth,
                      const char * propertyName,
                      const AString & value );
    void WriteArray( uint32_t indentDepth,
                     const char * propertyName,
                     const Array<AString> & values );
    void EscapeArgument( const AString & arg,
                         AString & outEscapedArgument ) const;
    static void ProcessString( const AString & string, AString & outString );

    // Additional Input Data
    AString m_ProjectName;
    AString m_XCodeOrganizationName;
    AString m_XCodeBuildToolPath;
    AString m_XCodeBuildToolArgs;
    AString m_XCodeBuildWorkingDir;
    bool m_XCodeDocumentVersioning = false;
    Array<AString> m_XCodeCommandLineArguments;
    Array<AString> m_XCodeCommandLineArgumentsDisabled;
};

//------------------------------------------------------------------------------
