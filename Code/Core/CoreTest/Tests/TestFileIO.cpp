// TestFileIO.cpp
//------------------------------------------------------------------------------

// Includes
//------------------------------------------------------------------------------
#include "TestFramework/UnitTest.h"

// Core
#include "Core/FileIO/FileIO.h"
#include "Core/FileIO/FileStream.h"
#include "Core/Math/Random.h"
#include "Core/Process/Process.h"
#include "Core/Process/Thread.h"
#include "Core/Strings/AStackString.h"

// system
#if defined( __LINUX__ )
    #include <unistd.h>
#endif

// TestFileIO
//------------------------------------------------------------------------------
class TestFileIO : public UnitTest
{
private:
    DECLARE_TESTS

    void FileExists() const;
    void FileDelete() const;
    void FileCopy() const;
    void FileCopySymlink() const;
    void FileMove() const;
    void ReadOnly() const;
    void FileTime() const;
    void LongPaths() const;

    // Helpers
    mutable Random m_Random;
    void GenerateTempFileName( AString & tmpFileName ) const;
};

// Register Tests
//------------------------------------------------------------------------------
REGISTER_TESTS_BEGIN( TestFileIO )
    REGISTER_TEST( FileExists )
    REGISTER_TEST( FileDelete )
    REGISTER_TEST( FileCopy )
    REGISTER_TEST( FileCopySymlink )
    REGISTER_TEST( FileMove )
    REGISTER_TEST( ReadOnly )
    REGISTER_TEST( FileTime )
    REGISTER_TEST( LongPaths )
REGISTER_TESTS_END

// FileExists
//------------------------------------------------------------------------------
void TestFileIO::FileExists() const
{
    // generate a process unique file path
    AStackString<> path;
    GenerateTempFileName( path );

    // ensure doesn't exist
    FileIO::FileDelete( path.Get() ); // delete in case left over from previous test run
    TEST_ASSERT( FileIO::FileExists( path.Get() ) == false );

    // create it
    FileStream f;
    TEST_ASSERT( f.Open( path.Get(), FileStream::WRITE_ONLY ) == true );
    f.Close();

    // ensure exists
    TEST_ASSERT( FileIO::FileExists( path.Get() ) == true );

    // clean up
    TEST_ASSERT( FileIO::FileDelete( path.Get() ) == true );
    TEST_ASSERT( FileIO::FileExists( path.Get() ) == false );
}

// FileDelete
//------------------------------------------------------------------------------
void TestFileIO::FileDelete() const
{
    // generate a process unique file path
    AStackString<> path;
    GenerateTempFileName( path );

    // create it
    FileStream f;
    TEST_ASSERT( f.Open( path.Get(), FileStream::WRITE_ONLY ) == true );
    f.Close();

    // ensure exists
    TEST_ASSERT( FileIO::FileExists( path.Get() ) == true );

    // delete
    TEST_ASSERT( FileIO::FileDelete( path.Get() ) == true );
    TEST_ASSERT( FileIO::FileExists( path.Get() ) == false );
}

// FileCopy
//------------------------------------------------------------------------------
void TestFileIO::FileCopy() const
{
    // generate a process unique file path
    AStackString<> path;
    GenerateTempFileName( path );

    // generate copy file name
    AStackString<> pathCopy( path );
    pathCopy += ".copy";

    // make sure nothing is left from previous runs
    FileIO::FileDelete( path.Get() );
    FileIO::FileDelete( pathCopy.Get() );
    TEST_ASSERT( FileIO::FileExists( path.Get() ) == false );
    TEST_ASSERT( FileIO::FileExists( pathCopy.Get() ) == false );

    // create it
    FileStream f;
    TEST_ASSERT( f.Open( path.Get(), FileStream::WRITE_ONLY ) == true );
    f.Close();

    // copy it
    TEST_ASSERT( FileIO::FileCopy( path.Get(), pathCopy.Get() ) );
    TEST_ASSERT( FileIO::FileExists( pathCopy.Get() ) == true );

    // ensure attributes are transferred properly
    FileIO::FileInfo sourceInfo;
    TEST_ASSERT( FileIO::GetFileInfo( path, sourceInfo ) == true );
    FileIO::FileInfo destInfo;
    TEST_ASSERT( FileIO::GetFileInfo( pathCopy, destInfo ) == true );
    TEST_ASSERT( destInfo.m_Attributes == sourceInfo.m_Attributes );

    // copy without overwrite allowed should fail
    const bool allowOverwrite = false;
    TEST_ASSERT( FileIO::FileCopy( path.Get(), pathCopy.Get(), allowOverwrite ) == false );

    // cleanup
    VERIFY( FileIO::FileDelete( path.Get() ) );
    VERIFY( FileIO::FileDelete( pathCopy.Get() ) );
}

