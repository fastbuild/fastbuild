// TestNodeReflection.cpp
//------------------------------------------------------------------------------

// Includes
//------------------------------------------------------------------------------
#include "FBuildTest.h"

// FBuildCore
#include "Tools/FBuild/FBuildCore/BFF/BFFStackFrame.h"
#include "Tools/FBuild/FBuildCore/BFF/Functions/Function.h"
#include "Tools/FBuild/FBuildCore/FBuild.h"
#include "Tools/FBuild/FBuildCore/Graph/NodeGraph.h"

// Core
#include "Core/FileIO/PathUtils.h"
#include "Core/Strings/AStackString.h"

// TestLinker
//------------------------------------------------------------------------------
class TestNodeReflection : public FBuildTest
{
private:
    DECLARE_TESTS

    // String - Optional
    void String_Optional_NotSet() const;
    void String_Optional_Set() const;
    void String_Optional_Empty() const;

    // String - Required
    void String_Required_NotSet() const;
    void String_Required_Set() const;
    void String_Required_Empty() const;

    // ArrayOfStrings - Optional
    void ArrayOfStrings_Optional_NotSet() const;
    void ArrayOfStrings_Optional_Set() const;
    void ArrayOfStrings_Optional_Empty() const;
    void ArrayOfStrings_Optional_EmptyElement() const;

    // ArrayOfStrings - Required
    void ArrayOfStrings_Required_NotSet() const;
    void ArrayOfStrings_Required_Set() const;
    void ArrayOfStrings_Required_Empty() const;
    void ArrayOfStrings_Required_EmptyElement() const;

    // MetaFile - String - Optional
    void MetaFile_String_Optional_NotSet() const;
    void MetaFile_String_Optional_Set() const;
    void MetaFile_String_Optional_Empty() const;

    // MetaFile - String - Required
    void MetaFile_String_Required_NotSet() const;
    void MetaFile_String_Required_Set() const;
    void MetaFile_String_Required_Empty() const;

    // MetaFile - ArrayOfStrings - Optional
    void MetaFile_ArrayOfStrings_Optional_NotSet() const;
    void MetaFile_ArrayOfStrings_Optional_Set() const;
    void MetaFile_ArrayOfStrings_Optional_Empty() const;
    void MetaFile_ArrayOfStrings_Optional_EmptyElement() const;

    // MetaFile - ArrayOfStrings - Required
    void MetaFile_ArrayOfStrings_Required_NotSet() const;
    void MetaFile_ArrayOfStrings_Required_Set() const;
    void MetaFile_ArrayOfStrings_Required_Empty() const;
    void MetaFile_ArrayOfStrings_Required_EmptyElement() const;

    // MetaPath - String - Optional
    void MetaPath_String_Optional_NotSet() const;
    void MetaPath_String_Optional_Set() const;
    void MetaPath_String_Optional_Empty() const;

    // MetaFile - String - Required
    void MetaPath_String_Required_NotSet() const;
    void MetaPath_String_Required_Set() const;
    void MetaPath_String_Required_Empty() const;

    // MetaPath - ArrayOfStrings - Optional
    void MetaPath_ArrayOfStrings_Optional_NotSet() const;
    void MetaPath_ArrayOfStrings_Optional_Set() const;
    void MetaPath_ArrayOfStrings_Optional_Empty() const;
    void MetaPath_ArrayOfStrings_Optional_EmptyElement() const;

    // MetaPath - ArrayOfStrings - Required
    void MetaPath_ArrayOfStrings_Required_NotSet() const;
    void MetaPath_ArrayOfStrings_Required_Set() const;
    void MetaPath_ArrayOfStrings_Required_Empty() const;
    void MetaPath_ArrayOfStrings_Required_EmptyElement() const;
};

