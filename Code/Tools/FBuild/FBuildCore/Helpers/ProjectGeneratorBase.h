// ProjectGeneratorBase
//------------------------------------------------------------------------------
#pragma once

// Includes
//------------------------------------------------------------------------------
#include "Core/Containers/Array.h"
#include "Core/Strings/AString.h"

// Forward Declarations
//------------------------------------------------------------------------------'
class Dependencies;
class Node;
class ObjectListNode;

// ProjectGeneratorBase
//------------------------------------------------------------------------------
class ProjectGeneratorBase
{
public:
    ProjectGeneratorBase();
    ~ProjectGeneratorBase();

    inline void SetBasePaths( const Array< AString > & paths ) { m_BasePaths = paths; }
    void AddFile( const AString & fileName );

    void AddConfig( const AString & name, const Node * targetNode );

    static bool WriteIfDifferent( const char * generatorId, const AString & content, const AString & fileName );
    static bool WriteIfMissing( const char * generatorId, const AString & content, const AString & fileName );
    static bool WriteToDisk( const char * generatorId, const AString & content, const AString & fileName );

    static void GetDefaultAllowedFileExtensions( Array< AString > & extensions );
    static void FixupAllowedFileExtensions( Array< AString > & extensions );

    // Intellisense Helpers
    static const ObjectListNode * FindTargetForIntellisenseInfo( const Node * node );
    static const ObjectListNode * FindTargetForIntellisenseInfo( const Dependencies & deps );
    static void ExtractIntellisenseOptions( const AString & compilerArgs,
                                            const char * option,
                                            const char * alternateOption,
                                            Array< AString > & outOptions,
                                            bool escapeQuotes,
                                            bool keepFullOption );
    static void ConcatIntellisenseOptions( const Array< AString > & tokens,
                                           AString & outTokenString,
                                           const char* preToken,
                                           const char* postToken );

    // Helpers
    static void GetRelativePath( const AString & projectFolderPath,
                                 const AString & fileName,
                                 AString & outRelativeFileName );
protected:
    // Helper to format some text
    void Write( const char * fmtString, ... ) FORMAT_STRING( 2, 3 );

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
        AString             m_Name;         // Project Base Path(s) relative
        AString             m_FullPath;     // Full path
        Folder *            m_Folder;       // Index into m_Folders
        uint32_t            m_SortedIndex;

        bool operator < (const File& other) const { return m_FullPath < other.m_FullPath; }
    };
    struct Config
    {
        AString         m_Name;                     // Config name (e.g. Debug, Release etc.)
        const Node *    m_TargetNode = nullptr;     // Target to pass on cmdline to FASTBuild and used for Intellisense
    };

    // Input Data
    Array< AString >    m_BasePaths;
    Folder *            m_RootFolder;
    Array< Folder * >   m_Folders;
    Array< File * >     m_Files;
    Array< Config >     m_Configs;

    // working buffer
    AString m_Tmp;
};

//------------------------------------------------------------------------------
