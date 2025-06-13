// TestCompilationDatabase.cpp
//------------------------------------------------------------------------------

// Includes
//------------------------------------------------------------------------------
#include "FBuildTest.h"

#include "Tools/FBuild/FBuildCore/BFF/BFFParser.h"
#include "Tools/FBuild/FBuildCore/FBuild.h"
#include "Tools/FBuild/FBuildCore/Graph/NodeGraph.h"
#include "Tools/FBuild/FBuildCore/Helpers/CompilationDatabase.h"
#include "Tools/FBuild/FBuildCore/Helpers/JSON.h"

// Core
#include "Core/FileIO/PathUtils.h"
#include "Core/Strings/AStackString.h"

// Defines
//------------------------------------------------------------------------------
#define COMPDB_COMMON_ARGS "\"-c\", " \
                           "\"-Ipath with spaces\", " \
                           "\"-DSTRING_DEFINE=\\\"foobar\\\"\", " \
                           "\"-DSTRING_DEFINE_SPACES=\\\"foo bar\\\"\","

// TestCompilationDatabase
//------------------------------------------------------------------------------
class TestCompilationDatabase : public FBuildTest
{
private:
    DECLARE_TESTS

    void JSONEscape() const;
    void Unquote() const;
    void TestObjectListInputFile() const;
    void TestObjectListInputPath() const;
    void TestUnityInputFile() const;
    void TestUnityInputPath() const;

    void DoTest( const char * bffFile, const char * target, const char * result ) const;
    static void PrepareExpectedResult( AString & result );
};

// Register Tests
//------------------------------------------------------------------------------
REGISTER_TESTS_BEGIN( TestCompilationDatabase )
    REGISTER_TEST( JSONEscape )
    REGISTER_TEST( TestObjectListInputFile )
    REGISTER_TEST( TestObjectListInputPath )
    REGISTER_TEST( TestUnityInputFile )
    REGISTER_TEST( TestUnityInputPath )
REGISTER_TESTS_END

// JSONEscape
//------------------------------------------------------------------------------
void TestCompilationDatabase::JSONEscape() const
{
#define CHECK_JSONESCAPE( str, result ) \
    { \
        AStackString string( str ); \
        JSON::Escape( string ); \
        TEST_ASSERT( string == result ); \
    }

    CHECK_JSONESCAPE( "", "" )
    CHECK_JSONESCAPE( "foo", "foo" )
    CHECK_JSONESCAPE( "\"bar\"", "\\\"bar\\\"" )
    CHECK_JSONESCAPE( "first\\second\\third", "first\\\\second\\\\third" )
    CHECK_JSONESCAPE( "\b \t \n \f \r \\ \"", "\\b \\t \\n \\f \\r \\\\ \\\"" )
    CHECK_JSONESCAPE( "\x01 \x0B \x14 \x1E", "\\u0001 \\u000B \\u0014 \\u001E" )

#undef CHECK_JSONESCAPE
}

// TestObjectListInputFile
//------------------------------------------------------------------------------
void TestCompilationDatabase::TestObjectListInputFile() const
{
    DoTest( "Tools/FBuild/FBuildTest/Data/TestCompilationDatabase/fbuild.bff",
            "ObjectList_InputFile",
            "[\n"
            "  {\n"
            "    \"directory\": \"{WORKDIR}\",\n"
            "    \"file\": \"{TESTDIR}file.cpp\",\n"
            "    \"output\": \"{OUTDIR}file.result\",\n"
            "    \"arguments\": [\"{TESTDIR}clang.exe\", " COMPDB_COMMON_ARGS
            " \"{TESTDIR}file.cpp\", \"-o\", \"{OUTDIR}file.result\"]\n"
            "  }\n"
            "]\n" );
}