// Register Tests
//------------------------------------------------------------------------------
REGISTER_TESTS_BEGIN( TestNodeReflection )
    // String - Optional
    REGISTER_TEST( String_Optional_NotSet )
    REGISTER_TEST( String_Optional_Set )
    REGISTER_TEST( String_Optional_Empty )

    // String - Required
    REGISTER_TEST( String_Required_NotSet )
    REGISTER_TEST( String_Required_Set )
    REGISTER_TEST( String_Required_Empty )

    // ArrayOfStrings - Optional
    REGISTER_TEST( ArrayOfStrings_Optional_NotSet )
    REGISTER_TEST( ArrayOfStrings_Optional_Set )
    REGISTER_TEST( ArrayOfStrings_Optional_Empty )
    REGISTER_TEST( ArrayOfStrings_Optional_EmptyElement )

    // ArrayOfStrings - Required
    REGISTER_TEST( ArrayOfStrings_Required_NotSet )
    REGISTER_TEST( ArrayOfStrings_Required_Set )
    REGISTER_TEST( ArrayOfStrings_Required_Empty )
    REGISTER_TEST( ArrayOfStrings_Required_EmptyElement )

    // MetaFile - String - Optional
    REGISTER_TEST( MetaFile_String_Optional_NotSet )
    REGISTER_TEST( MetaFile_String_Optional_Set )
    REGISTER_TEST( MetaFile_String_Optional_Empty )

    // MetaFile - String - Required
    REGISTER_TEST( MetaFile_String_Required_NotSet )
    REGISTER_TEST( MetaFile_String_Required_Set )
    REGISTER_TEST( MetaFile_String_Required_Empty )

    // MetaFile - ArrayOfStrings - Optional
    REGISTER_TEST( MetaFile_ArrayOfStrings_Optional_NotSet )
    REGISTER_TEST( MetaFile_ArrayOfStrings_Optional_Set )
    REGISTER_TEST( MetaFile_ArrayOfStrings_Optional_Empty )
    REGISTER_TEST( MetaFile_ArrayOfStrings_Optional_EmptyElement )

    // MetaFile - ArrayOfStrings - Required
    REGISTER_TEST( MetaFile_ArrayOfStrings_Required_NotSet )
    REGISTER_TEST( MetaFile_ArrayOfStrings_Required_Set )
    REGISTER_TEST( MetaFile_ArrayOfStrings_Required_Empty )
    REGISTER_TEST( MetaFile_ArrayOfStrings_Required_EmptyElement )

    // MetaPath - String - Optional
    REGISTER_TEST( MetaPath_String_Optional_NotSet )
    REGISTER_TEST( MetaPath_String_Optional_Set )
    REGISTER_TEST( MetaPath_String_Optional_Empty )

    // MetaPath - String - Required
    REGISTER_TEST( MetaPath_String_Required_NotSet )
    REGISTER_TEST( MetaPath_String_Required_Set )
    REGISTER_TEST( MetaPath_String_Required_Empty )

    // MetaPath - ArrayOfStrings - Optional
    REGISTER_TEST( MetaPath_ArrayOfStrings_Optional_NotSet )
    REGISTER_TEST( MetaPath_ArrayOfStrings_Optional_Set )
    REGISTER_TEST( MetaPath_ArrayOfStrings_Optional_Empty )
    REGISTER_TEST( MetaPath_ArrayOfStrings_Optional_EmptyElement )

    // MetaPath - ArrayOfStrings - Required
    REGISTER_TEST( MetaPath_ArrayOfStrings_Required_NotSet )
    REGISTER_TEST( MetaPath_ArrayOfStrings_Required_Set )
    REGISTER_TEST( MetaPath_ArrayOfStrings_Required_Empty )
    REGISTER_TEST( MetaPath_ArrayOfStrings_Required_EmptyElement )
REGISTER_TESTS_END

// Helper classese
//------------------------------------------------------------------------------

// BaseNode
//  - De-duplicate common elements
//------------------------------------------------------------------------------
class BaseNode : public Node
{
    REFLECT_DECLARE( BaseNode )
public:
    BaseNode()
        : Node( AStackString<>( "dummy" ), Node::PROXY_NODE, 0 )
    {}
    virtual bool Initialize( NodeGraph & /*nodeGraph*/, const BFFToken * /*funcStartIter*/, const Function * /*function*/ ) override
    {
        ASSERT( false );
        return false;
    }
    virtual bool IsAFile() const override { return true; }

    AString         m_String;
    Array<AString>  m_ArrayOfStrings;
};
REFLECT_BEGIN( BaseNode, Node, MetaNone() )
REFLECT_END( BaseNode )

// FunctionWrapper
//  - Allow test to directly invoke PopulateProperties outside of normal BFF parsing
//------------------------------------------------------------------------------
class FunctionWrapper : public Function
{
public:
    FunctionWrapper()
        : Function( "dummyfunction" )
    {}

    bool Populate( NodeGraph & ng, BFFToken * iter, Node & n )
    {
        return Function::PopulateProperties( ng, iter, &n );
    }

    virtual Node * CreateNode() const override { return FNEW( BaseNode ); }
};

// TestHelper
//  - De-duplicate common setup for tests
//------------------------------------------------------------------------------
class TestHelper
{
public:
    explicit TestHelper( BaseNode * node )
        : m_Node( node ) {}
    ~TestHelper()
    {
        delete m_Node;
        delete m_Function;
    }

    NodeGraph           m_NodeGraph;
    FBuild              m_FBuild;
    BFFToken *          m_Token = nullptr;
    BaseNode *          m_Node;
    FunctionWrapper *   m_Function = new FunctionWrapper(); // Freed by FBuild destructor
    BFFStackFrame       m_Frame;

    bool Populate() { return m_Function->Populate( m_NodeGraph, m_Token, *m_Node ); }

    void CheckFile( const AString & file ) const
    {
        #if defined( ASSERTS_ENABLED ) // IsCleanPath only available in debug builds
            TEST_ASSERT( m_NodeGraph.IsCleanPath( file ) );
        #endif
        TEST_ASSERT( PathUtils::IsFullPath( file ) );
        TEST_ASSERT( PathUtils::IsFolderPath( file ) == false );
    }