// FileCopySymlink
//------------------------------------------------------------------------------
void TestFileIO::FileCopySymlink() const
{
    #if defined( __WINDOWS__ ) || defined ( __APPLE__ )
        // Not tested on Windows/MacOS as symlinks are directly supported
        // by the file copy API.  Also on Windows, it would make unit
        // tests require administrator privileges.
    #elif defined ( __LINUX__ )
        AStackString<> symlinkTarget( "symlink" );

        // generate a process unique file path
        AStackString<> path;
        GenerateTempFileName( path );

        // generate copy file name
        AStackString<> pathCopy( path );
        pathCopy += ".copy";

        // make sure nothing is left from previous runs
        FileIO::FileDelete( path.Get() );
        FileIO::FileDelete( pathCopy.Get() );
        TEST_ASSERT( FileIO::FileExists( path.Get() ) == false );
        TEST_ASSERT( FileIO::FileExists( pathCopy.Get() ) == false );

        // create it
        TEST_ASSERT( symlink( symlinkTarget.Get(), path.Get() ) == 0 );

        // copy it
        TEST_ASSERT( FileIO::FileCopy( path.Get(), pathCopy.Get() ) );
        TEST_ASSERT( FileIO::FileExists( pathCopy.Get() ) == true );

        // validate link
        AStackString<> linkPath;
        ssize_t length = readlink( pathCopy.Get(), linkPath.Get(), linkPath.GetReserved() );
        TEST_ASSERT( length == symlinkTarget.GetLength() );
        linkPath.SetLength( length );
        TEST_ASSERT( linkPath == symlinkTarget );

        // ensure attributes are transferred properly
        FileIO::FileInfo sourceInfo;
        TEST_ASSERT( FileIO::GetFileInfo( path, sourceInfo ) == true );
        FileIO::FileInfo destInfo;
        TEST_ASSERT( FileIO::GetFileInfo( pathCopy, destInfo ) == true );
        TEST_ASSERT( destInfo.m_Attributes == sourceInfo.m_Attributes );

        // copy without overwrite allowed should fail
        const bool allowOverwrite = false;
        TEST_ASSERT( FileIO::FileCopy( path.Get(), pathCopy.Get(), allowOverwrite ) == false );

        // cleanup
        VERIFY( FileIO::FileDelete( path.Get() ) );
        VERIFY( FileIO::FileDelete( pathCopy.Get() ) );
    #else
        #error Unknown platform
    #endif
}

// FileMove
//------------------------------------------------------------------------------
void TestFileIO::FileMove() const
{
    // generate a process unique file path
    AStackString<> path;
    GenerateTempFileName( path );

    // generate copy file name
    AStackString<> pathCopy( path );
    pathCopy += ".copy";

    // make sure nothing is left from previous runs
    FileIO::FileDelete( path.Get() );
    FileIO::FileDelete( pathCopy.Get() );

    // create it
    FileStream f;
    TEST_ASSERT( f.Open( path.Get(), FileStream::WRITE_ONLY ) == true );
    f.Close();

    // move it
    TEST_ASSERT( FileIO::FileMove( path, pathCopy ) );
    TEST_ASSERT( FileIO::FileExists( path.Get() ) == false );
    TEST_ASSERT( FileIO::FileExists( pathCopy.Get() ) == true );

    // cleanup
    VERIFY( FileIO::FileDelete( pathCopy.Get() ) );
}

