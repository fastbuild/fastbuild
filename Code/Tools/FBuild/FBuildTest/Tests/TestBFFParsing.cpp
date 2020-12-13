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

// TestBFFParsing
//------------------------------------------------------------------------------
class TestBFFParsing : public FBuildTest
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
    void Array_TypeMismatch() const;
    void Integers() const;
    void UnnamedScope() const;
    void UnnamedScope_Unterminated() const;
    void Directives() const;
    void DefineDirective() const;
    void ExistsDirective() const;
    void IncludeDirective() const;
    void Include_ExcessiveDepth() const;
    void ImportDirective() const;
    void OnceDirective() const;
    void Structs() const;
    void Struct_Concatenation() const;
    void Struct_ConcatenationMismatch() const;
    void Struct_ConcatenationOrder() const;
    void Struct_Unterminated() const;
    void Struct_MemberShadowsSelf() const;
    void IncludeScope() const;
    void IfDirective() const;
    void IfExistsDirective() const;
    void IfFileExistsDirective() const;
    void IfFileExistsDirective_RelativePaths() const;
    void IfBooleanOperators() const;
    void ElseDirective() const;
    void ElseDirective_Bad() const;
    void ElseDirective_Bad2() const;
    void ElseDirective_Bad3() const;
    void ElseDirective_Bad4() const;
    void ElseDirective_Bad5() const;
    void ElseDirective_Bad6() const;
    void InvalidDirective() const;
    void DefineUndefineDirectives() const;
    void BadDefineDirective() const;
    void BadUndefDirective() const;
    void BadUndefBuiltInDirective() const;
    void ParentScope() const;
    void ParentScopeBug() const;
    void ParentScopeBug2() const;
    void ParentScopeUnknown() const;
    void FrozenVariable() const;
    void FrozenVariable_Nested() const;
    void DynamicVarNameConstruction() const;
    void OperatorMinus() const;
    void BuiltInVariables() const;
    void CyclicDependency() const;
    void SelfAssignment() const;
    void SelfAssignment2() const;
    void Variables() const;
    void Functions() const;
    void ErrorRowAndColumn() const;
    void ForEach() const;
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
    REGISTER_TEST( Array_TypeMismatch )
    REGISTER_TEST( Integers )
    REGISTER_TEST( UnnamedScope )
    REGISTER_TEST( UnnamedScope_Unterminated )
    REGISTER_TEST( Directives )
    REGISTER_TEST( DefineDirective )
    REGISTER_TEST( ExistsDirective )
    REGISTER_TEST( IncludeDirective )
    REGISTER_TEST( Include_ExcessiveDepth )
    REGISTER_TEST( ImportDirective )
    REGISTER_TEST( OnceDirective )
    REGISTER_TEST( Structs )
    REGISTER_TEST( Struct_Concatenation )
    REGISTER_TEST( Struct_ConcatenationMismatch )
    REGISTER_TEST( Struct_ConcatenationOrder )
    REGISTER_TEST( Struct_Unterminated )
    REGISTER_TEST( Struct_MemberShadowsSelf )
    REGISTER_TEST( IncludeScope )
    REGISTER_TEST( IfDirective )
    REGISTER_TEST( IfExistsDirective )
    REGISTER_TEST( IfFileExistsDirective )
    REGISTER_TEST( IfFileExistsDirective_RelativePaths )
    REGISTER_TEST( IfBooleanOperators )    
    REGISTER_TEST( ElseDirective )
    REGISTER_TEST( ElseDirective_Bad )
    REGISTER_TEST( ElseDirective_Bad2 )
    REGISTER_TEST( ElseDirective_Bad3 )
    REGISTER_TEST( ElseDirective_Bad4 )
    REGISTER_TEST( ElseDirective_Bad5 )
    REGISTER_TEST( ElseDirective_Bad6 )
    REGISTER_TEST( InvalidDirective )
    REGISTER_TEST( DefineUndefineDirectives )
    REGISTER_TEST( BadDefineDirective )
    REGISTER_TEST( BadUndefDirective )
    REGISTER_TEST( BadUndefBuiltInDirective )
    REGISTER_TEST( ParentScope )
    REGISTER_TEST( ParentScopeBug )
    REGISTER_TEST( ParentScopeBug2 )
    REGISTER_TEST( ParentScopeUnknown )
    REGISTER_TEST( FrozenVariable )
    REGISTER_TEST( FrozenVariable_Nested )
    REGISTER_TEST( DynamicVarNameConstruction )
    REGISTER_TEST( OperatorMinus )
    REGISTER_TEST( BuiltInVariables )
    REGISTER_TEST( CyclicDependency )
    REGISTER_TEST( SelfAssignment )
    REGISTER_TEST( SelfAssignment2 )
    REGISTER_TEST( Variables )
    REGISTER_TEST( Functions )
    REGISTER_TEST( ErrorRowAndColumn )
    REGISTER_TEST( ForEach )
