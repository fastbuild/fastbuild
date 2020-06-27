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
    REFLECT_NODE_DECLARE( DirectoryListNode )
public:
    DirectoryListNode();
    virtual bool Initialize( NodeGraph & nodeGraph, const BFFToken * iter, const Function * function ) override;
    virtual ~DirectoryListNode() override;

    const AString & GetPath() const { return m_Path; }
    const Array< FileIO::FileInfo > & GetFiles() const { return m_Files; }

    static inline Node::Type GetTypeS() { return Node::DIRECTORY_LIST_NODE; }

    virtual bool IsAFile() const override { return false; }

    virtual const AString & GetPrettyName() const override { return m_PrettyName.IsEmpty() ? m_Name : m_PrettyName; }

    static void FormatName( const AString & path,
                            const Array< AString > * patterns,
                            bool recursive,
                            bool includeReadOnlyStatusInHash,
                            const Array< AString > & excludePaths,
                            const Array< AString > & excludeFiles,
                            const Array< AString > & excludePatterns,
                            AString & result );

private:
    virtual BuildResult DoBuild( Job * job ) override;

    void MakePrettyName( const size_t totalFiles );

    friend class CompilationDatabase; // For DoBuild - TODO:C This is not ideal

    // Reflected Properties
    friend class Function; // TODO:C Remove
    friend class TestGraph; // TODO:C Remove
    AString m_Path;
    Array< AString > m_Patterns;
    Array< AString > m_ExcludePaths;
    Array< AString > m_FilesToExclude;
    Array< AString > m_ExcludePatterns;
    bool m_Recursive;
    bool m_IncludeReadOnlyStatusInHash;

    // Internal State
    Array< FileIO::FileInfo > m_Files;
    AString m_PrettyName;
};

//------------------------------------------------------------------------------
