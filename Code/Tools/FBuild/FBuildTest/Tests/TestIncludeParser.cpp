// TestIncludeParser.cpp
//------------------------------------------------------------------------------

// Includes
//------------------------------------------------------------------------------
#include "TestFramework/UnitTest.h"

#include "Tools/FBuild/FBuildCore/FBuild.h"
#include "Tools/FBuild/FBuildCore/Helpers/CIncludeParser.h"

// Core
#include "Core/Containers/AutoPtr.h"
#include "Core/FileIO/FileStream.h"
#include "Core/Strings/AStackString.h"
#include "Core/Time/Timer.h"
#include "Core/Tracing/Tracing.h"

// TestIncludeParser
//------------------------------------------------------------------------------
class TestIncludeParser : public UnitTest
{
private:
	DECLARE_TESTS

	void TestMSVCPreprocessedOutput() const;
	void TestMSVCShowIncludesOutput() const;
	void TestGCCPreprocessedOutput() const;
	void TestClangPreprocessedOutput() const;
	void TestClangMSExtensionsPreprocessedOutput() const;
	void TestEdgeCases() const;
};

// Register Tests
//------------------------------------------------------------------------------
REGISTER_TESTS_BEGIN( TestIncludeParser )
    #if defined( __WINDOWS__ )
        REGISTER_TEST( TestMSVCPreprocessedOutput );
        REGISTER_TEST( TestMSVCShowIncludesOutput );
    #endif
	REGISTER_TEST( TestGCCPreprocessedOutput );
	REGISTER_TEST( TestClangPreprocessedOutput );
	REGISTER_TEST( TestClangMSExtensionsPreprocessedOutput );
	REGISTER_TEST( TestEdgeCases );
REGISTER_TESTS_END

// TestMSVCPreprocessedOutput
//------------------------------------------------------------------------------
void TestIncludeParser::TestMSVCPreprocessedOutput() const
{
	FileStream f;
	TEST_ASSERT( f.Open( "Data/TestIncludeParser/fbuildcore.msvc.ii", FileStream::READ_ONLY) )
	const size_t fileSize = (size_t)f.GetFileSize();
	AutoPtr< char > mem( (char *)ALLOC( fileSize + 1 ) );
	TEST_ASSERT( f.Read( mem.Get(), fileSize ) == fileSize );
	mem.Get()[ fileSize ] = 0;

	Timer t;

	const size_t repeatCount( 100 );
	for ( size_t i=0; i<repeatCount; ++i )
	{
		CIncludeParser parser;
		TEST_ASSERT( parser.ParseMSCL_Preprocessed( mem.Get(), fileSize ) );

		// check number of includes found to prevent future regressions
		const Array< AString > & includes = parser.GetIncludes();
		TEST_ASSERT( includes.GetSize() == 284 );
		#ifdef DEBUG
			TEST_ASSERT( parser.GetNonUniqueCount() == 4701 );
		#endif
	}

	float time = t.GetElapsed();
	OUTPUT( "MSVC                 : %2.3fs (%2.1f MiB/sec)\n", time, ( (float)( fileSize * repeatCount / ( 1024.0f * 1024.0f ) ) / time ) );
}

// TestMSVCShowIncludesOutput
//------------------------------------------------------------------------------
void TestIncludeParser::TestMSVCShowIncludesOutput() const
{
	FileStream f;
	TEST_ASSERT( f.Open( "Data/TestIncludeParser/fbuildcore.msvc.showincludes", FileStream::READ_ONLY) )
	const size_t fileSize = (size_t)f.GetFileSize();
	AutoPtr< char > mem( (char *)Alloc( fileSize + 1 ) );
	TEST_ASSERT( f.Read( mem.Get(), fileSize ) == fileSize );
	mem.Get()[ fileSize ] = 0;

	Timer t;

	const size_t repeatCount( 100 );
	for ( size_t i=0; i<repeatCount; ++i )
	{
		CIncludeParser parser;
		TEST_ASSERT( parser.ParseMSCL_Output( mem.Get(), fileSize ) );

		// check number of includes found to prevent future regressions
		const Array< AString > & includes = parser.GetIncludes();
		TEST_ASSERT( includes.GetSize() == 189 );
		#ifdef DEBUG
			TEST_ASSERT( parser.GetNonUniqueCount() == 258 );
		#endif
	}

	float time = t.GetElapsed();
	OUTPUT( "MSVC /showincludes   : %2.3fs (%2.1f MiB/sec)\n", time, ( (float)( fileSize * repeatCount / ( 1024.0f * 1024.0f ) ) / time ) );
}

// TestGCCPreprocessedOutput
//------------------------------------------------------------------------------
void TestIncludeParser::TestGCCPreprocessedOutput() const
{
	FBuild fBuild; // needed fer CleanPath for relative dirs

	FileStream f;
	TEST_ASSERT( f.Open( "Data/TestIncludeParser/fbuildcore.gcc.ii", FileStream::READ_ONLY) )
	const size_t fileSize = (size_t)f.GetFileSize();
	AutoPtr< char > mem( (char *)ALLOC( fileSize + 1 ) );
	TEST_ASSERT( f.Read( mem.Get(), fileSize ) == fileSize );
	mem.Get()[ fileSize ] = 0;

	Timer t;

	const size_t repeatCount( 100 );
	for ( size_t i=0; i<repeatCount; ++i )
	{
		CIncludeParser parser;
		TEST_ASSERT( parser.ParseGCC_Preprocessed( mem.Get(), fileSize ) );

		// check number of includes found to prevent future regressions
		const Array< AString > & includes = parser.GetIncludes();
		TEST_ASSERT( includes.GetSize() == 222 );
		#ifdef DEBUG
			TEST_ASSERT( parser.GetNonUniqueCount() == 1029 );
		#endif
	}

	float time = t.GetElapsed();
	OUTPUT( "GCC                  : %2.3fs (%2.1f MiB/sec)\n", time, ( (float)( fileSize * repeatCount / ( 1024.0f * 1024.0f ) ) / time ) );	
}