REGISTER_TESTS_END

// Empty
//------------------------------------------------------------------------------
void TestBFFParsing::Empty() const
{
    TEST_PARSE_OK( "" );
}

// AlmostEmpty
//------------------------------------------------------------------------------
void TestBFFParsing::AlmostEmpty() const
{
    TEST_PARSE_OK( " " );
    TEST_PARSE_OK( "\t" );
    TEST_PARSE_OK( "\r" );
    TEST_PARSE_OK( "\n" );
    TEST_PARSE_OK( "\r\n" );
    TEST_PARSE_OK( " \t\r\n     " );
}

// Comments
//------------------------------------------------------------------------------
void TestBFFParsing::Comments() const
{
    Parse( "Tools/FBuild/FBuildTest/Data/TestBFFParsing/comments.bff" );

    TEST_PARSE_OK( "//" );
    TEST_PARSE_OK( ";" );
    TEST_PARSE_OK( "// Comments" );
    TEST_PARSE_OK( "; Comments" );
    TEST_PARSE_OK( "// Comments\n" );
    TEST_PARSE_OK( "; Comments\n" );
}

// Strings
//------------------------------------------------------------------------------
void TestBFFParsing::Strings() const
{
    Parse( "Tools/FBuild/FBuildTest/Data/TestBFFParsing/strings.bff" );
}

// String_Unterminated
//------------------------------------------------------------------------------
void TestBFFParsing::String_Unterminated() const
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

// Arrays
//------------------------------------------------------------------------------
void TestBFFParsing::Arrays() const
{
    Parse( "Tools/FBuild/FBuildTest/Data/TestBFFParsing/arrays.bff" );
}

// Array_Unterminated
//------------------------------------------------------------------------------
void TestBFFParsing::Array_Unterminated() const
{
    TEST_PARSE_FAIL( ".Array={",                        "Error #1002" );
    TEST_PARSE_FAIL( ".Array={;}",                      "Error #1002" );
    TEST_PARSE_FAIL( ".Array={//}",                     "Error #1002" );
    TEST_PARSE_FAIL( ".Array={\n#if FALSE\n}\n#endif",  "Error #1002" );
}

// Array_TypeMismatch
//------------------------------------------------------------------------------
void TestBFFParsing::Array_TypeMismatch() const
{
    Parse( "Tools/FBuild/FBuildTest/Data/TestBFFParsing/array_typemismatch.bff", true ); // expect failure
    TEST_ASSERT( GetRecordedOutput().Find( "FASTBuild Error #1034 - Operation not supported: 'ArrayOfStructs' = 'String'." ) );
}

// Integers
//------------------------------------------------------------------------------
void TestBFFParsing::Integers() const
{
    Parse( "Tools/FBuild/FBuildTest/Data/TestBFFParsing/integers.bff" );
}

// UnnamedScope
//------------------------------------------------------------------------------
void TestBFFParsing::UnnamedScope() const
{
    Parse( "Tools/FBuild/FBuildTest/Data/TestBFFParsing/unnamedscope.bff" );
}

