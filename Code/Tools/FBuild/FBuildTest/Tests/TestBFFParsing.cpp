// TestBFFParsing.cpp
//------------------------------------------------------------------------------

// Includes
//------------------------------------------------------------------------------
#include "Tools/FBuild/FBuildTest/Tests/FBuildTest.h"

// FBuildCore
#include "Tools/FBuild/FBuildCore/BFF/BFFParser.h"
#include "Tools/FBuild/FBuildCore/FBuild.h"
#include "Tools/FBuild/FBuildCore/Graph/NodeGraph.h"

// Core
#include "Core/Env/Env.h"
#include "Core/FileIO/FileIO.h"
#include "Core/FileIO/FileStream.h"

//------------------------------------------------------------------------------
TEST_GROUP( TestBFFParsing, FBuildTest )
{
public:
};

//------------------------------------------------------------------------------
TEST_CASE( TestBFFParsing, Empty )
{
    TEST_PARSE_OK( "" );
}

//------------------------------------------------------------------------------
TEST_CASE( TestBFFParsing, AlmostEmpty )
{
    TEST_PARSE_OK( " " );
    TEST_PARSE_OK( "\t" );
    TEST_PARSE_OK( "\r" );
    TEST_PARSE_OK( "\n" );
    TEST_PARSE_OK( "\r\n" );
    TEST_PARSE_OK( " \t\r\n     " );
}

//------------------------------------------------------------------------------
TEST_CASE( TestBFFParsing, Comments )
{
    Parse( "Tools/FBuild/FBuildTest/Data/TestBFFParsing/comments.bff" );

    TEST_PARSE_OK( "//" );
    TEST_PARSE_OK( ";" );
    TEST_PARSE_OK( "// Comments" );
    TEST_PARSE_OK( "; Comments" );
    TEST_PARSE_OK( "// Comments\n" );
    TEST_PARSE_OK( "; Comments\n" );
}

//------------------------------------------------------------------------------
TEST_CASE( TestBFFParsing, Strings )
{
    Parse( "Tools/FBuild/FBuildTest/Data/TestBFFParsing/strings.bff" );
}

//------------------------------------------------------------------------------
TEST_CASE( TestBFFParsing, String_Unterminated )
{
    TEST_PARSE_FAIL( ".A='",        "Error #1002" );
    TEST_PARSE_FAIL( ".A='text",    "Error #1002" );
    TEST_PARSE_FAIL( ".A=\"",       "Error #1002" );
    TEST_PARSE_FAIL( ".A=\"text",   "Error #1002" );
    TEST_PARSE_FAIL( ".A=\"''",     "Error #1002" );
    TEST_PARSE_FAIL( ".A='\"\"",    "Error #1002" );
    TEST_PARSE_FAIL( ".A\n=\n'",    "Error #1002" );
    TEST_PARSE_FAIL( ".A\n=\n\"",   "Error #1002" );
    TEST_PARSE_FAIL( ".A='^'",      "Error #1002" );
    TEST_PARSE_FAIL( ".A=\"^\"",    "Error #1002" );
}

//------------------------------------------------------------------------------
TEST_CASE( TestBFFParsing, Arrays )
{
    Parse( "Tools/FBuild/FBuildTest/Data/TestBFFParsing/arrays.bff" );
}

//------------------------------------------------------------------------------
TEST_CASE( TestBFFParsing, Array_Unterminated )
{
    TEST_PARSE_FAIL( ".Array={",                        "Error #1002" );
    TEST_PARSE_FAIL( ".Array={;}",                      "Error #1002" );
    TEST_PARSE_FAIL( ".Array={//}",                     "Error #1002" );
    TEST_PARSE_FAIL( ".Array={\n#if FALSE\n}\n#endif",  "Error #1002" );
}

//------------------------------------------------------------------------------
TEST_CASE( TestBFFParsing, Array_TypeMismatch )
{
    Parse( "Tools/FBuild/FBuildTest/Data/TestBFFParsing/array_typemismatch.bff", true ); // expect failure
    TEST_ASSERT( GetRecordedOutput().Find( "FASTBuild Error #1034 - Operation not supported: 'ArrayOfStructs' = 'String'." ) );
}

//------------------------------------------------------------------------------
TEST_CASE( TestBFFParsing, Integers )
{
    Parse( "Tools/FBuild/FBuildTest/Data/TestBFFParsing/integers.bff" );
}

//------------------------------------------------------------------------------
TEST_CASE( TestBFFParsing, UnnamedScope )
{
    Parse( "Tools/FBuild/FBuildTest/Data/TestBFFParsing/unnamedscope.bff" );
}

//------------------------------------------------------------------------------
TEST_CASE( TestBFFParsing, UnnamedScope_Unterminated )
{
    TEST_PARSE_FAIL( "{",       "Error #1002" );
    TEST_PARSE_FAIL( "{\n",     "Error #1002" );
}

//------------------------------------------------------------------------------
TEST_CASE( TestBFFParsing, Directives )
{
    TEST_PARSE_FAIL( "#define #",     "Error #1010 - Unknown construct" );
    TEST_PARSE_FAIL( "#########",     "Error #1010 - Unknown construct" );
}

//------------------------------------------------------------------------------
TEST_CASE( TestBFFParsing, DefineDirective )
{
    TEST_PARSE_OK( "#define X" );
    TEST_PARSE_OK( "#define X123" );
    TEST_PARSE_OK( "#define X_" );
    TEST_PARSE_OK( "#define _X_" );

    TEST_PARSE_FAIL( "#define X Y",     "Error #1045 - Extraneous token(s)" );
}

//------------------------------------------------------------------------------
TEST_CASE( TestBFFParsing, ExistsDirective )
{
    TEST_PARSE_FAIL( "#if exists",      "Error #1031" );
    TEST_PARSE_FAIL( "#if exists(",     "Error #1030" );
    TEST_PARSE_FAIL( "#if exists(x",    "Error #1031" );
}

//------------------------------------------------------------------------------
TEST_CASE( TestBFFParsing, IncludeDirective )
{
    // Invalid include directives
    TEST_PARSE_FAIL( "#include",                        "Error #1031" );
    TEST_PARSE_FAIL( "#include BLAH",                   "Error #1031" );
    TEST_PARSE_FAIL( "#once\n#include \"test.bff\" X",  "Error #1045 - Extraneous token(s)" );

    // Including a directory
    TEST_PARSE_FAIL( "#include \"/\"",                  "Error #1032" );

    // Missing include file
    TEST_PARSE_FAIL( "#include \"missing.bff\"",        "Error #1032" );

    // Missing root file
    Parse( "missing.bff", true ); // Expect failure

    Parse( "Tools/FBuild/FBuildTest/Data/TestBFFParsing/includes.bff" );
}

//------------------------------------------------------------------------------
TEST_CASE( TestBFFParsing, Include_ExcessiveDepth )
{
    Parse( "Tools/FBuild/FBuildTest/Data/TestBFFParsing/include_excessivedepth.bff", true ); // expect failure
    TEST_ASSERT( GetRecordedOutput().Find( "Error #1035 - Excessive depth complexity" ) );
}

