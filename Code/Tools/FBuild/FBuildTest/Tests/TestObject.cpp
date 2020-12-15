// TestObject.cpp
//------------------------------------------------------------------------------

// Includes
//------------------------------------------------------------------------------
#include "FBuildTest.h"

// FBuildCore
#include "Tools/FBuild/FBuildCore/BFF/BFFParser.h"
#include "Tools/FBuild/FBuildCore/FBuild.h"
#include "Tools/FBuild/FBuildCore/Graph/NodeGraph.h"
#include "Tools/FBuild/FBuildCore/Graph/ObjectNode.h"

// Core
#include "Core/FileIO/FileIO.h"
#include "Core/FileIO/FileStream.h"
#include "Core/FileIO/PathUtils.h"
#include "Core/Process/Thread.h"
#include "Core/Strings/AStackString.h"

// TestObject
//------------------------------------------------------------------------------
class TestObject : public FBuildTest
{
private:
    DECLARE_TESTS

    // Tests
    void MSVCArgHelpers() const;
    void Preprocessor() const;
    void TestStaleDynamicDeps() const;
    void ModTimeChangeBackwards() const;
    void CacheUsingRelativePaths() const;
    void SourceMapping() const;
};

// Register Tests
//------------------------------------------------------------------------------
REGISTER_TESTS_BEGIN( TestObject )
    REGISTER_TEST( MSVCArgHelpers )             // Test functions that check for MSVC args
    REGISTER_TEST( Preprocessor )
    REGISTER_TEST( TestStaleDynamicDeps )       // Test dynamic deps are cleared when necessary
    REGISTER_TEST( ModTimeChangeBackwards )
    REGISTER_TEST( CacheUsingRelativePaths )
    REGISTER_TEST( SourceMapping )
REGISTER_TESTS_END

// MSVCArgHelpers
//------------------------------------------------------------------------------
void TestObject::MSVCArgHelpers() const
{
    // Exact match args, using /
    {
        AStackString<> token( "/Zi" );
        TEST_ASSERT( ObjectNode::IsCompilerArg_MSVC( token, "Zi" ) );
    }

    // Exact match args, using -
    {
        AStackString<> token( "-Zi" );
        TEST_ASSERT( ObjectNode::IsCompilerArg_MSVC( token, "Zi" ) );
    }

    // Starts with args, using /
    {
        AStackString<> token( "/Ipath/path" );
        TEST_ASSERT( ObjectNode::IsStartOfCompilerArg_MSVC( token, "I" ) );
    }

    // Starts with args, using -
    {
        AStackString<> token( "-Ipath/path" );
        TEST_ASSERT( ObjectNode::IsStartOfCompilerArg_MSVC( token, "I" ) );
    }
}

// Preprocessor
//------------------------------------------------------------------------------
void TestObject::Preprocessor() const
{
    const char * configFile = "Tools/FBuild/FBuildTest/Data/TestObject/CustomPreprocessor/custompreprocessor.bff";
    const char * database = "../tmp/Test/Object/CustomPreprocessor/fbuild.fdb";

    // Build
    {
        // Init
        FBuildTestOptions options;
        options.m_ConfigFile = configFile;
        options.m_ForceCleanBuild = true;
        FBuild fBuild( options );
        TEST_ASSERT( fBuild.Initialize() );

        // Compile
        TEST_ASSERT( fBuild.Build( "CustomPreprocessor" ) );
        fBuild.SaveDependencyGraph( database );

        // Check stats
        //               Seen,  Built,  Type
        CheckStatsNode ( 1,     1,      Node::COMPILER_NODE );
        CheckStatsNode ( 1,     1,      Node::OBJECT_NODE ); // 1x cpp
    }

    // No Rebuild
    {
        // Init
        FBuildTestOptions options;
        options.m_ConfigFile = configFile;
        FBuild fBuild( options );
        TEST_ASSERT( fBuild.Initialize( database ) );

        // Compile
        TEST_ASSERT( fBuild.Build( "CustomPreprocessor" ) );

        // Check stats
        //               Seen,  Built,  Type
        CheckStatsNode ( 1,     0,      Node::COMPILER_NODE );
        CheckStatsNode ( 1,     0,      Node::OBJECT_NODE ); // 1x cpp
    }
}