// UnnamedScope_Unterminated
//------------------------------------------------------------------------------
void TestBFFParsing::UnnamedScope_Unterminated() const
{
    TEST_PARSE_FAIL( "{",       "Error #1002" );
    TEST_PARSE_FAIL( "{\n",     "Error #1002" );
}

// Directives
//------------------------------------------------------------------------------
void TestBFFParsing::Directives() const
{
    TEST_PARSE_FAIL( "#define #",     "Error #1010 - Unknown construct" );
    TEST_PARSE_FAIL( "#########",     "Error #1010 - Unknown construct" );
}

// DefineDirective
//------------------------------------------------------------------------------
void TestBFFParsing::DefineDirective() const
{
    TEST_PARSE_OK( "#define X");
    TEST_PARSE_OK( "#define X123");
    TEST_PARSE_OK( "#define X_");
    TEST_PARSE_OK( "#define _X_");

    TEST_PARSE_FAIL( "#define X Y",     "Error #1045 - Extraneous token(s)" );
}

// ExistsDirective
//------------------------------------------------------------------------------
void TestBFFParsing::ExistsDirective() const
{
    TEST_PARSE_FAIL( "#if exists",      "Error #1031" );
    TEST_PARSE_FAIL( "#if exists(",     "Error #1030" );
    TEST_PARSE_FAIL( "#if exists(x",    "Error #1031" );
}

