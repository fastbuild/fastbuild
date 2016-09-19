// FileStream.cpp
//------------------------------------------------------------------------------

// Includes
//------------------------------------------------------------------------------
#include "Core/PrecompiledHeader.h"

#include "FileStream.h"

// Core
#include "Core/Env/Assert.h"
#include "Core/Strings/AString.h"
#include "Core/Strings/AStackString.h"

// system
#include <stdio.h>
#if defined( __WINDOWS__ )
    #include <windows.h>
#endif

// Defines
//------------------------------------------------------------------------------
//#define FILESTREAM_DETAILED_DEBUG // uncomment this for lots of spam
#ifdef FILESTREAM_DETAILED_DEBUG
    #include "Core/Tracing/Tracing.h"
    #include "Core/Process/Thread.h"
    #define FSDEBUG( x ) x
#else
    #define FSDEBUG( x ) PRAGMA_DISABLE_PUSH_MSVC(4127) \
                        do {} while ( false );          \
                        PRAGMA_DISABLE_POP_MSVC
#endif

#if defined( __APPLE__ ) || defined( __LINUX__ )
    #define INVALID_HANDLE_VALUE ( 0 )
#endif

// CONSTRUCTOR
//------------------------------------------------------------------------------
FileStream::FileStream()
: m_Handle( (void *)INVALID_HANDLE_VALUE )
{
}

// DESTRUCTOR
//------------------------------------------------------------------------------
FileStream::~FileStream()
{
    if ( IsOpen() )
    {
        Close();
    }
}

// Open
//------------------------------------------------------------------------------
bool FileStream::Open( const char * fileName, uint32_t fileMode )
{
    ASSERT( !IsOpen() );

#if defined( __WINDOWS__ )
    DWORD desiredAccess( 0 );
    DWORD shareMode( 0 );
    DWORD creationDisposition( 0 );
    DWORD flags( FILE_ATTRIBUTE_NORMAL );

    // access mode
    if ( ( fileMode & READ_ONLY ) != 0 )
    {
        ASSERT( fileMode == READ_ONLY ); // no extra flags allowed
        desiredAccess       |= GENERIC_READ;
        shareMode           |= FILE_SHARE_READ; // allow other readers
        creationDisposition |= OPEN_EXISTING;
    }
    else if ( ( fileMode & WRITE_ONLY ) != 0 )
    {
        desiredAccess       |= GENERIC_WRITE;
        shareMode           |= FILE_SHARE_READ; // allow other readers
        creationDisposition |= CREATE_ALWAYS; // overwrite existing
    }
    else
    {
        ASSERT( false ); // must specify an access mode
    }

    // extra flags
    if ( ( fileMode & TEMP ) != 0 )
    {
        flags |= FILE_ATTRIBUTE_TEMPORARY; // don't flush to disk if possible
    }

    // for sharing violations, we'll retry a few times as per http://support.microsoft.com/kb/316609
    size_t retryCount = 0;
    while ( retryCount < 5 )
    {
        HANDLE h = CreateFile( fileName,            // _In_     LPCTSTR lpFileName,
                               desiredAccess,       // _In_     DWORD dwDesiredAccess,
                               shareMode,           // _In_     DWORD dwShareMode,
                               nullptr,             // _In_opt_ LPSECURITY_ATTRIBUTES lpSecurityAttributes,
                               creationDisposition, // _In_     DWORD dwCreationDisposition,
                               flags,               // _In_     DWORD dwFlagsAndAttributes,
                               nullptr );           // _In_opt_ HANDLE hTemplateFile

        if ( h != INVALID_HANDLE_VALUE )
        {
            // file opened ok
            m_Handle = (void *)h;
            FSDEBUG( DEBUGSPAM( "Open file OK: %x - '%s' on %x\n", m_Handle, fileName, Thread::GetCurrentThreadId() ); )
            return true;
        }

        // problem opening file...

        // was it a sharing violation?
        if ( GetLastError() == ERROR_SHARING_VIOLATION )
        {
            if ( ( fileMode & NO_RETRY_ON_SHARING_VIOLATION ) != 0 )
            {
                break; // just fail
            }

            ++retryCount;
            FSDEBUG( DEBUGSPAM( "Open file sharing violation: '%s' on %x (try %u)\n", fileName, Thread::GetCurrentThreadId(), retryCount ); )
            Sleep( 100 ); // sleep and
            continue;     // try again as per http://support.microsoft.com/kb/316609
        }

        // some other kind of error...
        break;
    }
#elif defined ( __APPLE__ ) || defined( __LINUX__ )
    // Read/Write mode
    AStackString<8> modeStr;
    if ( ( fileMode & READ_ONLY ) != 0 )
    {
        modeStr += "rb";
    }
    else if ( ( fileMode & WRITE_ONLY ) != 0 )
    {
        modeStr += "wb";
    }
    else
    {
        ASSERT( false ); // must specify an access mode
    }

    // extra flags
    if ( ( fileMode & TEMP ) != 0 )
    {
        // hint flag - unsupported
    }

    m_Handle = fopen( fileName, modeStr.Get() );
    if ( m_Handle != INVALID_HANDLE_VALUE )
    {
        // file opened ok
        FSDEBUG( DEBUGSPAM( "Open file OK: %x - '%s' on %x\n", m_Handle, fileName, Thread::GetCurrentThreadId() ); )
        return true;
    }
#else
    #error Unknown platform
#endif

    FSDEBUG( DEBUGSPAM( "Open file error: %d - '%s' on %x\n", GetLastError(), fileName, Thread::GetCurrentThreadId() ); )
    m_Handle = INVALID_HANDLE_VALUE;
    return false;
}

