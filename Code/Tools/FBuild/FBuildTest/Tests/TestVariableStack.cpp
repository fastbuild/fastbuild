// TestVariableStack.cpp
//------------------------------------------------------------------------------

// Includes
//------------------------------------------------------------------------------
#include "Tools/FBuild/FBuildTest/Tests/FBuildTest.h"

#include "Tools/FBuild/FBuildCore/BFF/BFFStackFrame.h"
#include "Tools/FBuild/FBuildCore/BFF/BFFVariable.h"
#include "Tools/FBuild/FBuildCore/BFF/Tokenizer/BFFToken.h"

#include "Core/Strings/AStackString.h"

//------------------------------------------------------------------------------
TEST_GROUP( TestVariableStack, FBuildTest )
{
public:
};

//------------------------------------------------------------------------------
TEST_CASE( TestVariableStack, TestStackFramesEmpty )
{
    BFFStackFrame sf;
    TEST_ASSERT( BFFStackFrame::GetVar( "shouldNotFind" ) == nullptr );
}

//------------------------------------------------------------------------------
TEST_CASE( TestVariableStack, TestStackFramesAdditional )
{
    // a stack frame with a variable
    BFFStackFrame sf1;
    BFFStackFrame::SetVarString( AStackString( "myVarA" ), BFFToken::GetBuiltInToken(), AStackString( "valueA" ), nullptr );

    TEST_ASSERT( BFFStackFrame::GetVar( "myVarA" ) );
    TEST_ASSERT( BFFStackFrame::GetVar( "myVarA" )->GetString() == "valueA" );

    // another stack frame
    {
        BFFStackFrame sf2;
        BFFStackFrame::SetVarString( AStackString( "myVarB" ), BFFToken::GetBuiltInToken(), AStackString( "valueB" ), nullptr );
        TEST_ASSERT( BFFStackFrame::GetVar( "myVarA" ) );
        TEST_ASSERT( BFFStackFrame::GetVar( "myVarA" )->GetString() == "valueA" );
        TEST_ASSERT( BFFStackFrame::GetVar( "myVarB" ) );
        TEST_ASSERT( BFFStackFrame::GetVar( "myVarB" )->GetString() == "valueB" );
    }

    // sf2 should have fallen out of scope
    TEST_ASSERT( BFFStackFrame::GetVar( "myVarA" ) );
    TEST_ASSERT( BFFStackFrame::GetVar( "myVarA" )->GetString() == "valueA" );
    TEST_ASSERT( BFFStackFrame::GetVar( "myVarB" ) == nullptr );
}

//------------------------------------------------------------------------------
TEST_CASE( TestVariableStack, TestStackFramesOverride )
{
    // a stack frame with a variable
    BFFStackFrame sf1;
    BFFStackFrame::SetVarString( AStackString( "myVar" ), BFFToken::GetBuiltInToken(), AStackString( "originalValue" ), nullptr );

    TEST_ASSERT( BFFStackFrame::GetVar( "myVar" ) );
    TEST_ASSERT( BFFStackFrame::GetVar( "myVar" )->GetString() == "originalValue" );

    // another stack frame
    {
        // which replaces the same variable
        BFFStackFrame sf2;
        BFFStackFrame::SetVarString( AStackString( "myVar" ), BFFToken::GetBuiltInToken(), AStackString( "replacedValue" ), nullptr );

        // we should get the replaced value
        TEST_ASSERT( BFFStackFrame::GetVar( "myVar" ) );
        TEST_ASSERT( BFFStackFrame::GetVar( "myVar" )->GetString() == "replacedValue" );
    }

    // sf2 should have fallen out of scope, and we should get the original again
    TEST_ASSERT( BFFStackFrame::GetVar( "myVar" ) );
    TEST_ASSERT( BFFStackFrame::GetVar( "myVar" )->GetString() == "originalValue" );
}

//------------------------------------------------------------------------------
TEST_CASE( TestVariableStack, TestStackFramesParent )
{
    const BFFVariable * v = nullptr;

    // a stack frame with a variable
    BFFStackFrame sf1;
    BFFStackFrame::SetVarString( AStackString( "myVar" ), BFFToken::GetBuiltInToken(), AStackString( "originalValue" ), nullptr );

    TEST_ASSERT( BFFStackFrame::GetVar( "myVar", &sf1 ) );
    TEST_ASSERT( BFFStackFrame::GetVar( "myVar", &sf1 )->GetString() == "originalValue" );

    // there is no previous declaration
    TEST_ASSERT( BFFStackFrame::GetParentDeclaration( "myVar", nullptr, v ) == nullptr );
    TEST_ASSERT( BFFStackFrame::GetParentDeclaration( "myVar", &sf1, v ) == nullptr );

    // another stack frame
    {
        BFFStackFrame sf2;

        TEST_ASSERT( BFFStackFrame::GetVar( "myVar", &sf2 ) == nullptr );

        // which replaces the same variable
        BFFStackFrame::SetVarString( AStackString( "myVar" ), BFFToken::GetBuiltInToken(), AStackString( "replacedValue" ), &sf2 );

        // we should bet the original value
        TEST_ASSERT( BFFStackFrame::GetVar( "myVar", &sf1 ) );
        TEST_ASSERT( BFFStackFrame::GetVar( "myVar", &sf1 )->GetString() == "originalValue" );

        // we should get the replaced value
        TEST_ASSERT( BFFStackFrame::GetVar( "myVar", &sf2 ) );
        TEST_ASSERT( BFFStackFrame::GetVar( "myVar", &sf2 )->GetString() == "replacedValue" );

        // but there is a previous declaration
        TEST_ASSERT( BFFStackFrame::GetParentDeclaration( "myVar", nullptr, v ) == &sf1 );
        TEST_ASSERT( BFFStackFrame::GetParentDeclaration( "myVar", &sf2, v ) == &sf1 );

        // another stack frame
        {
            BFFStackFrame sf3;

            // but there is still a previous declaration
            TEST_ASSERT( BFFStackFrame::GetParentDeclaration( "myVar", nullptr, v ) == &sf2 );
            TEST_ASSERT( BFFStackFrame::GetParentDeclaration( "myVar", &sf3, v ) == &sf2 );
            TEST_ASSERT( BFFStackFrame::GetParentDeclaration( "myVar", &sf2, v ) == &sf1 );

            // we should bet the original value
            TEST_ASSERT( BFFStackFrame::GetVar( "myVar", &sf1 ) );
            TEST_ASSERT( BFFStackFrame::GetVar( "myVar", &sf1 )->GetString() == "originalValue" );

            // we should get the replaced value
            TEST_ASSERT( BFFStackFrame::GetVar( "myVar", nullptr ) );
            TEST_ASSERT( BFFStackFrame::GetVar( "myVar", nullptr )->GetString() == "replacedValue" );
        }
    }

    // sf2 should have fallen out of scope, and we should get the original again
    TEST_ASSERT( BFFStackFrame::GetVar( "myVar", nullptr ) );
    TEST_ASSERT( BFFStackFrame::GetVar( "myVar", nullptr )->GetString() == "originalValue" );

    // there is still no previous declaration
    TEST_ASSERT( BFFStackFrame::GetParentDeclaration( "myVar", nullptr, v ) == nullptr );
    TEST_ASSERT( BFFStackFrame::GetParentDeclaration( "myVar", &sf1, v ) == nullptr );
}

//------------------------------------------------------------------------------
