// FileIO.cpp
//------------------------------------------------------------------------------

// Includes
//------------------------------------------------------------------------------
#include "FileIO.h"
#include "FileStream.h"

// Core
#include "Core/Env/ErrorFormat.h"
#include "Core/FileIO/PathUtils.h"
#include "Core/Process/Thread.h"
#include "Core/Profile/Profile.h"
#include "Core/Strings/AStackString.h"
#include "Core/Time/Timer.h"

// system
#if defined( __WINDOWS__ )
    #include "Core/Env/WindowsHeader.h"
    #include "Core/Time/Time.h"
    #include <Aclapi.h>
#endif
#if defined( __LINUX__ ) || defined( __APPLE__ )
    #include <dirent.h>
    #include <errno.h>
    #include <libgen.h>
    #include <limits.h>
    #include <stdio.h>
    #include <sys/stat.h>
    #include <unistd.h>
#endif
#if defined( __LINUX__ )
    #include <fcntl.h>
    #include <sys/sendfile.h>
#endif
#if defined( __APPLE__ )
    #include <copyfile.h>
    #include <dlfcn.h>
    #include <sys/time.h>
#endif

// OSXHelper_utimensat
//------------------------------------------------------------------------------
#if defined( __APPLE__ )
    // OS X 10.13 (High Sierra) adds support for utimensat
    // OS X 10.13 (High Sierra) adds the Apple File System (APFS) which supports nsec time resolution
    // OS X 10.14 (Mojave) uses APFS for all drives/devices
    //
    // We want to set higher resolution timestamps where possible, but we want to
    // retain compatibility with older versions of OS X. To do this, we get the
    // utimensat symbol dynamically from the C runtime.
    //
    class OSXHelper_utimensat
    {
    public:
        typedef int (*FuncPtr)(int dirfd, const char *pathname, const struct timespec times[2], int flags);
        
        OSXHelper_utimensat()
        {
            // Open the c runtime library
            m_LibCHandle = dlopen( "libc.dylib", RTLD_LAZY );
            ASSERT( m_LibCHandle ); // This should never fail
        
            // See if utimensat exists
            m_FuncPtr = (FuncPtr)dlsym( m_LibCHandle, "utimensat" );
        }
        ~OSXHelper_utimensat()
        {
            VERIFY( dlclose( m_LibCHandle ) == 0 );
        }
        void *          m_LibCHandle    = nullptr;
        FuncPtr         m_FuncPtr       = nullptr;
    } gOSXHelper_utimensat;
#endif

// Exists
//------------------------------------------------------------------------------
/*static*/ bool FileIO::FileExists( const char * fileName )
{
    PROFILE_FUNCTION
#if defined( __WINDOWS__ )
    // see if we can get attributes
    DWORD attributes = GetFileAttributes( fileName );
    if ( attributes == INVALID_FILE_ATTRIBUTES )
    {
        return false;
    }
    return true; // note this might not be file!
#elif defined( __LINUX__ ) || defined( __APPLE__ )
    struct stat st;
    if ( lstat( fileName, &st ) == 0 )
    {
        if ( ( st.st_mode & S_IFDIR ) != S_IFDIR )
        {
            return true; // exists and is NOT a folder
        }
    }
    return false;
#else
    #error Unknown platform
#endif
}

// Delete directory
//------------------------------------------------------------------------------
/*static*/ bool FileIO::DirectoryDelete( const AString & path )
{
#if defined( __WINDOWS__ )
    BOOL result = RemoveDirectory( path.Get() );
    if ( result == FALSE )
    {
        return false; // failed to delete
    }
    return true; // delete ok
#elif defined( __LINUX__ ) || defined( __APPLE__ )
    int result = rmdir( path.Get() );
    if ( result != 0 )
    {
        return false; // failed to delete
    }
    return true; // delete ok
#else
    #error Unknown platform
#endif
}

// Delete
//------------------------------------------------------------------------------
/*static*/ bool FileIO::FileDelete( const char * fileName )
{
    PROFILE_FUNCTION
#if defined( __WINDOWS__ )
    BOOL result = DeleteFile( fileName );
    if ( result == FALSE )
    {
        return false; // failed to delete
    }
    return true; // delete ok
#elif defined( __LINUX__ ) || defined( __APPLE__ )
    if ( GetReadOnly( fileName ) )
    {
        return false;
    }
    return ( remove( fileName ) == 0 );
#else
    #error Unknown platform
#endif
}

// Copy
//------------------------------------------------------------------------------
/*static*/ bool FileIO::FileCopy( const char * srcFileName, const char * dstFileName,
                              bool allowOverwrite )
{
#if defined( __WINDOWS__ )
    DWORD flags = COPY_FILE_COPY_SYMLINK;
    flags = ( allowOverwrite ? flags : flags | COPY_FILE_FAIL_IF_EXISTS );

    BOOL result = CopyFileEx( srcFileName, dstFileName, nullptr, nullptr, nullptr, flags );
    if ( result == FALSE )
    {
        // even if we allow overwrites, Windows will fail if the dest file
        // was read only, so we have to un-mark the read only status and try again
        if ( ( GetLastError() == ERROR_ACCESS_DENIED ) && ( allowOverwrite ) )
        {
            // see if dst file is read-only
            DWORD dwAttrs = GetFileAttributes( dstFileName );
            if ( dwAttrs == INVALID_FILE_ATTRIBUTES )
            {
                return false; // can't even get the attributes, nothing more we can do
            }
            if ( 0 == ( dwAttrs & FILE_ATTRIBUTE_READONLY ) )
            {
                return false; // file is not read only, so we don't know what the problem is
            }

            // try to remove read-only flag on dst file
            dwAttrs = ( dwAttrs & ~FILE_ATTRIBUTE_READONLY );
            if ( FALSE == SetFileAttributes( dstFileName, dwAttrs ) )
            {
                return false; // failed to remove read-only flag
            }

            // try to copy again
            result = CopyFileEx( srcFileName, dstFileName, nullptr, nullptr, nullptr, flags );
            return ( result == TRUE );
        }
    }

    return ( result == TRUE );
#elif defined( __APPLE__ )
    if ( allowOverwrite == false )
    {
        if ( FileExists( dstFileName ) )
        {
            return false;
        }
    }
    // If the state parameter is the return value from copyfile_state_alloc(),
    // then copyfile() and fcopyfile() will use the information from the state
    // object; if it is NULL, then both functions will work normally, but less
    // control will be available to the caller.
    bool result = ( copyfile( srcFileName, dstFileName, nullptr, COPYFILE_DATA | COPYFILE_XATTR | COPYFILE_NOFOLLOW ) == 0 );
    return result;
#elif defined( __LINUX__ )
    if ( allowOverwrite == false )
    {
        if ( FileExists( dstFileName ) )
        {
            return false;
        }
    }

    struct stat stat_source;
    VERIFY( lstat( srcFileName, &stat_source ) == 0 );

    // Special case symlinks.
    if ( S_ISLNK( stat_source.st_mode ) )
    {
        AString linkPath( stat_source.st_size + 1 );
        ssize_t length = readlink( srcFileName, linkPath.Get(), linkPath.GetReserved() );
        if ( length != stat_source.st_size )
        {
            return false;
        }
        linkPath.SetLength( length );
        return symlink( linkPath.Get(), dstFileName ) == 0;
    }

    int source = open( srcFileName, O_RDONLY, 0 );
    if ( source < 0 )
    {
        return false;
    }

    // Ensure dest file will be writable if it exists
    FileIO::SetReadOnly( dstFileName, false );

    int dest = open( dstFileName, O_WRONLY | O_CREAT | O_TRUNC, 0644 );
    if ( dest < 0 )
    {
        close( source );
        return false;
    }

    // set permissions to match the source file's
    // we can't do this during open because in some filesystems (e.g. CIFS) that can fail
    if ( fchmod( dest, stat_source.st_mode ) < 0 )
    {
        close( source );
        close( dest );
        return false;
    }

    ssize_t bytesCopied = sendfile( dest, source, 0, stat_source.st_size );

    close(source);
    close(dest);

    return ( bytesCopied == stat_source.st_size );
#else
    #error Unknown platform
#endif
}

// FileMove
//------------------------------------------------------------------------------
/*static*/ bool FileIO::FileMove( const AString & srcFileName, const AString & dstFileName )
{
#if defined( __WINDOWS__ )
    return ( TRUE == ::MoveFileEx( srcFileName.Get(), dstFileName.Get(), MOVEFILE_REPLACE_EXISTING ) );
#elif defined( __LINUX__ ) || defined( __APPLE__ )
    return ( rename( srcFileName.Get(), dstFileName.Get() ) == 0 );
#else
    #error Unknown platform
#endif
}

