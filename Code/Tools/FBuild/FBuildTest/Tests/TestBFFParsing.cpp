// TestBFFParsing.cpp
//------------------------------------------------------------------------------

// Includes
//------------------------------------------------------------------------------
#include "TestFramework/UnitTest.h"

#include "Tools/FBuild/FBuildCore/FBuild.h"
#include "Tools/FBuild/FBuildCore/BFF/BFFParser.h"

#include "Core/Containers/AutoPtr.h"
#include "Core/FileIO/FileStream.h"

// TestBFFParsing
//------------------------------------------------------------------------------
class TestBFFParsing : public UnitTest
{
private:
	DECLARE_TESTS

	void Empty() const;
	void AlmostEmpty() const;
	void Comments() const;
	void Strings() const;
	void String_Unterminated() const;
	void Arrays() const;
	void Array_Unterminated() const;
	void Integers() const;
	void UnnamedScope() const;
	void IncludeDirective() const;
	void OnceDirective() const;
	void Structs() const;
	void Struct_Concatenation() const;
	void Struct_Unterminated() const;
	void IncludeScope() const;
	void IfDirective() const;

	void Parse( const char * fileName, bool expectFailure = false ) const;
};

// Register Tests
//------------------------------------------------------------------------------
REGISTER_TESTS_BEGIN( TestBFFParsing )
	REGISTER_TEST( Empty )
	REGISTER_TEST( AlmostEmpty )
	REGISTER_TEST( Comments )
	REGISTER_TEST( Strings )
	REGISTER_TEST( String_Unterminated )
	REGISTER_TEST( Arrays )
	REGISTER_TEST( Array_Unterminated )
	REGISTER_TEST( Integers )
	REGISTER_TEST( UnnamedScope )
	REGISTER_TEST( IncludeDirective )
	REGISTER_TEST( OnceDirective )
	REGISTER_TEST( Structs )
	REGISTER_TEST( Struct_Concatenation )
	REGISTER_TEST( Struct_Unterminated )
	REGISTER_TEST( IncludeScope )
	REGISTER_TEST( IfDirective )
REGISTER_TESTS_END

// Empty
//------------------------------------------------------------------------------
void TestBFFParsing::Empty() const
{
	// an empty file should pass without problem
	char buffer[ 1 ] = { '\000' }; // post data sentinel
	BFFParser p;
	TEST_ASSERT( p.Parse( buffer, 0, "empty.bff", 0 ) );
}

// AlmostEmpty
//------------------------------------------------------------------------------
void TestBFFParsing::AlmostEmpty() const
{
	// an empty file should pass without problem
	const char * buffer = "\r\n\000"; // empty line + post data sentinel
	BFFParser p;
	TEST_ASSERT( p.Parse( buffer, 2, "empty.bff", 0 ) );
}

// Comments
//------------------------------------------------------------------------------
void TestBFFParsing::Comments() const
{
	Parse( "Data/TestBFFParsing/comments.bff" );
}

// Strings
//------------------------------------------------------------------------------
void TestBFFParsing::Strings() const
{
	Parse( "Data/TestBFFParsing/strings.bff" );
}

// String_Unterminated
//------------------------------------------------------------------------------
void TestBFFParsing::String_Unterminated() const
{
	Parse( "Data/TestBFFParsing/string_unterminated.bff", true ); // expect failure
}

// Arrays
//------------------------------------------------------------------------------
void TestBFFParsing::Arrays() const
{
	Parse( "Data/TestBFFParsing/arrays.bff" );
}

// Array_Unterminated
//------------------------------------------------------------------------------
void TestBFFParsing::Array_Unterminated() const
{
	Parse( "Data/TestBFFParsing/array_unterminated.bff", true ); // expect failure
}

// Integers
//------------------------------------------------------------------------------
void TestBFFParsing::Integers() const
{
	Parse( "Data/TestBFFParsing/integers.bff" );
}

// UnnamedScope
//------------------------------------------------------------------------------
void TestBFFParsing::UnnamedScope() const
{
	Parse( "Data/TestBFFParsing/unnamedscope.bff" );
}

// IncludeDirective
//------------------------------------------------------------------------------
void TestBFFParsing::IncludeDirective() const
{
	Parse( "Data/TestBFFParsing/includes.bff" );
}

// OnceDirective
//------------------------------------------------------------------------------
void TestBFFParsing::OnceDirective() const
{
	Parse( "Data/TestBFFParsing/include_once.bff" );
}

// Structs
//------------------------------------------------------------------------------
void TestBFFParsing::Structs() const
{
	Parse( "Data/TestBFFParsing/structs.bff" );
}

// Struct_Concatenation
//------------------------------------------------------------------------------
void TestBFFParsing::Struct_Concatenation() const
{
	Parse( "Data/TestBFFParsing/struct_concatenation.bff" );
}

// Struct_Unterminated
//------------------------------------------------------------------------------
void TestBFFParsing::Struct_Unterminated() const
{
	Parse( "Data/TestBFFParsing/struct_unterminated.bff", true ); // expect failure
}

// Parse
//------------------------------------------------------------------------------
void TestBFFParsing::Parse( const char * fileName, bool expectFailure ) const
{
	FileStream f;
	TEST_ASSERT( f.Open( fileName, FileStream::READ_ONLY ) );
	uint32_t fileSize = (uint32_t)f.GetFileSize();
	AutoPtr< char > mem( (char *)ALLOC( fileSize + 1 ) );
	mem.Get()[ fileSize ] = '\000'; // parser requires sentinel
	TEST_ASSERT( f.Read( mem.Get(), fileSize ) == fileSize );

	FBuild fBuild;
	BFFParser p;
	bool parseResult = p.Parse( mem.Get(), fileSize, fileName, 0 );
	if ( expectFailure )
	{
		TEST_ASSERT( parseResult == false ); // Make sure it failed as expected
	}
	else
	{
		TEST_ASSERT( parseResult == true );
	}
}

// IncludeScope
//------------------------------------------------------------------------------
void TestBFFParsing::IncludeScope() const
{
	Parse( "Data/TestBFFParsing/include_scope.bff" );
}

// IfDerective
//------------------------------------------------------------------------------
void TestBFFParsing::IfDirective() const
{
	Parse( "Data/TestBFFParsing/if_directive.bff" );
}

//------------------------------------------------------------------------------
