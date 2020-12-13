// ToolManifest
//------------------------------------------------------------------------------

// Includes
//------------------------------------------------------------------------------
#include "ToolManifest.h"

// Core
#include "Core/Containers/UniquePtr.h"
#include "Core/Env/Env.h"
#include "Core/FileIO/ConstMemoryStream.h"
#include "Core/FileIO/FileIO.h"
#include "Core/FileIO/FileStream.h"
#include "Core/FileIO/MemoryStream.h"
#include "Core/FileIO/PathUtils.h"
#include "Core/Math/xxHash.h"
#include "Core/Profile/Profile.h"
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
    REFLECT(        m_ToolId,                       "ToolId",                       MetaHidden() )
    REFLECT(        m_TimeStamp,                    "TimeStamp",                    MetaHidden() )
    REFLECT(        m_MainExecutableRootPath,       "MainExecutableRootPath",       MetaHidden() )
    REFLECT_ARRAY_OF_STRUCT( m_Files,               "Files",    ToolManifestFile,   MetaHidden() )
    REFLECT_ARRAY(  m_CustomEnvironmentVariables,   "CustomEnvironmentVariables",   MetaHidden() )
REFLECT_END( ToolManifest )

REFLECT_STRUCT_BEGIN( ToolManifestFile, Struct, MetaNone() )
    REFLECT( m_Name,        "Name",         MetaHidden() )
    REFLECT( m_TimeStamp,   "TimeStamp",    MetaHidden() )
    REFLECT( m_Hash,        "Hash",         MetaHidden() )
    REFLECT( m_UncompressedContentSize, "UncompressedContentSize",  MetaHidden() )
    REFLECT( m_CompressedContentSize, "CompressedContentSize",  MetaHidden() )
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

// StoreCompressedContent (ToolManifestFile)
//------------------------------------------------------------------------------
void ToolManifestFile::StoreCompressedContent( const void * uncompressedData, const uint32_t uncompressedDataSize ) const
{
    ASSERT( m_CompressedContent == nullptr );
    m_UncompressedContentSize = uncompressedDataSize;
    Compressor c;
    c.Compress( uncompressedData, m_UncompressedContentSize );
    m_CompressedContentSize = (uint32_t)c.GetResultSize();
    m_CompressedContent = c.ReleaseResult();
}

// DoBuild
//------------------------------------------------------------------------------
bool ToolManifestFile::DoBuild()
{
    // Name should be set
    ASSERT( m_Name.IsEmpty() == false );

    // Should not have any file data in memory
    ASSERT( m_CompressedContent == nullptr );
    ASSERT( m_CompressedContentSize == 0 );

    // Do we already have a hash?
    if ( m_Hash != 0 )
    {
        // Can we trust the hash? (timestamp has not changed)
        if ( m_TimeStamp == FileIO::GetFileLastWriteTime( m_Name ) )
        {
            // Nothing to do
            return true;
        }
    }

    // Load the file content
    void * uncompressedContent;
    uint32_t uncompressedContentSize;
    if ( LoadFile( uncompressedContent, uncompressedContentSize ) == false )
    {
        return false; // LoadFile emits an error
    }

    // Take note of the uncompressed size
    m_UncompressedContentSize = uncompressedContentSize;

    // Store the hash and timestamp
    m_Hash = xxHash::Calc32( uncompressedContent, uncompressedContentSize ); // TODO:C Switch to 64 bit hash
    m_TimeStamp = FileIO::GetFileLastWriteTime( m_Name );

    // Compress and keep the data if it might be useful
    if ( FBuild::Get().GetOptions().m_AllowDistributed )
    {
        StoreCompressedContent( uncompressedContent, uncompressedContentSize );
    }

    FREE( uncompressedContent );

    return true;
}

// Migrate
//------------------------------------------------------------------------------
void ToolManifestFile::Migrate( const ToolManifestFile & oldFile )
{
    ASSERT( m_Name == oldFile.m_Name );
    m_TimeStamp = oldFile.m_TimeStamp;
    m_Hash = oldFile.m_Hash;
}

// Generate
//------------------------------------------------------------------------------
void ToolManifest::Initialize( const AString & mainExecutableRoot, const Dependencies & dependencies, const Array<AString> & customEnvironmentVariables )
{
    m_MainExecutableRootPath = mainExecutableRoot;
    m_CustomEnvironmentVariables = customEnvironmentVariables;

    // Pre-reserve the list of files, but loading/hashing until later
    ASSERT( m_Files.IsEmpty() );
    m_Files.SetCapacity( dependencies.GetSize() );
    for ( const Dependency & dep : dependencies )
    {
        m_Files.EmplaceBack( dep.GetNode()->GetName(), (uint64_t)0, (uint32_t)0, (uint32_t)0 );
    }
}

