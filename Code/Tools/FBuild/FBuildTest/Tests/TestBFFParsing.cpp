// TestBFFParsing.cpp
//------------------------------------------------------------------------------

// Includes
//------------------------------------------------------------------------------
#include "Tools/FBuild/FBuildTest/Tests/FBuildTest.h"

#include "Tools/FBuild/FBuildCore/FBuild.h"
#include "Tools/FBuild/FBuildCore/BFF/BFFParser.h"
#include "Tools/FBuild/FBuildCore/Graph/NodeGraph.h"

#include "Core/Containers/AutoPtr.h"
#include "Core/Env/Env.h"
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
    void IncludeDirective() const;
    void OnceDirective() const;
    void Structs() const;
    void Struct_Concatenation() const;
    void Struct_ConcatenationMismatch() const;
    void Struct_ConcatenationOrder() const;
    void Struct_Unterminated() const;
    void IncludeScope() const;
    void IfDirective() const;
    void IfExistsDirective() const;
    void ElseDirective() const;
    void ElseDirective_Bad() const;
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
    void DynamicVarNameConstruction() const;
    void OperatorMinus() const;
    void IfFunctionTrue() const;
    void IfNotFunctionTrue() const;
    void IfSetFunctionTrue() const;
    void IfNotSetFunctionTrue() const;
    void IfFunctionFalse() const;
    void IfNotFunctionFalse() const;
    void IfSetFunctionFalse() const;
    void IfNotSetFunctionFalse() const;
    void BuiltInVariables() const;

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
    REGISTER_TEST( Array_TypeMismatch )
    REGISTER_TEST( Integers )
    REGISTER_TEST( UnnamedScope )
    REGISTER_TEST( IncludeDirective )
    REGISTER_TEST( OnceDirective )
    REGISTER_TEST( Structs )
    REGISTER_TEST( Struct_Concatenation )
    REGISTER_TEST( Struct_ConcatenationMismatch )
    REGISTER_TEST( Struct_ConcatenationOrder )
    REGISTER_TEST( Struct_Unterminated )
    REGISTER_TEST( IncludeScope )
    REGISTER_TEST( IfDirective )
    REGISTER_TEST( IfExistsDirective )
    REGISTER_TEST( ElseDirective )
    REGISTER_TEST( ElseDirective_Bad )
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
    REGISTER_TEST( DynamicVarNameConstruction )
    REGISTER_TEST( OperatorMinus )
    REGISTER_TEST( IfFunctionTrue )
    REGISTER_TEST( IfNotFunctionTrue )
    REGISTER_TEST( IfSetFunctionTrue )
    REGISTER_TEST( IfNotSetFunctionTrue )
    REGISTER_TEST( IfFunctionFalse )
    REGISTER_TEST( IfNotFunctionFalse )
    REGISTER_TEST( IfSetFunctionFalse )
    REGISTER_TEST( IfNotSetFunctionFalse )    
    REGISTER_TEST( BuiltInVariables )
REGISTER_TESTS_END

// Empty
//------------------------------------------------------------------------------
void TestBFFParsing::Empty() const
{
    // an empty file should pass without problem
    char buffer[ 1 ] = { '\000' }; // post data sentinel
    NodeGraph ng;
    BFFParser p( ng );
    TEST_ASSERT( p.Parse( buffer, 0, "empty.bff", 0, 0 ) );
}

// AlmostEmpty
//------------------------------------------------------------------------------
void TestBFFParsing::AlmostEmpty() const
{
    // an empty file should pass without problem
    const char * buffer = "\r\n\000"; // empty line + post data sentinel
    NodeGraph ng;
    BFFParser p( ng );
    TEST_ASSERT( p.Parse( buffer, 2, "empty.bff", 0, 0 ) );
}