// ReadOnly
//------------------------------------------------------------------------------
void TestFileIO::ReadOnly() const
{
    // generate a process unique file path
    AStackString<> path;
    GenerateTempFileName( path );

    // create it
    FileStream f;
    TEST_ASSERT( f.Open( path.Get(), FileStream::WRITE_ONLY ) == true );
    f.Close();

    // should not be read only
    TEST_ASSERT( FileIO::GetReadOnly( path ) == false );

    // set readonly
    TEST_ASSERT( FileIO::SetReadOnly( path.Get(), true ) == true );

    // should be read only
    TEST_ASSERT( FileIO::GetReadOnly( path ) == true );

    // delete should fail
    TEST_ASSERT( FileIO::FileDelete( path.Get() ) == false );

    // clean up
    TEST_ASSERT( FileIO::SetReadOnly( path.Get(), false ) == true );
    TEST_ASSERT( FileIO::GetReadOnly( path ) == false );
    TEST_ASSERT( FileIO::FileDelete( path.Get() ) == true );
}

// FileTime
//------------------------------------------------------------------------------
void TestFileIO::FileTime() const
{
    // generate a process unique file path
    AStackString<> path;
    GenerateTempFileName( path );

    // create it
    FileStream f;
    TEST_ASSERT( f.Open( path.Get(), FileStream::WRITE_ONLY ) == true );
    f.Close();

    // get last write time
    const uint64_t oldTime = FileIO::GetFileLastWriteTime( path );
    TEST_ASSERT( oldTime != 0 );

    // Modify the file until the filetime changes. This can be surprisingly
    // long on OSX due to poor HFS+ filetime granularity
    uint64_t newTime = 0;
    for ( ;; )
    {
        Thread::Sleep( 10 );

        // modify file
        FileStream f2;
        TEST_ASSERT( f.Open( path.Get(), FileStream::WRITE_ONLY ) == true );
        f.Write( (uint32_t)0 );
        f.Close();

        // get new last write time
        newTime = FileIO::GetFileLastWriteTime( path );
        if ( newTime > oldTime )
        {
            break;
        }
    }

    // manually set time back
    TEST_ASSERT( FileIO::SetFileLastWriteTime( path, oldTime ) == true );
    uint64_t timeNow = FileIO::GetFileLastWriteTime( path );
    TEST_ASSERT( timeNow == oldTime );
}