    void CheckPath( const AString & path ) const
    {
        #if defined( ASSERTS_ENABLED ) // IsCleanPath only available in debug builds
            TEST_ASSERT( m_NodeGraph.IsCleanPath( path ) );
        #endif
        TEST_ASSERT( PathUtils::IsFullPath( path ) );
        TEST_ASSERT( PathUtils::IsFolderPath( path ) );
    }
};

// Macros
//  - Simplify exposing properties
//------------------------------------------------------------------------------
#define TEST_NODE_BEGIN( name ) \
    class name : public BaseNode \
    { \
        REFLECT_NODE_DECLARE( name ) \
    }; \
    REFLECT_NODE_BEGIN( name, BaseNode, MetaNone() )

#define TEST_NODE_END( name ) \
    REFLECT_END( name )

//==============================================================================
// String - Optional
//==============================================================================
TEST_NODE_BEGIN( Node_String_Optional )
    REFLECT( m_String,    "String",   MetaOptional() ) // Optional
TEST_NODE_END( Node_String_Optional )

// String_Optional_NotSet
//------------------------------------------------------------------------------
void TestNodeReflection::String_Optional_NotSet() const
{
    TestHelper helper( new Node_String_Optional );

    // String not set

    // Ok to not set since string is optional
    TEST_ASSERT( helper.Populate() );
    TEST_ASSERT( helper.m_Node->m_String.IsEmpty() );
}

// String_Optional_Set
//------------------------------------------------------------------------------
void TestNodeReflection::String_Optional_Set() const
{
    TestHelper helper( new Node_String_Optional );

    // Set string
    helper.m_Frame.SetVarString( AStackString<>( ".String" ), AStackString<>( "value" ), nullptr );

    // Check string was set
    TEST_ASSERT( helper.Populate() );
    TEST_ASSERT( helper.m_Node->m_String == "value" );
}

// String_Optional_Empty
//------------------------------------------------------------------------------
void TestNodeReflection::String_Optional_Empty() const
{
    TestHelper helper( new Node_String_Optional );

    // Set string to empty
    helper.m_Frame.SetVarString( AStackString<>( ".String" ), AString::GetEmpty(), nullptr );

    // Check ok (setting empty on optional property is ok)
    TEST_ASSERT( helper.Populate() );
    TEST_ASSERT( helper.m_Node->m_String.IsEmpty() );
}

//==============================================================================
// String - Required
//==============================================================================
TEST_NODE_BEGIN( Node_String_Required )
    REFLECT( m_String,    "String",   MetaNone() ) // Required
TEST_NODE_END( Node_String_Required )

// String_Required_NotSet
//------------------------------------------------------------------------------
void TestNodeReflection::String_Required_NotSet() const
{
    TestHelper helper( new Node_String_Required );

    // String not set

    // Check for failure and specific error
    TEST_ASSERT( helper.Populate() == false );
    TEST_ASSERT( GetRecordedOutput().Find( "Missing required property" ) );
}

// String_Required_Set
//------------------------------------------------------------------------------
void TestNodeReflection::String_Required_Set() const
{
    TestHelper helper( new Node_String_Required );

    // Set string
    helper.m_Frame.SetVarString( AStackString<>( ".String" ), AStackString<>( "value" ), nullptr );

    // Check string was set
    TEST_ASSERT( helper.Populate() );
    TEST_ASSERT( helper.m_Node->m_String == "value" );
}

// String_Required_Empty
//------------------------------------------------------------------------------
void TestNodeReflection::String_Required_Empty() const
{
    TestHelper helper( new Node_String_Required );

    // Set string to empty
    helper.m_Frame.SetVarString( AStackString<>( ".String" ), AString::GetEmpty(), nullptr );

    // Check for failure (required strings can't be empty)
    TEST_ASSERT( helper.Populate() == false );
    TEST_ASSERT( helper.m_Node->m_String.IsEmpty() );
    TEST_ASSERT( GetRecordedOutput().Find( "Empty string not allowed" ) );
}

//==============================================================================
// ArrayOfStrings - Optional
//==============================================================================
TEST_NODE_BEGIN( Node_ArrayOfStrings_Optional )
    REFLECT_ARRAY( m_ArrayOfStrings,    "ArrayOfStrings",   MetaOptional() ) // Optional
TEST_NODE_END( Node_ArrayOfStrings_Optional )

// ArrayOfStrings_Optional_NotSet
//------------------------------------------------------------------------------
void TestNodeReflection::ArrayOfStrings_Optional_NotSet() const
{
    TestHelper helper( new Node_ArrayOfStrings_Optional );

    // Array not set

    // Ok to not set since array is optional
    TEST_ASSERT( helper.Populate() );
    TEST_ASSERT( helper.m_Node->m_String.IsEmpty() );
}