// IncludeDirective
//------------------------------------------------------------------------------
void TestBFFParsing::IncludeDirective() const
{
    // Invalid incldue directives
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

// Include_ExcessiveDepth
//------------------------------------------------------------------------------
void TestBFFParsing::Include_ExcessiveDepth() const
{
    Parse( "Tools/FBuild/FBuildTest/Data/TestBFFParsing/include_excessivedepth.bff", true ); // expect failure
    TEST_ASSERT( GetRecordedOutput().Find( "Error #1035 - Excessive depth complexity" ) );
}

// ImportDirective
//------------------------------------------------------------------------------
void TestBFFParsing::ImportDirective() const
{
    TEST_PARSE_FAIL( "#import",             "Error #1031" );
    TEST_PARSE_FAIL( "#import 'string'",    "Error #1031" );

    // Invalid syntax
    Env::SetEnvVariable("BFF_TEST_IMPORT_VAR", AString("VALUE"));
    TEST_PARSE_FAIL( "#import BFF_TEST_IMPORT_VAR X",   "Error #1045 - Extraneous token(s)" );

    // Ensure special characters are not lost
    Env::SetEnvVariable("BFF_TEST_IMPORT_VAR2", AString("Special^And$Special"));
    TEST_PARSE_OK( "#import BFF_TEST_IMPORT_VAR2\n"
                   ".Expected = 'Special^^And^$Special'\n"
                   "If( .BFF_TEST_IMPORT_VAR2 == .Expected )\n"
                   "{\n"
                   "    Print( 'Special Characters OK' )\n"
                   "}",                     "Special Characters OK" );
}

// OnceDirective
//------------------------------------------------------------------------------
void TestBFFParsing::OnceDirective() const
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

// Structs
//------------------------------------------------------------------------------
void TestBFFParsing::Structs() const
{
    Parse( "Tools/FBuild/FBuildTest/Data/TestBFFParsing/structs.bff" );
}

// Struct_Concatenation
//------------------------------------------------------------------------------
void TestBFFParsing::Struct_Concatenation() const
{
    Parse( "Tools/FBuild/FBuildTest/Data/TestBFFParsing/struct_concatenation.bff" );
}

// Struct_ConcatenationMismatch
//------------------------------------------------------------------------------
void TestBFFParsing::Struct_ConcatenationMismatch() const
{
    Parse( "Tools/FBuild/FBuildTest/Data/TestBFFParsing/struct_concatenation_mismatch.bff", true ); // expect failure
    TEST_ASSERT( GetRecordedOutput().Find( "#1034 - Operation not supported: 'ArrayOfStrings' + 'Int'" ) );
}

// Struct_ConcatenationOrder
//------------------------------------------------------------------------------
void TestBFFParsing::Struct_ConcatenationOrder() const
{
    Parse( "Tools/FBuild/FBuildTest/Data/TestBFFParsing/struct_concatenation_order.bff" );

    // Ensure all properties are concatenated in a consistent order, regardless
    // of depth of recursion
    TEST_ASSERT( GetRecordedOutput().Find( ".Value1 = 'AB'" ) );
    TEST_ASSERT( GetRecordedOutput().Find( ".Value2 = 'AB'" ) );
    TEST_ASSERT( GetRecordedOutput().Find( ".Value3 = 'AB'" ) );
    TEST_ASSERT( GetRecordedOutput().Find( ".Value4 = 'AB'" ) );
}

// Struct_Unterminated
//------------------------------------------------------------------------------
void TestBFFParsing::Struct_Unterminated() const
{
    Parse( "Tools/FBuild/FBuildTest/Data/TestBFFParsing/struct_unterminated.bff", true ); // expect failure
    TEST_ASSERT( GetRecordedOutput().Find( "Error #1002" ) );
}

// Struct_MemberShadowsSelf
//------------------------------------------------------------------------------
void TestBFFParsing::Struct_MemberShadowsSelf() const
{
    Parse( "Tools/FBuild/FBuildTest/Data/TestBFFParsing/struct_membershadowsself.bff" );
}

// IncludeScope
//------------------------------------------------------------------------------
void TestBFFParsing::IncludeScope() const
{
    Parse( "Tools/FBuild/FBuildTest/Data/TestBFFParsing/include_scope.bff" );
}

// IfDirective
//------------------------------------------------------------------------------
void TestBFFParsing::IfDirective() const
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

// IfExistsDirective
//------------------------------------------------------------------------------
void TestBFFParsing::IfExistsDirective() const
{
    Env::SetEnvVariable("BFF_TEST_ENV_VAR1", AString("1"));
    Env::SetEnvVariable("BFF_TEST_ENV_VAR2", AString("2"));
    Parse( "Tools/FBuild/FBuildTest/Data/TestBFFParsing/if_exists_directive.bff" );
}

// IfFileExistsDirective
//------------------------------------------------------------------------------
void TestBFFParsing::IfFileExistsDirective() const
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
        const char * rootBFF    = "../tmp/Test/BFFParsing/FileExistsDirective/if_file_exists_directive.dat";
        const char * fileName   = "../tmp/Test/BFFParsing/FileExistsDirective/file.dat";
        const char * db         = "../tmp/Test/BFFParsing/FileExistsDirective/fbuild.fdb";

        // Copy root bff to temp dir
        FileIO::SetReadOnly( rootBFF, false );
        EnsureFileDoesNotExist( rootBFF );
        EnsureDirExists("../tmp/Test/BFFParsing/FileExistsDirective/");
        FileIO::FileCopy( "Tools/FBuild/FBuildTest/Data/TestBFFParsing/if_file_exists_directive.bff", rootBFF );

        // Delete extra file from previous test run
        EnsureFileDoesNotExist( fileName );

        FBuildTestOptions options;
        options.m_ConfigFile = rootBFF;

        // Parse bff, which checks if file exists
        {
            FBuild fBuild( options );
            TEST_ASSERT( fBuild.Initialize() );
            fBuild.SaveDependencyGraph( db );

            const AString & output( GetRecordedOutput() );

            // File should NOT exist
            TEST_ASSERT( output.Find( "File does not exist" ) );
        }

        // Check existence re-parsing does not occur with not change to file existence
        {
            const size_t sizeOfRecordedOutput = GetRecordedOutput().GetLength();

            FBuild fBuild( options );
            TEST_ASSERT( fBuild.Initialize( db ) );
            fBuild.SaveDependencyGraph( db );

            // Should not re-parse
            const AStackString<> output( GetRecordedOutput().Get() + sizeOfRecordedOutput );
            TEST_ASSERT( output.Find( "BFF will be re-parsed" ) == nullptr );
        }

        // Create file
        {
            FileIO::EnsurePathExistsForFile( AStackString<>( fileName ) );
            FileStream f;
            TEST_ASSERT( f.Open( fileName, FileStream::WRITE_ONLY ) );
        }

        // Load db and ensure it is re-parsed
        {
            const size_t sizeOfRecordedOutput = GetRecordedOutput().GetLength();

            FBuild fBuild( options );
            TEST_ASSERT( fBuild.Initialize( db ) );
            fBuild.SaveDependencyGraph( db );

            const AStackString<> output( GetRecordedOutput().Get() + sizeOfRecordedOutput );

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

            FBuild fBuild( options );
            TEST_ASSERT( fBuild.Initialize( db ) );

            const AStackString<> output( GetRecordedOutput().Get() + sizeOfRecordedOutput );

            // Check re-parse was triggered
            TEST_ASSERT( output.Find( "File used in file_exists was removed" ) );

            // File should exist
            TEST_ASSERT( output.Find( "File does not exist" ) );
        }
    }
}

