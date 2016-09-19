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
#include "Core/FileIO/FileStream.h"
#if defined( __OSX__ )
    #include "Core/Process/Thread.h"
#endif
#include "Core/Strings/AStackString.h"

// TestObject
//------------------------------------------------------------------------------
class TestObject : public FBuildTest
{
private:
    DECLARE_TESTS

    // Tests
    void MSVCArgHelpers() const;
    void TestStaleDynamicDeps() const;
};

// Register Tests
//------------------------------------------------------------------------------
REGISTER_TESTS_BEGIN( TestObject )
    REGISTER_TEST( MSVCArgHelpers )             // Test functions that check for MSVC args
    REGISTER_TEST( TestStaleDynamicDeps )       // Test dynamic deps are cleared when necessary
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

// TestStaleDynamicDeps
//------------------------------------------------------------------------------
void TestObject::TestStaleDynamicDeps() const
{
    const char* fileA = "../../../../tmp/Test/Object/StaleDynamicDeps/GeneratedInput/FileA.h";
    const char* fileB = "../../../../tmp/Test/Object/StaleDynamicDeps/GeneratedInput/FileB.h";
    const char* fileC = "../../../../tmp/Test/Object/StaleDynamicDeps/GeneratedInput/FileC.h";
    const char* database = "../../../../tmp/Test/Object/StaleDynamicDeps/fbuild.fdb";

    // Generate some header files
    {
        // Need FBuild for CleanPath
        FBuildOptions options;
        FBuild fBuild( options );

        // Ensure output path exists
        AStackString<> fullOutputPath;
        NodeGraph::CleanPath( AStackString<>( fileA ), fullOutputPath );
        TEST_ASSERT( Node::EnsurePathExistsForFile( fullOutputPath ) );

        // Create files
        FileStream f;
        TEST_ASSERT( f.Open( fileA, FileStream::WRITE_ONLY ) );
        f.Close();
        TEST_ASSERT( f.Open( fileB, FileStream::WRITE_ONLY ) );
        f.Close();
        TEST_ASSERT( f.Open( fileC, FileStream::WRITE_ONLY ) );
        f.Close();
    }

    // Build CPP Generator
    {
        // Init
        FBuildOptions options;
        options.m_ConfigFile = "Data/TestObject/StaleDynamicDeps/cppgenerator.bff";
        options.m_ForceCleanBuild = true;
        FBuild fBuild( options );
        TEST_ASSERT( fBuild.Initialize() );

        // Compile
        TEST_ASSERT( fBuild.Build( AStackString<>( "CPPGenerator" ) ) );
    }

    // Build using CPP Generator (clean)
    {
        // Init
        FBuildOptions options;
        options.m_ConfigFile = "Data/TestObject/StaleDynamicDeps/staledeps.bff";
        options.m_ForceCleanBuild = true;
        options.m_ShowSummary = true; // required to generate stats for node count checks
        FBuild fBuild( options );
        TEST_ASSERT( fBuild.Initialize() );

        // Compile
        TEST_ASSERT( fBuild.Build( AStackString<>( "StaleDynamicDeps" ) ) );

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

    // Work around poor time resolution of file system on OSX by waiting at least 1 second
    // TODO:C Changes to the way dependencies are managed might make this unnecessary
    #if defined( __OSX__ )
        Thread::Sleep(1001);
    #endif

    // Build Again
    {
        // Init
        FBuildOptions options;
        options.m_ConfigFile = "Data/TestObject/StaleDynamicDeps/staledeps.bff";
        options.m_ShowSummary = true; // required to generate stats for node count checks
        FBuild fBuild( options );
        TEST_ASSERT( fBuild.Initialize( database ) );

        // Compile
        TEST_ASSERT( fBuild.Build( AStackString<>( "StaleDynamicDeps" ) ) );

        // Check stats
        //               Seen,  Built,  Type
        CheckStatsNode ( 1,     1,      Node::DIRECTORY_LIST_NODE );
        CheckStatsNode ( 2,     0,      Node::COMPILER_NODE );
        CheckStatsNode ( 3,     1,      Node::OBJECT_NODE ); // 3xCPPGen + 1xUnity, rebuild of unity
    }
}

//------------------------------------------------------------------------------