// TestClangPreprocessedOutput
//------------------------------------------------------------------------------
void TestIncludeParser::TestClangPreprocessedOutput() const
{
    FBuild fBuild; // needed fer CleanPath for relative dirs

	FileStream f;
	TEST_ASSERT( f.Open( "Data/TestIncludeParser/fbuildcore.clang.ii", FileStream::READ_ONLY) )
	const size_t fileSize = (size_t)f.GetFileSize();
	AutoPtr< char > mem( (char *)ALLOC( fileSize + 1 ) );
	TEST_ASSERT( f.Read( mem.Get(), fileSize ) == fileSize );
	mem.Get()[ fileSize ] = 0;

	Timer t;

	const size_t repeatCount( 100 );
	for ( size_t i=0; i<repeatCount; ++i )
	{
		CIncludeParser parser;
		TEST_ASSERT( parser.ParseGCC_Preprocessed( mem.Get(), fileSize ) );

		// check number of includes found to prevent future regressions
		const Array< AString > & includes = parser.GetIncludes();
		TEST_ASSERT( includes.GetSize() == 280 );
		#ifdef DEBUG
			TEST_ASSERT( parser.GetNonUniqueCount() == 1280 );
		#endif
	}

	float time = t.GetElapsed();
	OUTPUT( "Clang                : %2.3fs (%2.1f MiB/sec)\n", time, ( (float)( fileSize * repeatCount / ( 1024.0f * 1024.0f ) ) / time ) );	
}

// TestClangMSExtensionsPreprocessedOutput
//------------------------------------------------------------------------------
void TestIncludeParser::TestClangMSExtensionsPreprocessedOutput() const
{
    FBuild fBuild; // needed fer CleanPath for relative dirs

	FileStream f;
	TEST_ASSERT( f.Open( "Data/TestIncludeParser/fbuildcore.clang.ms-extensions.ii", FileStream::READ_ONLY) )
	const size_t fileSize = (size_t)f.GetFileSize();
	AutoPtr< char > mem( (char *)ALLOC( fileSize + 1 ) );
	TEST_ASSERT( f.Read( mem.Get(), fileSize ) == fileSize );
	mem.Get()[ fileSize ] = 0;

	Timer t;

	const size_t repeatCount( 100 );
	for ( size_t i=0; i<repeatCount; ++i )
	{
		CIncludeParser parser;
		TEST_ASSERT( parser.ParseGCC_Preprocessed( mem.Get(), fileSize ) );

		// check number of includes found to prevent future regressions
		const Array< AString > & includes = parser.GetIncludes();
		TEST_ASSERT( includes.GetSize() == 285 );
		#ifdef DEBUG
			TEST_ASSERT( parser.GetNonUniqueCount() == 4758 );
		#endif
	}

	float time = t.GetElapsed();
	OUTPUT( "Clang (ms-extensions): %2.3fs (%2.1f MiB/sec)\n", time, ( (float)( fileSize * repeatCount / ( 1024.0f * 1024.0f ) ) / time ) );	
}

// 
//------------------------------------------------------------------------------
void TestIncludeParser::TestEdgeCases() const
{
	FBuild fBuild; // needed fer CleanPath for relative dirs

	// include on last line
	{
		AStackString<> data( "#line \"hello\"" );
		CIncludeParser parser;
		TEST_ASSERT( parser.ParseMSCL_Preprocessed( data.Get(), data.GetLength() ) );
		TEST_ASSERT( parser.GetIncludes().GetSize() == 1 );
		#ifdef DEBUG
			TEST_ASSERT( parser.GetNonUniqueCount() == 1 );
		#endif
	}

	// empty
	{
		AStackString<> data( "" );
		CIncludeParser parser;
		TEST_ASSERT( parser.ParseMSCL_Preprocessed( data.Get(), data.GetLength() ) );
		TEST_ASSERT( parser.GetIncludes().GetSize() == 0 );
		#ifdef DEBUG
			TEST_ASSERT( parser.GetNonUniqueCount() == 0 );
		#endif
	}

	// #pragma or #   pragma should be ignored
	{
		AStackString<> data( "#pragma message\"hello\"\n#   pragma message\"hello\"\n" );
		uint32_t dataLen = data.GetLength();
		CIncludeParser parser;
		TEST_ASSERT( parser.ParseGCC_Preprocessed( data.Get(), dataLen ) );
		TEST_ASSERT( parser.GetIncludes().GetSize() == 0 );
		#ifdef DEBUG
			TEST_ASSERT( parser.GetNonUniqueCount() == 0 );
		#endif
	}

    // "#line..." should both be found for clang (-fms-compat)
    {
		AStackString<> data( "#line 15 \"hello\"\n#line 2 \"hello\"" );
		uint32_t dataLen = data.GetLength();
		CIncludeParser parser;
		TEST_ASSERT( parser.ParseGCC_Preprocessed( data.Get(), dataLen ) );
		TEST_ASSERT( parser.GetIncludes().GetSize() == 1 );
		#ifdef DEBUG
			TEST_ASSERT( parser.GetNonUniqueCount() == 2 );
		#endif
    }
}

//------------------------------------------------------------------------------