// Close
//------------------------------------------------------------------------------
void FileStream::Close()
{
    ASSERT( IsOpen() );
#if defined( __WINDOWS__ )
    VERIFY( CloseHandle( (HANDLE)m_Handle ) );
#elif defined( __APPLE__ ) || defined( __LINUX__ )
    fclose( (FILE*)m_Handle );
#else
    #error Unknown platform
#endif
    FSDEBUG( DEBUGSPAM( "Close file: %x on %x\n", m_Handle, Thread::GetCurrentThreadId() ); )
    m_Handle = INVALID_HANDLE_VALUE;
}

// IsOpen
//------------------------------------------------------------------------------
bool FileStream::IsOpen() const
{
    return ( m_Handle != INVALID_HANDLE_VALUE );
}

// Read
//------------------------------------------------------------------------------
/*virtual*/ uint64_t FileStream::ReadBuffer( void * buffer, uint64_t bytesToRead )
{
    ASSERT( buffer );
    ASSERT( IsOpen() );

    uint64_t totalBytesRead = 0;
#if defined( __WINDOWS__ )
    do
    {
        uint64_t remaining = ( bytesToRead - totalBytesRead );
        uint32_t tryToReadNow = ( remaining > MEGABYTE ) ? MEGABYTE : (uint32_t)remaining;
        uint32_t bytesReadNow = 0;
        if ( FALSE == ReadFile( (HANDLE)m_Handle,                           // _In_         HANDLE hFile,
                                (char *)buffer + (size_t)totalBytesRead,    // _Out_        LPVOID lpBuffer,
                                tryToReadNow,                               // _In_         DWORD nNumberOfBytesToRead,
                                (LPDWORD)&bytesReadNow,                     // _Out_opt_    LPDWORD lpNumberOfBytesRead,
                                nullptr ) )                                 // _Inout_opt_  LPOVERLAPPED lpOverlapped
        {
            break; // failed
        }
        if ( bytesReadNow == 0 )
        {
            return totalBytesRead; // end of file
        }
        totalBytesRead += bytesReadNow;
    } while ( totalBytesRead < bytesToRead );
#elif defined( __APPLE__ ) || defined( __LINUX__ )
    totalBytesRead += fread( buffer, 1, bytesToRead, (FILE*)m_Handle );
#else
    #error Unknown platform
#endif

    FSDEBUG( DEBUGSPAM( "Read %llu bytes to %x on %x\n", totalBytesRead, m_Handle, Thread::GetCurrentThreadId() ); )

    return totalBytesRead;
}

// Write
//------------------------------------------------------------------------------
/*virtual*/ uint64_t FileStream::WriteBuffer( const void * buffer, uint64_t bytesToWrite )
{
    ASSERT( buffer );
    ASSERT( IsOpen() );

    uint64_t totalBytesWritten = 0;
#if defined( __WINDOWS__ )
    do
    {
        uint64_t remaining = ( bytesToWrite - totalBytesWritten );
        uint32_t tryToWriteNow = ( remaining > MEGABYTE ) ? MEGABYTE : (uint32_t)remaining;
        uint32_t bytesWrittenNow = 0;
        if ( FALSE == WriteFile( (HANDLE)m_Handle,                              // _In_         HANDLE hFile,
                                 (char *)buffer + (size_t)totalBytesWritten,    // _In_         LPCVOID lpBuffer,
                                 tryToWriteNow,                                 // _In_         DWORD nNumberOfBytesToWrite,
                                 (LPDWORD)&bytesWrittenNow,                     // _Out_opt_    LPDWORD lpNumberOfBytesWritten,
                                 nullptr ) )                                    // _Inout_opt_  LPOVERLAPPED lpOverlapped
        {
            break; // failed
        }
        totalBytesWritten += bytesWrittenNow;
    } while ( totalBytesWritten < bytesToWrite );
#elif defined( __APPLE__ ) || defined( __LINUX__ )
    totalBytesWritten = fwrite( buffer, 1, bytesToWrite, (FILE*)m_Handle );
#else
    #error Unknown platform
#endif

    FSDEBUG( DEBUGSPAM( "Write %llu bytes to %x on %x\n", totalBytesWritten, m_Handle, Thread::GetCurrentThreadId() ); )

    return totalBytesWritten;
}