//------------------------------------------------------------------------------
TEST_CASE( TestBFFParsing, ImportDirective )
{
    TEST_PARSE_FAIL( "#import",             "Error #1031" );
    TEST_PARSE_FAIL( "#import 'string'",    "Error #1031" );

    // Invalid syntax
    Env::SetEnvVariable( "BFF_TEST_IMPORT_VAR", AString( "VALUE" ) );
    TEST_PARSE_FAIL( "#import BFF_TEST_IMPORT_VAR X",   "Error #1045 - Extraneous token(s)" );

    // Ensure special characters are not lost
    Env::SetEnvVariable( "BFF_TEST_IMPORT_VAR2", AString( "Special^And$Special" ) );
    TEST_PARSE_OK( "#import BFF_TEST_IMPORT_VAR2\n"
                   ".Expected = 'Special^^And^$Special'\n"
                   "If( .BFF_TEST_IMPORT_VAR2 == .Expected )\n"
                   "{\n"
                   "    Print( 'Special Characters OK' )\n"
                   "}",                     "Special Characters OK" );
}

//------------------------------------------------------------------------------
TEST_CASE( TestBFFParsing, OnceDirective )
{
    // Valid cases with varying whitespace
    TEST_PARSE_OK( "#once" );
    TEST_PARSE_OK( "\t#\t\tonce" );
    TEST_PARSE_OK( "\r\n# once\n" );

    // Invalid cases
    TEST_PARSE_FAIL( "#once X",    "Error #1045 - Extraneous token(s)" );

    // #once used to prevent infinitely recursive includes
    Parse( "Tools/FBuild/FBuildTest/Data/TestBFFParsing/include_once.bff" );
}

//------------------------------------------------------------------------------
TEST_CASE( TestBFFParsing, Structs )
{
    Parse( "Tools/FBuild/FBuildTest/Data/TestBFFParsing/structs.bff" );
}

//------------------------------------------------------------------------------
TEST_CASE( TestBFFParsing, Struct_Concatenation )
{
    Parse( "Tools/FBuild/FBuildTest/Data/TestBFFParsing/struct_concatenation.bff" );
}

//------------------------------------------------------------------------------
TEST_CASE( TestBFFParsing, Struct_ConcatenationMismatch )
{
    Parse( "Tools/FBuild/FBuildTest/Data/TestBFFParsing/struct_concatenation_mismatch.bff", true ); // expect failure
    TEST_ASSERT( GetRecordedOutput().Find( "#1034 - Operation not supported: 'ArrayOfStrings' + 'Int'" ) );
}

//------------------------------------------------------------------------------
TEST_CASE( TestBFFParsing, Struct_ConcatenationOrder )
{
    Parse( "Tools/FBuild/FBuildTest/Data/TestBFFParsing/struct_concatenation_order.bff" );

    // Ensure all properties are concatenated in a consistent order, regardless
    // of depth of recursion
    TEST_ASSERT( GetRecordedOutput().Find( ".Value1 = 'AB'" ) );
    TEST_ASSERT( GetRecordedOutput().Find( ".Value2 = 'AB'" ) );
    TEST_ASSERT( GetRecordedOutput().Find( ".Value3 = 'AB'" ) );
    TEST_ASSERT( GetRecordedOutput().Find( ".Value4 = 'AB'" ) );
}

//------------------------------------------------------------------------------
TEST_CASE( TestBFFParsing, Struct_Unterminated )
{
    Parse( "Tools/FBuild/FBuildTest/Data/TestBFFParsing/struct_unterminated.bff", true ); // expect failure
    TEST_ASSERT( GetRecordedOutput().Find( "Error #1002" ) );
}

//------------------------------------------------------------------------------
TEST_CASE( TestBFFParsing, Struct_MemberShadowsSelf )
{
    Parse( "Tools/FBuild/FBuildTest/Data/TestBFFParsing/struct_membershadowsself.bff" );
}

//------------------------------------------------------------------------------
TEST_CASE( TestBFFParsing, IncludeScope )
{
    Parse( "Tools/FBuild/FBuildTest/Data/TestBFFParsing/include_scope.bff" );
}

//------------------------------------------------------------------------------
TEST_CASE( TestBFFParsing, IfDirective )
{
    Parse( "Tools/FBuild/FBuildTest/Data/TestBFFParsing/if_directive.bff" );

    TEST_PARSE_FAIL( "#if",             "Error #1031 - Unknown char" );
    TEST_PARSE_FAIL( "#if 'string'",    "Error #1031 - Unknown char" );
    TEST_PARSE_FAIL( "#if X Y\n"
                     "#endif",          "Error #1045 - Extraneous token(s)" );
    TEST_PARSE_FAIL( "##if!X\n ",       "Error #1010 - Unknown construct" );

    // Ensure directives inside lists are handled correctly
    TEST_PARSE_OK( ".A = {\n"
                   "#if __UNDEFINED__\n"
                   "    'X'\n"
                   "#endif\n"
                   "}" );
    TEST_PARSE_OK( ".A = {\n"
                   "#if !__UNDEFINED__\n"
                   "    'X'\n"
                   "#endif\n"
                   "}" );
}

//------------------------------------------------------------------------------
TEST_CASE( TestBFFParsing, IfExistsDirective )
{
    Env::SetEnvVariable( "BFF_TEST_ENV_VAR1", AString( "1" ) );
    Env::SetEnvVariable( "BFF_TEST_ENV_VAR2", AString( "2" ) );
    Parse( "Tools/FBuild/FBuildTest/Data/TestBFFParsing/if_exists_directive.bff" );
}

//------------------------------------------------------------------------------
TEST_CASE( TestBFFParsing, IfFileExistsDirective )
{
    // Detect existence (or not) of files
    TEST_PARSE_OK( "#if file_exists(\"doesnotexist.dat\")\n"
                   "    #error\n"
                   "#endif" );
    TEST_PARSE_OK( "#if !file_exists(\"fbuild.bff\")\n"
                   "    #error\n"
                   "#endif" );

    // Check changes are detected (or not) between builds
    {
        const char * rootBFF = "../tmp/Test/BFFParsing/FileExistsDirective/if_file_exists_directive.dat";
        const char * fileName = "../tmp/Test/BFFParsing/FileExistsDirective/file.dat";
        const char * db = "../tmp/Test/BFFParsing/FileExistsDirective/fbuild.fdb";

        // Copy root bff to temp dir
        FileIO::SetReadOnly( rootBFF, false );
        EnsureFileDoesNotExist( rootBFF );
        EnsureDirExists( "../tmp/Test/BFFParsing/FileExistsDirective/" );
        FileIO::FileCopy( "Tools/FBuild/FBuildTest/Data/TestBFFParsing/if_file_exists_directive.bff", rootBFF );

        // Delete extra file from previous test run
        EnsureFileDoesNotExist( fileName );

        FBuildTestOptions options;
        options.m_ConfigFile = rootBFF;

        // Parse bff, which checks if file exists
        {
            FBuildForTest fBuild( options );
            TEST_ASSERT( fBuild.Initialize() );
            fBuild.SaveDependencyGraph( db );

            const AString & output( GetRecordedOutput() );

            // File should NOT exist
            TEST_ASSERT( output.Find( "File does not exist" ) );
        }

        // Check existence re-parsing does not occur with not change to file existence
        {
            const size_t sizeOfRecordedOutput = GetRecordedOutput().GetLength();

            FBuildForTest fBuild( options );
            TEST_ASSERT( fBuild.Initialize( db ) );
            fBuild.SaveDependencyGraph( db );

            // Should not re-parse
            const AStackString output( GetRecordedOutput().Get() + sizeOfRecordedOutput );
            TEST_ASSERT( output.Find( "BFF will be re-parsed" ) == nullptr );
        }

        // Create file
        {
            FileIO::EnsurePathExistsForFile( AStackString( fileName ) );
            FileStream f;
            TEST_ASSERT( f.Open( fileName, FileStream::WRITE_ONLY ) );
        }

        // Load db and ensure it is re-parsed
        {
            const size_t sizeOfRecordedOutput = GetRecordedOutput().GetLength();

            FBuildForTest fBuild( options );
            TEST_ASSERT( fBuild.Initialize( db ) );
            fBuild.SaveDependencyGraph( db );

            const AStackString output( GetRecordedOutput().Get() + sizeOfRecordedOutput );

            // Check re-parse was triggered
            TEST_ASSERT( output.Find( "File used in file_exists was added" ) );

            // File should exist
            TEST_ASSERT( output.Find( "File exists" ) );
        }

        // Delete file
        EnsureFileDoesNotExist( fileName );

        // Load db and ensure it is re-parsed again
        {
            const size_t sizeOfRecordedOutput = GetRecordedOutput().GetLength();

            FBuildForTest fBuild( options );
            TEST_ASSERT( fBuild.Initialize( db ) );

            const AStackString output( GetRecordedOutput().Get() + sizeOfRecordedOutput );

            // Check re-parse was triggered
            TEST_ASSERT( output.Find( "File used in file_exists was removed" ) );

            // File should exist
            TEST_ASSERT( output.Find( "File does not exist" ) );
        }
    }
}

