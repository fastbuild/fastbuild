// TestLinker.cpp
//------------------------------------------------------------------------------

// Includes
//------------------------------------------------------------------------------
#include "FBuildTest.h"

// FBuildCore
#include "Tools/FBuild/FBuildCore/BFF/LinkerNodeFileExistsCache.h"
#include "Tools/FBuild/FBuildCore/FBuild.h"
#include "Tools/FBuild/FBuildCore/Graph/Dependencies.h"
#include "Tools/FBuild/FBuildCore/Graph/LinkerNode.h"
#include "Tools/FBuild/FBuildCore/Graph/NodeGraph.h"

// Core
#include "Core/FileIO/FileIO.h"
#include "Core/Strings/AStackString.h"

// TestLinker
//------------------------------------------------------------------------------
class TestLinker : public FBuildTest
{
private:
    DECLARE_TESTS

    // Tests
    void ArgHelpers() const;
    void ArgHelpers_MSVC() const;
    void LibrariesOnCommandLine() const;
    void IncrementalLinking_MSVC() const;
    void LinkerType() const;
};

// Register Tests
//------------------------------------------------------------------------------
REGISTER_TESTS_BEGIN( TestLinker )
    REGISTER_TEST( ArgHelpers )                 // Test functions that check for non-MSVC args
    REGISTER_TEST( ArgHelpers_MSVC )            // Test functions that check for MSVC args
    REGISTER_TEST( LibrariesOnCommandLine )     // Discovery of additional libraries on command line
    #if defined( __WINDOWS__ )
        REGISTER_TEST( IncrementalLinking_MSVC )
    #endif
    REGISTER_TEST( LinkerType )                 // Test linker detection code
REGISTER_TESTS_END

// ArgHelpers
//------------------------------------------------------------------------------
void TestLinker::ArgHelpers() const
{
    // Exact match
    {
        AStackString<> token( "-L" );
        TEST_ASSERT( LinkerNode::IsStartOfLinkerArg( token, "L" ) );
    }

    // Starts with
    {
        AStackString<> token( "-Lthing" );
        TEST_ASSERT( LinkerNode::IsStartOfLinkerArg( token, "L" ) );
    }

    // Check case sensitive is respected
    {
        AStackString<> token( "-l" );
        TEST_ASSERT( LinkerNode::IsStartOfLinkerArg( token, "L" ) == false );
    }
}

// ArgHelpers_MSVC
//------------------------------------------------------------------------------
void TestLinker::ArgHelpers_MSVC() const
{
    // Exact match args, using /
    {
        AStackString<> token( "/DLL" );
        TEST_ASSERT( LinkerNode::IsLinkerArg_MSVC( token, "DLL" ) );
    }

    // Exact match args, using -
    {
        AStackString<> token( "-DLL" );
        TEST_ASSERT( LinkerNode::IsLinkerArg_MSVC( token, "DLL" ) );
    }

    // Exact match args, lower-case, using /
    {
        AStackString<> token( "/dll" );
        TEST_ASSERT( LinkerNode::IsLinkerArg_MSVC( token, "DLL" ) );
    }

    // Exact match args, lower-case, using -
    {
        AStackString<> token( "-dll" );
        TEST_ASSERT( LinkerNode::IsLinkerArg_MSVC( token, "DLL" ) );
    }

    // Starts with args, using /
    {
        AStackString<> token( "/ORDER:@orderfile.txt" );
        TEST_ASSERT( LinkerNode::IsStartOfLinkerArg_MSVC( token, "ORDER:" ) );
    }

    // Starts with args, using -
    {
        AStackString<> token( "-ORDER:@orderfile.txt" );
        TEST_ASSERT( LinkerNode::IsStartOfLinkerArg_MSVC( token, "ORDER:" ) );
    }

    // Starts with args, lower-case, using /
    {
        AStackString<> token( "/order:@orderfile.txt" );
        TEST_ASSERT( LinkerNode::IsStartOfLinkerArg_MSVC( token, "ORDER:" ) );
    }

    // Starts with args, lower-case, using -
    {
        AStackString<> token( "-order:@orderfile.txt" );
        TEST_ASSERT( LinkerNode::IsStartOfLinkerArg_MSVC( token, "ORDER:" ) );
    }
}

