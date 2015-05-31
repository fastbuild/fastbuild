// ToolManifest
//------------------------------------------------------------------------------
#pragma once
#ifndef FBUILD_HELPERS_TOOLMANIFEST_H
#define FBUILD_HELPERS_TOOLMANIFEST_H

// Forward Declarations
//------------------------------------------------------------------------------
class Dependencies;
class IOStream;
class Node;

// Includes
//------------------------------------------------------------------------------
#include "Core/Containers/Array.h"
#include "Core/Env/Types.h"
#include "Core/Process/Mutex.h"
#include "Core/Strings/AString.h"

// ToolManifest
//------------------------------------------------------------------------------
class ToolManifest
{
public:
	explicit ToolManifest();
	explicit ToolManifest( uint64_t toolId );
	~ToolManifest();

	bool Generate( const Node * mainExecutable, const Dependencies & dependencies );

	inline uint64_t GetToolId() const { return m_ToolId; }
	inline uint64_t GetTimeStamp() const { return m_TimeStamp; }

	void Serialize( IOStream & ms ) const;
	void Deserialize( IOStream & ms, bool remote );

	inline bool IsSynchronized() const { return m_Synchronized; }
	bool GetSynchronizationStatus( uint32_t & syncDone, uint32_t & syncTotal ) const;

	// operator for FindDeref
	inline bool operator == ( uint64_t toolId ) const
	{
		return ( m_ToolId == toolId );
	}

	inline void		SetUserData( void * data )	{ m_UserData = data; }
	inline void *	GetUserData() const			{ return m_UserData; }

	struct File
	{
		explicit File( const AString & name, uint64_t stamp, uint32_t hash, const Node * node, uint32_t size );
		~File();

		enum SyncState
		{
			NOT_SYNCHRONIZED,
			SYNCHRONIZING,
			SYNCHRONIZED,
		};

		// common members
		AString			m_Name;
		uint64_t		m_TimeStamp;
		uint32_t		m_Hash;
		mutable uint32_t m_ContentSize;

		// "local" members
		const Node *	m_Node;
		mutable void *	m_Content;

		// "remote" members
		SyncState		m_SyncState;
		FileStream *	m_FileLock; // keep the file locked when sync'd
	};
	const Array< File > & GetFiles() const { return m_Files; }

	void MarkFileAsSynchronizing( size_t fileId ) { ASSERT( m_Files[ fileId ].m_SyncState == File::NOT_SYNCHRONIZED ); m_Files[ fileId ].m_SyncState = File::SYNCHRONIZING; }
	void CancelSynchronizingFiles();

	const void *	GetFileData( uint32_t fileId, size_t & dataSize ) const;
	bool			ReceiveFileData( uint32_t fileId, const void * data, size_t & dataSize );

	void			GetRemotePath( AString & path ) const;
	void			GetRemoteFilePath( uint32_t fileId, AString & exe, bool fullPath = true ) const;
	const char *	GetRemoteEnvironmentString() const { return m_RemoteEnvironmentString; }
private:
	bool			AddFile( const Node * node );
	bool			LoadFile( const AString & fileName, void * & content, uint32_t & contentSize ) const;

	uint64_t		m_ToolId;	// Global identifier for this toolchain
	uint64_t		m_TimeStamp;// Time stamp of most recent file
	mutable Mutex	m_Mutex;
	Array< File >	m_Files;
	bool			m_Synchronized;
	const char *	m_RemoteEnvironmentString;
	void *			m_UserData;
};

//------------------------------------------------------------------------------
#endif // FBUILD_HELPERS_TOOLMANIFEST_H
