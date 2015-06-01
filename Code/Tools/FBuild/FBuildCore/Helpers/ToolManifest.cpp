// ToolManifest
//------------------------------------------------------------------------------

// Includes
//------------------------------------------------------------------------------
#include "Tools/FBuild/FBuildCore/PrecompiledHeader.h"

#include "ToolManifest.h"

// Core
#include "Core/Containers/AutoPtr.h"
#include "Core/Env/Env.h"
#include "Core/FileIO/ConstMemoryStream.h"
#include "Core/FileIO/FileIO.h"
#include "Core/FileIO/FileStream.h"
#include "Core/FileIO/MemoryStream.h"
#include "Core/FileIO/PathUtils.h"
#include "Core/Math/Murmur3.h"
#include "Core/Strings/AStackString.h"
#include "Tools/FBuild/FBuildCore/Graph/FileNode.h"
#include "Tools/FBuild/FBuildCore/FLog.h"

// system
#include <memory.h> // memcpy

// CONSTRUCTOR (File)
//------------------------------------------------------------------------------
ToolManifest::File::File( const AString & name, uint64_t stamp, uint32_t hash, const Node * node, uint32_t size ) 
	: m_Name( name ), 
	m_TimeStamp( stamp ), 
	m_Hash( hash ), 
	m_ContentSize( size ), 
	m_Node( node ), 
	m_Content( nullptr ), 
	m_SyncState( NOT_SYNCHRONIZED ),
	m_FileLock( nullptr )
{}

// DESTRUCTOR (File)
//------------------------------------------------------------------------------
ToolManifest::File::~File()
{ 
	FREE( m_Content );
	FDELETE( m_FileLock );
}

// CONSTRUCTOR
//------------------------------------------------------------------------------
ToolManifest::ToolManifest()
	: m_ToolId( 0 )
	, m_TimeStamp( 0 )
	, m_Files( 0, true )
	, m_Synchronized( false )
	, m_RemoteEnvironmentString( nullptr )
	, m_UserData( nullptr )
{
}

// CONSTRUCTOR
//------------------------------------------------------------------------------
ToolManifest::ToolManifest( uint64_t toolId )
	: m_ToolId( toolId )
	, m_TimeStamp( 0 )
	, m_Files( 0, true )
	, m_Synchronized( false )
	, m_RemoteEnvironmentString( nullptr )
	, m_UserData( nullptr )
{
}

// DESTRUCTOR
//------------------------------------------------------------------------------
ToolManifest::~ToolManifest()
{
	FREE( (void *)m_RemoteEnvironmentString );
}

// Generate
//------------------------------------------------------------------------------
bool ToolManifest::Generate( const Node * mainExecutable, const Dependencies & dependencies )
{
	m_Files.Clear();
	m_TimeStamp = 0;
	m_Files.SetCapacity( 1 + dependencies.GetSize() );

	// unify "main executable" and "extra files"
	// (loads contents of file into memory, and creates hashes)
    if ( !AddFile( mainExecutable ) )
    {
        return false; // AddFile will have emitted error
    }
	for ( size_t i=0; i<dependencies.GetSize(); ++i )
	{
		const FileNode & n = *( dependencies[ i ].GetNode()->CastTo< FileNode >() );
		if ( !AddFile( &n ) )
		{
			return false; // AddFile will have emitted error
		}
	}

	// create a hash for the whole tool chain
	const size_t numFiles( m_Files.GetSize() );
	const size_t memSize( numFiles * sizeof( uint32_t ) * 2 );
	uint32_t * mem = (uint32_t *)ALLOC( memSize );
	uint32_t * pos = mem;
	for ( size_t i=0; i<numFiles; ++i )
	{
		const File & f = m_Files[ i ];

		// file contents
		*pos = f.m_Hash;
		++pos;

		// file name & sub-path (relative to remote folder)
		AStackString<> relativePath;
		GetRemoteFilePath( (uint32_t)i, relativePath, false ); // false = don't use full path
		*pos = Murmur3::Calc32( relativePath );
		++pos;
	}
	uint64_t hashA, hashB;
	hashA = Murmur3::Calc128( mem, memSize, hashB );
	m_ToolId = hashA ^ hashB; // xor merge
	FREE( mem );

	// update time stamp (most recent file in manifest)
	for ( size_t i=0; i<numFiles; ++i )
	{
		const File & f = m_Files[ i ];
		ASSERT( f.m_TimeStamp ); // should have had an error before if the file was missing
		m_TimeStamp = Math::Max( m_TimeStamp, f.m_TimeStamp );
	}

	return true;
}

