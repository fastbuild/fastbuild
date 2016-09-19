// DirectoryListNode
//------------------------------------------------------------------------------
#pragma once

// Includes
//------------------------------------------------------------------------------
#include "FileNode.h"

// Core
#include "Core/FileIO/FileIO.h"

// DirectoryListNode
//------------------------------------------------------------------------------
class DirectoryListNode : public Node
{
public:
    explicit DirectoryListNode( const AString & name,
                                const AString & path,
                                const Array< AString > * patterns,
                                bool recursive,
                                const Array< AString > & excludePaths,
                                const Array< AString > & filesToExclude,
                                const Array< AString > & excludePatterns );
    virtual ~DirectoryListNode();

    const AString & GetPath() const { return m_Path; }
    const Array< FileIO::FileInfo > & GetFiles() const { return m_Files; }

    static inline Node::Type GetTypeS() { return Node::DIRECTORY_LIST_NODE; }

    virtual bool IsAFile() const override { return false; }

    static void FormatName( const AString & path,
                            const Array< AString > * patterns,
                            bool recursive,
                            const Array< AString > & excludePaths,
                            const Array< AString > & excludeFiles,
                            const Array< AString > & excludePatterns,
                            AString & result );

    static Node * Load( NodeGraph & nodeGraph, IOStream & stream );
    virtual void Save( IOStream & stream ) const override;

private:
    virtual BuildResult DoBuild( Job * job ) override;

    AString m_Path;
    Array< AString > m_Patterns;
    Array< AString > m_ExcludePaths;
    Array< AString > m_FilesToExclude;
    Array< AString > m_ExcludePatterns;
    bool m_Recursive;

    Array< FileIO::FileInfo > m_Files;
};

//------------------------------------------------------------------------------