//------------------------------------------------------------------------------
TEST_CASE( TestBFFParsing, IfFileExistsDirective_RelativePaths )
{
    // file_exists treats paths the same way as #include
    // (paths are relative to the bff)

    FBuildTestOptions options;
    options.m_ConfigFile = "Tools/FBuild/FBuildTest/Data/TestBFFParsing/IfFileExistsDirective/RelativePaths/root.bff";

    FBuildForTest fBuild( options );
    TEST_ASSERT( fBuild.Initialize() );

    // Check for expected output
    const AString & output( GetRecordedOutput() );
    TEST_ASSERT( output.Find( "OK-1A" ) && output.Find( "OK-1B" ) );
    TEST_ASSERT( output.Find( "OK-2A" ) && output.Find( "OK-2B" ) );
    TEST_ASSERT( output.Find( "OK-3A" ) && output.Find( "OK-3B" ) );
    TEST_ASSERT( output.Find( "OK-4" ) );
}

//------------------------------------------------------------------------------
TEST_CASE( TestBFFParsing, IfBooleanOperators )
{
    // Failure cases
    TEST_PARSE_FAIL( "#if ||",              "#1046 - #if expression cannot start with boolean operator");
    TEST_PARSE_FAIL( "#if &&",              "#1046 - #if expression cannot start with boolean operator");
    TEST_PARSE_FAIL( " #if X && || Y\n"
                     "#endif",              "#1031 - Unknown char '|' following 'if' directive." );
    TEST_PARSE_FAIL( "#if X &&\n"
                     "#endif",              "#1031 - Unknown char '#' following 'if' directive. (Expected '?')." );
    TEST_PARSE_FAIL( "#if X && Y Z\n"
                     "#endif",              "#1045 - Extraneous token(s) following 'if' directive." );

    // Expression too complex
    AStackString complex( "#if " );
    for ( size_t i = 0; i < 256; ++i )
    {
        complex.AppendFormat( "A%u &&", (uint32_t)i );
    }
    complex += "B";
    TEST_PARSE_FAIL( complex.Get(),         "#1047 - #if expression too complex. Up to 256 boolean operators supported." );

    // OR
    TEST_PARSE_OK( "#define A\n"
                   "#if A || B\n"
                   "    Print( 'OK' )\n"
                   "#endif",                "OK" );
    TEST_PARSE_OK( "#if A || B\n"
                   "#else"
                   "    Print( 'OK' )\n"
                   "#endif",                "OK" );
    TEST_PARSE_OK( "#define CCC\n"
                   "#if DDD || CCC\n"
                   "    Print( 'OK' )\n"
                   "#endif",                "OK" );
    TEST_PARSE_OK( "#define AAA\n"
                   "#define BBB\n"
                   "#define CCC\n"
                   "#if CCC || DDD || AAA || BBB\n"
                   "    Print( 'OK' )\n"
                   "#endif",                "OK" );

    // AND
    TEST_PARSE_OK( "#define AAA\n"
                   "#define BBB\n"
                   "#if AAA && BBB\n"
                   "    Print( 'OK' )\n"
                   "#endif",                "OK" );
    TEST_PARSE_OK( "#define AAA\n"
                   "#define BBB\n"
                   "#define CCC\n"
                   "#if AAA && BBB && CCC\n"
                   "    Print( 'OK' )\n"
                   "#endif",                "OK" );

    // AND !
    TEST_PARSE_OK( "#define AAA\n"
                   "#if AAA && !DDD\n"
                   "    Print( 'OK' )\n"
                   "#endif",                "OK" );
    TEST_PARSE_OK( "#define AAA\n"
                   "#if !DDD && AAA\n"
                   "    Print( 'OK' )\n"
                   "#endif",                "OK" );

    // OR !
    TEST_PARSE_OK( "#if !EEE || !DDD\n"
                   "    Print( 'OK' )\n"
                   "#endif",                "OK" );
    TEST_PARSE_OK( "#define AAA\n"
                   "#if AAA || !DDD\n"
                   "    Print( 'OK' )\n"
                   "#endif",                "OK" );
    TEST_PARSE_OK( "#define aaa\n"
                   "#if !ddd || aaa\n"
                   "    Print( 'OK' )\n"
                   "#endif",                "OK" );

    // Check precedence
    TEST_PARSE_OK( "#define A\n"
                   "#define B\n"
                   "#if A || B && C\n"
                   "    Print( 'OK' )\n"
                   "#endif",                "OK" );

    TEST_PARSE_OK( "#define AAA\n"
                   "#define BBB\n"
                   "#define CCC\n"
                   "#if DDD || BBB && CCC\n"
                   "    Print( 'OK' )\n"
                   "#endif",                "OK" );

    TEST_PARSE_OK( "#define AAA\n"
                   "#define BBB\n"
                   "#define CCC\n"
                   "#if DDD || BBB && CCC || EEE\n"
                   "    Print( 'OK' )\n"
                   "#endif",                "OK" );

    TEST_PARSE_OK( "#define AAA\n"
                   "#define BBB\n"
                   "#define CCC\n"
                   "#if DDD || BBB && DDD || EEE && AAA\n"
                   "    #error Should not be here\n"
                   "#else\n"
                   "    Print( 'OK' )\n"
                   "#endif",                "OK" );

    TEST_PARSE_OK( "#define AAA\n"
                   "#define BBB\n"
                   "#define CCC\n"
                   "#if DDD || BBB && AAA || EEE && AAA\n"
                   "    Print( 'OK' )\n"
                   "#endif",                "OK" );

    TEST_PARSE_OK( "#define AAA\n"
                   "#define BBB\n"
                   "#define CCC\n"
                   "#if DDD || BBB && DDD || BBB && AAA\n"
                   "    Print( 'OK' )\n"
                   "#endif",                "OK" );

    TEST_PARSE_OK( "#define AAA\n"
                   "#define BBB\n"
                   "#define CCC\n"
                   "#if DDD || BBB && DDD || DDD && AAA || CCC\n"
                   "    Print( 'OK' )\n"
                   "#endif",                "OK" );

    TEST_PARSE_OK( "#define AAA\n"
                   "#define BBB\n"
                   "#define CCC\n"
                   "#if DDD || BBB && CCC && AAA || DDD && AAA\n"
                   "    Print( 'OK' )\n"
                   "#endif",                "OK" );
}