// ArrayOfStrings_Optional_Set
//------------------------------------------------------------------------------
void TestNodeReflection::ArrayOfStrings_Optional_Set() const
{
    TestHelper helper( new Node_ArrayOfStrings_Optional );

    // Set string array
    Array<AString> strings;
    strings.EmplaceBack( "value" );
    helper.m_Frame.SetVarArrayOfStrings( AStackString<>( ".ArrayOfStrings" ), strings, nullptr );

    // Check array was set
    TEST_ASSERT( helper.Populate() );
    TEST_ASSERT( helper.m_Node->m_ArrayOfStrings.GetSize() == 1 );
    TEST_ASSERT( helper.m_Node->m_ArrayOfStrings[ 0 ] == "value" );
}

// ArrayOfStrings_Optional_Empty
//------------------------------------------------------------------------------
void TestNodeReflection::ArrayOfStrings_Optional_Empty() const
{
    TestHelper helper( new Node_ArrayOfStrings_Optional );

    // Set array to empty
    Array<AString> empty;
    helper.m_Frame.SetVarArrayOfStrings( AStackString<>( ".ArrayOfStrings" ), empty, nullptr );

    // Check ok (setting empty on optional property is ok)
    TEST_ASSERT( helper.Populate() );
    TEST_ASSERT( helper.m_Node->m_ArrayOfStrings.IsEmpty() );
}

// ArrayOfStrings_Optional_EmptyElement
//------------------------------------------------------------------------------
void TestNodeReflection::ArrayOfStrings_Optional_EmptyElement() const
{
    TestHelper helper( new Node_ArrayOfStrings_Optional );

    // Non-empty array with empty element
    Array<AString> strings;
    strings.EmplaceBack( "value" );
    strings.EmplaceBack();
    helper.m_Frame.SetVarArrayOfStrings( AStackString<>( ".ArrayOfStrings" ), strings, nullptr );

    // Check failure (empty strings in arrays are not allowed)
    TEST_ASSERT( helper.Populate() == false );
    TEST_ASSERT( GetRecordedOutput().Find( "Empty string not allowed" ) );
}

//==============================================================================
// ArrayOfStrings - Required
//==============================================================================
TEST_NODE_BEGIN( Node_ArrayOfStrings_Required )
    REFLECT_ARRAY( m_ArrayOfStrings,    "ArrayOfStrings",   MetaNone() ) // Required
TEST_NODE_END( Node_ArrayOfStrings_Required )

// ArrayOfStrings_Required_NotSet
//------------------------------------------------------------------------------
void TestNodeReflection::ArrayOfStrings_Required_NotSet() const
{
    TestHelper helper( new Node_ArrayOfStrings_Required );

    // Array not set

    // Check for failure with appropriate error
    TEST_ASSERT( helper.Populate() == false );
    TEST_ASSERT( GetRecordedOutput().Find( "Missing required property" ) );
}

// ArrayOfStrings_Required_Set
//------------------------------------------------------------------------------
void TestNodeReflection::ArrayOfStrings_Required_Set() const
{
    TestHelper helper( new Node_ArrayOfStrings_Required );

    // Set string array
    Array<AString> strings;
    strings.EmplaceBack( "value" );
    helper.m_Frame.SetVarArrayOfStrings( AStackString<>( ".ArrayOfStrings" ), strings, nullptr );

    // Check array was set
    TEST_ASSERT( helper.Populate() );
    TEST_ASSERT( helper.m_Node->m_ArrayOfStrings.GetSize() == 1 );
    TEST_ASSERT( helper.m_Node->m_ArrayOfStrings[ 0 ] == "value" );
}

// ArrayOfStrings_Required_Empty
//------------------------------------------------------------------------------
void TestNodeReflection::ArrayOfStrings_Required_Empty() const
{
    TestHelper helper( new Node_ArrayOfStrings_Required );

    // Set array to empty
    Array<AString> empty;
    helper.m_Frame.SetVarArrayOfStrings( AStackString<>( ".ArrayOfStrings" ), empty, nullptr );

    // Check for failure (can't set empty array if property is required)
    TEST_ASSERT( helper.Populate() == false );
    TEST_ASSERT( GetRecordedOutput().Find( "Empty string not allowed" ) ); // TODO:B Array specific error?
}

// ArrayOfStrings_Required_EmptyElement
//------------------------------------------------------------------------------
void TestNodeReflection::ArrayOfStrings_Required_EmptyElement() const
{
    TestHelper helper( new Node_ArrayOfStrings_Required );

    // Non-empty array with empty element
    Array<AString> strings;
    strings.EmplaceBack( "value" );
    strings.EmplaceBack();
    helper.m_Frame.SetVarArrayOfStrings( AStackString<>( ".ArrayOfStrings" ), strings, nullptr );

    // Check failure (empty strings in arrays are not allowed)
    TEST_ASSERT( helper.Populate() == false );
    TEST_ASSERT( GetRecordedOutput().Find( "Empty string not allowed" ) );
}