// IfFileExistsDirective_RelativePaths
//------------------------------------------------------------------------------
void TestBFFParsing::IfFileExistsDirective_RelativePaths() const
{
    // file_exists treats paths the same way as #include
    // (paths are relative to the bff)
    
    FBuildTestOptions options;
    options.m_ConfigFile    = "Tools/FBuild/FBuildTest/Data/TestBFFParsing/IfFileExistsDirective/RelativePaths/root.bff";

    FBuild fBuild( options );
    TEST_ASSERT( fBuild.Initialize() );

    // Check for expected output
    const AString & output( GetRecordedOutput() );
    TEST_ASSERT( output.Find( "OK-1A" ) && output.Find( "OK-1B" ));
    TEST_ASSERT( output.Find( "OK-2A" ) && output.Find( "OK-2B" ));
    TEST_ASSERT( output.Find( "OK-3A" ) && output.Find( "OK-3B" ));
    TEST_ASSERT( output.Find( "OK-4" ) );
}

// IfBooleanOperators
//------------------------------------------------------------------------------
void TestBFFParsing::IfBooleanOperators() const
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
    AStackString<> complex( "#if " );
    for ( size_t i = 0; i < 256; ++i )
    {
        complex.AppendFormat( "A%u &&", (uint32_t)i );
    }
    complex += "B";
    TEST_PARSE_FAIL( complex.Get(),         "#1047 - If expression too complex. Up to 256 boolean operators supported." );

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

// ElseDirective
//------------------------------------------------------------------------------
void TestBFFParsing::ElseDirective() const
{
    Parse( "Tools/FBuild/FBuildTest/Data/TestBFFParsing/else_directive.bff" );
}

// ElseDirective_Bad
//------------------------------------------------------------------------------
void TestBFFParsing::ElseDirective_Bad() const
{
    Parse( "Tools/FBuild/FBuildTest/Data/TestBFFParsing/else_directive_bad.bff", true ); // Expect failure
    TEST_ASSERT( GetRecordedOutput().Find( "Error #1041" ) );
}

// ElseDirective_Bad2
//------------------------------------------------------------------------------
void TestBFFParsing::ElseDirective_Bad2() const
{
    Parse( "Tools/FBuild/FBuildTest/Data/TestBFFParsing/else_directive_bad2.bff", true ); // Expect failure
    TEST_ASSERT( GetRecordedOutput().Find( "Error #1041" ) );
}

// ElseDirective_Bad3
//------------------------------------------------------------------------------
void TestBFFParsing::ElseDirective_Bad3() const
{
    Parse( "Tools/FBuild/FBuildTest/Data/TestBFFParsing/else_directive_bad3.bff", true ); // Expect failure
    TEST_ASSERT( GetRecordedOutput().Find( "Error #1041" ) );
}

// ElseDirective_Bad4
//------------------------------------------------------------------------------
void TestBFFParsing::ElseDirective_Bad4() const
{
    Parse( "Tools/FBuild/FBuildTest/Data/TestBFFParsing/else_directive_bad4.bff", true ); // Expect failure
    TEST_ASSERT( GetRecordedOutput().Find( "Error #1041" ) );
}

