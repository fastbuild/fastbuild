// TestUserFunctions.cpp
//------------------------------------------------------------------------------

// Includes
//------------------------------------------------------------------------------
#include "Tools/FBuild/FBuildTest/Tests/FBuildTest.h"

// TestUserFunctions
//------------------------------------------------------------------------------
class TestUserFunctions : public FBuildTest
{
private:
    DECLARE_TESTS

    void Declare() const;
    void DeclareWithArgs() const;
    void Invoke() const;
    void InvokeWithArgs() const;
    void InvokeWithArgsLiteralHandling() const;
    void Scope() const;
    void DeferredEvaluation() const;
    void NestedFunctionCalls() const;
    void Recursion() const;
};

// Register Tests
//------------------------------------------------------------------------------
REGISTER_TESTS_BEGIN( TestUserFunctions )
    REGISTER_TEST( Declare )
    REGISTER_TEST( DeclareWithArgs )
    REGISTER_TEST( Invoke )
    REGISTER_TEST( InvokeWithArgs )
    REGISTER_TEST( InvokeWithArgsLiteralHandling )
    REGISTER_TEST( Scope )
    REGISTER_TEST( DeferredEvaluation )
    REGISTER_TEST( NestedFunctionCalls )
    REGISTER_TEST( Recursion )
REGISTER_TESTS_END

// Declare
//------------------------------------------------------------------------------
void TestUserFunctions::Declare() const
{
    // Error cases: Malformed functions
    TEST_PARSE_FAIL( "function",            "#1107 - Expected function name following 'function' keyword");
    TEST_PARSE_FAIL( "function true",       "#1107 - Expected function name following 'function' keyword");
    TEST_PARSE_FAIL( "function Func",       "#1023 - Function requires a header");
    TEST_PARSE_FAIL( "function Func{}",     "#1023 - Function requires a header");
    TEST_PARSE_FAIL( "function Func()",     "#1024 - Function requires a body");

    // Error case: Duplicate definition. Functions must be uniquely named.
    TEST_PARSE_FAIL( "function Func(){}\n"
                     "function Func()\n",   "#1108 - Function 'Func' already defined." );

    // Success cases: Basic no-args functions
    TEST_PARSE_OK( "function Func(){}" );
    TEST_PARSE_OK( "function Func(){ Print( 'X' ) }" );
}

// DeclareWithArgs
//------------------------------------------------------------------------------
void TestUserFunctions::DeclareWithArgs() const
{
    // Functions can take multiple args
    TEST_PARSE_OK( "function Func( .Arg ){}" );
    TEST_PARSE_OK( "function Func( .ArgA .ArgB ){}" );
    TEST_PARSE_OK( "function Func( .ArgA .ArgB .ArgC ){}" );
    TEST_PARSE_OK( "function Func( .ArgA, .ArgB, .ArgC ){}" ); // Commas between args are allowed

    // Function arg names must be unique
    TEST_PARSE_FAIL( "function Func( .ArgA .ArgA ){}",          "#1109 - Function argument '.ArgA' is already defined." );
    TEST_PARSE_FAIL( "function Func( .ArgA .ArgB .ArgA ){}",    "#1109 - Function argument '.ArgA' is already defined." );
}

// Invoke
//------------------------------------------------------------------------------
void TestUserFunctions::Invoke() const
{
    // Error case: Malformed invocation
    TEST_PARSE_FAIL( "function Func(){}\n"
                     "Func",                            "#1110 - Expected argument block following function call." );

    // Error case: Tyring to call a function that does not exist
    TEST_PARSE_FAIL( "FuncThatDoesNotExist()",          "#1010 - Unknown construct." );

    // Success cases: Basic invocations
    TEST_PARSE_OK( "function Func(){}\n"
                   "Func()" );
    TEST_PARSE_OK( "function Func(){ Print( 'Called' ) }\n"
                   "Func()",                            "Called" );
}