//==============================================================================
// MetaFile - String - Optional
//==============================================================================
TEST_NODE_BEGIN( Node_MetaFile_String_Optional )
    REFLECT( m_String,    "File",   MetaFile() + MetaOptional() ) // Optional
TEST_NODE_END( Node_MetaFile_String_Optional )

// MetaFile_String_Optional_NotSet
//------------------------------------------------------------------------------
void TestNodeReflection::MetaFile_String_Optional_NotSet() const
{
    TestHelper helper( new Node_MetaFile_String_Optional );

    // String not set

    // Ok for optional File to be missing
    TEST_ASSERT( helper.Populate() == true );
}

// MetaFile_String_Optional_Set
//------------------------------------------------------------------------------
void TestNodeReflection::MetaFile_String_Optional_Set() const
{
    TestHelper helper( new Node_MetaFile_String_Optional );

    // Push a string
    helper.m_Frame.SetVarString( AStackString<>( ".File" ), AStackString<>( "value" ), nullptr );

    // Check the property was set and converted to a full path
    TEST_ASSERT( helper.Populate() == true );
    TEST_ASSERT( helper.m_Node->m_String.EndsWith( "value" ) );
    helper.CheckFile( helper.m_Node->m_String );
}

// MetaFile_String_Optional_Empty
//------------------------------------------------------------------------------
void TestNodeReflection::MetaFile_String_Optional_Empty() const
{
    TestHelper helper( new Node_MetaFile_String_Optional );

    // Push an empty string
    helper.m_Frame.SetVarString( AStackString<>( ".File" ), AString::GetEmpty(), nullptr );

    // Ok for property to be empty because it is optional
    TEST_ASSERT( helper.Populate() == true );
}

//==============================================================================
// MetaFile - String - Required
//==============================================================================
TEST_NODE_BEGIN( Node_MetaFile_String_Required )
    REFLECT( m_String,    "File",   MetaFile() ) // Required
TEST_NODE_END( Node_MetaFile_String_Required )

// MetaFile_String_Required_NotSet
//------------------------------------------------------------------------------
void TestNodeReflection::MetaFile_String_Required_NotSet() const
{
    TestHelper helper( new Node_MetaFile_String_Required );

    // String not set

    // Check that populating properties fails and that appropriate error is reported
    TEST_ASSERT( helper.Populate() == false );
    TEST_ASSERT( GetRecordedOutput().Find( "Missing required property" ) );
}

// MetaFile_String_Required_Set
//------------------------------------------------------------------------------
void TestNodeReflection::MetaFile_String_Required_Set() const
{
    TestHelper helper( new Node_MetaFile_String_Required );

    // Push a string
    helper.m_Frame.SetVarString( AStackString<>( ".File" ), AStackString<>( "value" ), nullptr );

    // Check the property was set and converted to a full path
    TEST_ASSERT( helper.Populate() == true );
    TEST_ASSERT( helper.m_Node->m_String.EndsWith( "value" ) );
    helper.CheckFile( helper.m_Node->m_String );
}

// MetaFile_String_Required_Empty
//------------------------------------------------------------------------------
void TestNodeReflection::MetaFile_String_Required_Empty() const
{
    TestHelper helper( new Node_MetaFile_String_Required );

    // Push an empty string
    helper.m_Frame.SetVarString( AStackString<>( ".File" ), AString::GetEmpty(), nullptr );

    // Check that populating properties fails and that appropriate error is reported
    TEST_ASSERT( helper.Populate() == false );
    TEST_ASSERT( GetRecordedOutput().Find( "Empty string not allowed" ) );
}

//==============================================================================
// MetaFile - ArrayOfStrings - Optional
//==============================================================================
TEST_NODE_BEGIN( Node_MetaFile_ArrayOfStrings_Optional )
    REFLECT_ARRAY( m_ArrayOfStrings,    "Files",   MetaFile() + MetaOptional() ) // Optional
TEST_NODE_END( Node_MetaFile_ArrayOfStrings_Optional )

// MetaFile_ArrayOfStringsOptional_NotSet
//------------------------------------------------------------------------------
void TestNodeReflection::MetaFile_ArrayOfStrings_Optional_NotSet() const
{
    TestHelper helper( new Node_MetaFile_ArrayOfStrings_Optional );

    // ArrayOfStrings not set

    // Ok for optional File(s) to be missing
    TEST_ASSERT( helper.Populate() == true );
}

// MetaFile_ArrayOfStrings_Optional_Set
//------------------------------------------------------------------------------
void TestNodeReflection::MetaFile_ArrayOfStrings_Optional_Set() const
{
    TestHelper helper( new Node_MetaFile_ArrayOfStrings_Optional );

    // Set string array
    Array<AString> strings;
    strings.EmplaceBack( "value" );
    helper.m_Frame.SetVarArrayOfStrings( AStackString<>( ".Files" ), strings, nullptr );

    // Check the property was set and converted to a full paths
    TEST_ASSERT( helper.Populate() == true );
    TEST_ASSERT( helper.m_Node->m_ArrayOfStrings.GetSize() == 1 );
    TEST_ASSERT( helper.m_Node->m_ArrayOfStrings[ 0 ].EndsWith( "value" ) );
    helper.CheckFile( helper.m_Node->m_ArrayOfStrings[ 0 ] );
}

