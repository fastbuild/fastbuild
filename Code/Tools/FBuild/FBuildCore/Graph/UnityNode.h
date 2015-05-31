// UnityNode.h - a node that builds a unity.cpp file
//------------------------------------------------------------------------------
#pragma once
#ifndef FBUILD_GRAPH_UNITYNODE_H
#define FBUILD_GRAPH_UNITYNODE_H

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
#ifdef USE_NODE_REFLECTION
	REFLECT_DECLARE( UnityNode )
#endif
public:
#ifdef USE_NODE_REFLECTION
	friend class FunctionUnity;

	explicit UnityNode();
	virtual bool Initialize( const BFFIterator & iter, const Function * function );
#else
	explicit UnityNode( const AString & unityName,
						const Dependencies & dirNodes,
						const Array< AString > & files,
						const AString & outputPath,
						const AString & outputPattern,
						uint32_t numUnityFilesToCreate,
						const AString & precompiledHeader,
						bool isolateWritableFiles,
						uint32_t maxIsolatedFiles,
						const Array< AString > & excludePatterns );
#endif
	virtual ~UnityNode();

	static inline Node::Type GetType() { return Node::UNITY_NODE; }

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

	static Node * Load( IOStream & stream );
	virtual void Save( IOStream & stream ) const;
private:
	virtual BuildResult DoBuild( Job * job );

	virtual bool IsAFile() const { return false; }

	bool GetFiles( Array< FileAndOrigin > & files );

	// Exposed properties
#ifdef USE_NODE_REFLECTION
	Array< AString > m_InputPaths;
	bool m_InputPathRecurse;
	AString m_InputPattern;
#endif
	Array< AString > m_Files;
	AString m_OutputPath;
	AString m_OutputPattern;
	uint32_t m_NumUnityFilesToCreate;
	AString m_PrecompiledHeader;
#ifdef USE_NODE_REFLECTION
	Array< AString > m_PathsToExclude;
	Array< AString > m_FilesToExclude;
#endif
	bool m_IsolateWritableFiles;
	uint32_t m_MaxIsolatedFiles;
	Array< AString > m_ExcludePatterns;
	Array< FileAndOrigin > m_IsolatedFiles;

	// Temporary data
	Array< AString > m_UnityFileNames;
	Array< FileIO::FileInfo > m_FilesInfo;
};

//------------------------------------------------------------------------------
#endif // FBUILD_GRAPH_LIBRARYNODE_H
