// TestCompilationDatabase.cpp
//------------------------------------------------------------------------------

// Includes
//------------------------------------------------------------------------------
#include "FBuildTest.h"

#include "Tools/FBuild/FBuildCore/BFF/BFFParser.h"
#include "Tools/FBuild/FBuildCore/FBuild.h"
#include "Tools/FBuild/FBuildCore/Graph/NodeGraph.h"
#include "Tools/FBuild/FBuildCore/Helpers/CompilationDatabase.h"

// Core
#include "Core/FileIO/PathUtils.h"
#include "Core/Strings/AStackString.h"

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
    REGISTER_TEST( Unquote )
    REGISTER_TEST( TestObjectListInputFile )
    REGISTER_TEST( TestObjectListInputPath )
    REGISTER_TEST( TestUnityInputFile )
    REGISTER_TEST( TestUnityInputPath )
REGISTER_TESTS_END

// TestCompilationDatabase
//------------------------------------------------------------------------------
class CompilationDatabaseTestWrapper : public CompilationDatabase
{
public:
    static void JSONEscape( AString & string ) { CompilationDatabase::JSONEscape( string ); }
    static void Unquote( AString & string )    { CompilationDatabase::Unquote( string ); }
};

// JSONEscape
//------------------------------------------------------------------------------
void TestCompilationDatabase::JSONEscape() const
{
    #define CHECK_JSONESCAPE( str, result ) \
    { \
        AStackString<> string( str ); \
        CompilationDatabaseTestWrapper::JSONEscape( string ); \
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

// Unquote
//------------------------------------------------------------------------------
void TestCompilationDatabase::Unquote() const
{
    #define CHECK_UNQUOTE( str, result ) \
    { \
        AStackString<> string( str ); \
        CompilationDatabaseTestWrapper::Unquote( string ); \
        TEST_ASSERT( string == result ); \
    }

    CHECK_UNQUOTE( "", "" )
    CHECK_UNQUOTE( "\"\"", "" )
    CHECK_UNQUOTE( "''", "" )
    CHECK_UNQUOTE( "\"foo\"", "foo" )
    CHECK_UNQUOTE( "'foo'", "foo" )
    CHECK_UNQUOTE( "f\"o\"o", "foo" )
    CHECK_UNQUOTE( "f'o'o", "foo" )
    CHECK_UNQUOTE( "\"''\"", "''" )
    CHECK_UNQUOTE( "'\"\"'", "\"\"" )
    CHECK_UNQUOTE( "\"foo\"_\"bar\"", "foo_bar" )
    CHECK_UNQUOTE( "'foo'_'bar'", "foo_bar" )

    #undef CHECK_UNQUOTE
}

// TestObjectListInputFile
//------------------------------------------------------------------------------
void TestCompilationDatabase::TestObjectListInputFile() const
{
    DoTest( "Tools/FBuild/FBuildTest/Data/TestCompilationDatabase/fbuild.bff", "ObjectList_InputFile",
        "[\n"
        "  {\n"
        "    \"directory\": \"{WORKDIR}\",\n"
        "    \"file\": \"{TESTDIR}file.cpp\",\n"
        "    \"output\": \"{OUTDIR}file.result\",\n"
        "    \"arguments\": [\"{TESTDIR}clang\", \"-c\", \"-Ipath with spaces\", \"-DSTRING_DEFINE=\\\"foobar\\\"\", \"{TESTDIR}file.cpp\", \"-o\", \"{OUTDIR}file.result\"]\n"
        "  }\n"
        "]\n"
    );
}

// TestObjectListInputPath
//------------------------------------------------------------------------------
void TestCompilationDatabase::TestObjectListInputPath() const
{
    DoTest( "Tools/FBuild/FBuildTest/Data/TestCompilationDatabase/fbuild.bff", "ObjectList_InputPath",
        "[\n"
        "  {\n"
        "    \"directory\": \"{WORKDIR}\",\n"
        "    \"file\": \"{TESTDIR}dir{SLASH}subdir{SLASH}file.cpp\",\n"
        "    \"output\": \"{OUTDIR}subdir{SLASH}file.result\",\n"
        "    \"arguments\": [\"{TESTDIR}clang\", \"-c\", \"-Ipath with spaces\", \"-DSTRING_DEFINE=\\\"foobar\\\"\", \"{TESTDIR}dir{SLASH}subdir{SLASH}file.cpp\", \"-o\", \"{OUTDIR}subdir{SLASH}file.result\"]\n"
        "  }\n"
        "]\n"
    );
}

// TestUnityInputFile
//------------------------------------------------------------------------------
void TestCompilationDatabase::TestUnityInputFile() const
{
    DoTest( "Tools/FBuild/FBuildTest/Data/TestCompilationDatabase/fbuild.bff", "ObjectList_UnityInputFile",
        "[\n"
        "  {\n"
        "    \"directory\": \"{WORKDIR}\",\n"
        "    \"file\": \"{TESTDIR}file.cpp\",\n"
        "    \"output\": \"{OUTDIR}file.result\",\n"
        "    \"arguments\": [\"{TESTDIR}clang\", \"-c\", \"-Ipath with spaces\", \"-DSTRING_DEFINE=\\\"foobar\\\"\", \"{TESTDIR}file.cpp\", \"-o\", \"{OUTDIR}file.result\"]\n"
        "  }\n"
        "]\n"
    );
}

// TestUnityInputPath
//------------------------------------------------------------------------------
void TestCompilationDatabase::TestUnityInputPath() const
{
    DoTest( "Tools/FBuild/FBuildTest/Data/TestCompilationDatabase/fbuild.bff", "ObjectList_UnityInputPath",
        "[\n"
        "  {\n"
        "    \"directory\": \"{WORKDIR}\",\n"
        "    \"file\": \"{TESTDIR}dir{SLASH}subdir{SLASH}file.cpp\",\n"
        "    \"output\": \"{OUTDIR}subdir{SLASH}file.result\",\n"
        "    \"arguments\": [\"{TESTDIR}clang\", \"-c\", \"-Ipath with spaces\", \"-DSTRING_DEFINE=\\\"foobar\\\"\", \"{TESTDIR}dir{SLASH}subdir{SLASH}file.cpp\", \"-o\", \"{OUTDIR}subdir{SLASH}file.result\"]\n"
        "  }\n"
        "]\n"
    );
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
    Node * node = ng.FindNode( AStackString<>( target ) );
    TEST_ASSERT( node != nullptr );
    deps.EmplaceBack( node );

    CompilationDatabase compdb;
    const AString & actualResult = compdb.Generate( ng, deps );

    AStackString<> expectedResult( result );
    PrepareExpectedResult( expectedResult );

    TEST_ASSERT( actualResult == expectedResult );
}

// PrepareExpectedResult
//------------------------------------------------------------------------------
/*static*/ void TestCompilationDatabase::PrepareExpectedResult( AString & result )
{
    AStackString<> workDir;
    workDir = FBuild::Get().GetWorkingDir();

    AStackString<> testDir;
    testDir += workDir;
    testDir += "/Tools/FBuild/FBuildTest/Data/TestCompilationDatabase/";
    NodeGraph::CleanPath( testDir );

    AStackString<> outDir;
    outDir += workDir;
    outDir += "/../tmp/Test/CompilationDatabase/";
    NodeGraph::CleanPath( outDir );

    AStackString<> slash;
    slash = NATIVE_SLASH_STR;

    CompilationDatabaseTestWrapper::JSONEscape( workDir );
    CompilationDatabaseTestWrapper::JSONEscape( testDir );
    CompilationDatabaseTestWrapper::JSONEscape( outDir );
    CompilationDatabaseTestWrapper::JSONEscape( slash );

    result.Replace( "{WORKDIR}", workDir.Get() );
    result.Replace( "{TESTDIR}", testDir.Get() );
    result.Replace( "{OUTDIR}", outDir.Get() );
    result.Replace( "{SLASH}", slash.Get() );
}

//------------------------------------------------------------------------------