// Comments
//------------------------------------------------------------------------------
void TestBFFParsing::Comments() const
{
    Parse( "Tools/FBuild/FBuildTest/Data/TestBFFParsing/comments.bff" );
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
    Parse( "Tools/FBuild/FBuildTest/Data/TestBFFParsing/string_unterminated.bff", true ); // expect failure
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
    Parse( "Tools/FBuild/FBuildTest/Data/TestBFFParsing/array_unterminated.bff", true ); // expect failure
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

// IncludeDirective
//------------------------------------------------------------------------------
void TestBFFParsing::IncludeDirective() const
{
    Parse( "Tools/FBuild/FBuildTest/Data/TestBFFParsing/includes.bff" );
}

// OnceDirective
//------------------------------------------------------------------------------
void TestBFFParsing::OnceDirective() const
{
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
    NodeGraph ng;
    BFFParser p( ng );
    bool parseResult = p.Parse( mem.Get(), fileSize, fileName, 0, 0 );
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
    Parse( "Tools/FBuild/FBuildTest/Data/TestBFFParsing/include_scope.bff" );
}

// IfDirective
//------------------------------------------------------------------------------
void TestBFFParsing::IfDirective() const
{
    Parse( "Tools/FBuild/FBuildTest/Data/TestBFFParsing/if_directive.bff" );
}

// IfExistsDirective
//------------------------------------------------------------------------------
void TestBFFParsing::IfExistsDirective() const
{
    Env::SetEnvVariable("BFF_TEST_ENV_VAR1", AString("1"));
    Env::SetEnvVariable("BFF_TEST_ENV_VAR2", AString("2"));
    Parse( "Tools/FBuild/FBuildTest/Data/TestBFFParsing/if_exists_directive.bff" );
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
    Parse( "Tools/FBuild/FBuildTest/Data/TestBFFParsing/else_directive_bad2.bff", true ); // Expect failure
    Parse( "Tools/FBuild/FBuildTest/Data/TestBFFParsing/else_directive_bad3.bff", true ); // Expect failure
    Parse( "Tools/FBuild/FBuildTest/Data/TestBFFParsing/else_directive_bad4.bff", true ); // Expect failure
    Parse( "Tools/FBuild/FBuildTest/Data/TestBFFParsing/else_directive_bad5.bff", true ); // Expect failure
    Parse( "Tools/FBuild/FBuildTest/Data/TestBFFParsing/else_directive_bad6.bff", true ); // Expect failure
}

// InvalidDirective
//------------------------------------------------------------------------------
void TestBFFParsing::InvalidDirective() const
{
    Parse( "Tools/FBuild/FBuildTest/Data/TestBFFParsing/invalid_directive.bff", true ); // Expect failure
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
}

// BadUndefDirective
//------------------------------------------------------------------------------
void TestBFFParsing::BadUndefDirective() const
{
    Parse( "Tools/FBuild/FBuildTest/Data/TestBFFParsing/bad_undef.bff", true ); // expect failure
}

// BadUndefBuiltInDirective
//------------------------------------------------------------------------------
void TestBFFParsing::BadUndefBuiltInDirective() const
{
    Parse( "Tools/FBuild/FBuildTest/Data/TestBFFParsing/bad_undef_builtin.bff", true ); // expect failure
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
    TEST_ASSERT( GetRecordedOutput().Find( "FASTBuild Error #1026" ) ); // Variable '%s' not found for modification.
}

// ParentScopeUnknown
//------------------------------------------------------------------------------
void TestBFFParsing::ParentScopeUnknown() const
{
    Parse( "Tools/FBuild/FBuildTest/Data/TestBFFParsing/parent_scope_unknown.bff", true ); // expect failure
}

// FrozenVariable
//------------------------------------------------------------------------------
void TestBFFParsing::FrozenVariable() const
{
    Parse( "Tools/FBuild/FBuildTest/Data/TestBFFParsing/frozen_foreach.bff", true ); // expect failure
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

// IfFunctionTrue
//------------------------------------------------------------------------------
void TestBFFParsing::IfFunctionTrue() const
{
    Parse( "Tools/FBuild/FBuildTest/Data/TestBFFParsing/if_function_true.bff" );
    TEST_ASSERT( GetRecordedOutput().Find( "Success" ) );
}

// IfNotFunctionTrue
//------------------------------------------------------------------------------
void TestBFFParsing::IfNotFunctionTrue() const
{
    Parse( "Tools/FBuild/FBuildTest/Data/TestBFFParsing/if_function_true.bff" );
    TEST_ASSERT( GetRecordedOutput().Find( "Success" ) );
}

// IfSetFunctionTrue
//------------------------------------------------------------------------------
void TestBFFParsing::IfSetFunctionTrue() const
{
    Parse( "Tools/FBuild/FBuildTest/Data/TestBFFParsing/if_set_function_true.bff" );
    TEST_ASSERT( GetRecordedOutput().Find( "Success" ) );
}

// IfNotSetFunctionTrue
//------------------------------------------------------------------------------
void TestBFFParsing::IfNotSetFunctionTrue() const
{
    Parse( "Tools/FBuild/FBuildTest/Data/TestBFFParsing/if_not_set_function_true.bff" );
    TEST_ASSERT( GetRecordedOutput().Find( "Success" ) );
}

// IfFunctionFalse
//------------------------------------------------------------------------------
void TestBFFParsing::IfFunctionFalse() const
{
    Parse( "Tools/FBuild/FBuildTest/Data/TestBFFParsing/if_function_false.bff" );
    TEST_ASSERT( GetRecordedOutput().Find( "Failure" ) == nullptr );
}

// IfNotFunctionFalse
//------------------------------------------------------------------------------
void TestBFFParsing::IfNotFunctionFalse() const
{
    Parse( "Tools/FBuild/FBuildTest/Data/TestBFFParsing/if_function_false.bff" );
    TEST_ASSERT( GetRecordedOutput().Find( "Failure" ) == nullptr );
}

// IfSetFunctionFalse
//------------------------------------------------------------------------------
void TestBFFParsing::IfSetFunctionFalse() const
{
    Parse( "Tools/FBuild/FBuildTest/Data/TestBFFParsing/if_set_function_false.bff" );
    TEST_ASSERT( GetRecordedOutput().Find( "Failure" ) == nullptr );
}

// IfNotSetFunctionFalse
//------------------------------------------------------------------------------
void TestBFFParsing::IfNotSetFunctionFalse() const
{
    Parse( "Tools/FBuild/FBuildTest/Data/TestBFFParsing/if_not_set_function_false.bff" );
    TEST_ASSERT( GetRecordedOutput().Find( "Failure" ) == nullptr );
}

// BuiltInVariables
//------------------------------------------------------------------------------
void TestBFFParsing::BuiltInVariables() const
{
    Parse( "Tools/FBuild/FBuildTest/Data/TestBFFParsing/builtin_variables.bff" );
}

//------------------------------------------------------------------------------