// TestStaleDynamicDeps
//------------------------------------------------------------------------------
void TestObject::TestStaleDynamicDeps() const
{
    const char* fileA = "../tmp/Test/Object/StaleDynamicDeps/GeneratedInput/FileA.h";
    const char* fileB = "../tmp/Test/Object/StaleDynamicDeps/GeneratedInput/FileB.h";
    const char* fileC = "../tmp/Test/Object/StaleDynamicDeps/GeneratedInput/FileC.h";
    const char* database = "../tmp/Test/Object/StaleDynamicDeps/fbuild.fdb";

    // Build CPP Generator
    {
        // Init
        FBuildTestOptions options;
        options.m_ConfigFile = "Tools/FBuild/FBuildTest/Data/TestObject/StaleDynamicDeps/cppgenerator.bff";
        options.m_ForceCleanBuild = true;
        FBuild fBuild( options );
        TEST_ASSERT( fBuild.Initialize() );

        // Generate some header files
        EnsureDirExists( "../tmp/Test/Object/StaleDynamicDeps/GeneratedInput/" );
        FileStream f;
        TEST_ASSERT( f.Open( fileA, FileStream::WRITE_ONLY ) );
        f.Close();
        TEST_ASSERT( f.Open( fileB, FileStream::WRITE_ONLY ) );
        f.Close();
        TEST_ASSERT( f.Open( fileC, FileStream::WRITE_ONLY ) );
        f.Close();

        // Compile
        TEST_ASSERT( fBuild.Build( "CPPGenerator" ) );
    }

    // Build using CPP Generator (clean)
    {
        // Init
        FBuildTestOptions options;
        options.m_ConfigFile = "Tools/FBuild/FBuildTest/Data/TestObject/StaleDynamicDeps/staledeps.bff";
        options.m_ForceCleanBuild = true;
        FBuild fBuild( options );
        TEST_ASSERT( fBuild.Initialize() );

        // Compile
        TEST_ASSERT( fBuild.Build( "StaleDynamicDeps" ) );

        // Save DB
        TEST_ASSERT( fBuild.SaveDependencyGraph( database ) );

        // Check stats
        //               Seen,  Built,  Type
        CheckStatsNode ( 1,     1,      Node::DIRECTORY_LIST_NODE );
        CheckStatsNode ( 2,     2,      Node::COMPILER_NODE );
        CheckStatsNode ( 4,     4,      Node::OBJECT_NODE ); // 3xCPPGen + 1xUnity

    }

    // Delete one of the generated headers
    EnsureFileDoesNotExist( fileB );

    // TODO:B Get rid of this (needed to work around poor filetime granularity)
    #if defined( __OSX__ )
        Thread::Sleep( 1000 ); // Work around low time resolution of HFS+
    #endif

    // Build Again
    {
        // Init
        FBuildTestOptions options;
        options.m_ConfigFile = "Tools/FBuild/FBuildTest/Data/TestObject/StaleDynamicDeps/staledeps.bff";
        FBuild fBuild( options );
        TEST_ASSERT( fBuild.Initialize( database ) );

        // Compile
        TEST_ASSERT( fBuild.Build( "StaleDynamicDeps" ) );

        // Check stats
        //               Seen,  Built,  Type
        CheckStatsNode ( 1,     1,      Node::DIRECTORY_LIST_NODE );
        CheckStatsNode ( 2,     0,      Node::COMPILER_NODE );
        CheckStatsNode ( 3,     1,      Node::OBJECT_NODE ); // 3xCPPGen + 1xUnity, rebuild of unity
    }
}