//------------------------------------------------------------------------------
TEST_CASE( TestBFFParsing, ElifDirective )
{
    Parse( "Tools/FBuild/FBuildTest/Data/TestBFFParsing/elif_directive.bff" );

    TEST_PARSE_FAIL( "#elif X",         "Error #1048 - #elif without matching #if" );
    TEST_PARSE_FAIL( "#if X\n"
                     "#elif",           "Error #1031 - Unknown char" );
    TEST_PARSE_FAIL( "#if X\n"
                     "#elif 'string'",  "Error #1031 - Unknown char" );
    TEST_PARSE_FAIL( "#if Z\n"
                     "#elif X Y\n"
                     "#endif",          "Error #1045 - Extraneous token(s)" );
    TEST_PARSE_FAIL( "#if X\n"
                     "##elif!X\n"
                     "#endif",          "Error #1010 - Unknown construct" );

    // Ensure directives inside lists are handled correctly
    TEST_PARSE_OK( ".A = {\n"
                   "#if __UNDEFINED__\n"
                   "    'X'\n"
                   "#elif __UNDEFINED__\n"
                   "    'Y'\n"
                   "#endif\n"
                   "}" );
    TEST_PARSE_OK( ".A = {\n"
                   "#if !__UNDEFINED__\n"
                   "    'X'\n"
                   "#elif !__UNDEFINED__\n"
                   "    'Y'\n"
                   "#endif\n"
                   "}" );
    TEST_PARSE_OK( ".A = {\n"
                   "#if __UNDEFINED__\n"
                   "    'X'\n"
                   "#elif !__UNDEFINED__\n"
                   "    'Y'\n"
                   "#endif\n"
                   "}" );
    TEST_PARSE_OK( ".A = {\n"
                   "#if !__UNDEFINED__\n"
                   "    'X'\n"
                   "#elif __UNDEFINED__\n"
                   "    'Y'\n"
                   "#endif\n"
                   "}" );
}

//------------------------------------------------------------------------------
TEST_CASE( TestBFFParsing, ElifExistsDirective )
{
    Env::SetEnvVariable( "BFF_TEST_ENV_VAR1", AString( "1" ) );
    Env::SetEnvVariable( "BFF_TEST_ENV_VAR2", AString( "2" ) );
    Parse( "Tools/FBuild/FBuildTest/Data/TestBFFParsing/elif_exists_directive.bff" );
}

//------------------------------------------------------------------------------
TEST_CASE( TestBFFParsing, ElifFileExistsDirective )
{
    // Detect existence (or not) of files
    TEST_PARSE_OK( "#if __UNDEFINED__\n"
                   "#elif file_exists(\"doesnotexist.dat\")\n"
                   "    #error\n"
                   "#endif" );
    TEST_PARSE_OK( "#if __UNDEFINED__\n"
                   "#elif !file_exists(\"fbuild.bff\")\n"
                   "    #error\n"
                   "#endif" );

    // Check changes are detected (or not) between builds
    {
        const char * rootBFF = "../tmp/Test/BFFParsing/FileExistsDirective/elif_true_path_file_exists_directive.dat";
        const char * fileName = "../tmp/Test/BFFParsing/FileExistsDirective/file.dat";
        const char * db = "../tmp/Test/BFFParsing/FileExistsDirective/fbuild.fdb";

        // Copy root bff to temp dir
        FileIO::SetReadOnly( rootBFF, false );
        EnsureFileDoesNotExist( rootBFF );
        EnsureDirExists( "../tmp/Test/BFFParsing/FileExistsDirective/" );
        FileIO::FileCopy( "Tools/FBuild/FBuildTest/Data/TestBFFParsing/elif_true_path_file_exists_directive.bff", rootBFF );

        // Delete extra file from previous test run
        EnsureFileDoesNotExist( fileName );

        FBuildTestOptions options;
        options.m_ConfigFile = rootBFF;

        // Parse bff, which checks if file exists
        {
            FBuildForTest fBuild( options );
            TEST_ASSERT( fBuild.Initialize() );
            fBuild.SaveDependencyGraph( db );

            const AString & output( GetRecordedOutput() );

            // File should NOT exist
            TEST_ASSERT( output.Find( "File does not exist" ) );
        }

        // Check existence re-parsing does not occur with not change to file existence
        {
            const size_t sizeOfRecordedOutput = GetRecordedOutput().GetLength();

            FBuildForTest fBuild( options );
            TEST_ASSERT( fBuild.Initialize( db ) );
            fBuild.SaveDependencyGraph( db );

            // Should not re-parse
            const AStackString output( GetRecordedOutput().Get() + sizeOfRecordedOutput );
            TEST_ASSERT( output.Find( "BFF will be re-parsed" ) == nullptr );
        }

        // Create file
        {
            FileIO::EnsurePathExistsForFile( AStackString( fileName ) );
            FileStream f;
            TEST_ASSERT( f.Open( fileName, FileStream::WRITE_ONLY ) );
        }

        // Load db and ensure it is re-parsed
        {
            const size_t sizeOfRecordedOutput = GetRecordedOutput().GetLength();

            FBuildForTest fBuild( options );
            TEST_ASSERT( fBuild.Initialize( db ) );
            fBuild.SaveDependencyGraph( db );

            const AStackString output( GetRecordedOutput().Get() + sizeOfRecordedOutput );

            // Check re-parse was triggered
            TEST_ASSERT( output.Find( "File used in file_exists was added" ) );

            // File should exist
            TEST_ASSERT( output.Find( "File exists" ) );
        }

        // Delete file
        EnsureFileDoesNotExist( fileName );

        // Load db and ensure it is re-parsed again
        {
            const size_t sizeOfRecordedOutput = GetRecordedOutput().GetLength();

            FBuildForTest fBuild( options );
            TEST_ASSERT( fBuild.Initialize( db ) );

            const AStackString output( GetRecordedOutput().Get() + sizeOfRecordedOutput );

            // Check re-parse was triggered
            TEST_ASSERT( output.Find( "File used in file_exists was removed" ) );

            // File should exist
            TEST_ASSERT( output.Find( "File does not exist" ) );
        }
    }

    // Check changes are detected (or not) between builds
    {
        const char * rootBFF = "../tmp/Test/BFFParsing/FileExistsDirective/elif_false_path_file_exists_directive.dat";
        const char * fileName = "../tmp/Test/BFFParsing/FileExistsDirective/file.dat";
        const char * db = "../tmp/Test/BFFParsing/FileExistsDirective/fbuild.fdb";

        // Copy root bff to temp dir
        FileIO::SetReadOnly( rootBFF, false );
        EnsureFileDoesNotExist( rootBFF );
        EnsureDirExists( "../tmp/Test/BFFParsing/FileExistsDirective/" );
        FileIO::FileCopy( "Tools/FBuild/FBuildTest/Data/TestBFFParsing/elif_false_path_file_exists_directive.bff", rootBFF );

        // Delete extra file from previous test run
        EnsureFileDoesNotExist( fileName );

        FBuildTestOptions options;
        options.m_ConfigFile = rootBFF;

        // Parse bff, but it shouldn't check if the file exists
        {
            const size_t sizeOfRecordedOutput = GetRecordedOutput().GetLength();

            FBuildForTest fBuild( options );
            TEST_ASSERT( fBuild.Initialize() );
            fBuild.SaveDependencyGraph( db );

            const AStackString output( GetRecordedOutput().Get() + sizeOfRecordedOutput );

            // File existence does not matter because the check is in a false path
            TEST_ASSERT( output.Find( "File existence not checked" ) );
            TEST_ASSERT( output.Find( "File exists" ) == nullptr );
            TEST_ASSERT( output.Find( "File does not exist" ) == nullptr );
        }

        // Check existence re-parsing does not occur with not change to file existence
        {
            const size_t sizeOfRecordedOutput = GetRecordedOutput().GetLength();

            FBuildForTest fBuild( options );
            TEST_ASSERT( fBuild.Initialize( db ) );
            fBuild.SaveDependencyGraph( db );

            // Should not re-parse
            const AStackString output( GetRecordedOutput().Get() + sizeOfRecordedOutput );
            TEST_ASSERT( output.Find( "BFF will be re-parsed" ) == nullptr );
        }

        // Create file
        {
            FileIO::EnsurePathExistsForFile( AStackString( fileName ) );
            FileStream f;
            TEST_ASSERT( f.Open( fileName, FileStream::WRITE_ONLY ) );
        }

        // Load db, but it shouldn't trigger a reparse because the file existence exists on the false path
        {
            const size_t sizeOfRecordedOutput = GetRecordedOutput().GetLength();

            FBuildForTest fBuild( options );
            TEST_ASSERT( fBuild.Initialize( db ) );
            fBuild.SaveDependencyGraph( db );

            const AStackString output( GetRecordedOutput().Get() + sizeOfRecordedOutput );

            // Check re-parse was NOT triggered, because the file existence is on the false path
            TEST_ASSERT( output.Find( "BFF will be re-parsed" ) == nullptr );
        }

        // Delete file
        EnsureFileDoesNotExist( fileName );

        // Load db, but it shouldn't trigger a reparse because the file existence exists on the false path
        {
            const size_t sizeOfRecordedOutput = GetRecordedOutput().GetLength();

            FBuildForTest fBuild( options );
            TEST_ASSERT( fBuild.Initialize( db ) );

            const AStackString output( GetRecordedOutput().Get() + sizeOfRecordedOutput );

            // Check re-parse was NOT triggered, because the file existence is on the false path
            TEST_ASSERT( output.Find( "BFF will be re-parsed" ) == nullptr );
        }
    }
}

