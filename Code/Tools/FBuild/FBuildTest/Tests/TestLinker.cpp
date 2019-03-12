// TestLinker.cpp
//------------------------------------------------------------------------------

// Includes
//------------------------------------------------------------------------------
#include "FBuildTest.h"

// FBuildCore
#include "Tools/FBuild/FBuildCore/Graph/Dependencies.h"
#include "Tools/FBuild/FBuildCore/Graph/LinkerNode.h"
#include "Tools/FBuild/FBuildCore/Graph/NodeGraph.h"
#include "Tools/FBuild/FBuildCore/BFF/BFFIterator.h"
#include "Tools/FBuild/FBuildCore/FBuild.h"

// Core
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
};

// Register Tests
//------------------------------------------------------------------------------
REGISTER_TESTS_BEGIN( TestLinker )
    REGISTER_TEST( ArgHelpers )                 // Test functions that check for non-MSVC args
    REGISTER_TEST( ArgHelpers_MSVC )            // Test functions that check for MSVC args
    REGISTER_TEST( LibrariesOnCommandLine )     // Discovery of additional libraries on command line
REGISTER_TESTS_END

// ArgHelpers
//------------------------------------------------------------------------------
void TestLinker::ArgHelpers() const
{
    // Exact match
    {
        AStackString<> token( "-L" );
        TEST_ASSERT( LinkerNode::IsStartOfLinkerArg( token, "L" ) )
    }

    // Starts with
    {
        AStackString<> token( "-Lthing" );
        TEST_ASSERT( LinkerNode::IsStartOfLinkerArg( token, "L" ) )
    }

    // Check case sensitive is respected
    {
        AStackString<> token( "-l" );
        TEST_ASSERT( LinkerNode::IsStartOfLinkerArg( token, "L" ) == false )
    }
}

// ArgHelpers_MSVC
//------------------------------------------------------------------------------
void TestLinker::ArgHelpers_MSVC() const
{
    // Exact match args, using /
    {
        AStackString<> token( "/DLL" );
        TEST_ASSERT( LinkerNode::IsLinkerArg_MSVC( token, "DLL" ) )
    }

    // Exact match args, using -
    {
        AStackString<> token( "-DLL" );
        TEST_ASSERT( LinkerNode::IsLinkerArg_MSVC( token, "DLL" ) )
    }

    // Exact match args, lower-case, using /
    {
        AStackString<> token( "/dll" );
        TEST_ASSERT( LinkerNode::IsLinkerArg_MSVC( token, "DLL" ) )
    }

    // Exact match args, lower-case, using -
    {
        AStackString<> token( "-dll" );
        TEST_ASSERT( LinkerNode::IsLinkerArg_MSVC( token, "DLL" ) )
    }

    // Starts with args, using /
    {
        AStackString<> token( "/ORDER:@orderfile.txt" );
        TEST_ASSERT( LinkerNode::IsStartOfLinkerArg_MSVC( token, "ORDER:" ) )
    }

    // Starts with args, using -
    {
        AStackString<> token( "-ORDER:@orderfile.txt" );
        TEST_ASSERT( LinkerNode::IsStartOfLinkerArg_MSVC( token, "ORDER:" ) )
    }

    // Starts with args, lower-case, using /
    {
        AStackString<> token( "/order:@orderfile.txt" );
        TEST_ASSERT( LinkerNode::IsStartOfLinkerArg_MSVC( token, "ORDER:" ) )
    }

    // Starts with args, lower-case, using -
    {
        AStackString<> token( "-order:@orderfile.txt" );
        TEST_ASSERT( LinkerNode::IsStartOfLinkerArg_MSVC( token, "ORDER:" ) )
    }
}

// LibrariesOnCommandLine
//------------------------------------------------------------------------------
void TestLinker::LibrariesOnCommandLine() const
{
    FBuild fBuild;
    NodeGraph nodeGraph;
    BFFIterator iter;

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
        TEST_ASSERT( foundLibraries[ 0 ].GetNode()->GetName().EndsWith( "libdummy1.a" ) );
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

//------------------------------------------------------------------------------
