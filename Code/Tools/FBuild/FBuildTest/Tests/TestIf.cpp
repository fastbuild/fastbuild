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
    void IfFunctionSet() const;
    void IfFunctionBracket() const;
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
    REGISTER_TEST( IfFunctionBracket )
    REGISTER_TEST( UsageError_ExtraTokensAfterExpression )
    REGISTER_TEST( UsageError_UnsupportedTypeForIn )
    REGISTER_TEST( UsageError_UnsupportedOperation )
REGISTER_TESTS_END

#define TEST_EXP_TRUE( vars, exp )        TEST_ASSERT( ParseFromString( true, vars "\nIf ( " exp " )\n{\nPrint('Success')\n}", "Success") );
#define TEST_EXP_FALSE( vars, exp )       TEST_ASSERT( ParseFromString( true, vars "\nIf ( " exp " )\n{\nPrint('Failure')\n}", nullptr, "Failure") );
#define TEST_EXP_FAIL( vars, exp, error ) TEST_ASSERT( ParseFromString( false, vars "\nIf ( " exp " )\n{\n}", error ) );

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
    TEST_EXP_TRUE( "", "false != true" );
    
    TEST_EXP_TRUE( ".Bool = true", ".Bool" );
    TEST_EXP_TRUE( ".Bool = true", ".Bool != false" );
    TEST_EXP_TRUE( ".Bool = true", "false != .Bool" );
    TEST_EXP_FALSE( ".Bool = true", ".Bool != .Bool" );

    TEST_EXP_TRUE(R"(
        .True = true
        .False = false
    )", ".True && true || .False" );

    TEST_EXP_TRUE( R"(
        .True = true
        .False = false
    )", "true && .True && true && .True && true && .True || false || .False" );

    Parse( "Tools/FBuild/FBuildTest/Data/TestIf/if_function_boolean.bff" );
    TEST_ASSERT( GetRecordedOutput().Find( "Failure" ) == nullptr );
    TEST_ASSERT( GetRecordedOutput().Find( "Success" ) );
}

// IfFunctionInt
//------------------------------------------------------------------------------
void TestIf::IfFunctionInt() const
{
    // Literals vs Literals
    TEST_EXP_TRUE( "", "2 == 2" );
    TEST_EXP_TRUE( "", "2 != 1" );
    TEST_EXP_FALSE( "", "2 == 1" );

    TEST_EXP_TRUE( "", "2 > 1" );
    TEST_EXP_TRUE( "", "2 >= 1" );
    TEST_EXP_TRUE( "", "2 < 3" );
    TEST_EXP_TRUE( "", "2 <= 3" );
    

    // Vars vs Literals
    TEST_EXP_TRUE( ".Int = 2", ".Int == 2" );
    TEST_EXP_FALSE( ".Int = 2", ".Int == 1" );

    TEST_EXP_TRUE( ".Int = 2", ".Int > 1" );
    TEST_EXP_TRUE( ".Int = 2", ".Int >= 1" );
    TEST_EXP_TRUE( ".Int = 2", ".Int >= 2" );
    TEST_EXP_TRUE( ".Int = 2", "3 > .Int" );
    TEST_EXP_TRUE( ".Int = 2", "3 >= .Int" );
    TEST_EXP_TRUE( ".Int = 2", "2 >= .Int" );
    
    TEST_EXP_FALSE( ".Int = 2", "2 > .Int" );
    TEST_EXP_FALSE( ".Int = 2", "1 >= .Int" );

    TEST_EXP_TRUE( ".Int = 2", "1 < .Int" );
    TEST_EXP_TRUE( ".Int = 2", "1 <= .Int" );

    // Vars vs Vars
    Parse( "Tools/FBuild/FBuildTest/Data/TestIf/if_function_int.bff" );
    TEST_ASSERT( GetRecordedOutput().Find( "Failure" ) == nullptr );
    TEST_ASSERT( GetRecordedOutput().Find( "Success" ) );
}

