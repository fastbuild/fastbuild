// TestVariableStack.cpp
//------------------------------------------------------------------------------

// Includes
//------------------------------------------------------------------------------
#include "TestFramework/UnitTest.h"
#include "Tools/FBuild/FBuildCore/BFF/BFFStackFrame.h"
#include "Tools/FBuild/FBuildCore/BFF/BFFVariable.h"

#include "Core/Strings/AStackString.h"

// TestVariableStack
//------------------------------------------------------------------------------
class TestVariableStack : public UnitTest
{
private:
	DECLARE_TESTS

	void TestStackFramesEmpty() const;
	void TestStackFramesAdditional() const;
	void TestStackFramesOverride() const;
};

// Register Tests
//------------------------------------------------------------------------------
REGISTER_TESTS_BEGIN( TestVariableStack )
	REGISTER_TEST( TestStackFramesEmpty )
	REGISTER_TEST( TestStackFramesAdditional )
	REGISTER_TEST( TestStackFramesOverride )
REGISTER_TESTS_END

// TestStackFramesEmpty
//------------------------------------------------------------------------------
void TestVariableStack::TestStackFramesEmpty() const
{
	BFFStackFrame sf;
	TEST_ASSERT( BFFStackFrame::GetVar( "shouldNotFind" ) == nullptr );
}

// TestStackFramesAdditional
//------------------------------------------------------------------------------
void TestVariableStack::TestStackFramesAdditional() const
{
	// a stack frame with a variable
	BFFStackFrame sf1;
	BFFStackFrame::SetVarString( AStackString<>( "myVarA" ), AStackString<>( "valueA" ) );

	TEST_ASSERT( BFFStackFrame::GetVar( "myVarA" ) );
	TEST_ASSERT( BFFStackFrame::GetVar( "myVarA" )->GetString() == "valueA" );

	// another stack frame
	{
		BFFStackFrame sf2;
		BFFStackFrame::SetVarString( AStackString<>( "myVarB" ), AStackString<>( "valueB" ) );
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

// TestStackFramesOverride
//------------------------------------------------------------------------------
void TestVariableStack::TestStackFramesOverride() const
{
	// a stack frame with a variable
	BFFStackFrame sf1;
	BFFStackFrame::SetVarString( AStackString<>( "myVar" ), AStackString<>( "originalValue" ) );

	TEST_ASSERT( BFFStackFrame::GetVar( "myVar" ) );
	TEST_ASSERT( BFFStackFrame::GetVar( "myVar" )->GetString() == "originalValue" );

	// another stack frame
	{
		// which replaces the same variable
		BFFStackFrame sf2;
		BFFStackFrame::SetVarString( AStackString<>( "myVar" ), AStackString<>( "replacedValue" ) );

		// we should get the replaced value
		TEST_ASSERT( BFFStackFrame::GetVar( "myVar" ) );
		TEST_ASSERT( BFFStackFrame::GetVar( "myVar" )->GetString() == "replacedValue" );
	}

	// sf2 should have fallen out of scope, and we should get the original again
	TEST_ASSERT( BFFStackFrame::GetVar( "myVar" ) );
	TEST_ASSERT( BFFStackFrame::GetVar( "myVar" )->GetString() == "originalValue" );
}

//------------------------------------------------------------------------------
