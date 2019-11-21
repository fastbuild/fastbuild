// TestObject.cpp
//------------------------------------------------------------------------------

// Includes
//------------------------------------------------------------------------------
#include "FBuildTest.h"

// FBuildCore
#include "Tools/FBuild/FBuildCore/FBuild.h"
#include "Tools/FBuild/FBuildCore/BFF/BFFParser.h"
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
};

// Register Tests
//------------------------------------------------------------------------------
REGISTER_TESTS_BEGIN( TestObject )
    REGISTER_TEST( MSVCArgHelpers )             // Test functions that check for MSVC args
    REGISTER_TEST( Preprocessor )
    REGISTER_TEST( TestStaleDynamicDeps )       // Test dynamic deps are cleared when necessary
    REGISTER_TEST( ModTimeChangeBackwards )
REGISTER_TESTS_END

// MSVCArgHelpers
//------------------------------------------------------------------------------
void TestObject::MSVCArgHelpers() const
{
    // Exact match args, using /
    {
        AStackString<> token( "/Zi" );
        TEST_ASSERT( ObjectNode::IsCompilerArg_MSVC( token, "Zi" ) )
    }

    // Exact match args, using -
    {
        AStackString<> token( "-Zi" );
        TEST_ASSERT( ObjectNode::IsCompilerArg_MSVC( token, "Zi" ) )
    }

    // Starts with args, using /
    {
        AStackString<> token( "/Ipath/path" );
        TEST_ASSERT( ObjectNode::IsStartOfCompilerArg_MSVC( token, "I" ) )
    }

    // Starts with args, using -
    {
        AStackString<> token( "-Ipath/path" );
        TEST_ASSERT( ObjectNode::IsStartOfCompilerArg_MSVC( token, "I" ) )
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
    EnsureFileDoesNotExist(fileB);

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
    const char* database = "../tmp/Test/Object/ModTimeChangeBackwards/fbuild.fdb";

    // Create two empty files
    {
        // Generate some header files
        EnsureDirExists( "../tmp/Test/Object/ModTimeChangeBackwards/GeneratedInput/" );
        FileStream f;
        TEST_ASSERT( f.Open( fileA.Get(), FileStream::WRITE_ONLY ) );
        f.Close();
        TEST_ASSERT( f.Open( fileB.Get(), FileStream::WRITE_ONLY ) );
        f.Close();
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
    {
        AStackString<> fileAFullPath;
        FileIO::GetCurrentDir( fileAFullPath );
        fileAFullPath += '/';
        fileAFullPath += fileA;
        PathUtils::FixupFilePath( fileAFullPath );
        const uint64_t newModTime = FileIO::GetFileLastWriteTime( fileA ) - 1000000000ULL;
        TEST_ASSERT( FileIO::SetFileLastWriteTime( fileAFullPath, newModTime ) );
    }

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

//------------------------------------------------------------------------------
