// TestError.cpp
//------------------------------------------------------------------------------

// Includes
//------------------------------------------------------------------------------
#include "Tools/FBuild/FBuildTest/Tests/FBuildTest.h"

#include "Tools/FBuild/FBuildCore/FBuild.h"
#include "Tools/FBuild/FBuildCore/BFF/BFFParser.h"
#include "Tools/FBuild/FBuildCore/Graph/NodeGraph.h"

#include "Core/Containers/AutoPtr.h"
#include "Core/FileIO/FileStream.h"

// TestError
//------------------------------------------------------------------------------
class TestError : public FBuildTest
{
private:
    DECLARE_TESTS

    void Error() const;

    void Parse( const char * fileName, bool expectFailure = false ) const;
};

// Register Tests
//------------------------------------------------------------------------------
REGISTER_TESTS_BEGIN( TestError )
    REGISTER_TEST( Error )
REGISTER_TESTS_END

// Error
//------------------------------------------------------------------------------
void TestError::Error() const
{
    Parse( "Tools/FBuild/FBuildTest/Data/TestError/error.bff", true ); // true = Expect failure
    TEST_ASSERT( GetRecordedOutput().Find( "#1999 - Error() - User Error: String value is 'Value'" ) );
}

// Parse
//------------------------------------------------------------------------------
void TestError::Parse( const char * fileName, bool expectFailure ) const
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
