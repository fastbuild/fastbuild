// TestIncludeParser.cpp
//------------------------------------------------------------------------------

// Includes
//------------------------------------------------------------------------------
#include "FBuildTest.h"

#include "Tools/FBuild/FBuildCore/FBuild.h"
#include "Tools/FBuild/FBuildCore/Helpers/CIncludeParser.h"

// Core
#include "Core/FileIO/FileStream.h"
#include "Core/Strings/AStackString.h"
#include "Core/Time/Timer.h"
#include "Core/Tracing/Tracing.h"

// TestIncludeParser
//------------------------------------------------------------------------------
class TestIncludeParser : public FBuildTest
{
private:
    DECLARE_TESTS

    void TestMSVCPreprocessedOutput() const;
    void TestMSVCPreprocessedOutput_Indent() const;
    void TestMSVCShowIncludesOutput() const;
    void TestMSVC_P() const;
    void TestMSVC_ShowIncludesWithWarnings() const;
    void TestGCCPreprocessedOutput() const;
    void TestClangPreprocessedOutput() const;
    void TestClangMSExtensionsPreprocessedOutput() const;
    void TestEdgeCases() const;
    void ClangLineEndings() const;
};

// Register Tests
//------------------------------------------------------------------------------
REGISTER_TESTS_BEGIN( TestIncludeParser )
    #if defined( __WINDOWS__ )
        REGISTER_TEST( TestMSVCPreprocessedOutput )
        REGISTER_TEST( TestMSVCPreprocessedOutput_Indent )
        REGISTER_TEST( TestMSVCShowIncludesOutput )
        REGISTER_TEST( TestMSVC_P )
        REGISTER_TEST( TestMSVC_ShowIncludesWithWarnings )
    #endif
    REGISTER_TEST( TestGCCPreprocessedOutput )
    REGISTER_TEST( TestClangPreprocessedOutput )
    REGISTER_TEST( TestClangMSExtensionsPreprocessedOutput )
    REGISTER_TEST( TestEdgeCases )
    REGISTER_TEST( ClangLineEndings )
REGISTER_TESTS_END

// TestMSVCPreprocessedOutput
//------------------------------------------------------------------------------
void TestIncludeParser::TestMSVCPreprocessedOutput() const
{
    FileStream f;
    TEST_ASSERT( f.Open( "Tools/FBuild/FBuildTest/Data/TestIncludeParser/fbuildcore.msvc.ii", FileStream::READ_ONLY ) );
    const uint32_t fileSize = (uint32_t)f.GetFileSize();
    AString mem;
    mem.SetLength( fileSize );
    TEST_ASSERT( f.Read( mem.Get(), fileSize ) == fileSize );

    // Create a copy with alternate line endings
    AString mem2( mem );
    const uint32_t numReplaces = 200642;
    if ( mem2.Find( "\r" ) )
    {
        TEST_ASSERT( mem2.Replace( "\r\n", "\n" ) == numReplaces ); // Ensure we're actually changing the data
    }
    else
    {
        TEST_ASSERT( mem2.Replace( "\n", "\r\n" ) == numReplaces ); // Ensure we're actually changing the data
    }

    Timer t;

    const size_t repeatCount( 50 );
    const AString * buffers[ 2 ] = { &mem, &mem2 };
    for ( const AString * buffer : buffers )
    {
        for ( size_t i = 0; i < repeatCount; ++i )
        {
            CIncludeParser parser;
            TEST_ASSERT( parser.ParseMSCL_Preprocessed( buffer->Get(), buffer->GetLength() ) );

            // check number of includes found to prevent future regressions
            const Array< AString > & includes = parser.GetIncludes();
            TEST_ASSERT( includes.GetSize() == 284 );
            #ifdef DEBUG
                TEST_ASSERT( parser.GetNonUniqueCount() == 381 );
            #endif
        }
    }

    float time = t.GetElapsed();
    OUTPUT( "MSVC                 : %2.3fs (%2.1f MiB/sec)\n", (double)time, (double)( (float)( fileSize * repeatCount ) / ( 1024.0f * 1024.0f ) / time ) );
}

