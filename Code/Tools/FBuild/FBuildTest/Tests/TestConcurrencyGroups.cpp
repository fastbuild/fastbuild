// TestTest.cpp
//------------------------------------------------------------------------------

// Includes
//------------------------------------------------------------------------------
#include "FBuildTest.h"

// FBuildCore
#include "Tools/FBuild/FBuildCore/Graph/SettingsNode.h"

// Core
#include "Core/Strings/AStackString.h"

// TestConcurrencyGroups
//------------------------------------------------------------------------------
class TestConcurrencyGroups : public FBuildTest
{
private:
    DECLARE_TESTS

    void TooManyGroups() const;
    void DuplicateGroupName() const;
    void NoLimitsSet() const;
    void UndefinedGroup() const;
    void EnforceLimit() const;
};

// Register Tests
//------------------------------------------------------------------------------
REGISTER_TESTS_BEGIN( TestConcurrencyGroups )
    REGISTER_TEST( TooManyGroups )
    REGISTER_TEST( DuplicateGroupName )
    REGISTER_TEST( NoLimitsSet )
    REGISTER_TEST( UndefinedGroup )
    REGISTER_TEST( EnforceLimit )
REGISTER_TESTS_END

// TooManyGroups
//------------------------------------------------------------------------------
void TestConcurrencyGroups::TooManyGroups() const
{
    // Generate a BFF with too many Concurrency Groups and verify the correct
    // error is reported.
    AStackString groups;
    AStackString groupNames;
    for ( size_t i = 0; ( i < SettingsNode::kMaxConcurrencyGroups + 1 ); ++i )
    {
        groups.AppendFormat( " .GroupName%zu = [ .ConcurrencyGroupName = 'Group%zu' ]", i, i );
        groupNames.AppendFormat( " .GroupName%zu,", i );
    }
    AStackString bff;
    bff.Format( "Settings\n"
                "{\n"
                "    %s\n"
                "    .ConcurrencyGroups =\n"
                "    {\n"
                "        %s\n"
                "    }\n"
                "}",
                groups.Get(),
                groupNames.Get() );
    TEST_PARSE_FAIL( bff.Get(), "Error #1600" );
}

// DuplicateGroupName
//------------------------------------------------------------------------------
void TestConcurrencyGroups::DuplicateGroupName() const
{
    // Ensure duplicate/conflicting Concurrency Group names report an
    // appropriate error.
    TEST_PARSE_FAIL( "Settings\n"
                     "{\n"
                     "    .Group1 = [ .ConcurrencyGroupName = 'link' .ConcurrencyLimit = 8 ]\n"
                     "    .Group2 = [ .ConcurrencyGroupName = 'link' .ConcurrencyLimit = 8 ]\n"
                     "    .ConcurrencyGroups = { .Group1, .Group2 }\n"
                     "}",
                     "Error #1601" );
}

// NoLimitsSet
//------------------------------------------------------------------------------
void TestConcurrencyGroups::NoLimitsSet() const
{
    // Ensure Concurrency Groups that don't properly define a concurrency limit
    // report an appropriate error.
    TEST_PARSE_FAIL( "Settings\n"
                     "{\n"
                     "    .Group = [ .ConcurrencyGroupName = 'link' ]\n"
                     "    .ConcurrencyGroups = { .Group }\n"
                     "}",
                     "Error #1602" );

    // Ensure either limit set is valid
    TEST_PARSE_OK( "Settings\n"
                   "{\n"
                   "    .Group = [ .ConcurrencyGroupName = 'link' .ConcurrencyLimit = 4 ]\n"
                   "    .ConcurrencyGroups = { .Group }\n"
                   "}" );
    TEST_PARSE_OK( "Settings\n"
                   "{\n"
                   "    .Group = [ .ConcurrencyGroupName = 'link' .ConcurrencyPerJobMiB = 4 ]\n"
                   "    .ConcurrencyGroups = { .Group }\n"
                   "}" );
}

// UndefinedGroup
//------------------------------------------------------------------------------
void TestConcurrencyGroups::UndefinedGroup() const
{
    // Ensure references to an undefined Concurrency Group give an explicit
    // error.
    TEST_PARSE_FAIL( "Exec()\n"
                     "{\n"
                     "    .ConcurrencyGroupName = 'deploy'\n"
                     "    .ExecExecutable = 'deploy.exe'\n"
                     "    .ExecOutput     = 'deploy.log'\n"
                     "}\n",
                     "Error #1603" );
}

// EnforceLimit
//------------------------------------------------------------------------------
void TestConcurrencyGroups::EnforceLimit() const
{
    // Perform a build that would fail if concurrency limits were not properly
    // enforced.

    // Initialize
    FBuildTestOptions options;
    options.m_ConfigFile = "Tools/FBuild/FBuildTest/Data/TestConcurrencyGroups/EnforceLimit/enforcelimit.bff";
    FBuild fBuild( options );
    TEST_ASSERT( fBuild.Initialize() );

    // Build
    TEST_ASSERT( fBuild.Build( "EnforceLimit" ) );
}

//------------------------------------------------------------------------------