// GetFiles
//------------------------------------------------------------------------------
/*static*/ bool FileIO::GetFiles( const AString & path,
                                  const AString & wildCard,
                                  const bool recurse,
                                  const bool includeDirs,
                                  Array< AString > * results )
{
    ASSERT( results );

    size_t oldSize = results->GetSize();
    if ( recurse )
    {
        // make a copy of the path as it will be modified during recursion
        AStackString< 256 > pathCopy( path );
        PathUtils::EnsureTrailingSlash( pathCopy );
        GetFilesRecurse( pathCopy, wildCard, includeDirs, results );
    }
    else
    {
        GetFilesNoRecurse( path.Get(), wildCard.Get(), includeDirs, results );
    }

    return ( results->GetSize() != oldSize );
}

// GetFilesEx
//------------------------------------------------------------------------------
/*static*/ bool FileIO::GetFilesEx( const AString & path,
                                  const Array< AString > * patterns,
                                  bool recurse,
                                  Array< FileInfo > * results )
{
    ASSERT( results );

    size_t oldSize = results->GetSize();
    if ( recurse )
    {
        // make a copy of the path as it will be modified during recursion
        AStackString< 256 > pathCopy( path );
        PathUtils::EnsureTrailingSlash( pathCopy );
        GetFilesRecurseEx( pathCopy, patterns, results );
    }
    else
    {
        GetFilesNoRecurseEx( path.Get(), patterns, results );
    }

    return ( results->GetSize() != oldSize );
}

// GetFileInfo
//------------------------------------------------------------------------------
/*static*/ bool FileIO::GetFileInfo( const AString & fileName, FileIO::FileInfo & info )
{
    #if defined( __WINDOWS__ )
        WIN32_FILE_ATTRIBUTE_DATA fileAttribs;
        if ( GetFileAttributesEx( fileName.Get(), GetFileExInfoStandard, &fileAttribs ) )
        {
            info.m_Name = fileName;
            info.m_Attributes = fileAttribs.dwFileAttributes;
            info.m_LastWriteTime = (uint64_t)fileAttribs.ftLastWriteTime.dwLowDateTime | ( (uint64_t)fileAttribs.ftLastWriteTime.dwHighDateTime << 32 );
            info.m_Size = (uint64_t)fileAttribs.nFileSizeLow | ( (uint64_t)fileAttribs.nFileSizeHigh << 32 );
            return true;
        }
    #elif defined( __APPLE__ ) || defined( __LINUX__ )
        struct stat s;
        if ( lstat( fileName.Get(), &s ) == 0 )
        {
            info.m_Name = fileName;
            info.m_Attributes = s.st_mode;
            #if defined( __APPLE__ )
                info.m_LastWriteTime = ( ( (uint64_t)s.st_mtimespec.tv_sec * 1000000000ULL ) + (uint64_t)s.st_mtimespec.tv_nsec );
            #else
                info.m_LastWriteTime = ( ( (uint64_t)s.st_mtim.tv_sec * 1000000000ULL ) + (uint64_t)s.st_mtim.tv_nsec );
            #endif
            info.m_Size = s.st_size;
            return true;
        }
    #endif
    return false;
}

// GetCurrentDir
//------------------------------------------------------------------------------
/*static*/ bool FileIO::GetCurrentDir( AString & output )
{
    #if defined( __WINDOWS__ )
        char buffer[ MAX_PATH ];
        DWORD len = GetCurrentDirectory( MAX_PATH, buffer );
        if ( len != 0 )
        {
            output = buffer;
            return true;
        }
    #elif defined( __LINUX__ ) || defined( __APPLE__ )
        const size_t bufferSize( PATH_MAX );
        char buffer[ bufferSize ];
        if ( getcwd( buffer, bufferSize ) )
        {
            output = buffer;
            return true;
        }
    #else
        #error Unknown platform
    #endif
    return false;
}

// SetCurrentDir
//------------------------------------------------------------------------------
/*static*/ bool FileIO::SetCurrentDir( const AString & dir )
{
    #if defined( __WINDOWS__ )
        // Windows can have upper or lower case letters in the path for the drive
        // letter.  The case may be important for the user, but setting the current
        // dir with only a change in case is ignored.
        // To ensure we have the requested case, we have to change dir to another
        // location, and then the location we want.

        // get another valid location to set as the dir
        // (we'll use the windows directory)
        char otherFolder[ 512 ];
        otherFolder[ 0 ] = 0;
        UINT len = ::GetWindowsDirectory( otherFolder, 512 );
        if ( ( len == 0 ) || ( len > 511 ) )
        {
            return false;
        }

        // handle the case where the user actually wants the windows dir
        if ( _stricmp( otherFolder, dir.Get() ) == 0 )
        {
            // use the root of the drive containing the windows dir
            otherFolder[ 3 ] = 0;
        }

        // set "other" dir
        if ( ::SetCurrentDirectory( otherFolder ) == FALSE )
        {
            return false;
        }

        // set the actual directory we want
        if ( ::SetCurrentDirectory( dir.Get() ) == TRUE )
        {
            return true;
        }
    #elif defined( __LINUX__ ) || defined( __APPLE__ )
        if ( chdir( dir.Get() ) == 0 )
        {
            return true;
        }
    #else
        #error Unknown platform
    #endif
    return false;
}

// GetTempDir
//------------------------------------------------------------------------------
/*static*/ bool FileIO::GetTempDir( AString & output )
{
    #if defined( __WINDOWS__ )
        char buffer[ MAX_PATH ];
        DWORD len = GetTempPath( MAX_PATH, buffer );
        if ( len != 0 )
        {
            output = buffer;

            // Ensure slash terminated
            const bool slashTerminated = ( output.EndsWith( '/' ) || output.EndsWith( '\\' ) );
            if ( !slashTerminated )
            {
                output += '\\';
            }

            return true;
        }
    #elif defined( __LINUX__ ) || defined( __APPLE__ )
        output = "/tmp/";
        return true;
    #else
        #error Unknown platform
    #endif
    return false;
}

// DirectoryCreate
//------------------------------------------------------------------------------
/*static*/ bool FileIO::DirectoryCreate( const AString & path )
{
    #if defined( __WINDOWS__ )
        if ( CreateDirectory( path.Get(), nullptr ) )
        {
            return true;
        }

        // it failed - is it because it exists already?
        if ( GetLastError() == ERROR_ALREADY_EXISTS )
        {
            return true;
        }
    #elif defined( __LINUX__ ) || defined( __APPLE__ )
        umask( 0 ); // disable default creation mask // TODO:LINUX TODO:MAC Changes global program state; needs investigation
        mode_t mode = S_IRWXU | S_IRWXG | S_IRWXO; // TODO:LINUX TODO:MAC Check these permissions
        if ( mkdir( path.Get(), mode ) == 0 )
        {
            return true; // created ok
        }

        // failed to create - already exists?
        if ( errno == EEXIST )
        {
            return true;
        }
    #else
        #error Unknown platform
    #endif

    // failed, probably missing intermediate folders or an invalid name
    return false;
}

// DirectoryExists
//------------------------------------------------------------------------------
/*static*/ bool FileIO::DirectoryExists( const AString & path )
{
    #if defined( __WINDOWS__ )
        DWORD res = GetFileAttributes( path.Get() );
        if ( ( res != INVALID_FILE_ATTRIBUTES ) &&
            ( ( res & FILE_ATTRIBUTE_DIRECTORY ) != 0 ) )
        {
            return true; // exists and is a folder
        }
    #elif defined( __LINUX__ ) || defined( __APPLE__ )
        struct stat st;
        if ( lstat( path.Get(), &st ) == 0 )
        {
            if ( ( st.st_mode & S_IFDIR ) != 0 )
            {
                return true; // exists and is folder
            }
        }
    #else
        #error Unknown platform
    #endif
    return false; // doesn't exist, isn't a folder or some other problem
}

