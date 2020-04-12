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
class DirectoryListNode;
class Function;
class UnityFileAndOrigin;

// UnityIsolatedFile
//------------------------------------------------------------------------------
class UnityIsolatedFile : public Struct
{
    REFLECT_STRUCT_DECLARE( UnityIsolatedFile )
public:
    UnityIsolatedFile();
    UnityIsolatedFile( const AString & fileName, const DirectoryListNode * dirListOrigin );
    ~UnityIsolatedFile();

    inline const AString &      GetFileName() const             { return m_FileName; }
    inline const AString &      GetDirListOriginPath() const    { return m_DirListOriginPath; }

protected:
    AString m_FileName;
    AString m_DirListOriginPath;
};

// UnityNode
//------------------------------------------------------------------------------
class UnityNode : public Node
{
    REFLECT_NODE_DECLARE( UnityNode )
public:
    friend class FunctionUnity;

    explicit UnityNode();
    virtual bool Initialize( NodeGraph & nodeGraph, const BFFToken * iter, const Function * function ) override;
    virtual ~UnityNode() override;

    static inline Node::Type GetTypeS() { return Node::UNITY_NODE; }

    inline const Array< AString > & GetUnityFileNames() const { return m_UnityFileNames; }
    inline const Array< UnityIsolatedFile > & GetIsolatedFileNames() const { return m_IsolatedFiles; }

    void EnumerateInputFiles( void (*callback)( const AString & inputFile, const AString & baseDir, void * userData ), void * userData ) const;

private:
    virtual bool DetermineNeedToBuild( const Dependencies & deps ) const override;
    virtual BuildResult DoBuild( Job * job ) override;

    virtual bool IsAFile() const override { return false; }

    bool GetFiles( Array< UnityFileAndOrigin > & files );
    bool GetIsolatedFilesFromList( Array< AString > & files ) const;
    void FilterForceIsolated( Array< UnityFileAndOrigin > & files, Array< UnityIsolatedFile > & isolatedFiles );

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
    Array< AString > m_FilesToIsolate;
    bool m_IsolateWritableFiles;
    uint32_t m_MaxIsolatedFiles;
    AString m_IsolateListFile;
    Array< AString > m_ExcludePatterns;
    Array< AString > m_PreBuildDependencyNames;

    // Temporary data
    Array< FileIO::FileInfo* > m_FilesInfo;

    // Internal data persisted between builds
    Array< UnityIsolatedFile > m_IsolatedFiles;
    Array< AString > m_UnityFileNames;
};

//------------------------------------------------------------------------------