//------------------------------------------------------------------------------
TEST_CASE( TestBFFParsing, ElifFileExistsDirective_RelativePaths )
{
    // file_exists treats paths the same way as #include
    // (paths are relative to the bff)

    FBuildTestOptions options;
    options.m_ConfigFile = "Tools/FBuild/FBuildTest/Data/TestBFFParsing/ElifFileExistsDirective/RelativePaths/root.bff";

    FBuildForTest fBuild( options );
    TEST_ASSERT( fBuild.Initialize() );

    // Check for expected output
    const AString & output( GetRecordedOutput() );
    TEST_ASSERT( output.Find( "OK-1A" ) && output.Find( "OK-1B" ) );
    TEST_ASSERT( output.Find( "OK-2A" ) && output.Find( "OK-2B" ) );
    TEST_ASSERT( output.Find( "OK-3A" ) && output.Find( "OK-3B" ) );
    TEST_ASSERT( output.Find( "OK-4" ) );
}

//------------------------------------------------------------------------------
TEST_CASE( TestBFFParsing, ElifBooleanOperators )
{
    // Failure cases
    TEST_PARSE_FAIL( "#if __UNDEFINED__\n"
                     "#elif ||",              "#1046 - #elif expression cannot start with boolean operator");
    TEST_PARSE_FAIL( "#if __UNDEFINED__\n"
                     "#elif &&",              "#1046 - #elif expression cannot start with boolean operator");
    TEST_PARSE_FAIL( "#if __UNDEFINED__\n"
                     " #elif X && || Y\n"
                     "#endif",              "#1031 - Unknown char '|' following 'elif' directive." );
    TEST_PARSE_FAIL( "#if __UNDEFINED__\n"
                     "#elif X &&\n"
                     "#endif",              "#1031 - Unknown char '#' following 'elif' directive. (Expected '?')." );
    TEST_PARSE_FAIL( "#if __UNDEFINED__\n"
                     "#elif X && Y Z\n"
                     "#endif",              "#1045 - Extraneous token(s) following 'elif' directive." );

    // Failure cases, even in the false path
    TEST_PARSE_FAIL( "#if !__UNDEFINED__\n"
                     "#elif ||",              "#1046 - #elif expression cannot start with boolean operator");
    TEST_PARSE_FAIL( "#if !__UNDEFINED__\n"
                     "#elif &&",              "#1046 - #elif expression cannot start with boolean operator");
    TEST_PARSE_FAIL( "#if !__UNDEFINED__\n"
                     " #elif X && || Y\n"
                     "#endif",              "#1031 - Unknown char '|' following 'elif' directive." );
    TEST_PARSE_FAIL( "#if !__UNDEFINED__\n"
                     "#elif X &&\n"
                     "#endif",              "#1031 - Unknown char '#' following 'elif' directive. (Expected '?')." );
    TEST_PARSE_FAIL( "#if !__UNDEFINED__\n"
                     "#elif X && Y Z\n"
                     "#endif",              "#1045 - Extraneous token(s) following 'elif' directive." );

    // Expression too complex
    {
        AStackString complex( "#if __UNDEFINED__\n"
                              "#elif " );
        for ( size_t i = 0; i < 256; ++i )
        {
            complex.AppendFormat( "A%u &&", (uint32_t)i );
        }
        complex += "B";
        TEST_PARSE_FAIL( complex.Get(),         "#1047 - #elif expression too complex. Up to 256 boolean operators supported." );
    }

    // Expression too complex, even in false path
    {
        AStackString complex( "#if !__UNDEFINED__\n"
                              "#elif " );
        for ( size_t i = 0; i < 256; ++i )
        {
            complex.AppendFormat( "A%u &&", (uint32_t)i );
        }
        complex += "B";
        TEST_PARSE_FAIL( complex.Get(),         "#1047 - #elif expression too complex. Up to 256 boolean operators supported." );
    }

    // OR
    TEST_PARSE_OK( "#define A\n"
                   "#if __UNDEFINED__\n"
                   "#elif A || B\n"
                   "    Print( 'OK' )\n"
                   "#endif",                "OK" );
    TEST_PARSE_OK( "#if __UNDEFINED__\n"
                   "#elif A || B\n"
                   "#else"
                   "    Print( 'OK' )\n"
                   "#endif",                "OK" );
    TEST_PARSE_OK( "#define CCC\n"
                   "#if __UNDEFINED__\n"
                   "#elif DDD || CCC\n"
                   "    Print( 'OK' )\n"
                   "#endif",                "OK" );
    TEST_PARSE_OK( "#define AAA\n"
                   "#define BBB\n"
                   "#define CCC\n"
                   "#if __UNDEFINED__\n"
                   "#elif CCC || DDD || AAA || BBB\n"
                   "    Print( 'OK' )\n"
                   "#endif",                "OK" );

    // AND
    TEST_PARSE_OK( "#define AAA\n"
                   "#define BBB\n"
                   "#if __UNDEFINED__\n"
                   "#elif AAA && BBB\n"
                   "    Print( 'OK' )\n"
                   "#endif",                "OK" );
    TEST_PARSE_OK( "#define AAA\n"
                   "#define BBB\n"
                   "#define CCC\n"
                   "#if __UNDEFINED__\n"
                   "#elif AAA && BBB && CCC\n"
                   "    Print( 'OK' )\n"
                   "#endif",                "OK" );

    // AND !
    TEST_PARSE_OK( "#define AAA\n"
                   "#if __UNDEFINED__\n"
                   "#elif AAA && !DDD\n"
                   "    Print( 'OK' )\n"
                   "#endif",                "OK" );
    TEST_PARSE_OK( "#define AAA\n"
                   "#if __UNDEFINED__\n"
                   "#elif !DDD && AAA\n"
                   "    Print( 'OK' )\n"
                   "#endif",                "OK" );

    // OR !
    TEST_PARSE_OK( "#if __UNDEFINED__\n"
                   "#elif !EEE || !DDD\n"
                   "    Print( 'OK' )\n"
                   "#endif",                "OK" );
    TEST_PARSE_OK( "#define AAA\n"
                   "#if __UNDEFINED__\n"
                   "#elif AAA || !DDD\n"
                   "    Print( 'OK' )\n"
                   "#endif",                "OK" );
    TEST_PARSE_OK( "#define aaa\n"
                   "#if __UNDEFINED__\n"
                   "#elif !ddd || aaa\n"
                   "    Print( 'OK' )\n"
                   "#endif",                "OK" );

    // Check precedence
    TEST_PARSE_OK( "#define A\n"
                   "#define B\n"
                   "#if A || B && C\n"
                   "    Print( 'OK' )\n"
                   "#endif",                "OK" );

    TEST_PARSE_OK( "#define AAA\n"
                   "#define BBB\n"
                   "#define CCC\n"
                   "#if __UNDEFINED__\n"
                   "#elif DDD || BBB && CCC\n"
                   "    Print( 'OK' )\n"
                   "#endif",                "OK" );

    TEST_PARSE_OK( "#define AAA\n"
                   "#define BBB\n"
                   "#define CCC\n"
                   "#if __UNDEFINED__\n"
                   "#elif DDD || BBB && CCC || EEE\n"
                   "    Print( 'OK' )\n"
                   "#endif",                "OK" );

    TEST_PARSE_OK( "#define AAA\n"
                   "#define BBB\n"
                   "#define CCC\n"
                   "#if __UNDEFINED__\n"
                   "#elif DDD || BBB && DDD || EEE && AAA\n"
                   "    #error Should not be here\n"
                   "#else\n"
                   "    Print( 'OK' )\n"
                   "#endif",                "OK" );

    TEST_PARSE_OK( "#define AAA\n"
                   "#define BBB\n"
                   "#define CCC\n"
                   "#if __UNDEFINED__\n"
                   "#elif DDD || BBB && AAA || EEE && AAA\n"
                   "    Print( 'OK' )\n"
                   "#endif",                "OK" );

    TEST_PARSE_OK( "#define AAA\n"
                   "#define BBB\n"
                   "#define CCC\n"
                   "#if __UNDEFINED__\n"
                   "#elif DDD || BBB && DDD || BBB && AAA\n"
                   "    Print( 'OK' )\n"
                   "#endif",                "OK" );

    TEST_PARSE_OK( "#define AAA\n"
                   "#define BBB\n"
                   "#define CCC\n"
                   "#if __UNDEFINED__\n"
                   "#elif DDD || BBB && DDD || DDD && AAA || CCC\n"
                   "    Print( 'OK' )\n"
                   "#endif",                "OK" );

    TEST_PARSE_OK( "#define AAA\n"
                   "#define BBB\n"
                   "#define CCC\n"
                   "#if __UNDEFINED__\n"
                   "#elif DDD || BBB && CCC && AAA || DDD && AAA\n"
                   "    Print( 'OK' )\n"
                   "#endif",                "OK" );
}