// TestMSVCPreprocessedOutput_Indent
//------------------------------------------------------------------------------
void TestIncludeParser::TestMSVCPreprocessedOutput_Indent() const
{
    // Test line starting with various tabs/spaces
    const char * testData = "#line 1 \"C:\\fileA.cpp\"\r\n"
                            " #line 1 \"C:\\fileB.cpp\"\r\n"
                            "            #line 1 \"C:\\fileC.cpp\"\r\n"
                            "\t#line 1 \"C:\\fileD.cpp\"\r\n"
                            "\t\t\t\t\t#line 1 \"C:\\fileE.cpp\"\r\n"
                            " \t \t \t#line 1 \"C:\\fileF.cpp\"\r\n";
    const size_t testDataSize = AString::StrLen( testData );

    CIncludeParser parser;
    TEST_ASSERT( parser.ParseMSCL_Preprocessed( testData, testDataSize ) );

    // check number of includes found to prevent future regressions
    const Array< AString > & includes = parser.GetIncludes();
    TEST_ASSERT( includes.GetSize() == 6 );
    #ifdef DEBUG
        TEST_ASSERT( parser.GetNonUniqueCount() == 6 );
    #endif
}

// TestMSVCShowIncludesOutput
//------------------------------------------------------------------------------
void TestIncludeParser::TestMSVCShowIncludesOutput() const
{
    FileStream f;
    TEST_ASSERT( f.Open( "Tools/FBuild/FBuildTest/Data/TestIncludeParser/fbuildcore.msvc.showincludes", FileStream::READ_ONLY ) );
    const uint32_t fileSize = (uint32_t)f.GetFileSize();
    AString mem;
    mem.SetLength( fileSize );
    TEST_ASSERT( f.Read( mem.Get(), fileSize ) == fileSize );

    // Create a copy with alternate line endings
    AString mem2( mem );
    const uint32_t numReplaces = 326;
    if ( mem2.Find( "\r" ) )
    {
        TEST_ASSERT( mem2.Replace( "\r\n", "\n" ) == numReplaces ); // Ensure we're actually changing the data
    }
    else
    {
        TEST_ASSERT( mem2.Replace( "\n", "\r\n" ) == numReplaces ); // Ensure we're actually changing the data
    }

    Timer t;

    const size_t repeatCount( 50 );
    const AString * buffers[ 2 ] = { &mem, &mem2 };
    for ( const AString * buffer : buffers )
    {
        for ( size_t i = 0; i < repeatCount; ++i )
        {
            CIncludeParser parser;
            TEST_ASSERT( parser.ParseMSCL_Output( buffer->Get(), buffer->GetLength() ) );

            // check number of includes found to prevent future regressions
            const Array< AString > & includes = parser.GetIncludes();
            TEST_ASSERT( includes.GetSize() == 189 );
            #ifdef DEBUG
                TEST_ASSERT( parser.GetNonUniqueCount() == 258 );
            #endif
        }
    }

    float time = t.GetElapsed();
    OUTPUT( "MSVC /showincludes   : %2.3fs (%2.1f MiB/sec)\n", (double)time, (double)( (float)( fileSize * repeatCount ) / ( 1024.0f * 1024.0f ) / time ) );
}

// TestMSVC_P
//------------------------------------------------------------------------------
void TestIncludeParser::TestMSVC_P() const
{
    FBuildTestOptions options;
    options.m_ConfigFile = "Tools/FBuild/FBuildTest/Data/TestIncludeParser/MSVC-P/fbuild.bff";

    FBuild fBuild( options );
    fBuild.Initialize();

    const AStackString<> file( "../tmp/Test/IncludeParser/MSVC-P/test.i" );

    // clean up anything left over from previous runs
    EnsureFileDoesNotExist( file );

    // Build
    TEST_ASSERT( fBuild.Build( "MSVC-P" ) );

    // make sure all output files are as expected
    EnsureFileExists( file );

    // Check stats
    //               Seen,  Built,  Type
    CheckStatsNode ( 1,     1,      Node::OBJECT_LIST_NODE );
    CheckStatsNode ( 1,     1,      Node::FILE_NODE );
    CheckStatsNode ( 1,     1,      Node::COMPILER_NODE );
    CheckStatsNode ( 1,     1,      Node::OBJECT_NODE );
    CheckStatsTotal( 4,     4 );
}

