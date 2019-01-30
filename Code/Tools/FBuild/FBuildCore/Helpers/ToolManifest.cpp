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
#include "Core/Math/xxHash.h"
#include "Core/Strings/AStackString.h"
#include "Tools/FBuild/FBuildCore/FBuild.h"
#include "Tools/FBuild/FBuildCore/FLog.h"
#include "Tools/FBuild/FBuildCore/Graph/FileNode.h"
#include "Tools/FBuild/FBuildCore/Helpers/Compressor.h"

// system
#include <memory.h> // memcpy

// Reflection
//------------------------------------------------------------------------------
REFLECT_STRUCT_BEGIN( ToolManifest, Struct, MetaNone() )
    REFLECT(        m_ToolId,                       "ToolId",                       MetaNone() )
    REFLECT(        m_TimeStamp,                    "TimeStamp",                    MetaNone() )
    REFLECT(        m_MainExecutableRootPath,       "MainExecutableRootPath",       MetaNone() )
    REFLECT_ARRAY_OF_STRUCT( m_Files,               "Files",    ToolManifestFile,   MetaNone() )
    REFLECT_ARRAY(  m_CustomEnvironmentVariables,   "CustomEnvironmentVariables",   MetaNone() )
REFLECT_END( ToolManifest )

REFLECT_STRUCT_BEGIN( ToolManifestFile, Struct, MetaNone() )
    REFLECT( m_Name,        "Name",         MetaNone() )
    REFLECT( m_TimeStamp,   "TimeStamp",    MetaNone() )
    REFLECT( m_Hash,        "Hash",         MetaNone() )
    REFLECT( m_UncompressedContentSize, "UncompressedContentSize",  MetaNone() )
    REFLECT( m_CompressedContentSize, "CompressedContentSize",  MetaNone() )
REFLECT_END( ToolManifestFile )

// CONSTRUCTOR (ToolManifestFile)
//------------------------------------------------------------------------------
ToolManifestFile::ToolManifestFile() = default;

// CONSTRUCTOR (ToolManifestFile)
//------------------------------------------------------------------------------
ToolManifestFile::ToolManifestFile( const AString & name, uint64_t stamp, uint32_t hash, uint32_t size )
    : m_Name( name )
    , m_TimeStamp( stamp )
    , m_Hash( hash )
    , m_UncompressedContentSize( size )
{}

