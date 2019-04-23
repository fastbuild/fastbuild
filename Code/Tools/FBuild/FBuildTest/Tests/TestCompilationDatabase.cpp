// TestCompilationDatabase.cpp
//------------------------------------------------------------------------------

// Includes
//------------------------------------------------------------------------------
#include "FBuildTest.h"

#include "Tools/FBuild/FBuildCore/BFF/BFFParser.h"
#include "Tools/FBuild/FBuildCore/FBuild.h"
#include "Tools/FBuild/FBuildCore/Graph/NodeGraph.h"
#include "Tools/FBuild/FBuildCore/Helpers/CompilationDatabase.h"

#include "Core/Containers/AutoPtr.h"
#include "Core/FileIO/FileStream.h"
#include "Core/FileIO/PathUtils.h"
#include "Core/Strings/AStackString.h"

// TestCompilationDatabase
//------------------------------------------------------------------------------
class TestCompilationDatabase : public FBuildTest
{
private:
    DECLARE_TESTS

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
    REGISTER_TEST( TestObjectListInputFile )
    REGISTER_TEST( TestObjectListInputPath )
    REGISTER_TEST( TestUnityInputFile )
    REGISTER_TEST( TestUnityInputPath )
REGISTER_TESTS_END

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
    FileStream f;
    TEST_ASSERT( f.Open( bffFile, FileStream::READ_ONLY ) );
    uint32_t fileSize = (uint32_t)f.GetFileSize();
    AutoPtr< char > mem( (char *)ALLOC( fileSize + 1 ) );
    mem.Get()[ fileSize ] = '\000'; // parser requires sentinel
    TEST_ASSERT( f.Read( mem.Get(), fileSize ) == fileSize );

    FBuild fBuild;
    NodeGraph ng;
    BFFParser p( ng );
    TEST_ASSERT( p.Parse( mem.Get(), fileSize, bffFile, 0, 0 ) );

    Dependencies deps;
    Node * node = ng.FindNode( AStackString<>( target ) );
    TEST_ASSERT( node != nullptr );
    deps.Append( Dependency( node ) );

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

    workDir.JSONEscape();
    testDir.JSONEscape();
    outDir.JSONEscape();
    slash.JSONEscape();

    result.Replace( "{WORKDIR}", workDir.Get() );
    result.Replace( "{TESTDIR}", testDir.Get() );
    result.Replace( "{OUTDIR}", outDir.Get() );
    result.Replace( "{SLASH}", slash.Get() );
}

//------------------------------------------------------------------------------
