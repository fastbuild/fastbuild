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

    inline void SetProjectName( const AString & projectName ) { m_ProjectName = projectName; }
    inline void SetXCodeOrganizationName( const AString& organizationName ) { m_XCodeOrganizationName = organizationName; }
    inline void SetXCodeBuildToolPath( const AString& buildToolPath ) { m_XCodeBuildToolPath = buildToolPath; }
    inline void SetXCodeBuildToolArgs( const AString& buildToolArgs ) { m_XCodeBuildToolArgs = buildToolArgs; }
    inline void SetXCodeBuildWorkingDir( const AString& buildWorkingDir ){ m_XCodeBuildWorkingDir = buildWorkingDir; }
    inline void SetXCodeDocumentVersioning( bool documentVersioning ) { m_XCodeDocumentVersioning = documentVersioning; }
    inline void SetXCodeCommandLineArguments( const Array<AString> & commandLineArgs ) { m_XCodeCommandLineArguments = commandLineArgs; }
    inline void SetXCodeCommandLineArgumentsDisabled( const Array<AString> & commandLineArgs ) { m_XCodeCommandLineArgumentsDisabled = commandLineArgs; }

    const AString & GetProjectName() const { return m_ProjectName; }

    // NOTE: These return pointers to shared storage. User must not hold
    // refrences simultaneously.
    const AString & GeneratePBXProj();
    const AString & GenerateUserSchemeMangementPList();
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

    bool ShouldQuoteString( const AString & value ) const;
    void WriteString( uint32_t indentDepth,
                      const char * propertyName,
                      const AString & value );
    void WriteArray( uint32_t indentDepth,
                     const char * propertyName,
                     const Array<AString> & values );
    void EscapeArgument( const AString & arg,
                         AString & outEscapedArgument ) const;

    // Additional Input Data
    AString             m_ProjectName;
    AString             m_XCodeOrganizationName;
    AString             m_XCodeBuildToolPath;
    AString             m_XCodeBuildToolArgs;
    AString             m_XCodeBuildWorkingDir;
    bool                m_XCodeDocumentVersioning = false;
    Array<AString>      m_XCodeCommandLineArguments;
    Array<AString>      m_XCodeCommandLineArgumentsDisabled;
};

//------------------------------------------------------------------------------