// MetaFile_ArrayOfStrings_Optional_Empty
//------------------------------------------------------------------------------
void TestNodeReflection::MetaFile_ArrayOfStrings_Optional_Empty() const
{
    TestHelper helper( new Node_MetaFile_ArrayOfStrings_Optional );

    // Set string array with an empty element in in
    Array<AString> empty;
    helper.m_Frame.SetVarArrayOfStrings( AStackString<>( ".Files" ), empty, nullptr );

    // Ok for property to be empty because it is optional
    TEST_ASSERT( helper.Populate() == true );
}

// MetaFile_ArrayOfStrings_Optional_EmptyElement
//------------------------------------------------------------------------------
void TestNodeReflection::MetaFile_ArrayOfStrings_Optional_EmptyElement() const
{
    TestHelper helper( new Node_MetaFile_ArrayOfStrings_Optional );

    // Non-empty array with empty element
    Array<AString> strings;
    strings.EmplaceBack( "value" );
    strings.EmplaceBack();
    helper.m_Frame.SetVarArrayOfStrings( AStackString<>( ".Files" ), strings, nullptr );

    // Check failure (empty strings in arrays are not allowed)
    TEST_ASSERT( helper.Populate() == false );
    TEST_ASSERT( GetRecordedOutput().Find( "Empty string not allowed" ) );
}

//==============================================================================
// MetaFile - ArrayOfStrings - Required
//==============================================================================
TEST_NODE_BEGIN( Node_MetaFile_ArrayOfStrings_Required )
    REFLECT_ARRAY( m_ArrayOfStrings,    "Files",   MetaFile() ) // Required
TEST_NODE_END( Node_MetaFile_ArrayOfStrings_Required )

// MetaFile_ArrayOfStrings_Required_NotSet
//------------------------------------------------------------------------------
void TestNodeReflection::MetaFile_ArrayOfStrings_Required_NotSet() const
{
    TestHelper helper( new Node_MetaFile_ArrayOfStrings_Required );

    // String not set

    // Check that populating properties fails and that appropriate error is reported
    TEST_ASSERT( helper.Populate() == false );
    TEST_ASSERT( GetRecordedOutput().Find( "Missing required property" ) );
}

// MetaFile_ArrayOfStrings_Required_Set
//------------------------------------------------------------------------------
void TestNodeReflection::MetaFile_ArrayOfStrings_Required_Set() const
{
    TestHelper helper( new Node_MetaFile_ArrayOfStrings_Required );

    // Set string array
    Array<AString> strings;
    strings.EmplaceBack( "value" );
    helper.m_Frame.SetVarArrayOfStrings( AStackString<>( ".Files" ), strings, nullptr );

    // Check the property was set and converted to a full path
    TEST_ASSERT( helper.Populate() == true );
    TEST_ASSERT( helper.m_Node->m_ArrayOfStrings.GetSize() == 1 );
    TEST_ASSERT( helper.m_Node->m_ArrayOfStrings[ 0 ].EndsWith( "value" ) );
    helper.CheckFile( helper.m_Node->m_ArrayOfStrings[ 0 ] );
}

// MetaFile_ArrayOfStrings_Required_Empty
//------------------------------------------------------------------------------
void TestNodeReflection::MetaFile_ArrayOfStrings_Required_Empty() const
{
    TestHelper helper( new Node_MetaFile_ArrayOfStrings_Required );

    // Set string array with an empty element in in
    Array<AString> empty;
    helper.m_Frame.SetVarArrayOfStrings( AStackString<>( ".Files" ), empty, nullptr );

    // Check that populating properties fails and that appropriate error is reported
    TEST_ASSERT( helper.Populate() == false );
    TEST_ASSERT( GetRecordedOutput().Find( "Empty string not allowed" ) );
}

// MetaFile_ArrayOfStrings_Required_EmptyElement
//------------------------------------------------------------------------------
void TestNodeReflection::MetaFile_ArrayOfStrings_Required_EmptyElement() const
{
    TestHelper helper( new Node_MetaFile_ArrayOfStrings_Required );

    // Non-empty array with empty element
    Array<AString> strings;
    strings.EmplaceBack( "value" );
    strings.EmplaceBack();
    helper.m_Frame.SetVarArrayOfStrings( AStackString<>( ".Files" ), strings, nullptr );

    // Check failure (empty strings in arrays are not allowed)
    TEST_ASSERT( helper.Populate() == false );
    TEST_ASSERT( GetRecordedOutput().Find( "Empty string not allowed" ) );
}

