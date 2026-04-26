// TestError.cpp
//------------------------------------------------------------------------------

// Includes
//------------------------------------------------------------------------------
#include "Tools/FBuild/FBuildTest/Tests/FBuildTest.h"

//------------------------------------------------------------------------------
TEST_GROUP( TestError, FBuildTest )
{
public:
};

//------------------------------------------------------------------------------
TEST_CASE( TestError, Error )
{
    Parse( "Tools/FBuild/FBuildTest/Data/TestError/error.bff", true ); // true = Expect failure
    TEST_ASSERT( GetRecordedOutput().Find( "#1999 - Error() - User Error: String value is 'Value'" ) );
}

//------------------------------------------------------------------------------