// ElseDirective_Bad5
//------------------------------------------------------------------------------
void TestBFFParsing::ElseDirective_Bad5() const
{
    Parse( "Tools/FBuild/FBuildTest/Data/TestBFFParsing/else_directive_bad5.bff", true ); // Expect failure
    TEST_ASSERT( GetRecordedOutput().Find( "Error #1041" ) );
}

// ElseDirective_Bad6
//------------------------------------------------------------------------------
void TestBFFParsing::ElseDirective_Bad6() const
{
    Parse( "Tools/FBuild/FBuildTest/Data/TestBFFParsing/else_directive_bad6.bff", true ); // Expect failure
    TEST_ASSERT( GetRecordedOutput().Find( "Error #1041" ) );
}

// InvalidDirective
//------------------------------------------------------------------------------
void TestBFFParsing::InvalidDirective() const
{
    TEST_PARSE_FAIL( "#",           "Error #1030" );
    TEST_PARSE_FAIL( "#\n",         "Error #1030" );
    TEST_PARSE_FAIL( "#invalid\n",  "Error #1030" );
    TEST_PARSE_FAIL( "#define X\n"
                     "#if X\n"
                     "#invalid\n"
                     "#endif\n",    "Error #1030" );
}

// DefineUndefineDirectives
//------------------------------------------------------------------------------
void TestBFFParsing::DefineUndefineDirectives() const
{
    Parse( "Tools/FBuild/FBuildTest/Data/TestBFFParsing/define_undef_directives.bff" );
}

// BadDefineDirective
//------------------------------------------------------------------------------
void TestBFFParsing::BadDefineDirective() const
{
    Parse( "Tools/FBuild/FBuildTest/Data/TestBFFParsing/bad_define.bff", true ); // expect failure
    TEST_ASSERT( GetRecordedOutput().Find( "Error #1038" ) );
}

// BadUndefDirective
//------------------------------------------------------------------------------
void TestBFFParsing::BadUndefDirective() const
{
    Parse( "Tools/FBuild/FBuildTest/Data/TestBFFParsing/bad_undef.bff", true ); // expect failure
    TEST_ASSERT( GetRecordedOutput().Find( "Error #1039" ) );

    TEST_PARSE_FAIL( "#define A\n"
                     "#undef A X\n",    "Error #1045 - Extraneous token(s)" );
}

// BadUndefBuiltInDirective
//------------------------------------------------------------------------------
void TestBFFParsing::BadUndefBuiltInDirective() const
{
    Parse( "Tools/FBuild/FBuildTest/Data/TestBFFParsing/bad_undef_builtin.bff", true ); // expect failure
    TEST_ASSERT( GetRecordedOutput().Find( "Error #1040" ) );
}

// ParentScope
//------------------------------------------------------------------------------
void TestBFFParsing::ParentScope() const
{
    Parse( "Tools/FBuild/FBuildTest/Data/TestBFFParsing/parent_scope.bff" );
}

// ParentScopeBug
//------------------------------------------------------------------------------
void TestBFFParsing::ParentScopeBug() const
{
    Parse( "Tools/FBuild/FBuildTest/Data/TestBFFParsing/parent_scope_bug.bff" );
}

// ParentScopeBug2
//------------------------------------------------------------------------------
void TestBFFParsing::ParentScopeBug2() const
{
    Parse( "Tools/FBuild/FBuildTest/Data/TestBFFParsing/parent_scope_bug2.bff", true );
    TEST_ASSERT( GetRecordedOutput().Find( "Error #1009 - Unknown variable" ) );
}

// ParentScopeUnknown
//------------------------------------------------------------------------------
void TestBFFParsing::ParentScopeUnknown() const
{
    Parse( "Tools/FBuild/FBuildTest/Data/TestBFFParsing/parent_scope_unknown.bff", true ); // expect failure
    TEST_ASSERT( GetRecordedOutput().Find( "Error #1009" ) );
}