//==============================================================================
// MetaPath - String - Optional
//==============================================================================
TEST_NODE_BEGIN( Node_MetaPath_String_Optional )
    REFLECT( m_String,    "Path",   MetaPath() + MetaOptional() ) // Optional
TEST_NODE_END( Node_MetaPath_String_Optional )

// MetaPath_String_Optional_NotSet
//------------------------------------------------------------------------------
void TestNodeReflection::MetaPath_String_Optional_NotSet() const
{
    TestHelper helper( new Node_MetaPath_String_Optional );

    // String not set

    // Ok for optional File to be missing
    TEST_ASSERT( helper.Populate() == true );
}

// MetaPath_String_Optional_Set
//------------------------------------------------------------------------------
void TestNodeReflection::MetaPath_String_Optional_Set() const
{
    TestHelper helper( new Node_MetaPath_String_Optional );

    // Push a string
    helper.m_Frame.SetVarString( AStackString<>( ".Path" ), AStackString<>( "value" ), nullptr );

    // Check the property was set and converted to a full path
    TEST_ASSERT( helper.Populate() == true );
    TEST_ASSERT( helper.m_Node->m_String.Find( "value" ) );
    helper.CheckPath( helper.m_Node->m_String );
}

// MetaPath_String_Optional_Empty
//------------------------------------------------------------------------------
void TestNodeReflection::MetaPath_String_Optional_Empty() const
{
    TestHelper helper( new Node_MetaPath_String_Optional );

    // Push an empty string
    helper.m_Frame.SetVarString( AStackString<>( ".Path" ), AString::GetEmpty(), nullptr );

    // Ok for property to be empty because it is optional
    TEST_ASSERT( helper.Populate() == true );
}

//==============================================================================
// MetaPath - String - Required
//==============================================================================
TEST_NODE_BEGIN( Node_MetaPath_String_Required )
    REFLECT( m_String,    "Path",   MetaPath() ) // Required
TEST_NODE_END( Node_MetaPath_String_Required )

// MetaPath_String_Required_NotSet
//------------------------------------------------------------------------------
void TestNodeReflection::MetaPath_String_Required_NotSet() const
{
    TestHelper helper( new Node_MetaPath_String_Required );

    // String not set

    // Check that populating properties fails and that appropriate error is reported
    TEST_ASSERT( helper.Populate() == false );
    TEST_ASSERT( GetRecordedOutput().Find( "Missing required property" ) );
}

// MetaPath_String_Required_Set
//------------------------------------------------------------------------------
void TestNodeReflection::MetaPath_String_Required_Set() const
{
    TestHelper helper( new Node_MetaPath_String_Required );

    // Push a string
    helper.m_Frame.SetVarString( AStackString<>( ".Path" ), AStackString<>( "value" ), nullptr );

    // Check the property was set and converted to a full path
    TEST_ASSERT( helper.Populate() == true );
    TEST_ASSERT( helper.m_Node->m_String.Find( "value" ) );
    helper.CheckPath( helper.m_Node->m_String );
}

// MetaPath_String_Required_Empty
//------------------------------------------------------------------------------
void TestNodeReflection::MetaPath_String_Required_Empty() const
{
    TestHelper helper( new Node_MetaPath_String_Required );

    // Push an empty string
    helper.m_Frame.SetVarString( AStackString<>( ".Path" ), AString::GetEmpty(), nullptr );

    // Check that populating properties fails and that appropriate error is reported
    TEST_ASSERT( helper.Populate() == false );
    TEST_ASSERT( GetRecordedOutput().Find( "Empty string not allowed" ) );
}

//==============================================================================
// MetaPath - ArrayOfStrings - Optional
//==============================================================================
TEST_NODE_BEGIN( Node_MetaPath_ArrayOfStrings_Optional )
    REFLECT_ARRAY( m_ArrayOfStrings,    "Paths",   MetaPath() + MetaOptional() ) // Optional
TEST_NODE_END( Node_MetaPath_ArrayOfStrings_Optional )

// MetaPath_ArrayOfStringsOptional_NotSet
//------------------------------------------------------------------------------
void TestNodeReflection::MetaPath_ArrayOfStrings_Optional_NotSet() const
{
    TestHelper helper( new Node_MetaPath_ArrayOfStrings_Optional );

    // ArrayOfStrings not set

    // Ok for optional File(s) to be missing
    TEST_ASSERT( helper.Populate() == true );
}

// MetaPath_ArrayOfStrings_Optional_Set
//------------------------------------------------------------------------------
void TestNodeReflection::MetaPath_ArrayOfStrings_Optional_Set() const
{
    TestHelper helper( new Node_MetaPath_ArrayOfStrings_Optional );

    // Set string array
    Array<AString> strings;
    strings.EmplaceBack( "value" );
    helper.m_Frame.SetVarArrayOfStrings( AStackString<>( ".Paths" ), strings, nullptr );

    // Check the property was set and converted to a full paths
    TEST_ASSERT( helper.Populate() == true );
    TEST_ASSERT( helper.m_Node->m_ArrayOfStrings.GetSize() == 1 );
    TEST_ASSERT( helper.m_Node->m_ArrayOfStrings[ 0 ].Find( "value" ) );
    helper.CheckPath( helper.m_Node->m_ArrayOfStrings[ 0 ] );
}