// LibrariesOnCommandLine
//------------------------------------------------------------------------------
void TestLinker::LibrariesOnCommandLine() const
{
    FBuild fBuild;
    NodeGraph nodeGraph;
    const BFFToken * iter = nullptr;
    LinkerNodeFileExistsCache cache; // We're bypassing normal parsing so we have to manually create this

    // MSVC: 2 libraries
    {
        const bool isMSVC = true;
        AStackString<> args( "/LIBPATH:Tools/FBuild/FBuildTest/Data/TestLinker/LibrariesOnCommandLine dummy1.lib dummy2.lib" );

        Dependencies foundLibraries;
        LinkerNode::GetOtherLibraries( nodeGraph, iter, nullptr, args, foundLibraries, isMSVC );

        TEST_ASSERT( foundLibraries.GetSize() == 2 );
        TEST_ASSERT( foundLibraries[ 0 ].GetNode()->GetName().EndsWith( "dummy1.lib" ) );
        TEST_ASSERT( foundLibraries[ 1 ].GetNode()->GetName().EndsWith( "dummy2.lib" ) );
    }

    // MSVC: case different to on-disk
    #if defined( __WINDOWS__ )
        {
            const bool isMSVC = true;
            AStackString<> args( "/LIBPATH:Tools/FBuild/FBuildTest/Data/TestLinker/LibrariesOnCommandLine DUMMY1.lib" );

            Dependencies foundLibraries;
            LinkerNode::GetOtherLibraries( nodeGraph, iter, nullptr, args, foundLibraries, isMSVC );

            TEST_ASSERT( foundLibraries.GetSize() == 1 );
            TEST_ASSERT( foundLibraries[ 0 ].GetNode()->GetName().EndsWith( "dummy1.lib" ) ); // dependency is on actual file case
        }
    #endif

    // MSVC: libs missing
    {
        const bool isMSVC = true;
        AStackString<> args( "doesnotexist.lib" );

        Dependencies foundLibraries;
        LinkerNode::GetOtherLibraries( nodeGraph, iter, nullptr, args, foundLibraries, isMSVC );

        TEST_ASSERT( foundLibraries.GetSize() == 0 );
    }

    // Other: 2 libraries
    {
        const bool isMSVC = false;
        AStackString<> args( "-LTools/FBuild/FBuildTest/Data/TestLinker/LibrariesOnCommandLine libdummy1.a libdummy2.a" );

        Dependencies foundLibraries;
        LinkerNode::GetOtherLibraries( nodeGraph, iter, nullptr, args, foundLibraries, isMSVC );

        TEST_ASSERT( foundLibraries.GetSize() == 2 );
        TEST_ASSERT( foundLibraries[ 0 ].GetNode()->GetName().EndsWith( "libdummy1.a" ) );
        TEST_ASSERT( foundLibraries[ 1 ].GetNode()->GetName().EndsWith( "libdummy2.a" ) );
    }

    // Other: -l style
    {
        const bool isMSVC = false;
        AStackString<> args( "-LTools/FBuild/FBuildTest/Data/TestLinker/LibrariesOnCommandLine -ldummy1" );

        Dependencies foundLibraries;
        LinkerNode::GetOtherLibraries( nodeGraph, iter, nullptr, args, foundLibraries, isMSVC );

        TEST_ASSERT( foundLibraries.GetSize() == 1 );
        TEST_ASSERT( foundLibraries[ 0 ].GetNode()->GetName().EndsWith( "libdummy1.so" ) );
    }

    // Other: -l style with -Bstatic and -Ddynamic
    {
        const bool isMSVC = false;
        AStackString<> args( "-LTools/FBuild/FBuildTest/Data/TestLinker/LibrariesOnCommandLine -Wl,-Bstatic -ldummy1 -Wl,-Bdynamic -ldummy2" );

        Dependencies foundLibraries;
        LinkerNode::GetOtherLibraries( nodeGraph, iter, nullptr, args, foundLibraries, isMSVC );

        TEST_ASSERT( foundLibraries.GetSize() == 2 );
        // To not overcomplicate the test we depend on the order in which GetOtherLibraries() emits dependencies.
        // The order is not important and can be freely changed.
        TEST_ASSERT( foundLibraries[ 0 ].GetNode()->GetName().EndsWith( "libdummy2.so" ) );
        TEST_ASSERT( foundLibraries[ 1 ].GetNode()->GetName().EndsWith( "libdummy1.a" ) );
    }

    // Other: -l style, checking search order with -Bdynamic:
    //        Preferring static library in the directory that comes earlier in the search path
    //        over dynamic library in the directory that comes later in the search path.
    {
        const bool isMSVC = false;
        AStackString<> args( "-LTools/FBuild/FBuildTest/Data/TestLinker/LibrariesOnCommandLine/StaticOnly -LTools/FBuild/FBuildTest/Data/TestLinker/LibrariesOnCommandLine -Wl,-Bdynamic -ldummy1" );

        Dependencies foundLibraries;
        LinkerNode::GetOtherLibraries( nodeGraph, iter, nullptr, args, foundLibraries, isMSVC );

        TEST_ASSERT( foundLibraries.GetSize() == 1 );
        TEST_ASSERT( foundLibraries[ 0 ].GetNode()->GetName().EndsWith( "libdummy1.a" ) );
        TEST_ASSERT( foundLibraries[ 0 ].GetNode()->GetName().Find( "StaticOnly" ) );
    }

    // Other: -l: style
    {
        const bool isMSVC = false;
        AStackString<> args( "-LTools/FBuild/FBuildTest/Data/TestLinker/LibrariesOnCommandLine -l:dummy2.lib" );

        Dependencies foundLibraries;
        LinkerNode::GetOtherLibraries( nodeGraph, iter, nullptr, args, foundLibraries, isMSVC );

        TEST_ASSERT( foundLibraries.GetSize() == 1 );
        TEST_ASSERT( foundLibraries[ 0 ].GetNode()->GetName().EndsWith( "dummy2.lib" ) );
    }

    // Other - libs missing
    {
        const bool isMSVC = false;
        AStackString<> args( "doesnotexist.a" );

        Dependencies foundLibraries;
        LinkerNode::GetOtherLibraries( nodeGraph, iter, nullptr, args, foundLibraries, isMSVC );

        TEST_ASSERT( foundLibraries.GetSize() == 0 );
    }
}