//------------------------------------------------------------------------------
/*static*/ bool FileIO::EnsurePathExists( const AString & path )
{
    // if the entire path already exists, nothing is to be done
    if( DirectoryExists( path ) )
    {
        return true;
    }

    // take a copy to locally manipulate
    AStackString<> pathCopy( path );
    PathUtils::FixupFolderPath( pathCopy ); // ensure correct slash type and termination

    // handle UNC paths by skipping leading slashes and machine name
    char * slash = pathCopy.Get();
    #if defined( __WINDOWS__ )
        if ( *slash == NATIVE_SLASH )
        {
            while ( *slash == NATIVE_SLASH ) { ++slash; } // skip leading double slash
            while ( *slash != NATIVE_SLASH ) { ++slash; } // skip machine name
            ++slash; // move into first dir name, so next search will find dir name slash
        }
    #endif

    #if defined( __LINUX__ ) || defined( __APPLE__ )
        // for full paths, ignore the first slash
        if ( *slash == NATIVE_SLASH )
        {
            ++slash;
        }
    #endif

    slash = pathCopy.Find( NATIVE_SLASH, slash );
    if ( !slash )
    {
        return false;
    }
    do
    {
        // truncate the string to the sub path
        *slash = '\000';
        if ( DirectoryExists( pathCopy ) == false )
        {
            // create this level
            if ( DirectoryCreate( pathCopy ) == false )
            {
                return false; // something went wrong
            }
        }
        *slash = NATIVE_SLASH; // put back the slash
        slash = pathCopy.Find( NATIVE_SLASH, slash + 1 );
    }
    while ( slash );
    return true;
}

// EnsurePathExistsForFile
//------------------------------------------------------------------------------
/*static*/ bool FileIO::EnsurePathExistsForFile( const AString & name )
{
    const char * lastSlashA = name.FindLast( NATIVE_SLASH );
    const char * lastSlashB = name.FindLast( OTHER_SLASH );
    const char * lastSlash = lastSlashA > lastSlashB ? lastSlashA : lastSlashB;
    ASSERT( lastSlash ); // Caller must pass something valid
    AStackString<> pathOnly( name.Get(), lastSlash );
    return EnsurePathExists( pathOnly );
}

// GetDirectoryIsMountPoint
//------------------------------------------------------------------------------
#if !defined( __WINDOWS__ )
    /*static*/ bool FileIO::GetDirectoryIsMountPoint( const AString & path )
    {
        // stat the path
        struct stat pathStat;
        if ( stat( path.Get(), &pathStat ) != 0 )
        {
            return false; // Can't stat the path  (probably doesn't exist)
        }

        // Is it a dir?
        if ( ( pathStat.st_mode & S_IFDIR ) == 0 )
        {
            return false; // Not a directory, so can't be a mount point
        }

        // stat parent dir
        AStackString<> pathCopy( path ); // dirname modifies string, so we need a copy
        const char * parentName = dirname( pathCopy.Get() );
        struct stat parentStat;
        if ( stat( parentName, &parentStat ) != 0 )
        {
            return false; // Can't stat parent dir, then something is wrong
        }

        // Compare device ids
        if ( pathStat.st_dev != parentStat.st_dev )
        {
            return true; // On a different device, so must be a mount point
        }

        // If path and parent are the same, it's a root node (and therefore also a mount point)
        if ( ( pathStat.st_dev == parentStat.st_dev ) &&
             ( pathStat.st_ino == parentStat.st_ino ) )
        {
             return true;
        }

        return false; // Not a mount point
    }
#endif

// GetFileLastWriteTime
//------------------------------------------------------------------------------
/*static*/ uint64_t FileIO::GetFileLastWriteTime( const AString & fileName )
{
    #if defined( __WINDOWS__ )
        WIN32_FILE_ATTRIBUTE_DATA fileAttribs;
        if ( GetFileAttributesEx( fileName.Get(), GetFileExInfoStandard, &fileAttribs ) )
        {
            FILETIME ftWrite = fileAttribs.ftLastWriteTime;
            uint64_t lastWriteTime = (uint64_t)ftWrite.dwLowDateTime | ( (uint64_t)ftWrite.dwHighDateTime << 32 );
            return lastWriteTime;
        }
    #elif defined( __APPLE__ )
        struct stat st;
        if ( lstat( fileName.Get(), &st ) == 0 )
        {
            return ( ( (uint64_t)st.st_mtimespec.tv_sec * 1000000000ULL ) + (uint64_t)st.st_mtimespec.tv_nsec );
        }
    #elif defined( __LINUX__ )
        struct stat st;
        if ( lstat( fileName.Get(), &st ) == 0 )
        {
            return ( ( (uint64_t)st.st_mtim.tv_sec * 1000000000ULL ) + (uint64_t)st.st_mtim.tv_nsec );
        }
    #else
        #error Unknown platform
    #endif
    return 0;
}

// SetFileLastWriteTime
//------------------------------------------------------------------------------
/*static*/ bool FileIO::SetFileLastWriteTime( const AString & fileName, uint64_t fileTime )
{
    #if defined( __WINDOWS__ )
        // open the file
        // TOOD:B Check these args
        HANDLE hFile = CreateFile( fileName.Get(), GENERIC_WRITE, FILE_SHARE_WRITE, nullptr,
                                   OPEN_EXISTING, 0, nullptr);
        if( hFile == INVALID_HANDLE_VALUE )
        {
            return false;
        }

        // get the file time
        FILETIME ftWrite;
        ftWrite.dwLowDateTime = (uint32_t)( fileTime & 0x00000000FFFFFFFF );
        ftWrite.dwHighDateTime = (uint32_t)( ( fileTime & 0xFFFFFFFF00000000 ) >> 32 );
        if ( !SetFileTime( hFile, nullptr, nullptr, &ftWrite) ) // create, access, write
        {
            CloseHandle( hFile );
            return false;
        }

        // close the file
        CloseHandle( hFile );

        return true;
    #elif defined( __APPLE__ )
        // Use higher precision function if available
        if ( gOSXHelper_utimensat.m_FuncPtr )
        {
            struct timespec t[ 2 ];
            t[0].tv_sec = fileTime / 1000000000ULL;
            t[0].tv_nsec = ( fileTime % 1000000000ULL );
            t[1] = t[0];
            return ( (gOSXHelper_utimensat.m_FuncPtr)( 0, fileName.Get(), t, 0 ) == 0 );
        }
    
        // Fallback to regular low-resolution filetime setting
        struct timeval t[ 2 ];
        t[0].tv_sec = fileTime / 1000000000ULL;
        t[0].tv_usec = ( fileTime % 1000000000ULL ) / 1000;
        t[1] = t[0];
        return ( utimes( fileName.Get(), t ) == 0 );
    #elif defined( __LINUX__ )
        struct timespec t[ 2 ];
        t[0].tv_sec = fileTime / 1000000000ULL;
        t[0].tv_nsec = ( fileTime % 1000000000ULL );
        t[1] = t[0];
        return ( utimensat( 0, fileName.Get(), t, 0 ) == 0 );
    #else
        #error Unknown platform
    #endif
}

// SetFileLastWriteTimeToNow
//------------------------------------------------------------------------------
/*static*/ bool FileIO::SetFileLastWriteTimeToNow( const AString & fileName )
{
    #if defined( __WINDOWS__ )
        const uint64_t fileTimeNow = Time::GetCurrentFileTime();
        return SetFileLastWriteTime( fileName, fileTimeNow );
    #elif defined( __APPLE__ )
        // Use higher precision function if available
        if ( gOSXHelper_utimensat.m_FuncPtr )
        {
            return ( (gOSXHelper_utimensat.m_FuncPtr)( 0, fileName.Get(), nullptr, 0 ) == 0 );
        }
    
        // Fallback to regular low-resolution filetime setting
        return ( utimes( fileName.Get(), nullptr ) == 0 );
    #elif defined( __LINUX__ )
        return ( utimensat( 0, fileName.Get(), nullptr, 0 ) == 0 );
    #else
        #error Unknown platform
    #endif
}

// SetReadOnly
//------------------------------------------------------------------------------
/*static*/ bool FileIO::SetReadOnly( const char * fileName, bool readOnly )
{
    #if defined( __WINDOWS__ )
        // see if dst file is read-only
        DWORD dwAttrs = GetFileAttributes( fileName );
        if ( dwAttrs == INVALID_FILE_ATTRIBUTES )
        {
            return false; // can't even get the attributes, nothing more we can do
        }

        // determine the new attributes
        DWORD dwNewAttrs = ( readOnly ) ? ( dwAttrs | FILE_ATTRIBUTE_READONLY )
                                        : ( dwAttrs & ~FILE_ATTRIBUTE_READONLY );

        // nothing to do if they are the same
        if ( dwNewAttrs == dwAttrs )
        {
            return true;
        }

        // try to set change
        if ( FALSE == SetFileAttributes( fileName, dwNewAttrs ) )
        {
            return false; // failed
        }

        return true;
    #elif defined( __LINUX__ ) || defined( __APPLE__ )
        struct stat s;
        if ( lstat( fileName, &s ) != 0 )
        {
            return true; // can't even get the attributes, treat as not read only
        }
        const bool currentlyReadOnly = !( ( s.st_mode & S_IWUSR ) == S_IWUSR ); // TODO:LINUX Is this the correct behaviour?
        if ( readOnly == currentlyReadOnly )
        {
            return true; // already in desired state
        }

        // update writable flag
        if ( readOnly )
        {
            // remove writable flag
            s.st_mode &= ( ~S_IWUSR ); // TODO:LINUX Is this the correct behaviour?
        }
        else
        {
            // add writable flag
            s.st_mode |= S_IWUSR; // TODO:LINUX Is this the correct behaviour?
        }

        if ( chmod( fileName, s.st_mode ) == 0 )
        {
            return true;
        }
        return false; // failed to chmod
    #else
        #error Unknown platform
    #endif
}