//------------------------------------------------------------------------------
TEST_CASE( TestBFFParsing, ElseDirective )
{
    Parse( "Tools/FBuild/FBuildTest/Data/TestBFFParsing/else_directive.bff" );
}

//------------------------------------------------------------------------------
TEST_CASE( TestBFFParsing, ElseDirective_Bad )
{
    Parse( "Tools/FBuild/FBuildTest/Data/TestBFFParsing/else_directive_bad.bff", true ); // Expect failure
    TEST_ASSERT( GetRecordedOutput().Find( "Error #1041" ) );
}

//------------------------------------------------------------------------------
TEST_CASE( TestBFFParsing, ElseDirective_Bad2 )
{
    Parse( "Tools/FBuild/FBuildTest/Data/TestBFFParsing/else_directive_bad2.bff", true ); // Expect failure
    TEST_ASSERT( GetRecordedOutput().Find( "Error #1041" ) );
}

//------------------------------------------------------------------------------
TEST_CASE( TestBFFParsing, ElseDirective_Bad3 )
{
    Parse( "Tools/FBuild/FBuildTest/Data/TestBFFParsing/else_directive_bad3.bff", true ); // Expect failure
    TEST_ASSERT( GetRecordedOutput().Find( "Error #1041" ) );
}

//------------------------------------------------------------------------------
TEST_CASE( TestBFFParsing, ElseDirective_Bad4 )
{
    Parse( "Tools/FBuild/FBuildTest/Data/TestBFFParsing/else_directive_bad4.bff", true ); // Expect failure
    TEST_ASSERT( GetRecordedOutput().Find( "Error #1041" ) );
}

//------------------------------------------------------------------------------
TEST_CASE( TestBFFParsing, ElseDirective_Bad5 )
{
    Parse( "Tools/FBuild/FBuildTest/Data/TestBFFParsing/else_directive_bad5.bff", true ); // Expect failure
    TEST_ASSERT( GetRecordedOutput().Find( "Error #1041" ) );
}

//------------------------------------------------------------------------------
TEST_CASE( TestBFFParsing, ElseDirective_Bad6 )
{
    Parse( "Tools/FBuild/FBuildTest/Data/TestBFFParsing/else_directive_bad6.bff", true ); // Expect failure
    TEST_ASSERT( GetRecordedOutput().Find( "Error #1041" ) );
}

//------------------------------------------------------------------------------
TEST_CASE( TestBFFParsing, InvalidDirective )
{
    TEST_PARSE_FAIL( "#",           "Error #1030" );
    TEST_PARSE_FAIL( "#\n",         "Error #1030" );
    TEST_PARSE_FAIL( "#invalid\n",  "Error #1030" );
    TEST_PARSE_FAIL( "#define X\n"
                     "#if X\n"
                     "#invalid\n"
                     "#endif\n",    "Error #1030" );
}

//------------------------------------------------------------------------------
TEST_CASE( TestBFFParsing, DefineUndefineDirectives )
{
    Parse( "Tools/FBuild/FBuildTest/Data/TestBFFParsing/define_undef_directives.bff" );
}

//------------------------------------------------------------------------------
TEST_CASE( TestBFFParsing, BadDefineDirective )
{
    Parse( "Tools/FBuild/FBuildTest/Data/TestBFFParsing/bad_define.bff", true ); // expect failure
    TEST_ASSERT( GetRecordedOutput().Find( "Error #1038" ) );
}

//------------------------------------------------------------------------------
TEST_CASE( TestBFFParsing, BadUndefDirective )
{
    Parse( "Tools/FBuild/FBuildTest/Data/TestBFFParsing/bad_undef.bff", true ); // expect failure
    TEST_ASSERT( GetRecordedOutput().Find( "Error #1039" ) );

    TEST_PARSE_FAIL( "#define A\n"
                     "#undef A X\n",    "Error #1045 - Extraneous token(s)" );
}

