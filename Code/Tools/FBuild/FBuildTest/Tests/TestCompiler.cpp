// TestCompiler.cpp
//------------------------------------------------------------------------------

// Includes
//------------------------------------------------------------------------------
#include "TestFramework/UnitTest.h"

#include "Tools/FBuild/FBuildCore/FBuild.h"
#include "Tools/FBuild/FBuildCore/BFF/BFFParser.h"
#include "Tools/FBuild/FBuildCore/Graph/NodeGraph.h"

#include "Core/Containers/AutoPtr.h"
#include "Core/FileIO/FileStream.h"

// TestCompiler
//------------------------------------------------------------------------------
class TestCompiler : public UnitTest
{
private:
    DECLARE_TESTS

    void ConflictingFiles1() const;
    void ConflictingFiles2() const;
    void ConflictingFiles3() const;
    void ConflictingFiles4() const;

    void Parse( const char * fileName, bool expectFailure ) const;
};

// Register Tests
//------------------------------------------------------------------------------
REGISTER_TESTS_BEGIN( TestCompiler )
    REGISTER_TEST( ConflictingFiles1 )
    REGISTER_TEST( ConflictingFiles2 )
    REGISTER_TEST( ConflictingFiles3 )
    REGISTER_TEST( ConflictingFiles4 )
REGISTER_TESTS_END

// ConflictingFiles1
//------------------------------------------------------------------------------
void TestCompiler::ConflictingFiles1() const
{
    Parse( "Data/TestCompiler/conflict1.bff", true ); // Expect failure
}

// ConflictingFiles2
//------------------------------------------------------------------------------
void TestCompiler::ConflictingFiles2() const
{
    Parse( "Data/TestCompiler/conflict2.bff", true ); // Expect failure
}

// ConflictingFiles3
//------------------------------------------------------------------------------
void TestCompiler::ConflictingFiles3() const
{
    Parse( "Data/TestCompiler/conflict3.bff", true ); // Expect failure
}

// ConflictingFiles4
//------------------------------------------------------------------------------
void TestCompiler::ConflictingFiles4() const
{
    Parse( "Data/TestCompiler/conflict4.bff", true ); // Expect failure
}

// Parse
//------------------------------------------------------------------------------
void TestCompiler::Parse( const char * fileName, bool expectFailure ) const
{
    FileStream f;
    TEST_ASSERT( f.Open( fileName, FileStream::READ_ONLY ) );
    uint32_t fileSize = (uint32_t)f.GetFileSize();
    AutoPtr< char > mem( (char *)ALLOC( fileSize + 1 ) );
    mem.Get()[ fileSize ] = '\000'; // parser requires sentinel
    TEST_ASSERT( f.Read( mem.Get(), fileSize ) == fileSize );

    FBuild fBuild;
    NodeGraph ng;
    BFFParser p( ng );
    bool parseResult = p.Parse( mem.Get(), fileSize, fileName, 0, 0 );
    if ( expectFailure )
    {
        TEST_ASSERT( parseResult == false ); // Make sure it failed as expected
    }
    else
    {
        TEST_ASSERT( parseResult == true );
    }
}

//------------------------------------------------------------------------------