// MetaPath_ArrayOfStrings_Optional_Empty
//------------------------------------------------------------------------------
void TestNodeReflection::MetaPath_ArrayOfStrings_Optional_Empty() const
{
    TestHelper helper( new Node_MetaPath_ArrayOfStrings_Optional );

    // Set string array with an empty element in in
    Array<AString> empty;
    helper.m_Frame.SetVarArrayOfStrings( AStackString<>( ".Paths" ), empty, nullptr );

    // Ok for property to be empty because it is optional
    TEST_ASSERT( helper.Populate() == true );
}

// MetaPath_ArrayOfStrings_Optional_EmptyElement
//------------------------------------------------------------------------------
void TestNodeReflection::MetaPath_ArrayOfStrings_Optional_EmptyElement() const
{
    TestHelper helper( new Node_MetaPath_ArrayOfStrings_Optional );

    // Non-empty array with empty element
    Array<AString> strings;
    strings.EmplaceBack( "value" );
    strings.EmplaceBack();
    helper.m_Frame.SetVarArrayOfStrings( AStackString<>( ".Paths" ), strings, nullptr );

    // Check failure (empty strings in arrays are not allowed)
    TEST_ASSERT( helper.Populate() == false );
    TEST_ASSERT( GetRecordedOutput().Find( "Empty string not allowed" ) );
}

//==============================================================================
// MetaPath - ArrayOfStrings - Required
//==============================================================================
TEST_NODE_BEGIN( Node_MetaPath_ArrayOfStrings_Required )
    REFLECT_ARRAY( m_ArrayOfStrings,    "Paths",   MetaPath() ) // Required
TEST_NODE_END( Node_MetaPath_ArrayOfStrings_Required )

// MetaPath_ArrayOfStrings_Required_NotSet
//------------------------------------------------------------------------------
void TestNodeReflection::MetaPath_ArrayOfStrings_Required_NotSet() const
{
    TestHelper helper( new Node_MetaPath_ArrayOfStrings_Required );

    // String not set

    // Check that populating properties fails and that appropriate error is reported
    TEST_ASSERT( helper.Populate() == false );
    TEST_ASSERT( GetRecordedOutput().Find( "Missing required property" ) );
}

// MetaPath_ArrayOfStrings_Required_Set
//------------------------------------------------------------------------------
void TestNodeReflection::MetaPath_ArrayOfStrings_Required_Set() const
{
    TestHelper helper( new Node_MetaPath_ArrayOfStrings_Required );

    // Set string array
    Array<AString> strings;
    strings.EmplaceBack( "value" );
    helper.m_Frame.SetVarArrayOfStrings( AStackString<>( ".Paths" ), strings, nullptr );

    // Check the property was set and converted to a full path
    TEST_ASSERT( helper.Populate() == true );
    TEST_ASSERT( helper.m_Node->m_ArrayOfStrings.GetSize() == 1 );
    TEST_ASSERT( helper.m_Node->m_ArrayOfStrings[ 0 ].Find( "value" ) );
    helper.CheckPath( helper.m_Node->m_ArrayOfStrings[ 0 ] );
}

// MetaPath_ArrayOfStrings_Required_Empty
//------------------------------------------------------------------------------
void TestNodeReflection::MetaPath_ArrayOfStrings_Required_Empty() const
{
    TestHelper helper( new Node_MetaPath_ArrayOfStrings_Required );

    // Set string array with an empty element in in
    Array<AString> empty;
    helper.m_Frame.SetVarArrayOfStrings( AStackString<>( ".Paths" ), empty, nullptr );

    // Check that populating properties fails and that appropriate error is reported
    TEST_ASSERT( helper.Populate() == false );
    TEST_ASSERT( GetRecordedOutput().Find( "Empty string not allowed" ) );
}

// MetaPath_ArrayOfStrings_Required_EmptyElement
//------------------------------------------------------------------------------
void TestNodeReflection::MetaPath_ArrayOfStrings_Required_EmptyElement() const
{
    TestHelper helper( new Node_MetaPath_ArrayOfStrings_Required );

    // Non-empty array with empty element
    Array<AString> strings;
    strings.EmplaceBack( "value" );
    strings.EmplaceBack();
    helper.m_Frame.SetVarArrayOfStrings( AStackString<>( ".Paths" ), strings, nullptr );

    // Check failure (empty strings in arrays are not allowed)
    TEST_ASSERT( helper.Populate() == false );
    TEST_ASSERT( GetRecordedOutput().Find( "Empty string not allowed" ) );
}

//------------------------------------------------------------------------------