// LongPaths
//------------------------------------------------------------------------------
void TestFileIO::LongPaths() const
{
    //
    // Ensure long paths are correctly handled by various functions
    //
    
    #if defined( __WINDOWS__ )
        // On Windows, long path support must be enabled via a registry key
        // Only if this is enabled can we expect our test to pass
        if ( FileIO::IsWindowsLongPathSupportEnabled() == false )
        {
            return;
        }
    #endif

    // Constants used to build long paths
    AString a( 255 );
    AString b( 255 );
    for ( size_t i = 0; i < 255; ++i )
    {
        a += 'a';
        b += 'B';
    }

    // We'll operate in the tmp dir under a long sub folder (256 chars long)
    AStackString<> tmpPath1;
    AStackString<> tmpPath2;
    {
        VERIFY( FileIO::GetTempDir( tmpPath1 ) );
        tmpPath1 += "CoreTest_TestFileIO";
        tmpPath2.Format( "%s/%s", tmpPath1.Get(), a.Get() );
        TEST_ASSERT( tmpPath2.GetLength() > 260 );
    }

    // Create some file paths to work with
    AStackString<> filePathA;
    AStackString<> filePathB;
    AStackString<> subDir1;
    AStackString<> subDir2;
    AStackString<> filePathC;
    {
        // long file name A
        filePathA.Format( "%s/%s", tmpPath2.Get(), b.Get() );
        TEST_ASSERT( filePathA.GetLength() == ( tmpPath2.GetLength() + 1 + 255 ) );

        // long file name B
        filePathB = filePathA;
        filePathB.Trim( 0, 5 );
        filePathB += ".copy";
        TEST_ASSERT( filePathB.GetLength() == ( filePathA.GetLength() ) );
        
        // long subdir 1
        subDir1.Format( "%s/%s", tmpPath2.Get(), a.Get() );
        TEST_ASSERT( subDir1.GetLength() == ( tmpPath2.GetLength() + 1 + 255 ) );

        // long file name C
        filePathC.Format( "%s/%s", subDir1.Get(), b.Get() );
        TEST_ASSERT( filePathC.GetLength() == ( subDir1.GetLength() + 1 + 255 ) );

        // long subdir 2
        subDir2 = subDir1.Get();
        subDir2.Trim( 0, 2 );
        subDir2 += ".2";
        TEST_ASSERT( subDir2.GetLength() == subDir1.GetLength() );
    }

    // Cleanup anything left over from previous runs
    {
        FileIO::FileDelete( filePathA.Get() );
        FileIO::FileDelete( filePathB.Get() );
        FileIO::FileDelete( filePathC.Get() );
        FileIO::DirectoryDelete( subDir1 );
        FileIO::DirectoryDelete( subDir2 );
        FileIO::DirectoryDelete( tmpPath2 );
        FileIO::DirectoryDelete( tmpPath1 );
    }

    // EnsurePathExists (tmp)
    TEST_ASSERT( FileIO::EnsurePathExists( tmpPath2 ) );

    // Create a file (A)
    {
        FileStream f;
        TEST_ASSERT( f.Open( filePathA.Get(), FileStream::WRITE_ONLY ) );
    }

    // FileExists (A)
    TEST_ASSERT( FileIO::FileExists( filePathA.Get() ) );

    // Get/SetFileLastWriteTime
    {
        const uint64_t time = FileIO::GetFileLastWriteTime( filePathA );
        ASSERT( time != 0 );
        FileIO::SetFileLastWriteTime( filePathA, time );
    }

    // SetFileLastWriteTimeToNow
    TEST_ASSERT( FileIO::SetFileLastWriteTimeToNow( filePathA ) );

    // Get/SetReadOnly
    {
        TEST_ASSERT( FileIO::GetReadOnly( filePathA ) == false );
        TEST_ASSERT( FileIO::SetReadOnly( filePathA.Get(), true ) );
        TEST_ASSERT( FileIO::SetReadOnly( filePathA.Get(), false ) );
    }

    // EnsurePathExistsForFile (C) (subdir 1)
    TEST_ASSERT( FileIO::EnsurePathExistsForFile( filePathC ) );

    // FileCopy (A -> C)
    TEST_ASSERT( FileIO::FileCopy( filePathA.Get(), filePathC.Get() ) );

    // FileMove (C -> B)
    TEST_ASSERT( FileIO::FileExists( filePathC.Get() ) );
    TEST_ASSERT( FileIO::FileExists( filePathB.Get() ) == false );
    TEST_ASSERT( FileIO::FileMove( filePathC, filePathB ) );

    // DirectoryCreate (2)
    TEST_ASSERT( FileIO::DirectoryCreate( subDir2 ) );

    // DirectoryExists (2)
    TEST_ASSERT( FileIO::DirectoryExists( subDir2 ) );

    // GetFiles
    {
        StackArray<AString> files;
        TEST_ASSERT( FileIO::GetFiles( tmpPath1, AStackString<>( "*" ), true, &files ) );
        TEST_ASSERT( files.GetSize() == 2 );
        files.Sort();
        TEST_ASSERT( files[ 0 ].EndsWith( filePathB.FindLast( '/' ) + 1 ) );
        TEST_ASSERT( files[ 1 ].EndsWith( filePathA.FindLast( '/' ) + 1 ) );
    }

    // Get/SetCurrentDir
    {
        AStackString<> original;
        TEST_ASSERT( FileIO::GetCurrentDir( original ) );
        TEST_ASSERT( FileIO::SetCurrentDir( tmpPath2 ) );
        TEST_ASSERT( FileIO::SetCurrentDir( original ) );
    }

    // FileDelete
    TEST_ASSERT( FileIO::FileDelete( filePathA.Get() ) );
    TEST_ASSERT( FileIO::FileDelete( filePathB.Get() ) );

    // DirectoryDelete
    TEST_ASSERT( FileIO::DirectoryDelete( subDir1 ) );
    TEST_ASSERT( FileIO::DirectoryDelete( subDir2 ) );
    TEST_ASSERT( FileIO::DirectoryDelete( tmpPath2 ) );
    TEST_ASSERT( FileIO::DirectoryDelete( tmpPath1 ) );
}

// GenerateTempFileName
//------------------------------------------------------------------------------
void TestFileIO::GenerateTempFileName( AString & tmpFileName ) const
{
    // get system temp folder
    VERIFY( FileIO::GetTempDir( tmpFileName ) );

    // add process unique identifier
    AStackString<> buffer;
    buffer.Format( "TestFileIO.%u.%u", Process::GetCurrentId(), m_Random.GetRand() );
    tmpFileName += buffer;
}

//------------------------------------------------------------------------------
