// ProjectGeneratorBase
//------------------------------------------------------------------------------
#pragma once

// Includes
//------------------------------------------------------------------------------
#include "Core/Containers/Array.h"
#include "Core/Strings/AString.h"

// Forward Declarations
//------------------------------------------------------------------------------'
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

    static void GetDefaultAllowedFileExtensions( Array< AString > & extensions );
    static void FixupAllowedFileExtensions( Array< AString > & extensions );

    // Intellisense Helpers
    static const ObjectListNode * FindTargetForIntellisenseInfo( const Node * node );
    static void ExtractIntellisenseOptions( const AString & compilerArgs,
                                            const char * option,
                                            const char * alternateOption,
                                            Array< AString > & outOptions,
                                            bool escapeQuotes );
    static void ConcatIntellisenseOptions( const Array< AString > & tokens,
                                           AString & outTokenString,
                                           const char* preToken,
                                           const char* postToken );

protected:
    // Helper to format some text
    void Write( const char * fmtString, ... );

    // Internal helpers
    void        GetProjectRelativePath( const AString & fileName, AString & shortFileName ) const;
    uint32_t    GetFolderIndexFor( const AString & path );

    struct Folder
    {
        AString             m_Path;     // Project Base Path(s) relative
        Array< uint32_t >   m_Files;    // Indices into m_Files
        Array< uint32_t >   m_Folders;  // Indices into m_Folders
    };
    struct File
    {
        AString     m_Name;         // Project Base Path(s) relative
        AString     m_FullPath;     // Full path
        uint32_t    m_FolderIndex;  // Index into m_Folders
    };
    struct Config
    {
        AString         m_Name;                     // Config name (e.g. Debug, Release etc.)
        const Node *    m_TargetNode = nullptr;     // Target to pass on cmdline to FASTBuild and used for Intellisense
    };

    // Input Data
    Array< AString >    m_BasePaths;
    Array< Folder >     m_Folders;
    Array< File >       m_Files;
    Array< Config >     m_Configs;

    // working buffer
    AString m_Tmp;
};

//------------------------------------------------------------------------------