// InvokeWithArgs
//------------------------------------------------------------------------------
void TestUserFunctions::InvokeWithArgs() const
{
    // Cannot pass args to functions that don't take args
    TEST_PARSE_FAIL( "function Func(){}\n"
                     "Func( 'BadArg' )",                "#1111 - Function call does not take 1 args (it expects 0 args)." );

    // Can pass literals or variables. Other stuff should raise errors
    TEST_PARSE_FAIL( "function Func(){}\n"
                     "Func( XX )",                      "#1112 - Function call arguments should be literals or variables." );
    TEST_PARSE_FAIL( "function Func( .Arg ){}\n"
                     "Func( XX )",                      "#1112 - Function call arguments should be literals or variables." );

    // Number of arguments must match
    TEST_PARSE_FAIL( "function Func( .ArgA ){}\n"
                     "Func()",                          "#1111 - Function call does not take 0 args (it expects 1 args)." );
    TEST_PARSE_FAIL( "function Func( .ArgA ){}\n"
                     "Func( 'A', 'B' )",                "#1111 - Function call does not take 2 args (it expects 1 args)." );

    // Valid function calls - literals
    // - String
    TEST_PARSE_OK( "function Func( .ArgA ){ Print( 'A = $ArgA$' ) }\n"
                   "Func( 'Hello' )\n",                 "A = Hello" );
    // - Bool
    TEST_PARSE_OK( "function Func( .ArgA ){ Print( 'A = $ArgA$' ) }\n"
                   "Func( true )\n",                    "A = true" );
    // - Int
    TEST_PARSE_OK( "function Func( .ArgA ){ Print( 'A = $ArgA$' ) }\n"
                   "Func( 99 )\n",                      "A = 99" );

    // Valid function calls - variables
    // - String
    TEST_PARSE_OK( "function Func( .ArgA ){ Print( 'A = $ArgA$' ) }\n"
                   ".String = 'Hello'\n"
                   "Func( .String )\n",                 "A = Hello" );
    // - Bool
    TEST_PARSE_OK( "function Func( .ArgA ){ Print( 'A = $ArgA$' ) }\n"
                   ".Bool = true\n"
                   "Func( .Bool )\n",                   "A = true" );
    // - Int
    TEST_PARSE_OK( "function Func( .ArgA ){ Print( 'A = $ArgA$' ) }\n"
                   ".Int = 99\n"
                   "Func( .Int )\n",                    "A = 99" );
    // - Struct
    TEST_PARSE_OK( "function Func( .ArgA ){ Print( .ArgA ) }\n"
                   ".Struct = [ .A = 'Hello' .B = 'Yo' ]\n"
                   "Func( .Struct )\n",                 ".ArgA = // Struct\n"
                                                        "[\n"
                                                        "    .A = 'Hello'\n"
                                                        "    .B = 'Yo'\n"
                                                        "]" );
    // - Array of Strings
    TEST_PARSE_OK( "function Func( .ArgA ){ Print( .ArgA ) }\n"
                   ".Strings = { 'Hello' 'Yo' }\n"
                   "Func( .Strings )\n",                ".ArgA = // ArrayOfStrings, size: 2\n"
                                                        "{\n"
                                                        "    'Hello'\n"
                                                        "    'Yo'\n"
                                                        "}" );

    // Valid function calls - variables with substitutions
    // - String
    TEST_PARSE_OK( "function Func( .ArgA ){ Print( 'A = $ArgA$' ) }\n"
                   ".String = 'Hello'\n"
                   ".X = 'ing'\n"
                   "Func( .'Str$X$' )\n",               "A = Hello" );
    // - Bool
    TEST_PARSE_OK( "function Func( .ArgA ){ Print( 'A = $ArgA$' ) }\n"
                   ".Bool = true\n"
                   ".X = 'ool'\n"
                   "Func( .'B$X$' )\n",                 "A = true" );
    // - Int
    TEST_PARSE_OK( "function Func( .ArgA ){ Print( 'A = $ArgA$' ) }\n"
                   ".Int = 99\n"
                   ".X = 'nt'\n"
                   "Func( .'I$X$' )\n",                 "A = 99" );

    // Multiple Args
    TEST_PARSE_OK( "function Func( .Arg1 .Arg2 .Arg3 )\n"
                   "{\n"
                   "    Print( 'A = $Arg1$' )\n"
                   "    Print( 'B = $Arg2$' )\n"
                   "    Print( 'C = $Arg3$' )\n"
                   "}\n"
                   "Func( 'String', true, 99 )\n",     "A = String\nB = true\nC = 99" );
}