// IncrementalLinking_MSVC
//------------------------------------------------------------------------------
void TestLinker::IncrementalLinking_MSVC() const
{
    #if defined( _MSC_VER ) && ( _MSC_VER > 1900 ) // VS2015 link with /VERBOSE doesn't emit output we can use
        FBuildTestOptions options;
        options.m_ConfigFile = "Tools/FBuild/FBuildTest/Data/TestLinker/IncrementalLinking_MSVC/fbuild.bff";
        options.m_ShowCommandOutput = true; // Show linker output so we can check analyze /VERBOSE outptut

        // Files
        const char * dbFile     = "../tmp/Test/TestLinker/IncrementalLinking_MSVC/fbuild.fdb";
        const char * cppFileB   = "../tmp/Test/TestLinker/IncrementalLinking_MSVC/FileB.cpp";

        // Create temp output directory
        TEST_ASSERT( FileIO::EnsurePathExistsForFile( AStackString<>( cppFileB ) ) );

        // Create 10 source files
        for ( uint32_t i = 0; i < 10; ++i )
        {
            AStackString<> fileContents;
            if ( i == 0 )
            {
                fileContents += "int main(int, char **) { return 0; }\n";
            }
            const char c = ( 'A' + (char)i );
            fileContents.AppendFormat( "const char * Function%c() { return \"String%c\"; }\n", c, c );

            AStackString<> dst;
            dst.Format( "../tmp/Test/TestLinker/IncrementalLinking_MSVC/File%c.cpp", c );
            FileStream f;
            TEST_ASSERT( f.Open( dst.Get(), FileStream::WRITE_ONLY ) );
            TEST_ASSERT( f.WriteBuffer( fileContents.Get(), fileContents.GetLength() ) == fileContents.GetLength() );
        }

        // Build executable
        {
            FBuildForTest fBuild( options );
            TEST_ASSERT( fBuild.Initialize() );
            TEST_ASSERT( fBuild.Build( "Exe" ) );

            // Save DB for reloading below
            TEST_ASSERT( fBuild.SaveDependencyGraph( dbFile ) );

            // Check stats
            //               Seen,  Built,  Type
            CheckStatsNode(     10,     10, Node::OBJECT_NODE );
            CheckStatsNode(     1,      1,  Node::EXE_NODE );

            const AString & output( GetRecordedOutput() );

            // Should see no incremental linking messages (FASTBuild should have deleted the
            // old files because the DB is new
            TEST_ASSERT( output.Find( "modules have changed since prior linking" ) == nullptr );
            TEST_ASSERT( output.Find( "performing full link" ) == nullptr );
        }

        // Modify one of the files
        FileIO::SetFileLastWriteTimeToNow( AStackString<>( cppFileB ) );

        // Take note of output size (so we can check only stuff after)
        uint32_t sizeOfRecordedOutput = GetRecordedOutput().GetLength();

        // Compile again
        {
            FBuildForTest fBuild( options );
            TEST_ASSERT( fBuild.Initialize( dbFile ) );
            TEST_ASSERT( fBuild.Build( "Exe" ) );

            // Save DB for reloading below
            TEST_ASSERT( fBuild.SaveDependencyGraph( dbFile ) );

            // Check stats
            //               Seen,  Built,  Type
            CheckStatsNode(     10,     1,  Node::OBJECT_NODE );
            CheckStatsNode(     1,      1,  Node::EXE_NODE );

            const AStackString<> output( GetRecordedOutput().Get() + sizeOfRecordedOutput );

            // Should see incremental linking messsages..
            TEST_ASSERT( output.Find( "modules have changed since prior linking" ) );

            // .. but should not be a full link
            TEST_ASSERT( output.Find( "performing full link" ) == nullptr );
        }

        // Take note of output size (so we can check only stuff after)
        sizeOfRecordedOutput = GetRecordedOutput().GetLength();

        // Compile again (non-incremental)
        {
            options.m_ForceCleanBuild = true; // Force clean build prevents incremental

            FBuildForTest fBuild( options );
            TEST_ASSERT( fBuild.Initialize( dbFile ) );
            TEST_ASSERT( fBuild.Build( "Exe" ) );

            // Save DB for reloading below
            TEST_ASSERT( fBuild.SaveDependencyGraph( dbFile ) );

            // Check stats
            //               Seen,  Built,  Type
            CheckStatsNode(     10,     10, Node::OBJECT_NODE );
            CheckStatsNode(     1,      1,  Node::EXE_NODE );

            const AStackString<> output( GetRecordedOutput().Get() + sizeOfRecordedOutput );

            // Should see no incremental linking messages (FASTBuild should have deleted the
            // old files because of -clean)
            TEST_ASSERT( output.Find( "modules have changed since prior linking" ) == nullptr );
            TEST_ASSERT( output.Find( "performing full link" ) == nullptr );
        }
    #endif
}

// LinkerType
//------------------------------------------------------------------------------
void TestLinker::LinkerType() const
{
    #define TEST_LINKERTYPE( exeName, expectedFlag ) \
    do { \
        const uint32_t flags = LinkerNode::DetermineLinkerTypeFlags( AStackString<>( "auto" ), \
                                                                     AStackString<>( exeName ) ); \
        TEST_ASSERT( ( flags & expectedFlag ) == expectedFlag ); \
    } while( false )

    TEST_LINKERTYPE( "link",        LinkerNode::LINK_FLAG_MSVC );
    TEST_LINKERTYPE( "gcc",         LinkerNode::LINK_FLAG_GCC );
    TEST_LINKERTYPE( "ps3ppuld",    LinkerNode::LINK_FLAG_SNC );
    TEST_LINKERTYPE( "orbis-ld",    LinkerNode::LINK_FLAG_ORBIS_LD );
    TEST_LINKERTYPE( "elxr",        LinkerNode::LINK_FLAG_GREENHILLS_ELXR );
    TEST_LINKERTYPE( "mwldeppc",    LinkerNode::LINK_FLAG_CODEWARRIOR_LD );

    #undef TEST_LINKERTYPE
}

//------------------------------------------------------------------------------