//------------------------------------------------------------------------------
TEST_CASE( TestBFFParsing, BadUndefBuiltInDirective )
{
    Parse( "Tools/FBuild/FBuildTest/Data/TestBFFParsing/bad_undef_builtin.bff", true ); // expect failure
    TEST_ASSERT( GetRecordedOutput().Find( "Error #1040" ) );
}

//------------------------------------------------------------------------------
TEST_CASE( TestBFFParsing, ParentScope )
{
    Parse( "Tools/FBuild/FBuildTest/Data/TestBFFParsing/parent_scope.bff" );
}

//------------------------------------------------------------------------------
TEST_CASE( TestBFFParsing, ParentScopeBug )
{
    Parse( "Tools/FBuild/FBuildTest/Data/TestBFFParsing/parent_scope_bug.bff" );
}

//------------------------------------------------------------------------------
TEST_CASE( TestBFFParsing, ParentScopeBug2 )
{
    Parse( "Tools/FBuild/FBuildTest/Data/TestBFFParsing/parent_scope_bug2.bff", true );
    TEST_ASSERT( GetRecordedOutput().Find( "Error #1009 - Unknown variable" ) );
}

//------------------------------------------------------------------------------
TEST_CASE( TestBFFParsing, ParentScopeUnknown )
{
    Parse( "Tools/FBuild/FBuildTest/Data/TestBFFParsing/parent_scope_unknown.bff", true ); // expect failure
    TEST_ASSERT( GetRecordedOutput().Find( "Error #1009" ) );
}

//------------------------------------------------------------------------------
TEST_CASE( TestBFFParsing, FrozenVariable )
{
    Parse( "Tools/FBuild/FBuildTest/Data/TestBFFParsing/frozen_foreach.bff", true ); // expect failure
    TEST_ASSERT( GetRecordedOutput().Find( "Error #1060" ) );
}

//------------------------------------------------------------------------------
TEST_CASE( TestBFFParsing, FrozenVariable_Nested )
{
    Parse( "Tools/FBuild/FBuildTest/Data/TestBFFParsing/frozen_foreach_nested.bff", true ); // expect failure
    TEST_ASSERT( GetRecordedOutput().Find( "Error #1060" ) );
}

//------------------------------------------------------------------------------
TEST_CASE( TestBFFParsing, DynamicVarNameConstruction )
{
    Parse( "Tools/FBuild/FBuildTest/Data/TestBFFParsing/dynamic_var_name_construction.bff" );
}

//------------------------------------------------------------------------------
TEST_CASE( TestBFFParsing, OperatorMinus )
{
    Parse( "Tools/FBuild/FBuildTest/Data/TestBFFParsing/operator_minus.bff" );
}

//------------------------------------------------------------------------------
TEST_CASE( TestBFFParsing, BuiltInVariables )
{
    Parse( "Tools/FBuild/FBuildTest/Data/TestBFFParsing/builtin_variables.bff" );
}

//------------------------------------------------------------------------------
TEST_CASE( TestBFFParsing, CyclicDependency )
{
    // Check that the detection of cyclic dependencies is working
    // This can only detect static cyclic dependencies - i.e. those defined
    // in the BFF file. Dynamically discovered cyclic dependencies are
    // detected at build time and tested elsewhere.
    FBuildTestOptions options;
    options.m_ConfigFile = "Tools/FBuild/FBuildTest/Data/TestBFFParsing/cyclic_dependency.bff";
    FBuildForTest fBuild( options );

    // Parsing should fail due to cyclic dependency
    TEST_ASSERT( fBuild.Initialize() == false );
    TEST_ASSERT( GetRecordedOutput().Find( "Cyclic dependency detected for node" ) );
}

//------------------------------------------------------------------------------
TEST_CASE( TestBFFParsing, SelfAssignment )
{
    FBuildTestOptions options;
    options.m_ConfigFile = "Tools/FBuild/FBuildTest/Data/TestBFFParsing/self_assignment.bff";
    FBuildForTest fBuild( options );

    TEST_ASSERT( fBuild.Initialize() == true );
    TEST_ASSERT( GetRecordedOutput().Find( "FAILED" ) == nullptr );
}

//------------------------------------------------------------------------------
TEST_CASE( TestBFFParsing, SelfAssignment2 )
{
    // Check that self-assignment check doesn't prevent correct assignment when creating
    // a variable with the same name as a higher level scope (shadowing)
    FBuildTestOptions options;
    options.m_ConfigFile = "Tools/FBuild/FBuildTest/Data/TestBFFParsing/self_assignment2.bff";
    FBuildForTest fBuild( options );

    TEST_ASSERT( fBuild.Initialize() == true );
    TEST_ASSERT( GetRecordedOutput().Find( "FAILED" ) == nullptr );
}

//------------------------------------------------------------------------------
TEST_CASE( TestBFFParsing, Variables )
{
    // Incomplete declarations
    TEST_PARSE_FAIL( ".",       "Error #1017" );
    TEST_PARSE_FAIL( ".A",      "Error #1044" );
    TEST_PARSE_FAIL( ".A=",     "Error #1017" );
    TEST_PARSE_FAIL( ".A=[",    "Error #1002" );
    TEST_PARSE_FAIL( ".A={",    "Error #1002" );

    // Invalid declarations
    TEST_PARSE_FAIL( ".A=(",    "Error #1017" );
    TEST_PARSE_FAIL( ".A=]",    "Error #1017" );
    TEST_PARSE_FAIL( ".A=}",    "Error #1017" );

    // Invalid operations
    TEST_PARSE_FAIL( ".A<5",    "Error #1034" );
    TEST_PARSE_FAIL( ".A>5",    "Error #1034" );
    TEST_PARSE_FAIL( ".A<=5",   "Error #1034" );
    TEST_PARSE_FAIL( ".A>=5",   "Error #1034" );
    TEST_PARSE_FAIL( ".A!=5",   "Error #1034" );
    TEST_PARSE_FAIL( ".A==5",   "Error #1034" );

    // Dynamic variables (lhs)
    TEST_PARSE_OK( ".A = 'B'\n"
                   ".'$A$' = 'C'\n" );
    TEST_PARSE_OK( ".A = 'B'\n"
                   ".'Thing$A$' = 'C'\n" );

    // Dynamic variables (rhs)
    TEST_PARSE_OK( ".A = 'A'\n"
                   ".X = .'$A$'" );
    TEST_PARSE_OK( ".A = 'A'\n"
                   ".X = { .'$A$' }" );

    // Invalid dynamic variables (lhs)
    TEST_PARSE_FAIL( ".'$A$' = 'String'",   "Error #1009 - Unknown variable" );

    // Invalid dynamic variables (rhs)
    TEST_PARSE_FAIL( ".A = .'$B$'",         "Error #1009 - Unknown variable" );
    TEST_PARSE_FAIL( ".A = 'B'\n"
                     ".X = .'$A$'",         "Error #1009 - Unknown variable" );
    TEST_PARSE_FAIL( ".A = 'B'\n"
                     ".X = { .'$A$' }",     "Error #1009 - Unknown variable" );
}

//------------------------------------------------------------------------------
TEST_CASE( TestBFFParsing, Functions )
{
    TEST_PARSE_FAIL( "If",          "Error #1023" );
    TEST_PARSE_FAIL( "If(",         "Error #1002" );
    TEST_PARSE_FAIL( "If{",         "Error #1023" );
    TEST_PARSE_FAIL( "Settings",    "Error #1024" );
    TEST_PARSE_FAIL( "Settings(",   "Error #1021" );
    TEST_PARSE_FAIL( "Settings{",   "Error #1002" );
}

