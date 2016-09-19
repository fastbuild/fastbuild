// TestLinker.cpp
//------------------------------------------------------------------------------

// Includes
//------------------------------------------------------------------------------
#include "FBuildTest.h"

// FBuildCore
#include "Tools/FBuild/FBuildCore/Graph/LinkerNode.h"

// Core
#include "Core/Strings/AStackString.h"

// TestLinker
//------------------------------------------------------------------------------
class TestLinker : public FBuildTest
{
private:
    DECLARE_TESTS

    // Tests
    void MSVCArgHelpers() const;
};

// Register Tests
//------------------------------------------------------------------------------
REGISTER_TESTS_BEGIN( TestLinker )
    REGISTER_TEST( MSVCArgHelpers )             // Test functions that check for MVC args
REGISTER_TESTS_END

// MSVCArgHelpers
//------------------------------------------------------------------------------
void TestLinker::MSVCArgHelpers() const
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

//------------------------------------------------------------------------------
