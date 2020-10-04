// TestIf.cpp
//------------------------------------------------------------------------------

// Includes
//------------------------------------------------------------------------------
#include "Tools/FBuild/FBuildTest/Tests/FBuildTest.h"

// TestIf
//------------------------------------------------------------------------------
class TestIf : public FBuildTest
{
private:
    DECLARE_TESTS

    void IfFunctionTrue() const;
    void IfNotFunctionTrue() const;
    void IfSetFunctionTrue() const;
    void IfNotSetFunctionTrue() const;
    void IfFunctionFalse() const;
    void IfNotFunctionFalse() const;
    void IfSetFunctionFalse() const;
    void IfNotSetFunctionFalse() const;
    void IfFunctionBool() const;
    void IfFunctionInt() const;
    void IfFunctionStringCompare() const;
    void UsageError_ExtraTokensAfterExpression() const;
    void UsageError_UnsupportedTypeForIn() const;
    void UsageError_UnsupportedOperation() const;
};

// Register Tests
//------------------------------------------------------------------------------
REGISTER_TESTS_BEGIN( TestIf )
    REGISTER_TEST( IfFunctionTrue )
    REGISTER_TEST( IfNotFunctionTrue )
    REGISTER_TEST( IfSetFunctionTrue )
    REGISTER_TEST( IfNotSetFunctionTrue )
    REGISTER_TEST( IfFunctionFalse )
    REGISTER_TEST( IfNotFunctionFalse )
    REGISTER_TEST( IfSetFunctionFalse )
    REGISTER_TEST( IfNotSetFunctionFalse )
    REGISTER_TEST( IfFunctionBool )
    REGISTER_TEST( IfFunctionInt )
    REGISTER_TEST( IfFunctionStringCompare )
    REGISTER_TEST( UsageError_ExtraTokensAfterExpression )
    REGISTER_TEST( UsageError_UnsupportedTypeForIn )
    REGISTER_TEST( UsageError_UnsupportedOperation )
REGISTER_TESTS_END

// IfFunctionTrue
//------------------------------------------------------------------------------
void TestIf::IfFunctionTrue() const
{
    Parse( "Tools/FBuild/FBuildTest/Data/TestIf/if_function_true.bff" );
    TEST_ASSERT( GetRecordedOutput().Find( "Success" ) );
}

// IfNotFunctionTrue
//------------------------------------------------------------------------------
void TestIf::IfNotFunctionTrue() const
{
    Parse( "Tools/FBuild/FBuildTest/Data/TestIf/if_function_true.bff" );
    TEST_ASSERT( GetRecordedOutput().Find( "Success" ) );
}

// IfSetFunctionTrue
//------------------------------------------------------------------------------
void TestIf::IfSetFunctionTrue() const
{
    Parse( "Tools/FBuild/FBuildTest/Data/TestIf/if_set_function_true.bff" );
    TEST_ASSERT( GetRecordedOutput().Find( "Success" ) );
}

// IfNotSetFunctionTrue
//------------------------------------------------------------------------------
void TestIf::IfNotSetFunctionTrue() const
{
    Parse( "Tools/FBuild/FBuildTest/Data/TestIf/if_not_set_function_true.bff" );
    TEST_ASSERT( GetRecordedOutput().Find( "Success" ) );
}

// IfFunctionFalse
//------------------------------------------------------------------------------
void TestIf::IfFunctionFalse() const
{
    Parse( "Tools/FBuild/FBuildTest/Data/TestIf/if_function_false.bff" );
    TEST_ASSERT( GetRecordedOutput().Find( "Failure" ) == nullptr );
}

// IfNotFunctionFalse
//------------------------------------------------------------------------------
void TestIf::IfNotFunctionFalse() const
{
    Parse( "Tools/FBuild/FBuildTest/Data/TestIf/if_function_false.bff" );
    TEST_ASSERT( GetRecordedOutput().Find( "Failure" ) == nullptr );
}

// IfSetFunctionFalse
//------------------------------------------------------------------------------
void TestIf::IfSetFunctionFalse() const
{
    Parse( "Tools/FBuild/FBuildTest/Data/TestIf/if_set_function_false.bff" );
    TEST_ASSERT( GetRecordedOutput().Find( "Failure" ) == nullptr );
}

// IfNotSetFunctionFalse
//------------------------------------------------------------------------------
void TestIf::IfNotSetFunctionFalse() const
{
    Parse( "Tools/FBuild/FBuildTest/Data/TestIf/if_not_set_function_false.bff" );
    TEST_ASSERT( GetRecordedOutput().Find( "Failure" ) == nullptr );
}

// IfFunctionBool
//------------------------------------------------------------------------------
void TestIf::IfFunctionBool() const
{
    Parse( "Tools/FBuild/FBuildTest/Data/TestIf/if_function_boolean.bff" );
    TEST_ASSERT( GetRecordedOutput().Find( "Failure" ) == nullptr );
    TEST_ASSERT( GetRecordedOutput().Find( "Success" ) );
}

// IfFunctionInt
//------------------------------------------------------------------------------
void TestIf::IfFunctionInt() const
{
    Parse( "Tools/FBuild/FBuildTest/Data/TestIf/if_function_int.bff" );
    TEST_ASSERT( GetRecordedOutput().Find( "Failure" ) == nullptr );
    TEST_ASSERT( GetRecordedOutput().Find( "Success" ) );
}

// IfFunctionStringCompare
//------------------------------------------------------------------------------
void TestIf::IfFunctionStringCompare() const
{
    Parse( "Tools/FBuild/FBuildTest/Data/TestIf/if_function_stringcompare.bff" );
    TEST_ASSERT( GetRecordedOutput().Find( "Failure" ) == nullptr );
    TEST_ASSERT( GetRecordedOutput().Find( "Success" ) );
}

// UsageError_ExtraTokensAfterExpression
//------------------------------------------------------------------------------
void TestIf::UsageError_ExtraTokensAfterExpression() const
{
    Parse( "Tools/FBuild/FBuildTest/Data/TestIf/usageerror_extratokensafterexpression.bff", true ); // Expect failure
    TEST_ASSERT( GetRecordedOutput().Find( "Matching closing token ) not found" ) );
}

// UsageError_UnsupportedTypeForIn
//------------------------------------------------------------------------------
void TestIf::UsageError_UnsupportedTypeForIn() const
{
    Parse( "Tools/FBuild/FBuildTest/Data/TestIf/usageerror_unsupportedtypeforin.bff", true ); // Expect failure
    TEST_ASSERT( GetRecordedOutput().Find( "Property '.Int' must be of type <ArrayOfStrings> or <String> (found <Int>" ) );
}

// UsageError_UnsupportedOperation
//------------------------------------------------------------------------------
void TestIf::UsageError_UnsupportedOperation() const
{
    Parse( "Tools/FBuild/FBuildTest/Data/TestIf/usageerror_unsupportedoperation.bff", true ); // Expect failure
    TEST_ASSERT( GetRecordedOutput().Find( "Operation not supported: 'Bool' >= 'Bool'" ) );
}

//------------------------------------------------------------------------------