// DESTRUCTOR (ToolManifestFile)
//------------------------------------------------------------------------------
ToolManifestFile::~ToolManifestFile()
{
    FREE( m_CompressedContent );
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

// StoreCompressedContent (File)
//------------------------------------------------------------------------------
void ToolManifestFile::StoreCompressedContent( const void * uncompressedData, const uint32_t uncompressedDataSize ) const
{
    m_UncompressedContentSize = uncompressedDataSize;
    Compressor c;
    c.Compress( uncompressedData, m_UncompressedContentSize );
    m_CompressedContentSize = (uint32_t)c.GetResultSize();
    m_CompressedContent = c.ReleaseResult();
}

// Generate
//------------------------------------------------------------------------------
bool ToolManifest::Generate( const AString& mainExecutableRoot, const Dependencies & dependencies, const Array<AString> & customEnvironmentVariables )
{
    m_MainExecutableRootPath = mainExecutableRoot;
    m_CustomEnvironmentVariables = customEnvironmentVariables;
    m_Files.Clear();
    m_TimeStamp = 0;
    m_Files.SetCapacity( 1 + dependencies.GetSize() );

    // unify "main executable" and "extra files"
    // (loads contents of file into memory, and creates hashes)
    for ( size_t i=0; i<dependencies.GetSize(); ++i )
    {
        const FileNode & n = *( dependencies[ i ].GetNode()->CastTo< FileNode >() );
        if ( !AddFile( n.GetName(), n.GetStamp() ) )
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
        const ToolManifestFile & f = m_Files[ i ];

        // file contents
        *pos = f.m_Hash;
        ++pos;

        // file name & sub-path (relative to remote folder)
        AStackString<> relativePath;
        GetRelativePath( m_MainExecutableRootPath, f.m_Name, relativePath );
        *pos = xxHash::Calc32( relativePath );
        ++pos;
    }
    m_ToolId = xxHash::Calc64( mem, memSize );
    FREE( mem );

    // update time stamp (most recent file in manifest)
    for ( size_t i=0; i<numFiles; ++i )
    {
        const ToolManifestFile & f = m_Files[ i ];
        ASSERT( f.m_TimeStamp ); // should have had an error before if the file was missing
        m_TimeStamp = Math::Max( m_TimeStamp, f.m_TimeStamp );
    }

    return true;
}

// SerializeForRemote
//------------------------------------------------------------------------------
void ToolManifest::SerializeForRemote( IOStream & ms ) const
{
    ms.Write( m_ToolId );
    ms.Write( m_MainExecutableRootPath );

    const uint32_t numItems( (uint32_t)m_Files.GetSize() );
    ms.Write( numItems );
    const size_t numFiles( m_Files.GetSize() );
    for ( size_t i=0; i<numFiles; ++i )
    {
        const ToolManifestFile & f = m_Files[ i ];
        ms.Write( f.m_Name );
        ms.Write( f.m_TimeStamp );
        ms.Write( f.m_Hash );
        ms.Write( f.m_UncompressedContentSize );
    }

    const size_t numEnvVars( m_CustomEnvironmentVariables.GetSize() );
    ms.Write( (uint32_t)numEnvVars );
    for ( size_t i = 0; i < numEnvVars; ++i )
    {
        ms.Write( m_CustomEnvironmentVariables[ i ] );
    }
}

// DeserializeFromRemote
//------------------------------------------------------------------------------
void ToolManifest::DeserializeFromRemote( IOStream & ms )
{
    ms.Read( m_ToolId );
    ms.Read( m_MainExecutableRootPath );

    ASSERT( m_Files.IsEmpty() );

    uint32_t numFiles( 0 );
    ms.Read( numFiles );
    m_Files.SetCapacity( numFiles );

    for ( size_t i=0; i<(size_t)numFiles; ++i )
    {
        AStackString<> name;
        uint64_t timeStamp( 0 );
        uint32_t hash( 0 );
        uint32_t uncompressedContentSize( 0 );
        ms.Read( name );
        ms.Read( timeStamp );
        ms.Read( hash );
        ms.Read( uncompressedContentSize );
        m_Files.Append( ToolManifestFile( name, timeStamp, hash, uncompressedContentSize ) );
    }

    ASSERT( m_CustomEnvironmentVariables.IsEmpty() );

    uint32_t numEnvVars( 0 );
    ms.Read( numEnvVars );
    m_CustomEnvironmentVariables.SetCapacity( numEnvVars );
    for ( size_t i = 0; i < (size_t)numEnvVars; ++i )
    {
        AStackString<> envVar;
        ms.Read( envVar );
        m_CustomEnvironmentVariables.Append( envVar );
    }

    // determine if any files are remaining from a previous run
    size_t numFilesAlreadySynchronized = 0;
    for ( size_t i=0; i<(size_t)numFiles; ++i )
    {
        AStackString<> localFile;
        GetRemoteFilePath( (uint32_t)i, localFile );

        // is this file already present?
        AutoPtr< FileStream, DeleteDeletor > fileStream( FNEW( FileStream ) );
        FileStream & f = *( fileStream.Get() );
        if ( f.Open( localFile.Get() ) == false )
        {
            continue; // file not found
        }
        if ( f.GetFileSize() != m_Files[ i ].m_UncompressedContentSize )
        {
            continue; // file is not complete
        }
        AutoPtr< char > mem( (char *)ALLOC( (size_t)f.GetFileSize() ) );
        if ( f.Read( mem.Get(), (size_t)f.GetFileSize() ) != f.GetFileSize() )
        {
            continue; // problem reading file
        }
        if( xxHash::Calc32( mem.Get(), (size_t)f.GetFileSize() ) != m_Files[ i ].m_Hash )
        {
            continue; // file contents unexpected
        }

        // file present and ok
        m_Files[ i ].m_FileLock = fileStream.Release(); // NOTE: keep file open to prevent deletions
        m_Files[ i ].m_SyncState = ToolManifestFile::SYNCHRONIZED;
        numFilesAlreadySynchronized++;
    }

    // Generate Environment
    ASSERT( m_RemoteEnvironmentString == nullptr );

    // PATH=
    AStackString<> basePath;
    GetRemotePath( basePath );
    AStackString<> paths;
    paths.Format( "PATH=%s", basePath.Get() );

    #if defined( __WINDOWS__ )
        // TMP=
        AStackString<> normalTmp;
        Env::GetEnvVariable( "TMP", normalTmp );
        AStackString<> tmp;
        tmp.Format( "TMP=%s", normalTmp.Get() );

        // SystemRoot=
        AStackString<> sysRoot( "SystemRoot=C:\\Windows" );
    #endif

    // Calculate the length of the full environment string
    size_t len( paths.GetLength() + 1 );
    #if defined( __WINDOWS__ )
        len += ( tmp.GetLength() + 1 );
        len += ( sysRoot.GetLength() + 1 );
    #endif

    for ( size_t i = 0; i < numEnvVars; ++i )
    {
        const AString & envVar = m_CustomEnvironmentVariables[i];
        if ( envVar.Find( "%1" ) )
        {
            len += envVar.GetLength() - 2 + basePath.GetLength() + 1;   // If there is a %1 it will be removed and replaced by the basePath. +1 for the null terminator.
        }
        else
        {
            len += envVar.GetLength() + 1;
        }
    }

    len += 1; // for double null


    // Now that the environment string length is calculated, allocate and fill.
    char * mem = (char *)ALLOC( len );
    m_RemoteEnvironmentString = mem;

    AString::Copy( paths.Get(), mem, paths.GetLength() + 1 ); // including null
    mem += ( paths.GetLength() + 1 ); // including null

    #if defined( __WINDOWS__ )
        AString::Copy( tmp.Get(), mem, tmp.GetLength() + 1 ); // including null
        mem += ( tmp.GetLength() + 1 ); // including null

        AString::Copy( sysRoot.Get(), mem, sysRoot.GetLength() + 1 ); // including null
        mem += ( sysRoot.GetLength() + 1 ); // including null
    #endif

    for ( size_t i = 0; i < numEnvVars; ++i )
    {
        const AString & envVar = m_CustomEnvironmentVariables[i];
        const char * token = envVar.Find( "%1" );
        if ( token )
        {
            AString::Copy( envVar.Get(), mem, ( token - envVar.Get() ) );   // Copy the data up to the token
            mem += ( token - envVar.Get() );
            AString::Copy( basePath.Get(), mem, basePath.GetLength() );     // Append the basePath instead of the token
            mem += basePath.GetLength();
            AString::Copy( token + 2, mem, envVar.GetLength() - 2 - ( token - envVar.Get() ) + 1 ); // Append the trailing portion of the string.
            mem += ( envVar.GetLength() - 2 - ( token - envVar.Get() ) + 1 );
        }
        else
        {
            AString::Copy( envVar.Get(), mem, envVar.GetLength() + 1 );
            mem += ( envVar.GetLength() + 1 );
        }
    }

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
    const ToolManifestFile * const end = m_Files.End();
    for ( const ToolManifestFile * it = m_Files.Begin(); it != end; ++it )
    {
        syncTotal += it->m_UncompressedContentSize;
        if ( it->m_SyncState == ToolManifestFile::SYNCHRONIZED )
        {
            syncDone += it->m_UncompressedContentSize;
        }
        else if ( it->m_SyncState == ToolManifestFile::SYNCHRONIZING )
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
    ToolManifestFile * const end = m_Files.End();
    for ( ToolManifestFile * it = m_Files.Begin(); it != end; ++it )
    {
        if ( it->m_SyncState == ToolManifestFile::SYNCHRONIZING )
        {
            it->m_SyncState = ToolManifestFile::NOT_SYNCHRONIZED;
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
    const ToolManifestFile & f = m_Files[ fileId ];
    if ( f.m_CompressedContent == nullptr )
    {
        void * uncompressedContent( nullptr );
        uint32_t uncompressedContentSize( 0 );
        if ( !LoadFile( f.m_Name, uncompressedContent, uncompressedContentSize ) )
        {
            return nullptr;
        }
        // store compressed file content (take ownership of data)
        f.StoreCompressedContent( uncompressedContent, uncompressedContentSize );
        // free unused uncompressed data
        FREE( uncompressedContent );
    }
    dataSize = f.m_CompressedContentSize;
    return f.m_CompressedContent;
}

// ReceiveFileData
//------------------------------------------------------------------------------
bool ToolManifest::ReceiveFileData( uint32_t fileId, const void * data, size_t & dataSize )
{
    MutexHolder mh( m_Mutex );

    ToolManifestFile & f = m_Files[ fileId ];

    // gracefully handle multiple receipts of the same data
    if ( f.m_CompressedContent )
    {
        ASSERT( f.m_SyncState == ToolManifestFile::SYNCHRONIZED );
        return true;
    }

    ASSERT( f.m_SyncState == ToolManifestFile::SYNCHRONIZING );

    // do decompression
    Compressor c;
    if ( c.IsValidData( data, dataSize ) == false )
    {
        FLOG_WARN( "Invalid data received for fileId %u", fileId );
        return false;
    }
    c.Decompress( data );
    const void * uncompressedData = c.GetResult();
    const size_t uncompressedDataSize = c.GetResultSize();

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
    if ( fs.Write( uncompressedData, uncompressedDataSize ) != uncompressedDataSize )
    {
        return false; // FAILED
    }
    fs.Close();

    // mark executable
    #if defined( __LINUX__ ) || defined( __OSX__ )
        FileIO::SetExecutable( fileName.Get() );
    #endif

    // open read-only
    AutoPtr< FileStream, DeleteDeletor > fileStream( FNEW( FileStream ) );
    if ( fileStream.Get()->Open( fileName.Get(), FileStream::READ_ONLY ) == false )
    {
        return false; // FAILED
    }

    // This file is now synchronized
    f.m_FileLock = fileStream.Release(); // NOTE: Keep file open to prevent deletion
    f.m_SyncState = ToolManifestFile::SYNCHRONIZED;

    // is completely synchronized?
    const ToolManifestFile * const end = m_Files.End();
    for ( const ToolManifestFile * it = m_Files.Begin(); it != end; ++it )
    {
        if ( it->m_SyncState != ToolManifestFile::SYNCHRONIZED )
        {
            // still some files to be received
            return true; // file stored ok
        }
    }

    // all files received
    m_Synchronized = true;
    return true; // file stored ok
}

// GetRelativePath
//------------------------------------------------------------------------------
/*static*/ void ToolManifest::GetRelativePath( const AString & root, const AString & otherFile, AString & otherFileRelativePath )
{
    if ( otherFile.BeginsWithI( root ) )
    {
        // file is in sub dir on master machine, so store with same relative location
        otherFileRelativePath = ( otherFile.Get() + root.GetLength() );
    }
    else
    {
        // file is in some completely other directory, so put in same place as exe
        const char * lastSlash = otherFile.FindLast( NATIVE_SLASH );
        otherFileRelativePath = ( lastSlash ? lastSlash + 1 : otherFile.Get() );
    }
}

// GetRemoteFilePath
//------------------------------------------------------------------------------
void ToolManifest::GetRemoteFilePath( uint32_t fileId, AString & remotePath ) const
{
    // Get base directory
    GetRemotePath( remotePath );
    ASSERT( remotePath.EndsWith( NATIVE_SLASH ) );

    // Get relative path for file and append
    AStackString<> relativePath;
    GetRelativePath( m_MainExecutableRootPath, m_Files[ fileId ].m_Name, relativePath );
    remotePath += relativePath;
}

// GetRemotePath
//------------------------------------------------------------------------------
void ToolManifest::GetRemotePath( AString & path ) const
{
    VERIFY( FBuild::GetTempDir( path ) );
    AStackString<> subDir;
    #if defined( __WINDOWS__ )
        subDir.Format( ".fbuild.tmp\\worker\\toolchain.%016" PRIx64 "\\", m_ToolId );
    #else
        subDir.Format( "_fbuild.tmp/worker/toolchain.%016" PRIx64 "/", m_ToolId );
    #endif
    path += subDir;
}

// AddFile
//------------------------------------------------------------------------------
bool ToolManifest::AddFile( const AString & fileName, const uint64_t timeStamp )
{
    uint32_t uncompressedContentSize( 0 );
    void * uncompressedContent( nullptr );
    if ( !LoadFile( fileName, uncompressedContent, uncompressedContentSize ) )
    {
        return false; // LoadContent will have emitted an error
    }

    // create the file entry
    const uint32_t hash = xxHash::Calc32( uncompressedContent, uncompressedContentSize );
    m_Files.Append( ToolManifestFile( fileName, timeStamp, hash, uncompressedContentSize ) );

    ToolManifestFile & f = m_Files.Top();
    // store compressed file content (take ownership of data)
    f.StoreCompressedContent( uncompressedContent, uncompressedContentSize );
    // free unused uncompressed data
    FREE( uncompressedContent );

    return true;
}

// LoadFile
//------------------------------------------------------------------------------
bool ToolManifest::LoadFile( const AString & fileName, void * & uncompressedContent, uint32_t & uncompressedContentSize ) const
{
    // read the file into memory
    FileStream fs;
    if ( fs.Open( fileName.Get(), FileStream::READ_ONLY ) == false )
    {
        FLOG_ERROR( "Error: opening file '%s' in Compiler ToolManifest\n", fileName.Get() );
        return false;
    }
    uncompressedContentSize = (uint32_t)fs.GetFileSize();
    AutoPtr< void > mem( ALLOC( uncompressedContentSize ) );
    if ( fs.Read( mem.Get(), uncompressedContentSize ) != uncompressedContentSize )
    {
        FLOG_ERROR( "Error: reading file '%s' in Compiler ToolManifest\n", fileName.Get() );
        return false;
    }

    uncompressedContent = mem.Release();

    return true;
}

//------------------------------------------------------------------------------