// GetReadOnly
//------------------------------------------------------------------------------
/*static*/ bool FileIO::GetReadOnly( const char * fileName )
{
    #if defined( __WINDOWS__ )
        // see if dst file is read-only
        DWORD dwAttrs = GetFileAttributes( fileName );
        if ( dwAttrs == INVALID_FILE_ATTRIBUTES )
        {
            return false; // can't even get the attributes, treat as not read only
        }

        // determine the new attributes
        bool readOnly = ( dwAttrs & FILE_ATTRIBUTE_READONLY );
        return readOnly;
    #elif defined( __LINUX__ ) || defined( __APPLE__ )
        struct stat s;
        if ( lstat( fileName, &s ) != 0 )
        {
            return false; // can't even get the attributes, treat as not read only
        }
        return ( ( s.st_mode & S_IWUSR ) == 0 );// TODO:LINUX Is this the correct behaviour?
    #else
        #error Unknown platform
    #endif
}

// SetExecutable
//------------------------------------------------------------------------------
#if defined( __LINUX__ ) || defined( __APPLE__ )
    /*static*/ bool FileIO::SetExecutable( const char * fileName )
    {
        // rwxr-x--x (751) TODO:LINUX TODO:MAC Is this correct?
        mode_t mode = S_IRUSR | S_IWUSR | S_IXUSR |
                      S_IRGRP |           S_IXGRP |
                                          S_IXOTH;
        if ( chmod( fileName, mode ) == 0 )
        {
            return true;
        }
        return false; // failed to chmod
    }
#endif

#if defined( __WINDOWS__ )
// IsShortcutDir
//------------------------------------------------------------------------------
/*static*/ bool FileIO::IsShortcutDir(
    const void * findData )
{
    ASSERT( findData );
    WIN32_FIND_DATA * pFindData = (WIN32_FIND_DATA*)findData;
    // shortcut dirs are . and ..
    return ( pFindData->cFileName[ 0 ] == '.' &&
         ( ( pFindData->cFileName[ 1 ] == '.' ) || ( pFindData->cFileName[ 1 ] == '\000' ) ) );
}
#elif defined( __LINUX__ ) || defined( __APPLE__ )
// IsShortcutDir
//------------------------------------------------------------------------------
/*static*/ bool FileIO::IsShortcutDir( const void * entry )
{
    ASSERT( entry );
    dirent * pEntry = (dirent*)entry;
    // shortcut dirs are . and ..
    bool isShortcutDir = false;
    if ( pEntry->d_name[ 0 ] == '.' )
    {
        if ( ( pEntry->d_name[ 1 ] == 0 ) ||
             ( ( pEntry->d_name[ 1 ] == '.' ) && ( pEntry->d_name[ 2 ] == 0 ) ) )
        {
            isShortcutDir = true;
        }
    }
    return isShortcutDir;
}
#endif

#if defined( __WINDOWS__ )
// IncludeFileObjectInResults
//------------------------------------------------------------------------------
/*static*/ bool FileIO::IncludeFileObjectInResults(
    const void * findData,
    const bool includeDirs )
{
    ASSERT( findData );
    WIN32_FIND_DATA * pFindData = (WIN32_FIND_DATA*)findData;
    bool includeFileObject = true;  // first assume true
    if ( pFindData->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY )
    {
        if ( includeDirs )
        {
            // ignore . and ..
            if ( IsShortcutDir( findData ) )
            {
                includeFileObject = false;
            }
        }
        else
        {
            includeFileObject = false;
        }
    }
    return includeFileObject;
}
#endif

// GetFilesRecurse
//------------------------------------------------------------------------------
/*static*/ void FileIO::GetFilesRecurse( AString & pathCopy,
                                         const AString & wildCard,
                                         const bool includeDirs,
                                         Array< AString > * results )
{
    const uint32_t baseLength = pathCopy.GetLength();

    #if defined( __WINDOWS__ )
        pathCopy += '*'; // don't want to use wildcard to filter folders

        // recurse into directories
        WIN32_FIND_DATA findData;
        HANDLE hFind = FindFirstFileEx( pathCopy.Get(), FindExInfoBasic, &findData, FindExSearchLimitToDirectories, nullptr, 0 );
        if ( hFind == INVALID_HANDLE_VALUE)
        {
            return;
        }

        do
        {
            if ( findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY )
            {
                // ignore magic '.' and '..' folders
                // (don't need to check length of name, as all names are at least 1 char
                // which means index 0 and 1 are valid to access)
                if ( IsShortcutDir( &findData ) )
                {
                    continue;
                }

                pathCopy.SetLength( baseLength );
                pathCopy += findData.cFileName;
                pathCopy += NATIVE_SLASH;
                GetFilesRecurse( pathCopy, wildCard, includeDirs, results );
            }
        }
        while ( FindNextFile( hFind, &findData ) != 0 );
        FindClose( hFind );

        // do files in this directory
        pathCopy.SetLength( baseLength );
        pathCopy += '*';
        hFind = FindFirstFileEx( pathCopy.Get(), FindExInfoBasic, &findData, FindExSearchNameMatch, nullptr, 0 );
        if ( hFind == INVALID_HANDLE_VALUE)
        {
            return;
        }

        do
        {
            if ( !IncludeFileObjectInResults( &findData, includeDirs ) )
            {
                continue;
            }

            if ( PathUtils::IsWildcardMatch( wildCard.Get(), findData.cFileName ) )
            {
                pathCopy.SetLength( baseLength );
                pathCopy += findData.cFileName;
                results->Append( pathCopy );
            }
        }
        while ( FindNextFile( hFind, &findData ) != 0 );

        FindClose( hFind );

    #elif defined( __LINUX__ ) || defined( __APPLE__ )
        // Special case symlinks.
        struct stat stat_source;
        if ( lstat( pathCopy.Get(), &stat_source ) != 0 )
        {
            return;
        }
        if ( S_ISLNK( stat_source.st_mode ) )
        {
            return;
        }

        DIR * dir = opendir( pathCopy.Get() );
        if ( dir == nullptr )
        {
            return;
        }
        for ( ;; )
        {
            dirent * entry = readdir( dir );
            if ( entry == nullptr )
            {
                break; // no more entries
            }

            bool isDir = ( entry->d_type == DT_DIR );

            // Not all filesystems have support for returning the file type in
            // d_type and applications must properly handle a return of DT_UNKNOWN.
            if ( entry->d_type == DT_UNKNOWN )
            {
                pathCopy.SetLength( baseLength );
                pathCopy += entry->d_name;

                struct stat info;
                VERIFY( lstat( pathCopy.Get(), &info ) == 0 );
                isDir = S_ISDIR( info.st_mode );
            }

            // dir?
            if ( isDir )
            {
                // ignore . and ..
                if ( IsShortcutDir( entry ) )
                {
                    continue;
                }

                // regular dir
                pathCopy.SetLength( baseLength );
                pathCopy += entry->d_name;
                pathCopy += NATIVE_SLASH;
                GetFilesRecurse( pathCopy, wildCard, includeDirs, results );

                if ( !includeDirs )
                {
                    continue;
                }
            }

            // file - does it match wildcard?
            if ( PathUtils::IsWildcardMatch( wildCard.Get(), entry->d_name ) )
            {
                pathCopy.SetLength( baseLength );
                pathCopy += entry->d_name;
                results->Append( pathCopy );
            }
        }
        closedir( dir );
    #else
        #error Unknown platform
    #endif
}

