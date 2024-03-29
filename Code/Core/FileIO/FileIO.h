// FileIO.h
//------------------------------------------------------------------------------
#pragma once

// Includes
//------------------------------------------------------------------------------
#include "Core/Containers/Array.h"
#include "Core/FileIO/FileStream.h"
#include "Core/Strings/AString.h"

// Defines
//------------------------------------------------------------------------------
#define MAX_PATH 260

// Forward Declarations
//------------------------------------------------------------------------------
class GetFilesHelper;

// FileIO
//------------------------------------------------------------------------------
class FileIO
{
public:
    static bool FileExists( const char * fileName );
    static bool FileDelete( const char * fileName );
    static bool FileCopy( const char * srcFileName, const char * dstFileName, bool allowOverwrite = true );
    static bool FileMove( const AString & srcFileName, const AString & dstFileName );
    static bool DirectoryDelete( const AString & path );

    // directory listing
    static bool GetFiles( const AString & path,
                          const AString & wildCard,
                          bool recurse,
                          Array< AString > * results );
    static void GetFiles( const AString & path,
                          GetFilesHelper & helper );
    struct FileInfo
    {
        AString     m_Name;
        uint32_t    m_Attributes;
        uint64_t    m_LastWriteTime;
        uint64_t    m_Size;

        bool        IsReadOnly() const;
    };
    static bool GetFilesEx( const AString & path,
                            const Array< AString > * patterns,
                            bool recurse,
                            Array< FileInfo > * results );
    static bool GetFileInfo( const AString & fileName, FileInfo & info );

    static bool GetCurrentDir( AString & output );
    static bool SetCurrentDir( const AString & dir );
    static bool GetTempDir( AString & output );
    static bool DirectoryCreate( const AString & path );
    static bool DirectoryExists( const AString & path );
    static bool EnsurePathExists( const AString & path );
    static bool EnsurePathExistsForFile( const AString & name );
    #if defined( __WINDOWS__ )
        [[nodiscard]] static bool NormalizeWindowsPathCasing( const AString & path, AString & outNormalizedPath );
        [[nodiscard]] static bool IsValidDriveLetter( char c );
    #endif

    #if !defined( __WINDOWS__ )
        static bool GetDirectoryIsMountPoint( const AString & path );
    #endif

    static uint64_t GetFileLastWriteTime( const AString & fileName );
    static bool     SetFileLastWriteTime( const AString & fileName, uint64_t fileTime );
    static bool     SetFileLastWriteTimeToNow( const AString & fileName );

    static bool     SetReadOnly( const char * fileName, bool readOnly );
    static bool     GetReadOnly( const char * fileName );
    static inline bool GetReadOnly( const AString & fileName ) { return GetReadOnly( fileName.Get() ); }
    #if defined( __LINUX__ ) || defined( __APPLE__ )
        static bool SetExecutable( const char * fileName );
    #endif

    #if defined( __WINDOWS__ )
        static void     WorkAroundForWindowsFilePermissionProblem( const AString & fileName,
                                                                   const uint32_t openMode = FileStream::READ_ONLY,
                                                                   const uint32_t timeoutSeconds = 1 );
    #else
        FORCE_INLINE static void WorkAroundForWindowsFilePermissionProblem( const AString &, const uint32_t = 0, const uint32_t = 0 ) {}
    #endif

    #if defined( __WINDOWS__ )
        static bool IsWindowsLongPathSupportEnabled();
    #endif

private:
    #if defined( __WINDOWS__ )
        static bool IsWindowsLongPathSupportEnabledInternal();
    #endif

    static void GetFilesRecurse( AString & path,
                                 GetFilesHelper & helper );
    static void GetFilesRecurse( AString & path,
                                 const AString & wildCard,
                                 Array< AString > * results );
    static void GetFilesNoRecurse( const char * path,
                                   const char * wildCard,
                                   Array< AString > * results );
    static void GetFilesRecurseEx( AString & path,
                                   const Array< AString > * patterns,
                                   Array< FileInfo > * results );
    static void GetFilesNoRecurseEx( const char * path,
                                     const Array< AString > * patterns,
                                     Array< FileInfo > * results );

    friend class GetFilesHelper;
    static bool IsMatch( const Array< AString > * patterns, const char * fileName );
};

//------------------------------------------------------------------------------
class GetFilesHelper
{
public:
    explicit GetFilesHelper( size_t sizeHint = 1024 );
    explicit GetFilesHelper( const Array<AString> & patterns, size_t sizeHint = 1024 );
    virtual ~GetFilesHelper();

    // Default implementation can be extended or customized
    [[nodiscard]] virtual bool  OnDirectory( const AString & dirPath );
    [[nodiscard]] virtual bool  ShouldIncludeFile( const char * fileName );
    virtual void                OnFile( FileIO::FileInfo && fileInfo );

    // Access results
    const Array<FileIO::FileInfo> & GetFiles() const { return m_Files; }
    Array<FileIO::FileInfo> &       GetFiles() { return m_Files; }

protected:
    bool m_Recurse = true;
    const Array<AString> * m_Patterns = nullptr;
    Array<FileIO::FileInfo> m_Files;
};

//------------------------------------------------------------------------------
