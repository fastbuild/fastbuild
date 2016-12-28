// UnityNode.h - a node that builds a unity.cpp file
//------------------------------------------------------------------------------
#pragma once

// Includes
//------------------------------------------------------------------------------
#include "FileNode.h"
#include "Core/Containers/Array.h"
#include "Core/FileIO/FileIO.h"

// Forward Declarations
//------------------------------------------------------------------------------
class BFFIterator;
class DirectoryListNode;
class Function;

// UnityNode
//------------------------------------------------------------------------------
class UnityNode : public Node
{
    REFLECT_NODE_DECLARE( UnityNode )
public:
    friend class FunctionUnity;

    explicit UnityNode();
    bool Initialize( NodeGraph & nodeGraph, const BFFIterator & iter, const Function * function );
    virtual ~UnityNode();

    static inline Node::Type GetTypeS() { return Node::UNITY_NODE; }

    inline const Array< AString > & GetUnityFileNames() const { return m_UnityFileNames; }

    // For each file isolated from Unity, we track the original dir list (if available)
    // This allows ObjectList/Library to create a sensible (relative) output dir.
    class FileAndOrigin
    {
    public:
        FileAndOrigin( FileIO::FileInfo * info, DirectoryListNode * dirListOrigin )
         : m_Info( info )
         , m_DirListOrigin( dirListOrigin )
        {}

        inline const AString &              GetName() const             { return m_Info->m_Name; }
        inline bool                         IsReadOnly() const          { return m_Info->IsReadOnly(); }
        inline const DirectoryListNode *    GetDirListOrigin() const    { return m_DirListOrigin; }

    protected:
        FileIO::FileInfo *      m_Info;
        DirectoryListNode *     m_DirListOrigin;
    };
    inline const Array< FileAndOrigin > & GetIsolatedFileNames() const { return m_IsolatedFiles; }

    static Node * Load( NodeGraph & nodeGraph, IOStream & stream );
    virtual void Save( IOStream & stream ) const override;
private:
    virtual BuildResult DoBuild( Job * job ) override;

    virtual bool IsAFile() const override { return false; }

    bool GetFiles( Array< FileAndOrigin > & files );

    // Exposed properties
    Array< AString > m_InputPaths;
    bool m_InputPathRecurse;
    Array< AString > m_InputPattern;
    Array< AString > m_Files;
    Array< AString > m_ObjectLists;
    AString m_OutputPath;
    AString m_OutputPattern;
    uint32_t m_NumUnityFilesToCreate;
    AString m_PrecompiledHeader;
    Array< AString > m_PathsToExclude;
    Array< AString > m_FilesToExclude;
    bool m_IsolateWritableFiles;
    uint32_t m_MaxIsolatedFiles;
    Array< AString > m_ExcludePatterns;
    Array< FileAndOrigin > m_IsolatedFiles;
    Array< AString > m_PreBuildDependencyNames;

    // Temporary data
    Array< AString > m_UnityFileNames;
    Array< FileIO::FileInfo* > m_FilesInfo;
};

//------------------------------------------------------------------------------
