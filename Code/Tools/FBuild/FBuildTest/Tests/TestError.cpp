// TestError.cpp
//------------------------------------------------------------------------------

// Includes
//------------------------------------------------------------------------------
#include "Tools/FBuild/FBuildTest/Tests/FBuildTest.h"

// FBuildCore
#include "Tools/FBuild/FBuildCore/BFF/BFFParser.h"
#include "Tools/FBuild/FBuildCore/FBuild.h"
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

//------------------------------------------------------------------------------