// ModTimeChangeBackwards
//------------------------------------------------------------------------------
//  - Ensure a file rebuilds if the time changes into the past
void TestObject::ModTimeChangeBackwards() const
{
    const AStackString<> fileA( "../tmp/Test/Object/ModTimeChangeBackwards/GeneratedInput/FileA.cpp" );
    const AStackString<> fileB( "../tmp/Test/Object/ModTimeChangeBackwards/GeneratedInput/FileB.cpp" );
    const char * database = "../tmp/Test/Object/ModTimeChangeBackwards/fbuild.fdb";

    // Generate full path file fileA
    AStackString<> fileAFullPath;
    {
        FileIO::GetCurrentDir( fileAFullPath );
        fileAFullPath += '/';
        fileAFullPath += fileA;
        PathUtils::FixupFilePath( fileAFullPath );
    }

    // Create two empty files
    uint64_t oldModTime;
    {
        // Generate some header files
        EnsureDirExists( "../tmp/Test/Object/ModTimeChangeBackwards/GeneratedInput/" );
        FileStream f;
        TEST_ASSERT( f.Open( fileA.Get(), FileStream::WRITE_ONLY ) );
        f.Close();
        TEST_ASSERT( f.Open( fileB.Get(), FileStream::WRITE_ONLY ) );
        f.Close();

        // Take note of FileA's original time
        oldModTime = FileIO::GetFileLastWriteTime( fileAFullPath );

        // Modify FileA time (jump through hoops to handle poor filetime granularity)
        Timer timeout;
        for ( ;; )
        {
            TEST_ASSERT( timeout.GetElapsed() < 30.0f );

            Thread::Sleep( 10 );

            TEST_ASSERT( FileIO::SetFileLastWriteTimeToNow( fileAFullPath ) );
            if ( FileIO::GetFileLastWriteTime( fileAFullPath ) != oldModTime )
            {
                break;
            }
        }
    }

    // Compile library for the two files
    {
        // Init
        FBuildTestOptions options;
        options.m_ConfigFile = "Tools/FBuild/FBuildTest/Data/TestObject/ModTimeChangeBackwards/fbuild.bff";
        options.m_ForceCleanBuild = true;
        FBuild fBuild( options );
        TEST_ASSERT( fBuild.Initialize() );

        // Compile
        TEST_ASSERT( fBuild.Build( "ModTimeChangeBackwards" ) );

        // Save DB
        TEST_ASSERT( fBuild.SaveDependencyGraph( database ) );

        // Check stats
        //              Seen,   Built,  Type
        CheckStatsNode( 1,      1,      Node::DIRECTORY_LIST_NODE );
        CheckStatsNode( 1,      1,      Node::COMPILER_NODE );
        CheckStatsNode( 2,      2,      Node::OBJECT_NODE );
        CheckStatsNode( 1,      1,      Node::LIBRARY_NODE );
    }

    // Change modtime into the past
    TEST_ASSERT( FileIO::SetFileLastWriteTime( fileAFullPath, oldModTime ) );

    // Because of poor filetime granularity on OSX (pre-APFS) the object A we will rebuild
    // below could have an unchanged modtime. To work around this, we must wait until enough
    // time has elapsed to ensure the modtime will be different.
    // TODO:B Find a better solve for this. While unlikely, this could happen in real-world use
#if defined( __OSX__ )
    Thread::Sleep( 1000 ); // Work around low time resolution of HFS+
#endif

    // Compile library again
    {
        // Init
        FBuildTestOptions options;
        options.m_ConfigFile = "Tools/FBuild/FBuildTest/Data/BuildAndLinkLibrary/DeleteFile/fbuild.bff";
        FBuild fBuild( options );
        TEST_ASSERT( fBuild.Initialize( database ) );

        // Compile
        TEST_ASSERT( fBuild.Build( "ModTimeChangeBackwards" ) );

        // Save DB
        TEST_ASSERT( fBuild.SaveDependencyGraph( database ) );

        // Check stats
        //              Seen,   Built,  Type
        CheckStatsNode( 1,      1,      Node::DIRECTORY_LIST_NODE );
        CheckStatsNode( 1,      0,      Node::COMPILER_NODE );
        CheckStatsNode( 2,      1,      Node::OBJECT_NODE );    // Note: One object rebuilds
        CheckStatsNode( 1,      1,      Node::LIBRARY_NODE );   // Note: library rebuilds
    }

    // Ensure no rebuild
    {
        // Init
        FBuildTestOptions options;
        options.m_ConfigFile = "Tools/FBuild/FBuildTest/Data/BuildAndLinkLibrary/DeleteFile/fbuild.bff";
        FBuild fBuild( options );
        TEST_ASSERT( fBuild.Initialize( database ) );

        // Compile
        TEST_ASSERT( fBuild.Build( "ModTimeChangeBackwards" ) );

        // Check stats
        //              Seen,   Built,  Type
        CheckStatsNode( 1,      1,      Node::DIRECTORY_LIST_NODE );
        CheckStatsNode( 1,      0,      Node::COMPILER_NODE );
        CheckStatsNode( 2,      0,      Node::OBJECT_NODE );
        CheckStatsNode( 1,      0,      Node::LIBRARY_NODE );
    }
}

