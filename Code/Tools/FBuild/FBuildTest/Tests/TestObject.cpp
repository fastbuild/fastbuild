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
#include "Core/Process/Thread.h"
#include "Core/Strings/AStackString.h"
#include "Core/Time/Time.h"

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
	void TestDynamicDeps() const;
};

// Register Tests
//------------------------------------------------------------------------------
REGISTER_TESTS_BEGIN( TestObject )
    REGISTER_TEST( MSVCArgHelpers )             // Test functions that check for MSVC args
    REGISTER_TEST( Preprocessor )
    REGISTER_TEST( TestStaleDynamicDeps )       // Test dynamic deps are cleared when necessary
	REGISTER_TEST( TestDynamicDeps )			// Test dynamic dependency gathering using a preprocessor
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
        TEST_ASSERT( fBuild.Build( AStackString<>( "CustomPreprocessor" ) ) );
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
        TEST_ASSERT( fBuild.Build( AStackString<>( "CustomPreprocessor" ) ) );

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
        TEST_ASSERT( fBuild.Build( AStackString<>( "CPPGenerator" ) ) );
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
        TEST_ASSERT( fBuild.Build( AStackString<>( "StaleDynamicDeps" ) ) );

        // Save DB
        TEST_ASSERT( fBuild.SaveDependencyGraph( database ) );

        // Check stats
        //               Seen,  Built,  Type
        CheckStatsNode ( 1,     1,      Node::DIRECTORY_LIST_NODE );
        CheckStatsNode ( 2,     2,      Node::COMPILER_NODE );
        CheckStatsNode ( 4,     4,      Node::OBJECT_NODE ); // 3xCPPGen + 1xUnity

        // Delete one of the generated headers
        EnsureFileDoesNotExist( fileB );
    }

    // TODO:C Changes to the way dependencies are managed might make this unnecessary
    #if defined( __OSX__ )
        Thread::Sleep( 1000 ); // Work around low time resolution of HFS+
    #elif defined( __LINUX__ )
        Thread::Sleep( 1000 ); // Work around low time resolution of ext2/ext3/reiserfs and time caching used by used by others
    #endif

    // Build Again
    {
        // Init
        FBuildTestOptions options;
        options.m_ConfigFile = "Tools/FBuild/FBuildTest/Data/TestObject/StaleDynamicDeps/staledeps.bff";
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

// TestDynamicDeps
//
// Dynamic Dependencies can be gathered using a preprocessor executable that
// will list all dependent targets (FileNodes) on stdout. The build will ensure
// all listed dependent targets are up to date before finishing the execution 
// of the function.
//------------------------------------------------------------------------------
void TestObject::TestDynamicDeps() const
{
	const char* fileA = "../tmp/Test/Object/DynamicDeps/PossibleDependencies/a.h";
	const char* fileB = "../tmp/Test/Object/DynamicDeps/PossibleDependencies/b.h";
	const char* fileC = "../tmp/Test/Object/DynamicDeps/PossibleDependencies/c.h";
	const char* database = "../tmp/Test/Object/DynamicDeps/fbuild.fdb";

	AString DependencyProcessorMainCPP("Tools/FBuild/FBuildTest/Data/TestObject/DynamicDeps/DependencyProcessorMain.cpp");
	AString DependencyProcessorMainCPPTemp("../tmp/Test/Object/DynamicDeps/DependencyProcessorMain.cpp");

	// Build 
	{
		// Init
		FBuildTestOptions options;
		options.m_ConfigFile = "Tools/FBuild/FBuildTest/Data/TestObject/DynamicDeps/dynamicdeps.bff";
		options.m_ForceCleanBuild = true;
		FBuild fBuild(options);
		TEST_ASSERT(fBuild.Initialize());

		// Copy our pre build preprocessor source to tmp directory
		FileIO::FileCopy(DependencyProcessorMainCPP.Get(), DependencyProcessorMainCPPTemp.Get(), true);
		
		// Generate a few header files which may be dependencies for our cpp file.
		EnsureDirExists("../tmp/Test/Object/DynamicDeps/PossibleDependencies/");
		FileStream f;
		TEST_ASSERT(f.Open(fileA, FileStream::WRITE_ONLY));
		f.Close();
		TEST_ASSERT(f.Open(fileB, FileStream::WRITE_ONLY));
		f.Close();
		TEST_ASSERT(f.Open(fileC, FileStream::WRITE_ONLY));
		f.Close();

		// Make sure none of the possible dependencies exist.
		EnsureFileDoesNotExist("../tmp/Test/Object/DynamicDeps/Dependencies/a.h");
		EnsureFileDoesNotExist("../tmp/Test/Object/DynamicDeps/Dependencies/b.h");
		EnsureFileDoesNotExist("../tmp/Test/Object/DynamicDeps/Dependencies/c.h");
		EnsureDirDoesNotExist("../tmp/Test/Object/DynamicDeps/Dependencies/");
		EnsureFileDoesNotExist("../tmp/Test/Object/DynamicDeps/DependencyProcessor.exe");

		// Compile
		// This will first search for dependent targets and build them if they are out of date.
		// This will dynamically create a PreBuildDependencies list and execute them.
		// It will then run the preprocessor and the compiler on this target.
		TEST_ASSERT(fBuild.Build(AStackString<>("DynamicDeps")));

		// Save DB
		TEST_ASSERT(fBuild.SaveDependencyGraph(database));

		// Check stats
		//               Seen,  Built,  Type
		CheckStatsNode(2, 2, Node::COMPILER_NODE); // 1x compiler, 1x pre build preprocessor
		CheckStatsNode(1, 1, Node::OBJECT_LIST_NODE);
		CheckStatsNode(2, 2, Node::COPY_FILE_NODE);	// Three copy files exist (A,B,C) but only two are dependencies and should have been seen or built

		// Make sure we didn't copy the third file, which wasn't a dependency
		TEST_ASSERT(FileIO::FileExists("../tmp/Test/Object/DynamicDeps/Dependencies/c.h") == false);
	}

	// Build Again, should be up to date
	{
		// Init
		FBuildTestOptions options;
		options.m_ConfigFile = "Tools/FBuild/FBuildTest/Data/TestObject/DynamicDeps/dynamicdeps.bff";
		FBuild fBuild(options);
		TEST_ASSERT(fBuild.Initialize(database));

		// Compile
		TEST_ASSERT(fBuild.Build(AStackString<>("DynamicDeps")));

		// Check stats
		//               Seen,  Built,  Type
		CheckStatsNode(1, 0, Node::OBJECT_LIST_NODE);
		CheckStatsNode(2, 0, Node::COPY_FILE_NODE);
	}


	// Build Again, with one dependency now missing
	{
		// Init
		FBuildTestOptions options;
		options.m_ConfigFile = "Tools/FBuild/FBuildTest/Data/TestObject/DynamicDeps/dynamicdeps.bff";
		FBuild fBuild(options);
		TEST_ASSERT(fBuild.Initialize(database));

		EnsureFileDoesNotExist("../tmp/Test/Object/DynamicDeps/Dependencies/b.h");

		// Compile
		TEST_ASSERT(fBuild.Build(AStackString<>("DynamicDeps")));

		// Check stats
		//               Seen,  Built,  Type
		CheckStatsNode(2, 1, Node::COPY_FILE_NODE);	// two dependencies, but only one is stale (missing) and needs to be recreated
	}

	// Build Again, with the preprocessor needing a rebuild
	{
		// Init
		FBuildTestOptions options;
		options.m_ConfigFile = "Tools/FBuild/FBuildTest/Data/TestObject/DynamicDeps/dynamicdeps.bff";
		FBuild fBuild(options);
		TEST_ASSERT(fBuild.Initialize(database));

		// Copy our pre build preprocessor source to tmp directory again, this should trigger a rebuild of the preprocessor
		// and force all dependencies to be re-evaluated.
#if defined( __WINDOWS__ )
		FileIO::SetFileLastWriteTime(DependencyProcessorMainCPPTemp, Time::GetCurrentFileTime());
#elif defined( __APPLE__ ) || defined( __LINUX__ )
		utimes(fileNames[i].Get(), nullptr);
#endif

		// Compile
		TEST_ASSERT(fBuild.Build(AStackString<>("DynamicDeps")));

		// Check stats
		//               Seen,  Built,  Type
		CheckStatsNode(2, 1, Node::COMPILER_NODE);  // preprocessor compiler should be rebuilt
		CheckStatsNode(2, 0, Node::COPY_FILE_NODE);	// although the preprocessor should get run again, it shouldn't need to rebuild any of the dependencies
	}
}


//------------------------------------------------------------------------------