// TestMSVC_ShowIncludesWithWarnings
//------------------------------------------------------------------------------
void TestIncludeParser::TestMSVC_ShowIncludesWithWarnings() const
{
    FBuild fb; // needed for CleanPath

    FileStream f;
    TEST_ASSERT( f.Open( "Tools/FBuild/FBuildTest/Data/TestIncludeParser/MSVC-ShowIncludes/WithWarnings.output", FileStream::READ_ONLY ) );
    const uint32_t fileSize = (uint32_t)f.GetFileSize();
    AString mem;
    mem.SetLength( fileSize );
    TEST_ASSERT( f.Read( mem.Get(), fileSize ) == fileSize );

    // Create a copy with alternate line endings
    AString mem2( mem );
    const uint32_t numReplaces = 8;
    if ( mem2.Find( "\r" ) )
    {
        TEST_ASSERT( mem2.Replace( "\r\n", "\n" ) == numReplaces ); // Ensure we're actually changing the data
    }
    else
    {
        TEST_ASSERT( mem2.Replace( "\n", "\r\n" ) == numReplaces ); // Ensure we're actually changing the data
    }

    const AString * buffers[ 2 ] = { &mem, &mem2 };
    for ( const AString * buffer : buffers )
    {
        CIncludeParser parser;
        TEST_ASSERT( parser.ParseMSCL_Output( buffer->Get(), buffer->GetLength() ) );

        // check number of includes found to prevent future regressions
        const Array< AString > & includes = parser.GetIncludes();
        TEST_ASSERT( includes.GetSize() == 0 );
        #ifdef DEBUG
            TEST_ASSERT( parser.GetNonUniqueCount() == 0 );
        #endif
    }
}

// TestGCCPreprocessedOutput
//------------------------------------------------------------------------------
void TestIncludeParser::TestGCCPreprocessedOutput() const
{
    FBuild fBuild; // needed fer CleanPath for relative dirs

    FileStream f;
    TEST_ASSERT( f.Open( "Tools/FBuild/FBuildTest/Data/TestIncludeParser/fbuildcore.gcc.ii", FileStream::READ_ONLY ) );
    const uint32_t fileSize = (uint32_t)f.GetFileSize();
    AString mem;
    mem.SetLength( fileSize );
    TEST_ASSERT( f.Read( mem.Get(), fileSize ) == fileSize );

    // Create a copy with alternate line endings
    AString mem2( mem );
    const uint32_t numReplaces = 32600;
    if ( mem2.Find( "\r" ) )
    {
        TEST_ASSERT( mem2.Replace( "\r\n", "\n" ) == numReplaces ); // Ensure we're actually changing the data
    }
    else
    {
        TEST_ASSERT( mem2.Replace( "\n", "\r\n" ) == numReplaces ); // Ensure we're actually changing the data
    }

    Timer t;

    const size_t repeatCount( 50 );
    const AString * buffers[ 2 ] = { &mem, &mem2 };
    for ( const AString * buffer : buffers )
    {
        for ( size_t i = 0; i < repeatCount; ++i )
        {
            CIncludeParser parser;
            TEST_ASSERT( parser.ParseGCC_Preprocessed( buffer->Get(), buffer->GetLength() ) );

            // check number of includes found to prevent future regressions
            const Array< AString > & includes = parser.GetIncludes();
            TEST_ASSERT( includes.GetSize() == 221 );
            #ifdef DEBUG
                TEST_ASSERT( parser.GetNonUniqueCount() == 308 );
            #endif
        }
    }

    float time = t.GetElapsed();
    OUTPUT( "GCC                  : %2.3fs (%2.1f MiB/sec)\n", (double)time, (double)( (float)( fileSize * repeatCount ) / ( 1024.0f * 1024.0f ) / time ) );
}

// TestClangPreprocessedOutput
//------------------------------------------------------------------------------
void TestIncludeParser::TestClangPreprocessedOutput() const
{
    FBuild fBuild; // needed fer CleanPath for relative dirs

    // Load the preprocessed test data
    FileStream f;
    TEST_ASSERT( f.Open( "Tools/FBuild/FBuildTest/Data/TestIncludeParser/fbuildcore.clang.ii", FileStream::READ_ONLY ) );
    const uint32_t fileSize = (uint32_t)f.GetFileSize();
    AString mem;
    mem.SetLength( fileSize );
    TEST_ASSERT( f.Read( mem.Get(), fileSize ) == fileSize );

    // Create a copy with alternate line endings
    AString mem2( mem );
    const uint32_t numReplaces = 29979;
    if ( mem2.Find( "\r" ) )
    {
        TEST_ASSERT( mem2.Replace( "\r\n", "\n" ) == numReplaces ); // Ensure we're actually changing the data
    }
    else
    {
        TEST_ASSERT( mem2.Replace( "\n", "\r\n" ) == numReplaces ); // Ensure we're actually changing the data
    }

    Timer t;

    const size_t repeatCount( 50 );
    const AString * buffers[ 2 ] = { &mem, &mem2 };
    for ( const AString * buffer : buffers )
    {
        for ( size_t i = 0; i < repeatCount; ++i )
        {
            CIncludeParser parser;
            TEST_ASSERT( parser.ParseGCC_Preprocessed( buffer->Get(), buffer->GetLength() ) );

            // check number of includes found to prevent future regressions
            const Array< AString > & includes = parser.GetIncludes();
            TEST_ASSERT( includes.GetSize() == 279 );
            #ifdef DEBUG
                TEST_ASSERT( parser.GetNonUniqueCount() == 427 );
            #endif
        }
    }

    float time = t.GetElapsed();
    OUTPUT( "Clang                : %2.3fs (%2.1f MiB/sec)\n", (double)time, (double)( (float)( fileSize * repeatCount ) / ( 1024.0f * 1024.0f ) / time ) );
}