// Generate
//------------------------------------------------------------------------------
bool ToolManifest::DoBuild( const Dependencies & dependencies )
{
    ASSERT( m_Files.GetSize() == dependencies.GetSize() );
    (void)dependencies;

    m_TimeStamp = 0;

    // Get timestamps and hashes
    for ( ToolManifestFile & file : m_Files )
    {
        if ( !file.DoBuild() )
        {
            return false; // DoBuild will have emitted an rrror
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
        *pos = f.GetHash();
        ++pos;

        // file name & sub-path (relative to remote folder)
        AStackString<> relativePath;
        GetRelativePath( m_MainExecutableRootPath, f.GetName(), relativePath );
        *pos = xxHash::Calc32( relativePath );
        ++pos;
    }
    m_ToolId = xxHash::Calc64( mem, memSize );
    FREE( mem );

    // update time stamp (most recent file in manifest)
    for ( size_t i=0; i<numFiles; ++i )
    {
        const ToolManifestFile & f = m_Files[ i ];
        ASSERT( f.GetTimeStamp() ); // should have had an error before if the file was missing
        m_TimeStamp = Math::Max( m_TimeStamp, f.GetTimeStamp() );
    }

    return true;
}

// Migrate
//------------------------------------------------------------------------------
void ToolManifest::Migrate( const ToolManifest & oldManifest )
{
    const size_t numFiles = m_Files.GetSize();
    const Array< ToolManifestFile > & oldFiles = oldManifest.GetFiles();
    ASSERT( numFiles == oldFiles.GetSize() );
    for ( size_t i = 0; i < numFiles; ++i )
    {
        m_Files[ i ].Migrate( oldFiles[ i ] );
    }

    m_TimeStamp = oldManifest.m_TimeStamp;
    m_ToolId = oldManifest.m_ToolId;
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
        ms.Write( f.GetName() );
        ms.Write( f.GetTimeStamp() );
        ms.Write( f.GetHash() );
        ms.Write( f.GetUncompressedContentSize() );
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
        m_Files.EmplaceBack( name, timeStamp, hash, uncompressedContentSize );
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

        // Set modification time to now
        //  - On OSX (and possibly some Linux variants) this will prevent
        //    periodic deletion of files.
        //  - On Windows we lock files to prevent deletion, but setting the
        //    writable time for some additional usage visibility is nice
        // After this, we do it periodically in TouchFiles
        FileIO::SetFileLastWriteTimeToNow( localFile );

        // is this file already present?
        UniquePtr< FileStream, DeleteDeletor > fileStream( FNEW( FileStream ) );
        FileStream & f = *( fileStream.Get() );
        if ( f.Open( localFile.Get() ) == false )
        {
            continue; // file not found
        }
        if ( f.GetFileSize() != m_Files[ i ].GetUncompressedContentSize() )
        {
            continue; // file is not complete
        }
        UniquePtr< char > mem( (char *)ALLOC( (size_t)f.GetFileSize() ) );
        if ( f.Read( mem.Get(), (size_t)f.GetFileSize() ) != f.GetFileSize() )
        {
            continue; // problem reading file
        }
        if ( xxHash::Calc32( mem.Get(), (size_t)f.GetFileSize() ) != m_Files[ i ].GetHash() )
        {
            continue; // file contents unexpected
        }

        // file present and ok
        m_Files[ i ].SetFileLock( fileStream.Release() ); // NOTE: keep file open to prevent deletions
        m_Files[ i ].SetSyncState( ToolManifestFile::SYNCHRONIZED );
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
            AString::Copy( envVar.Get(), mem, (size_t)( token - envVar.Get() ) );   // Copy the data up to the token
            mem += ( token - envVar.Get() );
            AString::Copy( basePath.Get(), mem, basePath.GetLength() );     // Append the basePath instead of the token
            mem += basePath.GetLength();
            AString::Copy( token + 2, mem, (size_t)( envVar.GetLength() - 2 - ( token - envVar.Get() ) + 1 ) ); // Append the trailing portion of the string.
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
        syncTotal += it->GetUncompressedContentSize();
        if ( it->GetSyncState() == ToolManifestFile::SYNCHRONIZED )
        {
            syncDone += it->GetUncompressedContentSize();
        }
        else if ( it->GetSyncState() == ToolManifestFile::SYNCHRONIZING )
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
    const ToolManifestFile * const end = m_Files.End();
    for ( ToolManifestFile * it = m_Files.Begin(); it != end; ++it )
    {
        if ( it->GetSyncState() == ToolManifestFile::SYNCHRONIZING )
        {
            it->SetSyncState( ToolManifestFile::NOT_SYNCHRONIZED );
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
    return m_Files[ fileId ].GetFileData( dataSize );
}

// GetFileData (ToolManifestFile)
//------------------------------------------------------------------------------
const void * ToolManifestFile::GetFileData( size_t & outDataSize ) const
{
    // Should only be possible to access data if we know it's up-to-date
    ASSERT( m_TimeStamp );
    ASSERT( m_Hash );

    // Do we have data already available?
    if ( m_CompressedContent == nullptr )
    {
        // Load the file content
        void * uncompressedContent;
        uint32_t uncompressedContentSize;
        if ( LoadFile( uncompressedContent, uncompressedContentSize ) == false )
        {
            return nullptr; // LoadFile emits an error
        }

        // We should have previously recorded the uncompressed size
        ASSERT( uncompressedContentSize == m_UncompressedContentSize );

        // Store the compressed version
        StoreCompressedContent( uncompressedContent, uncompressedContentSize );
        FREE( uncompressedContent );
    }
    outDataSize = m_CompressedContentSize;
    return m_CompressedContent;
}

// ReceiveFileData
//------------------------------------------------------------------------------
bool ToolManifest::ReceiveFileData( uint32_t fileId, const void * data, size_t & dataSize )
{
    MutexHolder mh( m_Mutex );

    ToolManifestFile & f = m_Files[ fileId ];

    // gracefully handle multiple receipts of the same data
    if ( f.GetSyncState() == ToolManifestFile::SYNCHRONIZED )
    {
        return true;
    }

    ASSERT( f.GetSyncState() == ToolManifestFile::SYNCHRONIZING );

    // do decompression
    Compressor c;
    if ( c.IsValidData( data, dataSize ) == false )
    {
        FLOG_WARN( "Invalid data received for fileId %u", fileId );
        return false;
    }
    VERIFY( c.Decompress( data ) );
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
    UniquePtr< FileStream, DeleteDeletor > fileStream( FNEW( FileStream ) );
    if ( fileStream.Get()->Open( fileName.Get(), FileStream::READ_ONLY ) == false )
    {
        return false; // FAILED
    }

    // This file is now synchronized
    f.SetFileLock( fileStream.Release() ); // NOTE: Keep file open to prevent deletion
    f.SetSyncState( ToolManifestFile::SYNCHRONIZED );

    // is completely synchronized?
    const ToolManifestFile * const end = m_Files.End();
    for ( const ToolManifestFile * it = m_Files.Begin(); it != end; ++it )
    {
        if ( it->GetSyncState() != ToolManifestFile::SYNCHRONIZED )
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
        // file is in sub dir on fbuild client machine, so store with same relative location
        otherFileRelativePath = ( otherFile.Get() + root.GetLength() );
    }
    else
    {
        // file is in some completely other directory, so put in same place as exe
        const char * lastSlash = otherFile.FindLast( NATIVE_SLASH );
        otherFileRelativePath = ( lastSlash ? lastSlash + 1 : otherFile.Get() );
    }
}

// TouchFiles
//------------------------------------------------------------------------------
#if defined( __OSX__ ) || defined( __LINUX__ )
    void ToolManifest::TouchFiles() const
    {
        const size_t numFiles = m_Files.GetSize();
        for ( size_t fileId = 0; fileId < numFiles; ++fileId )
        {
            // Get path to file
            AStackString<> fileName;
            GetRemoteFilePath( fileId, fileName );
        
            // Make modification time now
            FileIO::SetFileLastWriteTimeToNow( fileName );
        }
    }
#endif

// GetRemoteFilePath
//------------------------------------------------------------------------------
void ToolManifest::GetRemoteFilePath( uint32_t fileId, AString & remotePath ) const
{
    // Get base directory
    GetRemotePath( remotePath );
    ASSERT( remotePath.EndsWith( NATIVE_SLASH ) );

    // Get relative path for file and append
    AStackString<> relativePath;
    GetRelativePath( m_MainExecutableRootPath, m_Files[ fileId ].GetName(), relativePath );
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

// LoadFile (ToolManifestFile)
//------------------------------------------------------------------------------
bool ToolManifestFile::LoadFile( void * & uncompressedContent, uint32_t & uncompressedContentSize ) const
{
    // read the file into memory
    FileStream fs;
    if ( fs.Open( m_Name.Get(), FileStream::READ_ONLY ) == false )
    {
        FLOG_ERROR( "Error: opening file '%s' in Compiler ToolManifest\n", m_Name.Get() );
        return false;
    }
    uncompressedContentSize = (uint32_t)fs.GetFileSize();
    UniquePtr< void > mem( ALLOC( uncompressedContentSize ) );
    if ( fs.Read( mem.Get(), uncompressedContentSize ) != uncompressedContentSize )
    {
        FLOG_ERROR( "Error: reading file '%s' in Compiler ToolManifest\n", m_Name.Get() );
        return false;
    }

    uncompressedContent = mem.Release();

    return true;
}

//------------------------------------------------------------------------------