// GetFilesNoRecurse
//------------------------------------------------------------------------------
/*static*/ void FileIO::GetFilesNoRecurse( const char * path,
                                           const char * wildCard,
                                           const bool includeDirs,
                                           Array< AString > * results )
{
    AStackString< 256 > pathCopy( path );
    PathUtils::EnsureTrailingSlash( pathCopy );
    const uint32_t baseLength = pathCopy.GetLength();

    #if defined( __WINDOWS__ )
        pathCopy += '*';

        WIN32_FIND_DATA findData;
        HANDLE hFind = FindFirstFileEx( pathCopy.Get(), FindExInfoBasic, &findData, FindExSearchNameMatch, nullptr, 0 );
        if ( hFind == INVALID_HANDLE_VALUE)
        {
            return;
        }

        do
        {
            if ( !IncludeFileObjectInResults( &findData, includeDirs ) )
            {
                continue;
            }

            if ( PathUtils::IsWildcardMatch( wildCard, findData.cFileName ) )
            {
                pathCopy.SetLength( baseLength );
                pathCopy += findData.cFileName;
                results->Append( pathCopy );
            }
        }
        while ( FindNextFile( hFind, &findData ) != 0 );

        FindClose( hFind );

    #elif defined( __LINUX__ ) || defined( __APPLE__ )
        // Special case symlinks.
        struct stat stat_source;
        if ( lstat( pathCopy.Get(), &stat_source ) != 0 )
        {
            return;
        }
        if ( S_ISLNK( stat_source.st_mode ) )
        {
            return;
        }

        DIR * dir = opendir( pathCopy.Get() );
        if ( dir == nullptr )
        {
            return;
        }
        for ( ;; )
        {
            dirent * entry = readdir( dir );
            if ( entry == nullptr )
            {
                break; // no more entries
            }

            bool isDir = ( entry->d_type == DT_DIR );

            // Not all filesystems have support for returning the file type in
            // d_type and applications must properly handle a return of DT_UNKNOWN.
            if ( entry->d_type == DT_UNKNOWN )
            {
                pathCopy.SetLength( baseLength );
                pathCopy += entry->d_name;

                struct stat info;
                VERIFY( lstat( pathCopy.Get(), &info ) == 0 );
                isDir = S_ISDIR( info.st_mode );
            }

            // dir?
            if ( isDir && !includeDirs )
            {
                continue;
            }

            // file - does it match wildcard?
            if ( PathUtils::IsWildcardMatch( wildCard, entry->d_name ) )
            {
                pathCopy.SetLength( baseLength );
                pathCopy += entry->d_name;
                results->Append( pathCopy );
            }
        }
        closedir( dir );
    #else
        #error Unknown platform
    #endif
}

// GetFilesRecurse
//------------------------------------------------------------------------------
/*static*/ void FileIO::GetFilesRecurseEx( AString & pathCopy,
                                         const Array< AString > * patterns,
                                         Array< FileInfo > * results )
{
    const uint32_t baseLength = pathCopy.GetLength();

    #if defined( __WINDOWS__ )
        pathCopy += '*'; // don't want to use wildcard to filter folders

        // recurse into directories
        WIN32_FIND_DATA findData;
        HANDLE hFind = FindFirstFileEx( pathCopy.Get(), FindExInfoBasic, &findData, FindExSearchLimitToDirectories, nullptr, 0 );
        if ( hFind == INVALID_HANDLE_VALUE)
        {
            return;
        }

        do
        {
            if ( findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY )
            {
                // ignore magic '.' and '..' folders
                // (don't need to check length of name, as all names are at least 1 char
                // which means index 0 and 1 are valid to access)
                if ( findData.cFileName[ 0 ] == '.' &&
                    ( ( findData.cFileName[ 1 ] == '.' ) || ( findData.cFileName[ 1 ] == '\000' ) ) )
                {
                    continue;
                }

                pathCopy.SetLength( baseLength );
                pathCopy += findData.cFileName;
                pathCopy += NATIVE_SLASH;
                GetFilesRecurseEx( pathCopy, patterns, results );
            }
        }
        while ( FindNextFile( hFind, &findData ) != 0 );
        FindClose( hFind );

        // do files in this directory
        pathCopy.SetLength( baseLength );
        pathCopy += '*';
        hFind = FindFirstFileEx( pathCopy.Get(), FindExInfoBasic, &findData, FindExSearchNameMatch, nullptr, 0 );
        if ( hFind == INVALID_HANDLE_VALUE)
        {
            return;
        }

        do
        {
            if ( findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY )
            {
                continue;
            }

            if ( IsMatch( patterns, findData.cFileName ) )
            {
                pathCopy.SetLength( baseLength );
                pathCopy += findData.cFileName;
                if ( results->GetSize() == results->GetCapacity() )
                {
                    results->SetCapacity( results->GetSize() * 2 );
                }
                results->SetSize( results->GetSize() + 1 );
                FileInfo & newInfo = results->Top();
                newInfo.m_Name = pathCopy;
                newInfo.m_Attributes = findData.dwFileAttributes;
                newInfo.m_LastWriteTime = (uint64_t)findData.ftLastWriteTime.dwLowDateTime | ( (uint64_t)findData.ftLastWriteTime.dwHighDateTime << 32 );
                newInfo.m_Size = (uint64_t)findData.nFileSizeLow | ( (uint64_t)findData.nFileSizeHigh << 32 );
            }
        }
        while ( FindNextFile( hFind, &findData ) != 0 );

        FindClose( hFind );

    #elif defined( __LINUX__ ) || defined( __APPLE__ )
        // Special case symlinks.
        struct stat stat_source;
        if ( lstat( pathCopy.Get(), &stat_source ) != 0 )
        {
            return;
        }
        if ( S_ISLNK( stat_source.st_mode ) )
        {
            return;
        }

        DIR * dir = opendir( pathCopy.Get() );
        if ( dir == nullptr )
        {
            return;
        }
        for ( ;; )
        {
            dirent * entry = readdir( dir );
            if ( entry == nullptr )
            {
                break; // no more entries
            }

            bool isDir = ( entry->d_type == DT_DIR );

            // Not all filesystems have support for returning the file type in
            // d_type and applications must properly handle a return of DT_UNKNOWN.
            if ( entry->d_type == DT_UNKNOWN )
            {
                pathCopy.SetLength( baseLength );
                pathCopy += entry->d_name;

                struct stat info;
                VERIFY( lstat( pathCopy.Get(), &info ) == 0 );
                isDir = S_ISDIR( info.st_mode );
            }

            // dir?
            if ( isDir )
            {
                // ignore . and ..
                if ( entry->d_name[ 0 ] == '.' )
                {
                    if ( ( entry->d_name[ 1 ] == 0 ) ||
                         ( ( entry->d_name[ 1 ] == '.' ) && ( entry->d_name[ 2 ] == 0 ) ) )
                    {
                        continue;
                    }
                }

                // regular dir
                pathCopy.SetLength( baseLength );
                pathCopy += entry->d_name;
                pathCopy += NATIVE_SLASH;
                GetFilesRecurseEx( pathCopy, patterns, results );
                continue;
            }

            // file - does it match wildcard?
            if ( IsMatch( patterns, entry->d_name ) )
            {
                pathCopy.SetLength( baseLength );
                pathCopy += entry->d_name;

                if ( results->GetSize() == results->GetCapacity() )
                {
                    results->SetCapacity( results->GetSize() * 2 );
                }
                results->SetSize( results->GetSize() + 1 );
                FileInfo & newInfo = results->Top();
                newInfo.m_Name = pathCopy;

                // get additional info
                struct stat info;
                VERIFY( lstat( pathCopy.Get(), &info ) == 0 );
                newInfo.m_Attributes = info.st_mode;
                #if defined( __APPLE__ )
                    newInfo.m_LastWriteTime = ( ( (uint64_t)info.st_mtimespec.tv_sec * 1000000000ULL ) + (uint64_t)info.st_mtimespec.tv_nsec );
                #else
                    newInfo.m_LastWriteTime = ( ( (uint64_t)info.st_mtim.tv_sec * 1000000000ULL ) + (uint64_t)info.st_mtim.tv_nsec );
                #endif
                newInfo.m_Size = info.st_size;
            }
        }
        closedir( dir );
    #else
        #error Unknown platform
    #endif
}