// IfFunctionStringCompare
//------------------------------------------------------------------------------
void TestIf::IfFunctionStringCompare() const
{
    // Literals vs Literals
    TEST_EXP_TRUE( "", "'2' == '2'" );
    TEST_EXP_TRUE( "", "'2' != '1'" );
    TEST_EXP_FALSE( "", "'2' == '1'" );

    TEST_EXP_TRUE( "", "'2' > '1'" );
    TEST_EXP_TRUE( "", "'2' >= '1'" );
    TEST_EXP_TRUE( "", "'2' < '3'" );
    TEST_EXP_TRUE( "", "'2' <= '3'" );


    // Vars vs Literals
    TEST_EXP_TRUE( ".String = '2'", ".String == '2'" );
    TEST_EXP_FALSE( ".String = '2'", ".String == '1'" );

    TEST_EXP_TRUE( ".String = '2'", ".String > '1'" );
    TEST_EXP_TRUE( ".String = '2'", ".String >= '1'" );
    TEST_EXP_TRUE( ".String = '2'", ".String >= '2'" );
    TEST_EXP_TRUE( ".String = '2'", "'3' > .String" );
    TEST_EXP_TRUE( ".String = '2'", "'3' >= .String" );
    TEST_EXP_TRUE( ".String = '2'", "'2' >= .String" );

    TEST_EXP_FALSE( ".String = '2'", "'2' > .String" );
    TEST_EXP_FALSE( ".String = '2'", "'1' >= .String" );

    TEST_EXP_TRUE( ".String = '2'", "'1' < .String" );
    TEST_EXP_TRUE( ".String = '2'", "'1' <= .String" );

    // Vars vs Vars
    Parse( "Tools/FBuild/FBuildTest/Data/TestIf/if_function_stringcompare.bff" );
    TEST_ASSERT( GetRecordedOutput().Find( "Failure" ) == nullptr );
    TEST_ASSERT( GetRecordedOutput().Find( "Success" ) );
}

// IfFunctionSet
//------------------------------------------------------------------------------
void TestIf::IfFunctionSet() const
{
#define VARS R"(
    .D = 'd'
    .A = {'a'}
    .AD = {'a', 'd'}
    .ABC = {'a', 'b', 'c'}
)"

    TEST_EXP_TRUE( VARS, ".D in .AD" );
    TEST_EXP_TRUE( VARS, ".D not in .ABC" );
    TEST_EXP_TRUE( VARS, ".AD in .ABC" );
    TEST_EXP_FALSE( VARS, ".D in .ABC" );

#undef VARS
}

void TestIf::IfFunctionBracket() const
{
    TEST_EXP_TRUE( "", "(true)" );
    TEST_EXP_TRUE( "", "(!false)" );
    TEST_EXP_FALSE( "", "(false)" );

    TEST_EXP_TRUE( "", "((((!false))))" );
    TEST_EXP_FAIL( "", "((((!false)))", "Matching closing token ) not found." );

    TEST_EXP_TRUE( "", "(!!true)" );
    TEST_EXP_TRUE( "", "(!(!true))" );
    TEST_EXP_TRUE( "", "(true == true)" );

    TEST_EXP_TRUE( "", "(1 == 1)" );
    TEST_EXP_TRUE( "", "!(1 != 1)" );

    TEST_EXP_TRUE( "", "('String' == 'String')" );
    TEST_EXP_TRUE( "", "!('String' != 'String')" );

    TEST_EXP_TRUE( "", "('String' == 'String') && (1 == 1)" );
    TEST_EXP_TRUE( "", "('String' == 'Hello') || (1 == 1)" );
    TEST_EXP_TRUE( "", "true && (false || true) || ((1 == 2) && ('abc' != 'def'))" );

#define VARS R"(
    .D = 'd'
    .A = {'a'}
    .AD = {'a', 'd'}
    .ABC = {'a', 'b', 'c'}
)"

    TEST_EXP_TRUE( VARS, "(.A in .AD)" );
    TEST_EXP_TRUE( VARS, "(.D in .AD)" );
    TEST_EXP_FALSE( VARS, "(.D in .ABC)" );
    TEST_EXP_TRUE( VARS, "(.D not in .ABC)" );
    TEST_EXP_TRUE( VARS, "(.D == 'd') && (.AD in .ABC)" );

#undef VARS
}

#undef TEST_EXP_OK
#undef TEST_EXP_FAIL

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
    TEST_ASSERT( GetRecordedOutput().Find( "Unexpected operator" ) );
}

// UsageError_UnsupportedOperation
//------------------------------------------------------------------------------
void TestIf::UsageError_UnsupportedOperation() const
{
    Parse( "Tools/FBuild/FBuildTest/Data/TestIf/usageerror_unsupportedoperation.bff", true ); // Expect failure
    TEST_ASSERT( GetRecordedOutput().Find( "Unexpected operator" ) );
}

//------------------------------------------------------------------------------