//------------------------------------------------------------------------------
TEST_CASE( TestBFFParsing, ErrorRowAndColumn )
{
    // Ensure error row/column indices are as expected
    // (indices are 1-based)

    // Normal with/without whitespace
    TEST_PARSE_FAIL( "X",           "(1,1)" );
    TEST_PARSE_FAIL( " X",          "(1,2)" );
    TEST_PARSE_FAIL( "    X",       "(1,5)" );

    // Tabs
    TEST_PARSE_FAIL( "\tX",         "(1,2)" );
    TEST_PARSE_FAIL( "\t\tX",       "(1,3)" );
    TEST_PARSE_FAIL( " \tX",        "(1,3)" );

    // Line endings
    TEST_PARSE_FAIL( "\n\nX",       "(3,1)" ); // \n line endings
    TEST_PARSE_FAIL( "\r\rX",       "(3,1)" ); // \r line endings
    TEST_PARSE_FAIL( "\r\n\r\nX",   "(3,1)" ); // \r\n line endings
    TEST_PARSE_FAIL( "\n\r\nX",     "(3,1)" ); // mixed line endings
    TEST_PARSE_FAIL( "\r\n\nX",     "(3,1)" ); // mixed line endings
}

//------------------------------------------------------------------------------
TEST_CASE( TestBFFParsing, ForEach )
{
    // Simple Loop
    TEST_PARSE_OK( ".Array1 = { 'A', 'B' }\n"
                   "ForEach( .A in .Array1 )"
                   "{}" );

    // Dual Loop
    // - Without comma separator
    TEST_PARSE_OK( ".Array1 = { 'A', 'B' }\n"
                   ".Array2 = { 'A', 'B' }\n"
                   "ForEach( .A in .Array1"
                   "         .B in .Array2 )"
                   "{}" );
    // - With comma separator
    TEST_PARSE_OK( ".Array1 = { 'A', 'B' }\n"
                   ".Array2 = { 'A', 'B' }\n"
                   "ForEach( .A in .Array1,"
                   "         .B in .Array2 )"
                   "{}" );
}

//------------------------------------------------------------------------------
TEST_CASE( TestBFFParsing, FunctionHeaders )
{
    // Ensure errors are reported for various types of malformed headers.
    // - Variable not declared
    TEST_PARSE_FAIL( "Copy( .Name )\n"
                     "{\n"
                     "    .Source = 'src1'\n"
                     "    .Dest = 'dst1'\n"
                     "}\n",     "Error #1009 - Unknown variable '.Name'." );
    // - Empty string
    TEST_PARSE_FAIL( "Copy( '' )\n"
                     "{\n"
                     "    .Source = 'src1'\n"
                     "    .Dest = 'dst1'\n"
                     "}\n",     "Error #1003 - Copy() - Empty string not allowed in Function header." );
    // - Missing required header
    TEST_PARSE_FAIL( "Alias()\n"
                     "{\n"
                     "    .Targets = ''\n"
                     "}\n",     "Error #1023 - Alias() - Function requires a header." );
    // - Variable substitution duplicate
    TEST_PARSE_FAIL( ".Name = 'X'\n"
                     "Copy( .Name )\n"
                     "{\n"
                     "    .Source = 'src1'\n"
                     "    .Dest = 'dst1'\n"
                     "}\n"
                     "Copy( .Name )\n" // <-- Duplicate
                     "{\n"
                     "    .Source = 'src2'\n"
                     "    .Dest = 'dst2'\n"
                     "}\n",     "#1100 - Copy() - Target 'X' already defined." );
    // - Unexpected function header
    TEST_PARSE_FAIL( "Settings( 'Bad' )\n"
                     "{}",      "#1021 - Settings() - Unexpected Function header." );
    TEST_PARSE_FAIL( ".Name= 'X'\n"
                     "Settings( .Name )\n"
                     "{}",      "#1021 - Settings() - Unexpected Function header." );
    TEST_PARSE_FAIL( "Settings( .Name )\n"
                     "{}",      "#1021 - Settings() - Unexpected Function header." );
    TEST_PARSE_FAIL( "Settings( '' )\n"
                     "{}",      "#1021 - Settings() - Unexpected Function header." );
    TEST_PARSE_FAIL( "Settings()\n"
                     "{}",      "#1021 - Settings() - Unexpected Function header." );

    // - Optional header missing is ok
    TEST_PARSE_OK( "Copy()\n"
                   "{\n"
                   "    .Source = 'src1'\n"
                   "    .Dest = 'dst1'\n"
                   "}\n" );

    // Variable substitutions used in header. We can infer that the substitution
    // worked as it results in an error as the target is already defined.
    // - Direct use of variable
    TEST_PARSE_FAIL( ".Compiler = 'fake-clang'\n"
                     ".CompilerOptions = '%1 -o %2'\n"
                     ".Name = 'X'\n"
                     "ObjectList( .Name ) {}\n"
                     "ObjectList( .Name ) {}\n",
                                "Error #1100 - ObjectList() - Target 'X' already defined." );
    // - Dynamic construction of variable name
    TEST_PARSE_FAIL( ".Compiler = 'fake-clang'\n"
                     ".CompilerOptions = '%1 -o %2'\n"
                     ".Name = 'X'\n"
                     ".Var = 'Name'"
                     "ObjectList( .'$Var$' ) {}\n"
                     "ObjectList( .'$Var$' ) {}\n",
                                "Error #1100 - ObjectList() - Target 'X' already defined." );
    // Valid case
    TEST_PARSE_OK( ".Compiler = 'fake-clang'\n"
                   ".CompilerOptions = '%1 -o %2'\n"
                   ".Name = 'X'\n"
                   "ObjectList( .Name ) {}\n"
                   ".Name = 'Y'\n"
                   "ObjectList( .Name ) {}\n" );
}

//------------------------------------------------------------------------------
TEST_CASE( TestBFFParsing, AlreadyDefined )
{
    // Alias
    TEST_PARSE_FAIL( "Alias( 'X' ) { .Targets = 'file' }\n"
                     "Alias( 'X' ) { .Targets = 'file' }\n",
                     "Error #1100 - Previously declared here:" );

    // Alias for a Function
    TEST_PARSE_FAIL( "Test( 'X' ) { .TestExecutable = 'exe' .TestOutput = 'out1' }\n"
                     "Test( 'X' ) { .TestExecutable = 'exe' .TestOutput = 'out2' }\n",
                     "Error #1100 - Previously declared here:" );

    // Direct name (not an Alias)
    TEST_PARSE_FAIL( "ObjectList( 'X' ) { .Compiler = 'clang' .CompilerOptions = '%1 %2' }\n"
                     "ObjectList( 'X' ) { .Compiler = 'clang' .CompilerOptions = '%1 %2' }\n",
                     "Error #1100 - Previously declared here:" );

    // Aliasing output
    TEST_PARSE_FAIL( "Alias( 'X' ) { .Targets = 'file' }\n"
                     "TextFile()   { .TextFileOutput = 'file' }\n",
                     "Error #1100 - Previously declared here:" );

    // Same target created multiple times in a loop
    TEST_PARSE_FAIL( ".Y = { 'a', 'b' }\n"
                     "ForEach( .X in .Y ) { Test() { .TestExecutable = 'exe' .TestOutput = 'out' } }\n",
                     "Error #1100 - Previously declared here:" );
}

//------------------------------------------------------------------------------