// GetFilesNoRecurseEx
//------------------------------------------------------------------------------
/*static*/ void FileIO::GetFilesNoRecurseEx( const char * path,
                                             const Array< AString > * patterns,
                                           Array< FileInfo > * results )
{
    AStackString< 256 > pathCopy( path );
    PathUtils::EnsureTrailingSlash( pathCopy );
    const uint32_t baseLength = pathCopy.GetLength();

    #if defined( __WINDOWS__ )
        pathCopy += '*';

        WIN32_FIND_DATA findData;
        HANDLE hFind = FindFirstFileEx( pathCopy.Get(), FindExInfoBasic, &findData, FindExSearchNameMatch, nullptr, 0 );
        if ( hFind == INVALID_HANDLE_VALUE)
        {
            return;
        }

        do
        {
            if ( findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY )
            {
                continue;
            }

            if ( IsMatch( patterns, findData.cFileName ) )
            {
                pathCopy.SetLength( baseLength );
                pathCopy += findData.cFileName;

                if ( results->GetSize() == results->GetCapacity() )
                {
                    results->SetCapacity( results->GetSize() * 2 );
                }
                results->SetSize( results->GetSize() + 1 );
                FileInfo & newInfo = results->Top();
                newInfo.m_Name = pathCopy;
                newInfo.m_Attributes = findData.dwFileAttributes;
                newInfo.m_LastWriteTime = (uint64_t)findData.ftLastWriteTime.dwLowDateTime | ( (uint64_t)findData.ftLastWriteTime.dwHighDateTime << 32 );
                newInfo.m_Size = (uint64_t)findData.nFileSizeLow | ( (uint64_t)findData.nFileSizeHigh << 32 );
            }
        }
        while ( FindNextFile( hFind, &findData ) != 0 );

        FindClose( hFind );

    #elif defined( __LINUX__ ) || defined( __APPLE__ )
        // Special case symlinks.
        struct stat stat_source;
        if ( lstat( pathCopy.Get(), &stat_source ) != 0 )
        {
            return;
        }
        if ( S_ISLNK( stat_source.st_mode ) )
        {
            return;
        }

        DIR * dir = opendir( pathCopy.Get() );
        if ( dir == nullptr )
        {
            return;
        }
        for ( ;; )
        {
            dirent * entry = readdir( dir );
            if ( entry == nullptr )
            {
                break; // no more entries
            }

            bool isDir = ( entry->d_type == DT_DIR );

            // Not all filesystems have support for returning the file type in
            // d_type and applications must properly handle a return of DT_UNKNOWN.
            if ( entry->d_type == DT_UNKNOWN )
            {
                pathCopy.SetLength( baseLength );
                pathCopy += entry->d_name;

                struct stat info;
                VERIFY( lstat( pathCopy.Get(), &info ) == 0 );
                isDir = S_ISDIR( info.st_mode );
            }

            // dir?
            if ( isDir )
            {
                // ingnore dirs
                continue;
            }

            // file - does it match wildcard?
            if ( IsMatch( patterns, entry->d_name ) )
            {
                pathCopy.SetLength( baseLength );
                pathCopy += entry->d_name;

                if ( results->GetSize() == results->GetCapacity() )
                {
                    results->SetCapacity( results->GetSize() * 2 );
                }
                results->SetSize( results->GetSize() + 1 );
                FileInfo & newInfo = results->Top();
                newInfo.m_Name = pathCopy;

                // get additional info
                struct stat info;
                VERIFY( lstat( pathCopy.Get(), &info ) == 0 );
                newInfo.m_Attributes = info.st_mode;
                #if defined( __APPLE__ )
                    newInfo.m_LastWriteTime = ( ( (uint64_t)info.st_mtimespec.tv_sec * 1000000000ULL ) + (uint64_t)info.st_mtimespec.tv_nsec );
                #else
                    newInfo.m_LastWriteTime = ( ( (uint64_t)info.st_mtim.tv_sec * 1000000000ULL ) + (uint64_t)info.st_mtim.tv_nsec );
                #endif
                newInfo.m_Size = info.st_size;
            }
        }
        closedir( dir );
    #else
        #error Unknown platform
    #endif
}

// WorkAroundForWindowsFilePermissionProblem
//------------------------------------------------------------------------------
#if defined( __WINDOWS__ )
    /*static*/ void FileIO::WorkAroundForWindowsFilePermissionProblem( const AString & fileName,
                                                                       const uint32_t openMode,
                                                                       const uint32_t timeoutSeconds )
    {
        // Sometimes after closing a file, subsequent operations on that file will
        // fail.  For example, trying to set the file time, or even another process
        // opening the file.
        //
        // This seems to be a known issue in windows, with multiple potential causes
        // like Virus scanners and possibly the behaviour of the kernel itself.
        //
        // A work-around for this problem is to attempt to open a file we just closed.
        // This will sometimes fail, but if we retry until it succeeds, we avoid the
        // problem on the subsequent operation.
        FileStream f;
        Timer timer;
        while ( f.Open( fileName.Get(), openMode ) == false )
        {
            Thread::Sleep( 1 );

            // timeout so we don't get stuck in here forever
            if ( timer.GetElapsed() > (float)timeoutSeconds )
            {
                ASSERT( false && "WorkAroundForWindowsFilePermissionProblem Failed!" );
                return;
            }
        }
    }
#endif

// FileInfo::IsReadOnly
//------------------------------------------------------------------------------
bool FileIO::FileInfo::IsReadOnly() const
{
    #if defined( __WINDOWS__ )
        return ( ( m_Attributes & FILE_ATTRIBUTE_READONLY ) == FILE_ATTRIBUTE_READONLY );
    #elif defined( __LINUX__ ) || defined( __APPLE__ )
        return ( ( m_Attributes & S_IWUSR ) == 0 );// TODO:LINUX TODO:MAC Is this the correct behaviour?
    #else
        #error Unknown platform
    #endif
}

// IsMatch
//------------------------------------------------------------------------------
/*static*/ bool FileIO::IsMatch( const Array< AString > * patterns, const char * fileName )
{
    // no wildcards means match all files (equivalent to *)
    if ( ( patterns == nullptr ) || ( patterns->IsEmpty() ) )
    {
        return true;
    }

    // Check each provided pattern
    for ( const AString & pattern : *patterns )
    {
        // Let PathUtils manage platform specific case-sensitivity etc
        if ( PathUtils::IsWildcardMatch( pattern.Get(), fileName ) )
        {
            return true;
        }
    }

    return false;
}

// ContainsValidDirChars
//------------------------------------------------------------------------------
/*static*/ bool FileIO::ContainsValidDirChars( const AString & string, AString & errorMsg )
{
    // so dirs can be valid on all OSes, compare against superset of all OSes invalid dir chars

    bool valid = true;  // first assume true
    errorMsg.Clear();
    
    // Mac OS X dirs cannot start with a period
    const char periodChar = '.';
    if ( string.BeginsWith( periodChar ) )
    {
        errorMsg += string;
        errorMsg += " cannot start with a period";
        valid = false;
    }

    // keep checking, so we show user all invalid chars found
    AStackString<> invalidChars( ":/\\,[]{}()!;\"'*?<>|" );
    AStackString<> invalidCharsFound;
    const size_t numInvalidChars = invalidChars.GetLength();
    for ( size_t i = 0; i < numInvalidChars; ++i )
    {
        if ( string.Find( invalidChars[ i ] ) )
        {
            if ( !invalidCharsFound.Find( invalidChars[ i ] ) )
            {
                invalidCharsFound += invalidChars[ i ];
            }
            valid = false;
            // don't break here, since want to show user all invalid chars found
        }
    }
    if ( !invalidCharsFound.IsEmpty() )
    {
        if ( errorMsg.IsEmpty() )
        {
            errorMsg += string;
            errorMsg += " ";
        }
        else
        {
            errorMsg += " and ";
        }
        errorMsg += "cannot contain chars ";
        errorMsg += invalidCharsFound;
    }
    return valid;
}