// CacheUsingRelativePaths
//------------------------------------------------------------------------------
void TestObject::CacheUsingRelativePaths() const
{
    // Source files
    const char * srcPath = "Tools/FBuild/FBuildTest/Data/TestObject/CacheUsingRelativePaths/";
    const char * fileA = "File.cpp";
    const char * fileB = "Subdir/Header.h";
    const char * fileC = "fbuild.bff";
    const char * files[] = { fileA, fileB, fileC };

    // Dest paths
    const char * dstPathA = "../tmp/Test/Object/CacheUsingRelativePaths/A/Code";
    const char * dstPathB = "../tmp/Test/Object/CacheUsingRelativePaths/B/Code";
    const char * dstPaths[] = { dstPathA, dstPathB };

    #if defined( __WINDOWS__ )
        const char * objFileA = "../tmp/Test/Object/CacheUsingRelativePaths/A/out/File.obj";
    #else
        const char * objFileA = "../tmp/Test/Object/CacheUsingRelativePaths/A/out/File.o";
    #endif

    // Copy file structure to both destinations
    for ( const char * dstPath : dstPaths )
    {
        for ( const char * file : files )
        {
            AStackString<> src, dst;
            src.Format( "%s/%s", srcPath, file );
            dst.Format( "%s/%s", dstPath, file );
            TEST_ASSERT( FileIO::EnsurePathExistsForFile( dst ) );
            TEST_ASSERT( FileIO::FileCopy( src.Get(), dst.Get() ) );
        }
    }

    // Build in path A, writing to the cache
    {
        // Init
        FBuildTestOptions options;
        options.m_ConfigFile = "fbuild.bff";
        options.m_UseCacheWrite = true;
        //options.m_ForceCleanBuild = true;
        AStackString<> codeDir;
        GetCodeDir( codeDir );
        codeDir.Trim( 0, 5 ); // Remove Code/
        codeDir += "tmp/Test/Object/CacheUsingRelativePaths/A/Code/";
        options.SetWorkingDir( codeDir );
        FBuild fBuild( options );
        TEST_ASSERT( fBuild.Initialize() );

        // Compile
        TEST_ASSERT( fBuild.Build( AStackString<>( "ObjectList" ) ) );

        TEST_ASSERT( fBuild.GetStats().GetCacheStores() == 1 );
    }

    // Check some problematic cases in the object file
    {
        // Read obj file into memory
        AString buffer;
        {
            FileStream f;
            TEST_ASSERT( f.Open( objFileA ) );
            buffer.SetLength( (uint32_t)f.GetFileSize() );
            TEST_ASSERT( f.ReadBuffer( buffer.Get(), f.GetFileSize() ) == f.GetFileSize() );
            buffer.Replace( (char)0, ' ' ); // Make string seaches simpler
        }

        // Check __FILE__ paths are relative
        TEST_ASSERT( buffer.Find( "FILE_MACRO_START_1(./Subdir/Header.h)FILE_MACRO_END_1" ) );
        TEST_ASSERT( buffer.Find( "FILE_MACRO_START_2(File.cpp)FILE_MACRO_END_2" ) );
    }

    // Build in path B, reading from the cache
    {
        // Init
        FBuildTestOptions options;
        options.m_ConfigFile = "fbuild.bff";
        options.m_UseCacheRead = true;
        //options.m_ForceCleanBuild = true;
        AStackString<> codeDir;
        GetCodeDir( codeDir );
        codeDir.Trim( 0, 5 ); // Remove Code/
        codeDir += "tmp/Test/Object/CacheUsingRelativePaths/B/Code/";
        options.SetWorkingDir( codeDir );
        FBuild fBuild( options );
        TEST_ASSERT( fBuild.Initialize() );

        // Compile
        TEST_ASSERT( fBuild.Build( AStackString<>( "ObjectList" ) ) );

        TEST_ASSERT( fBuild.GetStats().GetCacheHits() == 1 );
    }
}

// SourceMapping
//------------------------------------------------------------------------------
void TestObject::SourceMapping() const
{
    // Source files
    const char * srcPath = "Tools/FBuild/FBuildTest/Data/TestObject/SourceMapping/";
    const char * fileA = "File.cpp";
    const char * fileB = "fbuild.bff";
    const char * files[] = { fileA, fileB };

    // Dest paths
    const char * dstPath = "../tmp/Test/Object/SourceMapping/Code";

    #if defined( __WINDOWS__ )
        const char * objFile = "../tmp/Test/Object/SourceMapping/out/File.obj";
    #else
        const char * objFile = "../tmp/Test/Object/SourceMapping/out/File.o";
    #endif

    // Copy file structure to destination
    for ( const char * file : files )
    {
        AStackString<> src, dst;
        src.Format( "%s/%s", srcPath, file );
        dst.Format( "%s/%s", dstPath, file );
        TEST_ASSERT( FileIO::EnsurePathExistsForFile( dst ) );
        TEST_ASSERT( FileIO::FileCopy( src.Get(), dst.Get() ) );
    }

    // Build in destination path
    {
        // Init
        FBuildTestOptions options;
        options.m_ConfigFile = "fbuild.bff";
        AStackString<> codeDir;
        GetCodeDir( codeDir );
        codeDir.Trim( 0, 5 ); // Remove Code/
        codeDir += "tmp/Test/Object/SourceMapping/Code/";
        options.SetWorkingDir( codeDir );
        FBuild fBuild( options );
        TEST_ASSERT( fBuild.Initialize() );

        // Compile
        TEST_ASSERT( fBuild.Build( AStackString<>( "ObjectList" ) ) );
    }

    // Check the object file to make sure the debugging information has been remapped
    {
        // Read obj file into memory
        AString buffer;
        {
            FileStream f;
            TEST_ASSERT( f.Open( objFile ) );
            buffer.SetLength( (uint32_t)f.GetFileSize() );
            TEST_ASSERT( f.ReadBuffer( buffer.Get(), f.GetFileSize() ) == f.GetFileSize() );
            buffer.Replace( (char)0, ' ' ); // Make string seaches simpler
        }

        TEST_ASSERT( buffer.Find( "/fastbuild-test-mapping" ) );
    }
}

//------------------------------------------------------------------------------
