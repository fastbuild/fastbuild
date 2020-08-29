// TestListDependencies.cpp
//------------------------------------------------------------------------------

// Includes
//------------------------------------------------------------------------------
#include "FBuildTest.h"

// FBuildCore
#include "Tools/FBuild/FBuildCore/FBuild.h"
#include "Tools/FBuild/FBuildCore/Graph/NodeGraph.h"
#include "Tools/FBuild/FBuildCore/Graph/TestNode.h"

// Core
#include "Core/FileIO/FileIO.h"
#include "Core/FileIO/PathUtils.h"
#include "Core/Strings/AStackString.h"

// TestListDependencies
//------------------------------------------------------------------------------
class TestListDependencies : public FBuildTest
{
private:
    DECLARE_TESTS

    void Build() const;
    void Build_NoRebuild() const;
    void Build_NoRebuild_BFFChange() const;

    void Check_SourceResults( const FBuildTestOptions & options ) const;
};

// Register Tests
//------------------------------------------------------------------------------
REGISTER_TESTS_BEGIN( TestListDependencies )
    REGISTER_TEST( Build )
    REGISTER_TEST( Build_NoRebuild )
    REGISTER_TEST( Build_NoRebuild_BFFChange )
REGISTER_TESTS_END

// Build
//------------------------------------------------------------------------------
void TestListDependencies::Build() const
{
    FBuildTestOptions options;
    options.m_ConfigFile = "Tools/FBuild/FBuildTest/Data/TestListDependencies/fbuild.bff";
    FBuild fBuild( options );
    TEST_ASSERT( fBuild.Initialize() );

    // build (via alias)
    TEST_ASSERT( fBuild.Build( "ListDependencies" ) );
    TEST_ASSERT( fBuild.SaveDependencyGraph( "../tmp/Test/ListDependencies/fbuild.fdb" ) );

    // Check stats
    //               Seen,  Built,  Type
    CheckStatsNode ( 2,     2,      Node::LIST_DEPENDENCIES_NODE );
    CheckStatsNode ( 1,     1,      Node::OBJECT_LIST_NODE );
    CheckStatsNode ( 1,     1,      Node::ALIAS_NODE );

    // Check dependencies outputted
    Check_SourceResults( options );
}

// Build_NoRebuild
//------------------------------------------------------------------------------
void TestListDependencies::Build_NoRebuild() const
{
    FBuildTestOptions options;
    options.m_ConfigFile = "Tools/FBuild/FBuildTest/Data/TestListDependencies/fbuild.bff";
    FBuild fBuild( options );
    TEST_ASSERT( fBuild.Initialize( "../tmp/Test/ListDependencies/fbuild.fdb" ) );

    // build (via alias)
    TEST_ASSERT( fBuild.Build( "ListDependencies" ) );

    // Check stats
    //               Seen,  Built,  Type
    CheckStatsNode ( 2,     0,      Node::LIST_DEPENDENCIES_NODE );
    CheckStatsNode ( 1,     0,      Node::OBJECT_LIST_NODE );
    CheckStatsNode ( 1,     1,      Node::ALIAS_NODE );

    // Check dependencies outputted
    Check_SourceResults( options );
}

// Build_NoRebuild_BFFChange
//------------------------------------------------------------------------------
void TestListDependencies::Build_NoRebuild_BFFChange() const
{
    FBuildTestOptions options;
    options.m_ConfigFile = "Tools/FBuild/FBuildTest/Data/TestListDependencies/fbuild.bff";
    options.m_ForceDBMigration_Debug = true; // Force bff modified re-parse

    FBuild fBuild( options );
    TEST_ASSERT( fBuild.Initialize( "../tmp/Test/ListDependencies/fbuild.fdb" ) );

    // build (via alias)
    TEST_ASSERT( fBuild.Build( "ListDependencies" ) );

    // Check stats
    //               Seen,  Built,  Type
    CheckStatsNode ( 2,     0,      Node::LIST_DEPENDENCIES_NODE );
    CheckStatsNode ( 1,     0,      Node::OBJECT_LIST_NODE );
    CheckStatsNode ( 1,     1,      Node::ALIAS_NODE );

    // Check dependencies outputted
    Check_SourceResults( options );
}

// Check_SourceResults
//------------------------------------------------------------------------------
void TestListDependencies::Check_SourceResults( const FBuildTestOptions & options ) const
{
    AStackString<> outputFilename( options.GetWorkingDir() );
    outputFilename += "/../tmp/Test/ListDependencies/SourceDeps.txt";

    PathUtils::FixupFilePath( outputFilename );

    // Read file into memory
    AString dependencies;
    LoadFileContentsAsString( outputFilename.Get(), dependencies );

    // Split lines
    Array< AString > files;
    dependencies.Replace( '\r', '\n' );
    dependencies.Tokenize( files, '\n' ); // Will discard empty lines

    for ( AString & filename : files )
    {
        // Remove leading/trailing whitespace
        filename.TrimEnd( ' ' );
        filename.TrimEnd( '\t' );
        filename.TrimStart( ' ' );
        filename.TrimStart( '\t' );
    }

    TEST_ASSERTM( files.GetSize() == 4 , "Expected 4 dependencies, but got %zu instead" , files.GetSize() );

    TEST_ASSERTM( files[0].EndsWithI( "HeaderA.h" ) , "Expected 'HeaderA.h' in files[0], but got '%s' instead" , files[0].Get() );
    TEST_ASSERTM( files[1].EndsWithI( "HeaderB.h" ) , "Expected 'HeaderB.h' in files[1], but got '%s' instead" , files[1].Get() );
    TEST_ASSERTM( files[2].EndsWithI( "HeaderC.h" ) , "Expected 'HeaderC.h' in files[2], but got '%s' instead" , files[2].Get() );
    TEST_ASSERTM( files[3].EndsWithI( "SimpleObject.cpp" ) , "Expected 'SimpleObject.cpp' int file[3], but got '%s' instead" , files[3].Get() );
}

//------------------------------------------------------------------------------
