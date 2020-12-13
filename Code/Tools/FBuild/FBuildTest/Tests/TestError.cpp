// TestError.cpp
//------------------------------------------------------------------------------

// Includes
//------------------------------------------------------------------------------
#include "Tools/FBuild/FBuildTest/Tests/FBuildTest.h"

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
