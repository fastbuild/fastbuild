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

// TestFileIO
//------------------------------------------------------------------------------
class TestFileIO : public UnitTest
{
private:
    DECLARE_TESTS

    void FileExists() const;
    void FileDelete() const;
    void FileCopy() const;
    void FileMove() const;

    void ReadOnly() const;

    void FileTime() const;

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
    REGISTER_TEST( FileMove )
    REGISTER_TEST( ReadOnly )
    REGISTER_TEST( FileTime )
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

    // wait for some time that is bigger than filesystem time granularity
    #if defined( __OSX__ )
        // HFS+ has surprisingly poor time resolution (1 second)
        Thread::Sleep( 1100 );
    #else
        Thread::Sleep( 500 );
    #endif

    // modify file
    FileStream f2;
    TEST_ASSERT( f.Open( path.Get(), FileStream::WRITE_ONLY ) == true );
    f.Write( (uint32_t)0 );
    f.Close();

    // get new last write time
    const uint64_t newTime = FileIO::GetFileLastWriteTime( path );
    TEST_ASSERT( newTime > oldTime );

    // manually set time back
    TEST_ASSERT( FileIO::SetFileLastWriteTime( path, oldTime ) == true );
    uint64_t timeNow = FileIO::GetFileLastWriteTime( path );
    TEST_ASSERT( timeNow == oldTime );
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
