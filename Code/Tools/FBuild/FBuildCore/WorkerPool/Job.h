// Job - a work item to be processed
//------------------------------------------------------------------------------
#pragma once
#ifndef FBUILD_WORKERPOOL_JOB_H
#define FBUILD_WORKERPOOL_JOB_H

// Includes
//------------------------------------------------------------------------------
#include "Core/Env/Types.h"
#include "Core/Strings/AString.h"

// Forward Declarations
//------------------------------------------------------------------------------
class IOStream;
class Node;
class ToolManifest;

// Job
//------------------------------------------------------------------------------
class Job
{
public:
	explicit Job( Node * node );
	explicit Job( IOStream & stream );
			~Job();

	inline uint32_t GetJobId() const { return m_JobId; }
	inline bool operator == ( uint32_t jobId ) const { return ( m_JobId == jobId ); }

	inline Node * GetNode() const { return m_Node; }
	inline const AString & GetRemoteName() const { return m_RemoteName; }

	inline void SetCacheName( const AString & cacheName ) { m_CacheName = cacheName; }
	inline const AString & GetCacheName() const { return m_CacheName; }

	// associate some data with this object, and destroy it when freed
	void	OwnData( void * data, size_t size, bool compressed = false );

	inline void *	GetData() const		{ return m_Data; }
	inline size_t	GetDataSize() const { return m_DataSize; }

	inline void		SetUserData( void * data )	{ m_UserData = data; }
	inline void *	GetUserData() const			{ return m_UserData; }

	inline void				SetToolManifest( ToolManifest * manifest )	{ m_ToolManifest = manifest; }
	inline ToolManifest *	GetToolManifest() const						{ return m_ToolManifest; }

	inline bool		IsDataCompressed() const { return m_DataIsCompressed; }
	inline bool		IsLocal() const		{ return m_IsLocal; }

	inline const Array< AString > & GetMessages() const { return m_Messages; }

	// logging interface
	void Error( const char * format, ... );

	// Flag "system failures" - i.e. not a compilation failure, but some other problem (typically a remote worker misbehaving)
	void OnSystemError() { ++m_SystemErrorCount; }
	inline uint8_t GetSystemErrorCount() const { return m_SystemErrorCount; }

	// serialization for remote distribution
	void Serialize( IOStream & stream );
	void Deserialize( IOStream & stream );

private:
	uint32_t m_JobId;
	Node * m_Node;
	void * m_Data;
	uint32_t m_DataSize;
	void * m_UserData;
	bool m_DataIsCompressed;
	bool m_IsLocal;
	uint8_t m_SystemErrorCount; // On client, the total error count, on the worker a flag for the current attempt
	AString m_RemoteName;
	AString m_CacheName;

	ToolManifest * m_ToolManifest;

	Array< AString > m_Messages;
};

//------------------------------------------------------------------------------
#endif // FBUILD_WORKERPOOL_JOB_H 