// Serialize
//------------------------------------------------------------------------------
void ToolManifest::Serialize( IOStream & ms ) const
{
	ms.Write( m_ToolId );

	const uint32_t numItems( (uint32_t)m_Files.GetSize() );
	ms.Write( numItems );
	const size_t numFiles( m_Files.GetSize() );
	for ( size_t i=0; i<numFiles; ++i )
	{
		const File & f = m_Files[ i ];
		ms.Write( f.m_Name );
		ms.Write( f.m_TimeStamp );
		ms.Write( f.m_Hash );
		ms.Write( f.m_ContentSize );
	}
}

// Deserialize
//------------------------------------------------------------------------------
void ToolManifest::Deserialize( IOStream & ms, bool remote )
{
	ms.Read( m_ToolId );

	ASSERT( m_Files.IsEmpty() );

	uint32_t numFiles( 0 );
	ms.Read( numFiles );
	m_Files.SetCapacity( numFiles );

	for ( size_t i=0; i<(size_t)numFiles; ++i )
	{		
		AStackString<> name;
		uint64_t timeStamp( 0 );
		uint32_t hash( 0 );
		uint32_t contentSize( 0 );
		ms.Read( name );
		ms.Read( timeStamp );
		ms.Read( hash );
		ms.Read( contentSize );
		m_Files.Append( File( name, timeStamp, hash, nullptr, contentSize ) );
	}

    // everything else is only needed remotely (in the worker)
    if ( remote == false )
    {
        return;
    }

	// determine if any files are remaining from a previous run
	size_t numFilesAlreadySynchronized = 0;
	for ( size_t i=0; i<(size_t)numFiles; ++i )
	{
		AStackString<> localFile;
		GetRemoteFilePath( (uint32_t)i, localFile );

		// is this file already present?
		AutoPtr< FileStream > fileStream( FNEW( FileStream ) );
		FileStream & f = *( fileStream.Get() );
		if ( f.Open( localFile.Get() ) == false )
		{
			continue; // file not found
		}
		if ( f.GetFileSize() != m_Files[ i ].m_ContentSize )
		{
			continue; // file is not complete
		}
		AutoPtr< char > mem( (char *)ALLOC( (size_t)f.GetFileSize() ) );
		if ( f.Read( mem.Get(), (size_t)f.GetFileSize() ) != f.GetFileSize() )
		{
			continue; // problem reading file
		}
		if( Murmur3::Calc32( mem.Get(), (size_t)f.GetFileSize() ) != m_Files[ i ].m_Hash )
		{
			continue; // file contents unexpected
		}

		// file present and ok
		m_Files[ i ].m_FileLock = fileStream.Release(); // NOTE: keep file open to prevent deletions
		m_Files[ i ].m_SyncState = File::SYNCHRONIZED;
		numFilesAlreadySynchronized++;
	}

	// Generate Environment
	ASSERT( m_RemoteEnvironmentString == nullptr );

	// PATH=
	AStackString<> basePath;
	GetRemotePath( basePath );
	AStackString<> paths;
	paths.Format( "PATH=%s", basePath.Get() );

	// TMP=
	AStackString<> normalTmp;
	Env::GetEnvVariable( "TMP", normalTmp );
	AStackString<> tmp;
	tmp.Format( "TMP=%s", normalTmp.Get() );

	// SystemRoot=
	AStackString<> sysRoot( "SystemRoot=C:\\Windows" );

	char * mem = (char *)ALLOC( paths.GetLength() + 1 +
								  tmp.GetLength() + 1 +
								  sysRoot.GetLength() + 1 +
								  1 );
	m_RemoteEnvironmentString = mem;

	AString::Copy( paths.Get(), mem, paths.GetLength() + 1 ); // including null
	mem += ( paths.GetLength() + 1 ); // including null

	AString::Copy( tmp.Get(), mem, tmp.GetLength() + 1 ); // including null
	mem += ( tmp.GetLength() + 1 ); // including null

	AString::Copy( sysRoot.Get(), mem, sysRoot.GetLength() + 1 ); // including null
	mem += ( sysRoot.GetLength() + 1 ); // including null

	*mem = 0; ++mem; // double null

	// are all files already present?
	if ( numFilesAlreadySynchronized == m_Files.GetSize() )
	{
		m_Synchronized = true;		
	}
}