// TestClangMSExtensionsPreprocessedOutput
//------------------------------------------------------------------------------
void TestIncludeParser::TestClangMSExtensionsPreprocessedOutput() const
{
    FBuild fBuild; // needed fer CleanPath for relative dirs

    FileStream f;
    TEST_ASSERT( f.Open( "Tools/FBuild/FBuildTest/Data/TestIncludeParser/fbuildcore.clang.ms-extensions.ii", FileStream::READ_ONLY ) );
    const uint32_t fileSize = (uint32_t)f.GetFileSize();
    AString mem;
    mem.SetLength( fileSize );
    TEST_ASSERT( f.Read( mem.Get(), fileSize ) == fileSize );

    // Create a copy with alternate line endings
    AString mem2( mem );
    const uint32_t numReplaces = 76778;
    if ( mem2.Find( "\r" ) )
    {
        TEST_ASSERT( mem2.Replace( "\r\n", "\n" ) == numReplaces ); // Ensure we're actually changing the data
    }
    else
    {
        TEST_ASSERT( mem2.Replace( "\n", "\r\n" ) == numReplaces ); // Ensure we're actually changing the data
    }

    Timer t;

    const size_t repeatCount( 50 );
    const AString * buffers[ 2 ] = { &mem, &mem2 };
    for ( const AString * buffer : buffers )
    {
        for ( size_t i = 0; i < repeatCount; ++i )
        {
            CIncludeParser parser;
            TEST_ASSERT( parser.ParseGCC_Preprocessed( buffer->Get(), buffer->GetLength() ) );

            // check number of includes found to prevent future regressions
            const Array< AString > & includes = parser.GetIncludes();
            TEST_ASSERT( includes.GetSize() == 285 );
            #ifdef DEBUG
                TEST_ASSERT( parser.GetNonUniqueCount() == 4758 );
            #endif
        }
    }

    float time = t.GetElapsed();
    OUTPUT( "Clang (ms-extensions): %2.3fs (%2.1f MiB/sec)\n", (double)time, (double)( (float)( fileSize * repeatCount ) / ( 1024.0f * 1024.0f ) / time ) );
}

//
//------------------------------------------------------------------------------
void TestIncludeParser::TestEdgeCases() const
{
    FBuild fBuild; // needed fer CleanPath for relative dirs

    // include on last line
    {
        AStackString<> data( "#line 1 \"hello\"" );
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

// ClangLineEndings
//------------------------------------------------------------------------------
void TestIncludeParser::ClangLineEndings() const
{
    // Depending on the line endings of the source file and files being included
    // it's possible to end up with a variety of line ending types in the preprocessed
    // output when using Clang
    const char* preprocessedData    = "# 1 \"C:\\Test\\EmptyClang\\Unity.cpp\"\n"
                                      "# 1 \"C:\\Test\\EmptyClang\\Unity.cpp\" 2\r\n"   // Note: CR LF
                                      "# 1 \"./Empty1.cpp\" 1\r\n"                      // Note: CR LF
                                      "# 1 \"C:\\Test\\EmptyClang\\Unity.cpp\" 2\n"     // Note: LF
                                      "# 1 \"./Empty2.cpp\" 1\n"                        // Note: LF
                                      "# 2 \"C:\\Test\\EmptyClang\\Unity.cpp\" 2\r"     // Note: CR
                                      "# 1 \"./Empty3.cpp\" 1\r"                        // Note: CR
                                      "# 3 \"C:\\Test\\EmptyClang\\Unity.cpp\" 2\n";

    FBuild fb; // needed for CleanPath

    CIncludeParser parser;
    TEST_ASSERT( parser.ParseGCC_Preprocessed( preprocessedData, AString::StrLen( preprocessedData ) ) );

    // check number of includes found to prevent future regressions
    const Array< AString > & includes = parser.GetIncludes();
    TEST_ASSERT( includes.GetSize() == 3 );
    #ifdef DEBUG
        TEST_ASSERT( parser.GetNonUniqueCount() == 3 );
    #endif
}

//------------------------------------------------------------------------------
