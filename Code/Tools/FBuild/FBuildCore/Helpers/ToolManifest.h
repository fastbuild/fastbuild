// ToolManifest
//------------------------------------------------------------------------------
#pragma once

// Forward Declarations
//------------------------------------------------------------------------------
class Dependencies;
class FileStream;
class IOStream;
class Node;

// Includes
//------------------------------------------------------------------------------
#include "Core/Containers/Array.h"
#include "Core/Env/Types.h"
#include "Core/Process/Mutex.h"
#include "Core/Reflection/ReflectionMacros.h"
#include "Core/Reflection/Struct.h"
#include "Core/Strings/AString.h"


// ToolManifestFile
//------------------------------------------------------------------------------
class ToolManifestFile : public Struct
{
    REFLECT_STRUCT_DECLARE( ToolManifestFile )
public:
    ToolManifestFile();
    explicit ToolManifestFile( const AString & name, uint64_t stamp, uint32_t hash, uint32_t size );
    ~ToolManifestFile();

    enum SyncState
    {
        NOT_SYNCHRONIZED,
        SYNCHRONIZING,
        SYNCHRONIZED,
    };

    bool                DoBuild();
    void                StoreCompressedContent( const void * uncompressedData, const uint32_t uncompressedDataSize ) const;
    void                Migrate( const ToolManifestFile & oldFile );

    const void *        GetFileData( size_t & outDataSize ) const;

    // Access state
    const AString &     GetName() const                     { return m_Name; }
    uint64_t            GetTimeStamp() const                { return m_TimeStamp; }
    uint32_t            GetHash() const                     { return m_Hash; }
    uint32_t            GetUncompressedContentSize() const  { return m_UncompressedContentSize; }
    SyncState           GetSyncState() const                { return m_SyncState; }

    // Modify state
    void                SetSyncState( SyncState state )         { m_SyncState = state; }
    void                SetFileLock( FileStream * fileLock )    { m_FileLock = fileLock; }

protected:
    bool                LoadFile( void * & uncompressedContent, uint32_t & uncompressedContentSize ) const;

    // common members
    AString          m_Name;
    uint64_t         m_TimeStamp     = 0;
    uint32_t         m_Hash          = 0;
    mutable uint32_t m_UncompressedContentSize = 0;
    mutable uint32_t m_CompressedContentSize = 0;

    // "local" members
    mutable void *   m_CompressedContent = nullptr;

    // "remote" members
    SyncState       m_SyncState     = NOT_SYNCHRONIZED;
    FileStream *    m_FileLock      = nullptr; // keep the file locked when sync'd
};

// ToolManifest
//------------------------------------------------------------------------------
class ToolManifest : public Struct
{
    REFLECT_STRUCT_DECLARE( ToolManifest )
public:
    explicit ToolManifest();
    explicit ToolManifest( uint64_t toolId );
    ~ToolManifest();

    void Initialize( const AString & mainExecutableRoot, const Dependencies & dependencies, const Array<AString> & customEnvironmentVariables );
    bool DoBuild( const Dependencies & dependencies );
    void Migrate( const ToolManifest & oldManifest );

    inline uint64_t GetToolId() const { return m_ToolId; }
    inline uint64_t GetTimeStamp() const { return m_TimeStamp; }

    void SerializeForRemote( IOStream & ms ) const;
    void DeserializeFromRemote( IOStream & ms );

    inline bool IsSynchronized() const { return m_Synchronized; }
    bool GetSynchronizationStatus( uint32_t & syncDone, uint32_t & syncTotal ) const;

    // operator for FindDeref
    inline bool operator == ( uint64_t toolId ) const
    {
        return ( m_ToolId == toolId );
    }

    inline void     SetUserData( void * data )  { m_UserData = data; }
    inline void *   GetUserData() const         { return m_UserData; }
    const Array< ToolManifestFile > & GetFiles() const { return m_Files; }

    void MarkFileAsSynchronizing( size_t fileId ) { ASSERT( m_Files[ fileId ].GetSyncState() == ToolManifestFile::NOT_SYNCHRONIZED ); m_Files[ fileId ].SetSyncState( ToolManifestFile::SYNCHRONIZING ); }
    void CancelSynchronizingFiles();

    const void *    GetFileData( uint32_t fileId, size_t & dataSize ) const;
    bool            ReceiveFileData( uint32_t fileId, const void * data, size_t & dataSize );

    void            GetRemotePath( AString & path ) const;
    void            GetRemoteFilePath( uint32_t fileId, AString & exe ) const;
    const char *    GetRemoteEnvironmentString() const { return m_RemoteEnvironmentString; }

    static void     GetRelativePath( const AString & root, const AString & otherFile, AString & otherFileRelativePath );
    
    #if defined( __OSX__ ) || defined( __LINUX__ )
        void            TouchFiles() const;
    #endif

private:
    mutable Mutex   m_Mutex;

    // Reflected
    uint64_t                    m_ToolId;       // Global identifier for this toolchain
    uint64_t                    m_TimeStamp;    // Time stamp of most recent file
    AString                     m_MainExecutableRootPath;
    Array< ToolManifestFile >   m_Files;
    Array< AString >            m_CustomEnvironmentVariables;

    // Internal state
    bool            m_Synchronized;
    const char *    m_RemoteEnvironmentString;
    void *          m_UserData;
};

//------------------------------------------------------------------------------