// GetSynchronizationStatus
//------------------------------------------------------------------------------
bool ToolManifest::GetSynchronizationStatus( uint32_t & syncDone, uint32_t & syncTotal ) const
{
	syncDone = 0;
	syncTotal = 0;
	bool synching = false;

	MutexHolder mh( m_Mutex );

	// is completely synchronized?
	const File * const end = m_Files.End();
	for ( const File * it = m_Files.Begin(); it != end; ++it )
	{
		syncTotal += it->m_ContentSize;
		if ( it->m_SyncState == File::SYNCHRONIZED )
		{
			syncDone += it->m_ContentSize;
		}
		else if ( it->m_SyncState == File::SYNCHRONIZING )
		{
			synching = true;
		}
	}

	return synching;
}

// CancelSynchronizingFiles
//------------------------------------------------------------------------------
void ToolManifest::CancelSynchronizingFiles()
{
	MutexHolder mh( m_Mutex );

	bool atLeastOneFileCancelled = false;

	// is completely synchronized?
	File * const end = m_Files.End();
	for ( File * it = m_Files.Begin(); it != end; ++it )
	{
		if ( it->m_SyncState == File::SYNCHRONIZING )
		{
			it->m_SyncState = File::NOT_SYNCHRONIZED;
			atLeastOneFileCancelled = true;
		}
	}

	// We should not have called this function if we
	// were not synchronizing files in this ToolManifest
	ASSERT( atLeastOneFileCancelled ); 
	(void)atLeastOneFileCancelled;
}

// GetFileData
//------------------------------------------------------------------------------
const void * ToolManifest::GetFileData( uint32_t fileId, size_t & dataSize ) const
{
	const File & f = m_Files[ fileId ];
	if ( f.m_Content == nullptr )
	{
		if ( !LoadFile( f.m_Name, f.m_Content, f.m_ContentSize ) )
		{
			return nullptr;
		}
	}
	dataSize = f.m_ContentSize;
	return f.m_Content;
}

// ReceiveFileData
//------------------------------------------------------------------------------
bool ToolManifest::ReceiveFileData( uint32_t fileId, const void * data, size_t & dataSize )
{
	MutexHolder mh( m_Mutex );

	File & f = m_Files[ fileId ];

	// gracefully handle multiple receipts of the same data
	if ( f.m_Content )
	{
		ASSERT( f.m_SyncState == File::SYNCHRONIZED );
		return true;
	}

	ASSERT( f.m_SyncState == File::SYNCHRONIZING );

	// prepare name for this file
	AStackString<> fileName;
	GetRemoteFilePath( fileId, fileName );

	// prepare destination
	AStackString<> pathOnly( fileName.Get(), fileName.FindLast( NATIVE_SLASH ) );
	if ( !FileIO::EnsurePathExists( pathOnly ) )
	{
		return false; // FAILED
	}

	// write to disk
	FileStream fs;
	if ( !fs.Open( fileName.Get(), FileStream::WRITE_ONLY ) )
	{
		return false; // FAILED
	}
	if ( fs.Write( data, dataSize ) != dataSize )
	{
		return false; // FAILED
	}
	fs.Close();

	// open read-only 
	AutoPtr< FileStream > fileStream( FNEW( FileStream ) );
	if ( fileStream.Get()->Open( fileName.Get(), FileStream::READ_ONLY ) == false )
	{
		return false; // FAILED
	}

	// This file is now synchronized
	f.m_FileLock = fileStream.Release(); // NOTE: Keep file open to prevent deletion
	f.m_SyncState = File::SYNCHRONIZED;

	// is completely synchronized?
	const File * const end = m_Files.End();
	for ( const File * it = m_Files.Begin(); it != end; ++it )
	{
		if ( it->m_SyncState != File::SYNCHRONIZED )
		{
			// still some files to be received
			return true; // file stored ok
		}
	}

	// all files received
	m_Synchronized = true;
	return true; // file stored ok
}

