// ProjectGeneratorBase
//------------------------------------------------------------------------------
#pragma once

// Includes
//------------------------------------------------------------------------------
#include "Core/Containers/Array.h"
#include "Core/Env/MSVCStaticAnalysis.h"
#include "Core/Strings/AString.h"

// Forward Declarations
//------------------------------------------------------------------------------'
class Dependencies;
class FileNode;
class Node;
class ObjectListNode;

// ProjectGeneratorBaseConfig
//------------------------------------------------------------------------------
class ProjectGeneratorBaseConfig
{
public:
    AString         m_Config;                   // Config name (e.g. Debug, Release etc.)
    const Node *    m_TargetNode = nullptr;     // Target to pass on cmdline to FASTBuild and used for Intellisense
};

// ProjectGeneratorBase
//------------------------------------------------------------------------------
class ProjectGeneratorBase
{
public:
    ProjectGeneratorBase();
    ~ProjectGeneratorBase();

    inline void SetBasePaths( const Array< AString > & paths ) { m_BasePaths = paths; }
    void AddFile( const AString & fileName );

    void AddConfig( const ProjectGeneratorBaseConfig & config );

    static bool WriteIfDifferent( const char * generatorId, const AString & content, const AString & fileName );
    static bool WriteIfMissing( const char * generatorId, const AString & content, const AString & fileName );
    static bool WriteToDisk( const char * generatorId, const AString & content, const AString & fileName );

    static void GetDefaultAllowedFileExtensions( Array< AString > & extensions );
    static void FixupAllowedFileExtensions( Array< AString > & extensions );

    // Intellisense Helpers
    static const ObjectListNode * FindTargetForIntellisenseInfo( const Node * node );
    static const ObjectListNode * FindTargetForIntellisenseInfo( const Dependencies & deps );
    static void ExtractIncludePaths( const AString & compilerArgs,
                                     Array< AString > & outIncludes,
                                     bool escapeQuotes );
    static void ExtractDefines( const AString & compilerArgs,
                                Array< AString > & outDefines,
                                bool escapeQuotes );
    static void ExtractAdditionalOptions( const AString & compilerArgs,
                                          Array< AString > & outOptions );
    static void ExtractIntellisenseOptions( const AString & compilerArgs,
                                            const Array< AString > & prefixes,
                                            Array< AString > & outOptions,
                                            bool escapeQuotes,
                                            bool keepFullOption );
    static void ConcatIntellisenseOptions( const Array< AString > & tokens,
                                           AString & outTokenString,
                                           const char* preToken,
                                           const char* postToken );
    static const FileNode * FindExecutableDebugTarget( const Node * node );
    static const FileNode * FindExecutableDebugTarget( const Dependencies & deps );

    // Helpers
    static void GetRelativePath( const AString & projectFolderPath,
                                 const AString & fileName,
                                 AString & outRelativeFileName );
protected:
    // Helper to format some text
    void Write( MSVC_SAL_PRINTF const char * fmtString, ... ) FORMAT_STRING( 2, 3 );

    // Internal helpers
    void        GetProjectRelativePath_Deprecated( const AString & fileName, AString & shortFileName ) const;
    struct Folder;
    Folder *    GetFolderFor( const AString & path );
    void        SortFilesAndFolders();

    struct File;
    struct Folder
    {
        AString             m_Path;         // Project Base Path(s) relative
        Array< File * >     m_Files;        // Child Files
        Array< Folder * >   m_Folders;      // Child Folders
        uint32_t            m_SortedIndex;

        bool operator < (const Folder& other) const { return m_Path < other.m_Path; }
    };
    struct File
    {
        AString             m_FileName;     // FileName with no path info
        AString             m_FullPath;     // Full path
        Folder *            m_Folder;       // Index into m_Folders
        uint32_t            m_SortedIndex;

        bool operator < (const File& other) const { return m_FileName < other.m_FileName; }
    };

    // Input Data
    Array< AString >    m_BasePaths;
    Folder *            m_RootFolder;
    Array< Folder * >   m_Folders;
    Array< File * >     m_Files;
    Array< const ProjectGeneratorBaseConfig * > m_Configs;

    // working buffer
    AString m_Tmp;
};

//------------------------------------------------------------------------------