#if defined( __WINDOWS__ )
// CheckAndSetPermissions
//------------------------------------------------------------------------------
/*static*/ bool FileIO::CheckAndSetPermissions(
    const void * dir,
    const void * usersSID,
    const void * lowLabelSID,
    const char lowLabelAceFlags,
    const uint32_t usersDirAllowMask,
    const uint32_t usersDirDenyMask,
    const uint32_t usersChildAllowMask,
    const uint32_t usersChildDenyMask,
    const bool setPermissions,
    void * & dacl,
    bool & hasDesiredUsersDirPermissions,
    bool & hasDesiredUsersChildPermissions,
    bool & hasLowIntegrityPermission )
{
    // Check and set permissions on a dir
    const HANDLE hDir = (HANDLE)dir;
    const PSID pUsersSID = (PSID)usersSID;
    const PSID pLowLabelSID = (PSID)lowLabelSID;
    PACL pDACL = NULL;

    bool success = false;
    hasDesiredUsersDirPermissions = false;
    hasDesiredUsersChildPermissions = false;
    hasLowIntegrityPermission = false;
    PACL pSACL = NULL;
    PSECURITY_DESCRIPTOR pSD = NULL;
    success = ::GetSecurityInfo( hDir, SE_FILE_OBJECT, OWNER_SECURITY_INFORMATION | GROUP_SECURITY_INFORMATION |
        DACL_SECURITY_INFORMATION | LABEL_SECURITY_INFORMATION, NULL, NULL, &pDACL, &pSACL, &pSD ) == ERROR_SUCCESS;
    if ( success )
    {
        if ( pDACL )
        {
            ACL_SIZE_INFORMATION aclSizeInfo;
            ::ZeroMemory( &aclSizeInfo, sizeof( aclSizeInfo ) );
            success = ::GetAclInformation(
                pDACL,
                &aclSizeInfo,
                sizeof( aclSizeInfo ),
                AclSizeInformation ) == TRUE;
            // check if have built-in users permissions
            if ( success )
            {
                hasDesiredUsersDirPermissions = true;   // start with true and mark false, if any false in loop below
                hasDesiredUsersChildPermissions = true; // start with true and mark false, if any false in loop below
                BOOL anyUsersDirPermissions = false;
                BOOL anyUsersChildPermissions = false;
                PACCESS_ALLOWED_ACE aceItem;
                for ( DWORD aceIndex = 0; aceIndex < aclSizeInfo.AceCount; ++aceIndex )
                {
                    success = ::GetAce(pDACL, aceIndex, (LPVOID*)&aceItem ) == TRUE;
                    if ( success )
                    {
                        if ( aceItem->Header.AceType == ACCESS_ALLOWED_ACE_TYPE )
                        {
                            PSID sSID = (SID*)&( aceItem->SidStart );
                            if ( ::EqualSid( sSID, pUsersSID ) )
                            {
                                if ( aceItem->Header.AceFlags == NO_INHERITANCE )  // dir case
                                {
                                    anyUsersDirPermissions = true;
                                    if ( aceItem->Mask != ( usersDirAllowMask & ~usersDirDenyMask ) )
                                    {
                                        if ( setPermissions )
                                        {
                                            aceItem->Mask = usersDirAllowMask & ~usersDirDenyMask;
                                        }
                                        else
                                        {
                                            hasDesiredUsersDirPermissions = false;
                                        }
                                    }
                                    // don't break here, since want to check all aces
                                }
                                else  // any inheritance flags (children case)
                                {
                                    anyUsersChildPermissions = true;
                                    if ( ! ( aceItem->Header.AceFlags & INHERIT_ONLY_ACE &&
                                        aceItem->Mask == ( usersChildAllowMask & ~usersChildDenyMask ) ) )
                                    {
                                        if ( setPermissions )
                                        {
                                            // specifying INHERIT_ONLY_ACE, so that only the children get the child permissions, not the dir
                                            aceItem->Header.AceFlags = aceItem->Header.AceFlags | INHERIT_ONLY_ACE;
                                            aceItem->Mask = usersChildAllowMask & ~usersChildDenyMask;
                                        }
                                        else
                                        {
                                            hasDesiredUsersChildPermissions = false;
                                        }
                                    }
                                    // don't break here, since want to check all aces
                                }
                            }
                        }
                    }
                    else
                    {
                        break;
                    }
                }
                hasDesiredUsersDirPermissions = anyUsersDirPermissions && hasDesiredUsersDirPermissions;
                hasDesiredUsersChildPermissions = anyUsersChildPermissions && hasDesiredUsersChildPermissions;
            }
        }
        if ( pSACL )
        {
            ACL_SIZE_INFORMATION aclSizeInfo;
            ::ZeroMemory( &aclSizeInfo, sizeof( aclSizeInfo ) );
            success = ::GetAclInformation(
                pSACL,
                &aclSizeInfo,
                sizeof( aclSizeInfo ),
                AclSizeInformation ) == TRUE;
            // check if have low integrity permission
            if ( success )
            {
                PACCESS_ALLOWED_ACE aceItem;
                for ( DWORD aceIndex = 0; aceIndex < aclSizeInfo.AceCount; ++aceIndex )
                {
                    success = ::GetAce( pSACL, aceIndex, (LPVOID*)&aceItem ) == TRUE;
                    if ( success )
                    {
                        if ( aceItem->Header.AceType == SYSTEM_MANDATORY_LABEL_ACE_TYPE )
                        {
                            PSID sSID = (SID*)&( aceItem->SidStart );
                            if ( ::EqualSid( sSID, pLowLabelSID ) )
                            {
                                if ( aceItem->Header.AceFlags == lowLabelAceFlags )
                                {
                                    hasLowIntegrityPermission = true;
                                }
                                else if ( setPermissions )
                                {
                                    aceItem->Header.AceFlags = lowLabelAceFlags;
                                    hasLowIntegrityPermission = true;
                                }
                            }
                            // don't break here, since want to check all aces
                        }
                    }
                    else
                    {
                        break;
                    }
                }
            }
        }
    }
    // pDACL, pSACL, and pSD do not need clearing
    dacl = pDACL;
    return success;
}

