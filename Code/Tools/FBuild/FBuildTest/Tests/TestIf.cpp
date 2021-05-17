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
    REGISTER_TEST( IfFunctionSet )
    REGISTER_TEST( IfFunctionStringCompare )
    REGISTER_TEST( IfFunctionBracket )
    REGISTER_TEST( UsageError_ExtraTokensAfterExpression )
    REGISTER_TEST( UsageError_UnsupportedTypeForIn )
    REGISTER_TEST( UsageError_UnsupportedOperation )
REGISTER_TESTS_END

#define TEST_EXP_TRUE( vars, exp )        TEST_ASSERT( ParseFromString( true, vars "\nIf ( " exp " )\n{\nPrint('Success')\n}", "Success") )
#define TEST_EXP_FALSE( vars, exp )       TEST_ASSERT( ParseFromString( true, vars "\nIf ( " exp " )\n{\nPrint('Failure')\n}", nullptr, "Failure") )
#define TEST_EXP_FAIL( vars, exp, error ) TEST_ASSERT( ParseFromString( false, vars "\nIf ( " exp " )\n{\n}", error ) )

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
    // Literals
    TEST_EXP_TRUE( "", "true" );
    TEST_EXP_FALSE( "", "false" );

    // Unary
    TEST_EXP_TRUE( "", "!false" );
    TEST_EXP_FALSE( "", "!true" );
    
    // Binary
    TEST_EXP_TRUE( "", "false != true" );

    // Unary and Binary
    TEST_EXP_TRUE( "", "!false == true" );
    TEST_EXP_TRUE( "", "!true == false" );

    // Vars
    TEST_EXP_TRUE( ".Bool = true", ".Bool" );
    TEST_EXP_TRUE( ".Bool = true", ".Bool != false" );
    TEST_EXP_TRUE( ".Bool = true", "false != .Bool" );
    TEST_EXP_FALSE( ".Bool = true", ".Bool != .Bool" );

    // Compound Exps
    TEST_EXP_TRUE( ".True = true\n .False = false", ".True && true || .False" );

    TEST_EXP_TRUE( ".True = true\n .False = false", "true && .True && true || !false || .False" );

    // Legacy
    Parse( "Tools/FBuild/FBuildTest/Data/TestIf/if_function_boolean.bff" );
    TEST_ASSERT( GetRecordedOutput().Find( "Failure" ) == nullptr );
    TEST_ASSERT( GetRecordedOutput().Find( "Success" ) );
}