// GetRemoteFilePath
//------------------------------------------------------------------------------
void ToolManifest::GetRemoteFilePath( uint32_t fileId, AString & exe, bool fullPath ) const
{
	// we'll store in the sub dir
	if ( fullPath )
	{
		GetRemotePath( exe );
	}
	else
	{
		exe.Clear();
	}

	// determine primary root
	const File & primaryFile = m_Files[ 0 ];
	AStackString<> primaryPath( primaryFile.m_Name.Get(), primaryFile.m_Name.FindLast( NATIVE_SLASH ) + 1 ); // include backslash

	const File & f = m_Files[ fileId ];
	if ( f.m_Name.BeginsWithI( primaryPath ) )
	{
		// file is in sub dir on master machine, so store with same relative location
		exe += ( f.m_Name.Get() + primaryPath.GetLength() );
	}
	else
	{
		// file is in some completely other directory, so put in same place as exe
		const char * lastSlash = f.m_Name.FindLast( NATIVE_SLASH );
		lastSlash = lastSlash ? lastSlash + 1 : f.m_Name.Get();
		exe += AStackString<>( lastSlash, f.m_Name.GetEnd() );
	}
}

// GetRemotePath
//------------------------------------------------------------------------------
void ToolManifest::GetRemotePath( AString & path ) const
{
	VERIFY( FileIO::GetTempDir( path ) );
	AStackString<> subDir;
    #if defined( __WINDOWS__ )
        subDir.Format( ".fbuild.tmp\\worker\\toolchain.%016llx\\", m_ToolId );
    #else
        subDir.Format( "_fbuild.tmp/worker/toolchain.%016llx/", m_ToolId );
    #endif
	path += subDir;
}

// AddFile
//------------------------------------------------------------------------------
bool ToolManifest::AddFile( const Node * node )
{
	ASSERT( node->IsAFile() ); // we can only distribute files

	uint32_t contentSize( 0 );
	void * content( nullptr );
	if ( !LoadFile( node->GetName(), content, contentSize ) )
	{
		return false; // LoadContent will have emitted an error
	}

	// create the file entry
	const AString & name = node->GetName();
	const uint64_t timeStamp = node->GetStamp();
	const uint32_t hash = Murmur3::Calc32( content, contentSize );
	m_Files.Append( File(name, timeStamp, hash, node, contentSize ) );

	// store file content (take ownership of file data)
	// TODO:B Compress the data - less memory used + faster to send over network
	File & f = m_Files.Top();
	f.m_Content = content;

	return true;
}

// LoadFile
//------------------------------------------------------------------------------
bool ToolManifest::LoadFile( const AString & fileName, void * & content, uint32_t & contentSize ) const
{
	// read the file into memory
	FileStream fs;
	if ( fs.Open( fileName.Get(), FileStream::READ_ONLY ) == false )
	{
		FLOG_ERROR( "Error opening file '%s' in Compiler ToolManifest\n", fileName.Get() );
		return false;
	}
	contentSize = (uint32_t)fs.GetFileSize();
	AutoPtr< void > mem( ALLOC( contentSize ) );
	if ( fs.Read( mem.Get(), contentSize ) != contentSize )
	{
		FLOG_ERROR( "Error reading file '%s' in Compiler ToolManifest\n", fileName.Get() );
		return false;
	}

	content = mem.Release();
	return true;
}

//------------------------------------------------------------------------------