// FrozenVariable
//------------------------------------------------------------------------------
void TestBFFParsing::FrozenVariable() const
{
    Parse( "Tools/FBuild/FBuildTest/Data/TestBFFParsing/frozen_foreach.bff", true ); // expect failure
    TEST_ASSERT( GetRecordedOutput().Find( "Error #1060" ) );
}

// FrozenVariable_Nested
//------------------------------------------------------------------------------
void TestBFFParsing::FrozenVariable_Nested() const
{
    Parse( "Tools/FBuild/FBuildTest/Data/TestBFFParsing/frozen_foreach_nested.bff", true ); // expect failure
    TEST_ASSERT( GetRecordedOutput().Find( "Error #1060" ) );
}

// DynamicVarNameConstruction
//------------------------------------------------------------------------------
void TestBFFParsing::DynamicVarNameConstruction() const
{
    Parse( "Tools/FBuild/FBuildTest/Data/TestBFFParsing/dynamic_var_name_construction.bff" );
}

// OperatorMinus
//------------------------------------------------------------------------------
void TestBFFParsing::OperatorMinus() const
{
    Parse( "Tools/FBuild/FBuildTest/Data/TestBFFParsing/operator_minus.bff" );
}

// BuiltInVariables
//------------------------------------------------------------------------------
void TestBFFParsing::BuiltInVariables() const
{
    Parse( "Tools/FBuild/FBuildTest/Data/TestBFFParsing/builtin_variables.bff" );
}

// CyclicDependency
//------------------------------------------------------------------------------
void TestBFFParsing::CyclicDependency() const
{
    FBuildTestOptions options;
    options.m_ConfigFile = "Tools/FBuild/FBuildTest/Data/TestBFFParsing/cyclic_dependency.bff";
    FBuild fBuild( options );

    // Parsing should fail due to cyclic dependency
    TEST_ASSERT( fBuild.Initialize() == false );
    TEST_ASSERT( GetRecordedOutput().Find( "Cyclic dependency detected for node" ) );
}

// SelfAssignment
//------------------------------------------------------------------------------
void TestBFFParsing::SelfAssignment() const
{
    FBuildTestOptions options;
    options.m_ConfigFile = "Tools/FBuild/FBuildTest/Data/TestBFFParsing/self_assignment.bff";
    FBuild fBuild( options );

    TEST_ASSERT( fBuild.Initialize() == true );
    TEST_ASSERT( GetRecordedOutput().Find( "FAILED" ) == nullptr );
}

// SelfAssignment2
//------------------------------------------------------------------------------
void TestBFFParsing::SelfAssignment2() const
{
    // Check that self-assignment check doesn't prevent correct assignment when creating
    // a variable with the same name as a higher level scope (shadowing)
    FBuildTestOptions options;
    options.m_ConfigFile = "Tools/FBuild/FBuildTest/Data/TestBFFParsing/self_assignment2.bff";
    FBuild fBuild( options );

    TEST_ASSERT( fBuild.Initialize() == true );
    TEST_ASSERT( GetRecordedOutput().Find( "FAILED" ) == nullptr );
}

// Variables
//------------------------------------------------------------------------------
void TestBFFParsing::Variables() const
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
                     ".X = .'$A$'",         "Error #1009 - Unknown variable"  );
    TEST_PARSE_FAIL( ".A = 'B'\n"
                     ".X = { .'$A$' }",     "Error #1009 - Unknown variable"  );
}

// Functions
//------------------------------------------------------------------------------
void TestBFFParsing::Functions() const
{
    TEST_PARSE_FAIL( "If",          "Error #1023" );
    TEST_PARSE_FAIL( "If(",         "Error #1002" );
    TEST_PARSE_FAIL( "If{",         "Error #1023" );
    TEST_PARSE_FAIL( "Settings",    "Error #1024" );
    TEST_PARSE_FAIL( "Settings(",   "Error #1021" );
    TEST_PARSE_FAIL( "Settings{",   "Error #1002" );
}

// ErrorRowAndColumn
//------------------------------------------------------------------------------
void TestBFFParsing::ErrorRowAndColumn() const
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

// ForEach
//------------------------------------------------------------------------------
void TestBFFParsing::ForEach() const
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