// SetLowIntegrity
//------------------------------------------------------------------------------
/*static*/ bool FileIO::SetLowIntegrity(
    const AString & path,
    const uint32_t dirPermissions,
    const uint32_t filePermissions,
    AString & errorMsg )
{
    bool success = false;
    DWORD usersDirAllowMask = 0;
    DWORD usersChildAllowMask = 0;
    DWORD usersDirDenyMask = 0;
    DWORD usersChildDenyMask = 0;

    // read permissions
    DWORD tempDirMask = FILE_READ_EA | FILE_READ_ATTRIBUTES | READ_CONTROL | SYNCHRONIZE;
    DWORD tempFileMask = FILE_READ_DATA | FILE_READ_EA | FILE_READ_ATTRIBUTES | READ_CONTROL | SYNCHRONIZE;
    if (dirPermissions & Read)
    {
        usersDirAllowMask |= tempDirMask;
    }
    else
    {
        usersDirDenyMask |= tempDirMask;
    }
    if (filePermissions & Read)
    {
        usersChildAllowMask |= tempFileMask;
    }
    else
    {
        usersChildDenyMask |= tempFileMask;
    }

    // list permissions
    tempDirMask = FILE_LIST_DIRECTORY;
    if (dirPermissions & List)
    {
        usersDirAllowMask |= tempDirMask;
    }
    else
    {
        usersDirDenyMask |= tempDirMask;
    }
    // List permission does not apply to files, so skip for files

    // write permissions
    tempDirMask = FILE_ADD_FILE | FILE_ADD_SUBDIRECTORY | FILE_WRITE_EA | FILE_WRITE_ATTRIBUTES;
    tempFileMask = FILE_WRITE_DATA | FILE_APPEND_DATA | FILE_WRITE_EA | FILE_WRITE_ATTRIBUTES;
    if (dirPermissions & Write)
    {
        usersDirAllowMask |= tempDirMask;
    }
    else
    {
        usersDirDenyMask |= tempDirMask;
    }
    if (filePermissions & Write)
    {
        usersChildAllowMask |= tempFileMask;
    }
    else
    {
        usersChildDenyMask |= tempFileMask;
    }

    // execute permissions
    tempDirMask = FILE_TRAVERSE;  // traverse and execute
    tempFileMask = FILE_EXECUTE;
    if (dirPermissions & Execute)
    {
        usersDirAllowMask |= tempDirMask;
    }
    else
    {
        usersDirDenyMask |= tempDirMask;
    }
    if (filePermissions & Execute)
    {
        usersChildAllowMask |= tempFileMask;
    }
    else
    {
        usersChildDenyMask |= tempFileMask;
    }

    // delete permissions
    tempDirMask = FILE_DELETE_CHILD;
    tempFileMask = DELETE;
    if (dirPermissions & Delete)
    {
        usersDirAllowMask |= tempDirMask;
    }
    else
    {
        usersDirDenyMask |= tempDirMask;
    }
    if (filePermissions & Delete)
    {
        usersChildAllowMask |= tempFileMask;
    }
    else
    {
        usersChildDenyMask |= tempFileMask;
    }

    usersDirDenyMask |= WRITE_DAC | WRITE_OWNER;
    usersChildDenyMask |= WRITE_DAC | WRITE_OWNER;

    // Set permissions to allow low integrity code to access a dir and its children
    // from https://social.msdn.microsoft.com/Forums/windowsdesktop/en-US/ca60b920-e351-4cd6-aa40-b2bf149ad226
    // from https://stackoverflow.com/questions/690780/how-to-create-directory-with-all-rights-granted-to-everyone
    // open dir for reading permissions
    HANDLE hDir = ::CreateFile( path.Get(),
        READ_CONTROL,
        0,
        NULL,
        OPEN_EXISTING,
        FILE_FLAG_BACKUP_SEMANTICS,  // allow dir to be opened by CreateFile
        0 );
    success = hDir != INVALID_HANDLE_VALUE;
    if ( success )
    {
        // get SACL for low integrity user
        DWORD sidSize = SECURITY_MAX_SID_SIZE;
        PSID pLowLabelSID = NULL;
        PACL pLowLabelSACL = NULL;
        // set so that the dir and its children get the low integrity permission
        const BYTE lowLabelAceFlags = CONTAINER_INHERIT_ACE | OBJECT_INHERIT_ACE;

        success = ( pLowLabelSID = ::LocalAlloc( LMEM_FIXED, sidSize ) ) != nullptr;
        if ( success )
        {
            success = ::CreateWellKnownSid( WinLowLabelSid, 0, pLowLabelSID, &sidSize ) == TRUE;
            if ( success )
            {
                success = ( pLowLabelSACL = (PACL)::LocalAlloc( LPTR, sidSize += sizeof( ACL ) + sizeof( ACE_HEADER ) + sizeof( ACCESS_MASK) ) ) != nullptr;
                if ( success )
                {
                    success = ::InitializeAcl( pLowLabelSACL, sidSize, ACL_REVISION) == TRUE;
                    if ( success )
                    {
                        success = ::AddMandatoryAce( pLowLabelSACL, ACL_REVISION, lowLabelAceFlags, 0, pLowLabelSID ) == TRUE;
                    }
                }
            }
        }

        // get the built-in users group,
        // since the low integrity user is in this group
        sidSize = SECURITY_MAX_SID_SIZE;
        PSID pUsersSID = NULL;
        success = ( pUsersSID = ::LocalAlloc( LMEM_FIXED, sidSize ) ) != nullptr;
        if ( success )
        {
            success = ::CreateWellKnownSid( WinBuiltinUsersSid, 0, pUsersSID, &sidSize ) == TRUE;
        }

        // check if the dir already has the desired permissions
        // this is so can open the dir for read access
        // and not need write access to it if the permissions are already set
        bool hasDesiredUsersDirPermissions = false;
        bool hasDesiredUsersChildPermissions = false;
        bool hasLowIntegrityPermission = false;
        if ( success )
        {
            void * pDACL = NULL;
            const BOOL setPermissions = false;  // check only
            success = CheckAndSetPermissions(
                hDir,
                pUsersSID,
                pLowLabelSID,
                lowLabelAceFlags,
                usersDirAllowMask,
                usersDirDenyMask,
                usersChildAllowMask,
                usersChildDenyMask,
                setPermissions,
                pDACL,
                hasDesiredUsersDirPermissions,
                hasDesiredUsersChildPermissions,
                hasLowIntegrityPermission );
        }
        const BOOL hasDesiredPermissions =
            hasDesiredUsersDirPermissions &&
            hasDesiredUsersChildPermissions &&
            hasLowIntegrityPermission;
        ::CloseHandle( hDir );
        hDir = INVALID_HANDLE_VALUE;
        if ( success && !hasDesiredPermissions )
        {
            // open dir for setting permissions
            hDir = ::CreateFile( path.Get(),
                READ_CONTROL | WRITE_DAC | WRITE_OWNER,
                0,
                NULL,
                OPEN_EXISTING,
                FILE_FLAG_BACKUP_SEMANTICS,  // allow dir to be opened by CreateFile
                0 );
            success = hDir != INVALID_HANDLE_VALUE;
            if ( success )
            {
                void * pDACL = NULL;
                const BOOL setPermissions = true;  // set the permissions
                success = CheckAndSetPermissions(
                    hDir,
                    pUsersSID,
                    pLowLabelSID,
                    lowLabelAceFlags,
                    usersDirAllowMask,
                    usersDirDenyMask,
                    usersChildAllowMask,
                    usersChildDenyMask,
                    setPermissions,
                    pDACL,
                    hasDesiredUsersDirPermissions,
                    hasDesiredUsersChildPermissions,
                    hasLowIntegrityPermission );

                PACL daclToUse = (PACL)pDACL;
                PACL daclToClear = NULL;  // pDACL does not need clearing
                if ( success )
                {
                    if ( !hasDesiredUsersDirPermissions || !hasDesiredUsersChildPermissions )
                    {
                        // build-in users group was not in existing permissions,
                        // so add it
                        DWORD numEa = 1;
                        if ( !hasDesiredUsersDirPermissions && !hasDesiredUsersChildPermissions )
                        {
                            numEa = 2;
                        }
                        EXPLICIT_ACCESS* ea = (EXPLICIT_ACCESS*)::LocalAlloc( LPTR, sizeof( EXPLICIT_ACCESS ) * numEa );
                        DWORD eaIndex = 0;
                        if ( ea && !hasDesiredUsersDirPermissions )
                        {
                            ea[ eaIndex ].grfAccessMode = GRANT_ACCESS;
                            ea[ eaIndex ].grfAccessPermissions = usersDirAllowMask & ~usersDirDenyMask;
                            ea[ eaIndex ].grfInheritance = NO_INHERITANCE;  // permissions only for the dir
                            ea[ eaIndex ].Trustee.pMultipleTrustee = NULL;
                            ea[ eaIndex ].Trustee.MultipleTrusteeOperation = NO_MULTIPLE_TRUSTEE;
                            ea[ eaIndex ].Trustee.TrusteeForm = TRUSTEE_IS_SID;
                            ea[ eaIndex ].Trustee.TrusteeType = TRUSTEE_IS_WELL_KNOWN_GROUP;
                            ea[ eaIndex ].Trustee.ptstrName = (LPTSTR)pUsersSID;
                            ++eaIndex;
                        }
                        if ( ea && !hasDesiredUsersChildPermissions )
                        {
                            // specifying INHERIT_ONLY_ACE, so that only the children get the child permissions, not the dir
                            const BYTE childAceFlags = CONTAINER_INHERIT_ACE | OBJECT_INHERIT_ACE | INHERIT_ONLY_ACE;
                            ea[ eaIndex ].grfAccessMode = GRANT_ACCESS;
                            ea[ eaIndex ].grfAccessPermissions = usersChildAllowMask & ~usersChildDenyMask;
                            ea[ eaIndex ].grfInheritance = childAceFlags;
                            ea[ eaIndex ].Trustee.pMultipleTrustee = NULL;
                            ea[ eaIndex ].Trustee.MultipleTrusteeOperation = NO_MULTIPLE_TRUSTEE;
                            ea[ eaIndex ].Trustee.TrusteeForm = TRUSTEE_IS_SID;
                            ea[ eaIndex ].Trustee.TrusteeType = TRUSTEE_IS_WELL_KNOWN_GROUP;
                            ea[ eaIndex ].Trustee.ptstrName = (LPTSTR)pUsersSID;
                        }

                        PACL pAdditionsDACL = NULL;
                        success = ::SetEntriesInAcl( numEa, ea, daclToUse, &pAdditionsDACL ) == ERROR_SUCCESS;
                        ::LocalFree( ea );
                        daclToUse = pAdditionsDACL;
                        daclToClear = pAdditionsDACL;  // needs clearing since SetEntriesInAcl allocs
                    }
                    if ( success )
                    {
                        success = ::SetSecurityInfo(
                            hDir, SE_FILE_OBJECT,
                            DACL_SECURITY_INFORMATION | LABEL_SECURITY_INFORMATION |
                            PROTECTED_DACL_SECURITY_INFORMATION,  // replace DACL with our own
                            NULL, NULL, daclToUse, pLowLabelSACL ) == ERROR_SUCCESS;
                    }
                }
                if ( daclToClear )
                {
                    ::LocalFree( daclToClear );
                }
                ::CloseHandle( hDir );
            }
        }
        ::LocalFree( pUsersSID );
        ::LocalFree( pLowLabelSACL );
        ::LocalFree( pLowLabelSID );
    }
    if ( !success)
    {
        errorMsg.Format( "Failed to set permissions on dir %s Error: %s", path.Get(), LAST_ERROR_STR );
    }
    return success;
}
#endif  // Windows

//------------------------------------------------------------------------------