// InvokeWithArgsLiteralHandling
//------------------------------------------------------------------------------
void TestUserFunctions::InvokeWithArgsLiteralHandling() const
{
    // Literal
    TEST_PARSE_OK( "function Func( .ArgA ){ Print( 'A = $ArgA$' ) }\n"
                   "Func( 'Hello' )\n",                 "A = Hello" );

    // Literal with dynamic injection
    TEST_PARSE_OK( "function Func( .ArgA ){ Print( 'A = $ArgA$' ) }\n"
                   ".Str = 'ello'\n"
                   "Func( 'H$Str$' )\n",                "A = Hello" );

    // Literal with escape characters
    TEST_PARSE_OK( "function Func( .ArgA ){ Print( 'A = $ArgA$' ) }\n"
                   "Func( '^$Hello' )\n",               "A = $Hello" );

    // Ensure escaping & substitution occurs before function call and not
    // inside function
    TEST_PARSE_FAIL( "function Func( .ArgA )\n"
                     "{\n"
                     "    .Var = 'X'\n"
                     "    Print( 'A = $ArgA$' )\n"
                     "}\n"
                     "Func( '$X$' )\n",                 "#1009 - Unknown variable 'X'." );

    // Ensure substitution only occurs once
    TEST_PARSE_OK( "function Func( .ArgA, .ArgB )\n"
                   "{\n"
                   "    .X = 'Y'\n"
                   "    Print( 'A = $ArgA$' )\n"
                   "    Print( 'B = $ArgB$' )\n"
                   "    Print( .ArgA )\n"
                   "    Print( .ArgB )\n"
                   "}\n"
                   "Func( '^$X^$', '^^^^' )\n",         "A = $X$\n"
                                                        "B = ^^\n"
                                                        ".ArgA = '$X$'\n"
                                                        ".ArgB = '^^'\n" );
}

// Scope
//------------------------------------------------------------------------------
void TestUserFunctions::Scope() const
{
    // Functions can only see passed in arguments

    TEST_PARSE_FAIL( ".Var = 'X'\n"
                     "function Func(){ Print( .Var ) }\n"
                     "Func()",                          "#1009 - Print() - Unknown variable '.Var'." );
}

// DeferredEvaluation
//------------------------------------------------------------------------------
void TestUserFunctions::DeferredEvaluation() const
{
    // Function evaluation is deferred until invocation

    // Declaration works
    TEST_PARSE_OK( "function Func(){ Print( .Var ) }" );

    // Error occurs on invocation
    TEST_PARSE_FAIL( "function Func(){ Print( .Var ) }\n"
                     "Func()",                          "#1009 - Print() - Unknown variable '.Var'." );
}

// NestedFunctionCalls
//------------------------------------------------------------------------------
void TestUserFunctions::NestedFunctionCalls() const
{
    // Functions may call other functions

    TEST_PARSE_OK( "function FuncA() { Print( 'CalledA' ) }\n"
                   "function FuncB() { FuncA() }\n"
                   "FuncB()",                           "CalledA" );
}

// Recursion
//------------------------------------------------------------------------------
void TestUserFunctions::Recursion() const
{
    // Recursion is supported, and a general cap on depth complexity prevents
    // stack overflows.
    TEST_PARSE_FAIL( "function Func(){ Func() }\n"
                     "Func()",                          "#1035 - Excessive depth complexity when parsing BFFs" );
}

//------------------------------------------------------------------------------
