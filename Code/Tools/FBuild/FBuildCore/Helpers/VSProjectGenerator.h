// VSProjectGenerator - Generate a Visual Studio project
//------------------------------------------------------------------------------
#pragma once

// Includes
//------------------------------------------------------------------------------
#include "Core/Containers/Array.h"
#include "Core/Strings/AString.h"

// Forward Declarations
//------------------------------------------------------------------------------
class VSProjectConfig;
class VSProjectFileType;
class VSProjectImport;

// VSProjectFilePair
//------------------------------------------------------------------------------
class VSProjectFilePair
{
public:
    AString m_ProjectRelativePath;  // Paths to files are project-relatice
    AString m_AbsolutePath;         // Folder structure is relative to BasePaths which can be outside of the project folder
};

// VSProjectGenerator
//------------------------------------------------------------------------------
class VSProjectGenerator
{
public:
    VSProjectGenerator();
    ~VSProjectGenerator();

    void SetBasePaths( const Array< AString > & paths );

    void AddFile( const AString & file );
    void AddFiles( const Array< AString > & files );

    void SetRootNamespace( const AString & s )          { m_RootNamespace = s; }
    void SetProjectGuid( const AString & s )            { m_ProjectGuid = s; }
    void SetDefaultLanguage( const AString & s )        { m_DefaultLanguage = s; }
    void SetApplicationEnvironment( const AString & s ) { m_ApplicationEnvironment = s; }
    void SetProjectSccEntrySAK( const bool b )          { m_ProjectSccEntrySAK = b; }
    void SetReferences( const Array< AString > & a )    { m_References = a; }
    void SetProjectReferences( const Array< AString > & a ) { m_ProjectReferences = a; }

    const AString & GenerateVCXProj( const AString & projectFile,
                                     const Array< VSProjectConfig > & configs,
                                     const Array< VSProjectFileType > & fileTypes,
                                     const Array< VSProjectImport > & projectImports );

    const AString & GenerateVCXProjFilters( const AString & projectFile );

    static void FormatDeterministicProjectGUID( AString & guid, const AString & projectName );

private:
    // Helper to format some text
    void Write( const char * string );
    void WriteF( const char * fmtString, ... ) FORMAT_STRING( 2, 3 );

    // Helpers to format some xml
    void WritePGItem( const char * xmlTag, const AString & value );

    void GetFolderPath( const AString & fileName, AString & folder ) const;
    void CanonicalizeFilePaths( const AString & projectBasePath );

    // project details
    Array< AString > m_BasePaths;

    // Globals
    AString m_RootNamespace;
    AString m_ProjectGuid;
    AString m_DefaultLanguage;
    AString m_ApplicationEnvironment;
    bool m_ProjectSccEntrySAK;
    Array< AString > m_References;
    Array< AString > m_ProjectReferences;

    // intermediate data
    bool m_FilePathsCanonicalized;
    Array< VSProjectFilePair > m_Files;

    // working buffer
    AString m_Tmp;

    // final output
    AString m_OutputVCXProj;
    AString m_OutputVCXProjFilters;
};

//------------------------------------------------------------------------------