// IfFunctionInt
//------------------------------------------------------------------------------
void TestIf::IfFunctionInt() const
{
    // Literals vs Literals
    // Equality
    TEST_EXP_TRUE( "", "2 == 2" );
    TEST_EXP_TRUE( "", "2 != 1" );
    TEST_EXP_FALSE( "", "2 == 1" );

    // Less Than
    TEST_EXP_TRUE( "", "2 < 3" );
    TEST_EXP_TRUE( "", "2 <= 3" );
    TEST_EXP_FALSE( "", "2 <= 1" );
    
    // Greater Than
    TEST_EXP_TRUE( "", "2 > 1" );
    TEST_EXP_TRUE( "", "2 >= 1" );
    TEST_EXP_FALSE( "", "2 >= 3" );

    // Vars vs Literals
    // Equality
    TEST_EXP_TRUE( ".Int = 2", ".Int == 2" );
    TEST_EXP_FALSE( ".Int = 2", ".Int == 1" );

    // Less Than
    TEST_EXP_TRUE( ".Int = 2", ".Int < 3" );
    TEST_EXP_TRUE( ".Int = 2", ".Int <= 3" );
    TEST_EXP_TRUE( ".Int = 2", ".Int <= 2" );
    TEST_EXP_TRUE( ".Int = 2", "1 < .Int" );
    TEST_EXP_TRUE( ".Int = 2", "1 <= .Int" );
    TEST_EXP_TRUE( ".Int = 2", "2 <= .Int" );
    TEST_EXP_FALSE( ".Int = 2", ".Int >= 3" );

    // Greater Than
    TEST_EXP_TRUE( ".Int = 2", ".Int > 1" );
    TEST_EXP_TRUE( ".Int = 2", ".Int >= 1" );
    TEST_EXP_TRUE( ".Int = 2", ".Int >= 2" );
    TEST_EXP_TRUE( ".Int = 2", "3 > .Int" );
    TEST_EXP_TRUE( ".Int = 2", "3 >= .Int" );
    TEST_EXP_TRUE( ".Int = 2", "2 >= .Int" );
    TEST_EXP_FALSE( ".Int = 2", "1 >= .Int" );

    // Vars vs Vars
#define VARS \
    ".IntA = 7\n" \
    ".IntB = 100\n"

    // Equality
    TEST_EXP_FALSE( VARS, ".IntA == .IntB" );
    TEST_EXP_TRUE( VARS, ".IntA != .IntB" );

    // Less Than
    TEST_EXP_FALSE( VARS, ".IntB < .IntA" );
    TEST_EXP_FALSE( VARS, ".IntB <= .IntA" );

    // Greater Than
    TEST_EXP_FALSE( VARS, ".IntA > .IntB" );
    TEST_EXP_FALSE( VARS, ".IntA >= .IntB" );

#undef VARS

    // Legacy
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
    TEST_EXP_TRUE( ".String = '2'", ".String != '1'" );
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

#define VARS \
    ".String = 'Hello'\n" \
    ".OtherStringA = 'Hello'\n" \
    ".OtherStringB = 'Goodbye'\n"

    // Strings match
    TEST_EXP_TRUE( VARS, ".String == .OtherStringA" );
    TEST_EXP_FALSE( VARS, ".String != .OtherStringA" );

    // Strings don't match
    TEST_EXP_FALSE( VARS, ".String == .OtherStringB" );
    TEST_EXP_TRUE( VARS, ".String != .OtherStringB" );

#undef VARS

    // String compare
#define VARS \
    ".StringA = 'AAA'\n" \
    ".StringB = 'BBB'\n"

    TEST_EXP_FALSE( VARS, ".StringA > .StringB" );
    TEST_EXP_FALSE( VARS, ".StringA >= .StringB" );
    TEST_EXP_FALSE( VARS, ".StringB < .StringA" );
    TEST_EXP_FALSE( VARS, ".StringB <= .StringA" );

#undef VARS

    // Legacy
    Parse( "Tools/FBuild/FBuildTest/Data/TestIf/if_function_stringcompare.bff" );
    TEST_ASSERT( GetRecordedOutput().Find( "Failure" ) == nullptr );
    TEST_ASSERT( GetRecordedOutput().Find( "Success" ) );
}

// IfFunctionSet
//------------------------------------------------------------------------------
void TestIf::IfFunctionSet() const
{
#define VARS \
    ".D = 'd'\n" \
    ".A = {'a'}\n" \
    ".AD = {'a', 'd'}\n" \
    ".ABC = {'a', 'b', 'c'}\n"

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

#define VARS \
    ".D = 'd'\n" \
    ".A = {'a'}\n" \
    ".AD = {'a', 'd'}\n" \
    ".ABC = {'a', 'b', 'c'}\n"

    TEST_EXP_TRUE( VARS, "(.A in .AD)" );
    TEST_EXP_TRUE( VARS, "(.D in .AD)" );
    TEST_EXP_FALSE( VARS, "(.D in .ABC)" );
    TEST_EXP_TRUE( VARS, "(.D not in .ABC)" );
    TEST_EXP_TRUE( VARS, "(.D == 'd') && (.AD in .ABC)" );

#undef VARS
}

#undef TEST_EXP_TRUE
#undef TEST_EXP_FALSE
#undef TEST_EXP_FAIL

// UsageError_ExtraTokensAfterExpression
//------------------------------------------------------------------------------
void TestIf::UsageError_ExtraTokensAfterExpression() const
{
    Parse( "Tools/FBuild/FBuildTest/Data/TestIf/usageerror_extratokensafterexpression.bff", true ); // Expect failure
    TEST_ASSERT( GetRecordedOutput().Find( "Unexpected token 'and'" ) );
}

// UsageError_UnsupportedTypeForIn
//------------------------------------------------------------------------------
void TestIf::UsageError_UnsupportedTypeForIn() const
{
    Parse( "Tools/FBuild/FBuildTest/Data/TestIf/usageerror_unsupportedtypeforin.bff", true ); // Expect failure
    TEST_ASSERT( GetRecordedOutput().Find( "Unexpected operator 'in'" ) );
}

// UsageError_UnsupportedOperation
//------------------------------------------------------------------------------
void TestIf::UsageError_UnsupportedOperation() const
{
    Parse( "Tools/FBuild/FBuildTest/Data/TestIf/usageerror_unsupportedoperation.bff", true ); // Expect failure
    TEST_ASSERT( GetRecordedOutput().Find( "Unexpected operator '>='" ) );
}

//------------------------------------------------------------------------------