// Flush
//------------------------------------------------------------------------------
/*virtual*/ void FileStream::Flush()
{
    FSDEBUG( DEBUGSPAM( "Flush %x on %x\n", m_Handle, Thread::GetCurrentThreadId() ); )

    ASSERT( IsOpen() );
#if defined( __WINDOWS__ )
    VERIFY( FlushFileBuffers( (HANDLE)m_Handle ) );
#elif defined( __APPLE__ ) || defined( __LINUX__ )
    fflush( (FILE*)m_Handle );
#else
    #error Unknown platform
#endif
}

// Tell
//------------------------------------------------------------------------------
/*virtual*/ uint64_t FileStream::Tell() const
{
    FSDEBUG( DEBUGSPAM( "Tell %x on %x\n", m_Handle, Thread::GetCurrentThreadId() ); )

    ASSERT( IsOpen() );

#if defined( __WINDOWS__ )
    LARGE_INTEGER zeroPos, newPos;
    zeroPos.QuadPart = 0;
    VERIFY( SetFilePointerEx( (HANDLE)m_Handle, zeroPos, &newPos, FILE_CURRENT ) );
    return newPos.QuadPart;
#elif defined( __APPLE__ ) || defined( __LINUX__ )
    return ftell( (FILE*)m_Handle );
#else
    #error Unknown platform
#endif
}

// Seek
//------------------------------------------------------------------------------
/*virtual*/ bool FileStream::Seek( uint64_t pos ) const
{
    FSDEBUG( DEBUGSPAM( "Seek to %llu, %x on %x\n", pos, m_Handle, Thread::GetCurrentThreadId() ); )

    ASSERT( IsOpen() );

#if defined( __WINDOWS__ )
    LARGE_INTEGER newPos;
    newPos.QuadPart = pos;
    if ( FALSE == SetFilePointerEx( (HANDLE)m_Handle, newPos, nullptr, FILE_BEGIN ) )
    {
        return false;
    }
    return true;
#elif defined( __APPLE__ ) || defined( __LINUX__ )
    ASSERT( pos <= 0xFFFFFFFF ); // only support 4GB files for seek on OSX/IOS
    fseek( (FILE*)m_Handle, pos, SEEK_SET );
    return true; // TODO:MAC check EOF for consistency with windows
#else
    #error Unknown platform
#endif
}

// GetFileSize
//------------------------------------------------------------------------------
/*virtual*/ uint64_t FileStream::GetFileSize() const
{
    FSDEBUG( DEBUGSPAM( "GetFileSize %x on %x\n", m_Handle, Thread::GetCurrentThreadId() ); )

    ASSERT( IsOpen() );

    #if defined( __WINDOWS__ )
        // seek to end
        DWORD fileSizeHigh;
        DWORD fileSizeLow = ::GetFileSize( (HANDLE)m_Handle, &fileSizeHigh );
        return ( ( uint64_t( fileSizeHigh ) << 32 ) | uint64_t( fileSizeLow ) );
    #else
        // record current pos to restore
        uint64_t originalPos = Tell();

        // seek to end
        fseek( (FILE *)m_Handle, 0, SEEK_END );
        uint64_t size = ftell( (FILE*)m_Handle );

        // seek back to the original pos
        VERIFY( Seek( originalPos ) );

        return size;
    #endif
}

// SetLastWriteTime
//------------------------------------------------------------------------------
#if defined( __WINDOWS__ )
    bool FileStream::SetLastWriteTime( uint64_t lastWriteTime )
    {
        FILETIME ftWrite;
        ftWrite.dwLowDateTime = (uint32_t)( lastWriteTime & 0x00000000FFFFFFFF );
        ftWrite.dwHighDateTime = (uint32_t)( ( lastWriteTime & 0xFFFFFFFF00000000 ) >> 32 );
        if ( !SetFileTime( (HANDLE)m_Handle, nullptr, nullptr, &ftWrite ) ) // create, access, write
        {
            return false;
        }
        return true;
    }
#endif

//------------------------------------------------------------------------------
