// DirectoryListNode
//------------------------------------------------------------------------------
#pragma once
#ifndef FBUILD_GRAPH_DIRECTORYLISTNODE_H
#define FBUILD_GRAPH_DIRECTORYLISTNODE_H

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
                                const Array< AString > & filesToExclude );
	virtual ~DirectoryListNode();

	const AString & GetPath() const { return m_Path; }
	const Array< FileIO::FileInfo > & GetFiles() const { return m_Files; }

	static inline Node::Type GetTypeS() { return Node::DIRECTORY_LIST_NODE; }

	virtual bool IsAFile() const { return false; }

	static void FormatName( const AString & path,
							const Array< AString > * patterns,
							bool recursive,
							const Array< AString > & excludePaths,
                            const Array< AString > & excludeFiles,
							AString & result );

	static Node * Load( IOStream & stream );
	virtual void Save( IOStream & stream ) const;

private:
	virtual BuildResult DoBuild( Job * job );

	AString m_Path;
	Array< AString > m_Patterns;
	Array< AString > m_ExcludePaths;
    Array< AString > m_FilesToExclude;
	bool m_Recursive;

	Array< FileIO::FileInfo > m_Files;
};

//------------------------------------------------------------------------------
#endif // FBUILD_GRAPH_DIRECTORYLISTNODE_H