// TestObjectListInputPath
//------------------------------------------------------------------------------
void TestCompilationDatabase::TestObjectListInputPath() const
{
    DoTest( "Tools/FBuild/FBuildTest/Data/TestCompilationDatabase/fbuild.bff",
            "ObjectList_InputPath",
            "[\n"
            "  {\n"
            "    \"directory\": \"{WORKDIR}\",\n"
            "    \"file\": \"{TESTDIR}dir{SLASH}subdir{SLASH}file.cpp\",\n"
            "    \"output\": \"{OUTDIR}subdir{SLASH}file.result\",\n"
            "    \"arguments\": [\"{TESTDIR}clang.exe\", " COMPDB_COMMON_ARGS " \"{TESTDIR}dir{SLASH}subdir{SLASH}file.cpp\", \"-o\", \"{OUTDIR}subdir{SLASH}file.result\"]\n"
            "  }\n"
            "]\n" );
}

// TestUnityInputFile
//------------------------------------------------------------------------------
void TestCompilationDatabase::TestUnityInputFile() const
{
    DoTest( "Tools/FBuild/FBuildTest/Data/TestCompilationDatabase/fbuild.bff",
            "ObjectList_UnityInputFile",
            "[\n"
            "  {\n"
            "    \"directory\": \"{WORKDIR}\",\n"
            "    \"file\": \"{TESTDIR}file.cpp\",\n"
            "    \"output\": \"{OUTDIR}file.result\",\n"
            "    \"arguments\": [\"{TESTDIR}clang.exe\", " COMPDB_COMMON_ARGS
            " \"{TESTDIR}file.cpp\", \"-o\", \"{OUTDIR}file.result\"]\n"
            "  }\n"
            "]\n" );
}

// TestUnityInputPath
//------------------------------------------------------------------------------
void TestCompilationDatabase::TestUnityInputPath() const
{
    DoTest( "Tools/FBuild/FBuildTest/Data/TestCompilationDatabase/fbuild.bff",
            "ObjectList_UnityInputPath",
            "[\n"
            "  {\n"
            "    \"directory\": \"{WORKDIR}\",\n"
            "    \"file\": \"{TESTDIR}dir{SLASH}subdir{SLASH}file.cpp\",\n"
            "    \"output\": \"{OUTDIR}subdir{SLASH}file.result\",\n"
            "    \"arguments\": [\"{TESTDIR}clang.exe\", " COMPDB_COMMON_ARGS " \"{TESTDIR}dir{SLASH}subdir{SLASH}file.cpp\", \"-o\", \"{OUTDIR}subdir{SLASH}file.result\"]\n"
            "  }\n"
            "]\n" );
}

// DoTest
//------------------------------------------------------------------------------
void TestCompilationDatabase::DoTest( const char * bffFile, const char * target, const char * result ) const
{
    FBuild fBuild;
    NodeGraph ng;
    BFFParser p( ng );
    TEST_ASSERT( p.ParseFromFile( bffFile ) );

    Dependencies deps;
    Node * node = ng.FindNode( AStackString( target ) );
    TEST_ASSERT( node != nullptr );
    deps.Add( node );

    CompilationDatabase compdb;
    const AString & actualResult = compdb.Generate( ng, deps );

    AStackString expectedResult( result );
    PrepareExpectedResult( expectedResult );

    TEST_ASSERT( actualResult == expectedResult );
}

// PrepareExpectedResult
//------------------------------------------------------------------------------
/*static*/ void TestCompilationDatabase::PrepareExpectedResult( AString & result )
{
    AStackString workDir;
    workDir = FBuild::Get().GetWorkingDir();

    AStackString testDir;
    testDir += workDir;
    testDir += "/Tools/FBuild/FBuildTest/Data/TestCompilationDatabase/";
    NodeGraph::CleanPath( testDir );

    AStackString outDir;
    outDir += workDir;
    outDir += "/../tmp/Test/CompilationDatabase/";
    NodeGraph::CleanPath( outDir );

    AStackString slash;
    slash = NATIVE_SLASH_STR;

    JSON::Escape( workDir );
    JSON::Escape( testDir );
    JSON::Escape( outDir );
    JSON::Escape( slash );

    result.Replace( "{WORKDIR}", workDir.Get() );
    result.Replace( "{TESTDIR}", testDir.Get() );
    result.Replace( "{OUTDIR}", outDir.Get() );
    result.Replace( "{SLASH}", slash.Get() );
}

//------------------------------------------------------------------------------
#undef COMPDB_COMMON_ARGS
