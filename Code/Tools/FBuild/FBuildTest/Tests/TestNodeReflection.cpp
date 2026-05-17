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

//------------------------------------------------------------------------------
TEST_GROUP( TestNodeReflection, FBuildTest )
{
public:
};

// Helper classes
//------------------------------------------------------------------------------

// BaseNode
//  - De-duplicate common elements
//------------------------------------------------------------------------------
class BaseNode : public Node
{
    REFLECT_DECLARE( BaseNode )
public:
    BaseNode()
        : Node( Node::PROXY_NODE )
    {
        SetName( AStackString( "placeholder" ) );
    }
    virtual bool Initialize( NodeGraph & /*nodeGraph*/, const BFFToken * /*funcStartIter*/, const Function * /*function*/ ) override
    {
        ASSERT( false );
        return false;
    }
    virtual bool IsAFile() const override { return true; }

    AString m_String;
    Array<AString> m_ArrayOfStrings;
};
REFLECT_BEGIN( BaseNode, Node )
REFLECT_END( BaseNode )

// FunctionWrapper
//  - Allow test to directly invoke PopulateProperties outside of normal BFF parsing
//------------------------------------------------------------------------------
class FunctionWrapper : public Function
{
public:
    FunctionWrapper()
        : Function( "dummyfunction" )
    {
    }

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
        : m_Node( node )
    {
    }
    ~TestHelper()
    {
        delete m_Node;
        delete m_Function;
    }

    NodeGraph m_NodeGraph;
    FBuild m_FBuild;
    BFFToken * m_Token = nullptr;
    BaseNode * m_Node;
    FunctionWrapper * m_Function = new FunctionWrapper(); // Freed by FBuild destructor
    BFFStackFrame m_Frame;

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
    REFLECT_NODE_BEGIN( name, BaseNode )

#define TEST_NODE_END( name ) \
    REFLECT_END( name )

//==============================================================================
// String - Optional
//==============================================================================
TEST_NODE_BEGIN( Node_String_Optional )
    REFLECT( m_String )
TEST_NODE_END( Node_String_Optional )

//------------------------------------------------------------------------------
TEST_CASE( TestNodeReflection, String_Optional_NotSet )
{
    TestHelper helper( new Node_String_Optional );

    // String not set

    // Ok to not set since string is optional
    TEST_ASSERT( helper.Populate() );
    TEST_ASSERT( helper.m_Node->m_String.IsEmpty() );
}

//------------------------------------------------------------------------------
TEST_CASE( TestNodeReflection, String_Optional_Set )
{
    TestHelper helper( new Node_String_Optional );

    // Set string
    helper.m_Frame.SetVarString( AStackString( ".String" ), BFFToken::GetBuiltInToken(), AStackString( "value" ), nullptr );

    // Check string was set
    TEST_ASSERT( helper.Populate() );
    TEST_ASSERT( helper.m_Node->m_String == "value" );
}

//------------------------------------------------------------------------------
TEST_CASE( TestNodeReflection, String_Optional_Empty )
{
    TestHelper helper( new Node_String_Optional );

    // Set string to empty
    helper.m_Frame.SetVarString( AStackString( ".String" ), BFFToken::GetBuiltInToken(), AString::GetEmpty(), nullptr );

    // Check ok (setting empty on optional property is ok)
    TEST_ASSERT( helper.Populate() );
    TEST_ASSERT( helper.m_Node->m_String.IsEmpty() );
}

//==============================================================================
// String - Required
//==============================================================================
TEST_NODE_BEGIN( Node_String_Required )
    REFLECT( m_String, MetaRequired() )
TEST_NODE_END( Node_String_Required )

//------------------------------------------------------------------------------
TEST_CASE( TestNodeReflection, String_Required_NotSet )
{
    TestHelper helper( new Node_String_Required );

    // String not set

    // Check for failure and specific error
    TEST_ASSERT( helper.Populate() == false );
    TEST_ASSERT( GetRecordedOutput().Find( "Missing required property" ) );
}

//------------------------------------------------------------------------------
TEST_CASE( TestNodeReflection, String_Required_Set )
{
    TestHelper helper( new Node_String_Required );

    // Set string
    helper.m_Frame.SetVarString( AStackString( ".String" ), BFFToken::GetBuiltInToken(), AStackString( "value" ), nullptr );

    // Check string was set
    TEST_ASSERT( helper.Populate() );
    TEST_ASSERT( helper.m_Node->m_String == "value" );
}

//------------------------------------------------------------------------------
TEST_CASE( TestNodeReflection, String_Required_Empty )
{
    TestHelper helper( new Node_String_Required );

    // Set string to empty
    helper.m_Frame.SetVarString( AStackString( ".String" ), BFFToken::GetBuiltInToken(), AString::GetEmpty(), nullptr );

    // Check for failure (required strings can't be empty)
    TEST_ASSERT( helper.Populate() == false );
    TEST_ASSERT( helper.m_Node->m_String.IsEmpty() );
    TEST_ASSERT( GetRecordedOutput().Find( "Empty string not allowed" ) );
}

//==============================================================================
// ArrayOfStrings - Optional
//==============================================================================
TEST_NODE_BEGIN( Node_ArrayOfStrings_Optional )
    REFLECT( m_ArrayOfStrings )
TEST_NODE_END( Node_ArrayOfStrings_Optional )

//------------------------------------------------------------------------------
TEST_CASE( TestNodeReflection, ArrayOfStrings_Optional_NotSet )
{
    TestHelper helper( new Node_ArrayOfStrings_Optional );

    // Array not set

    // Ok to not set since array is optional
    TEST_ASSERT( helper.Populate() );
    TEST_ASSERT( helper.m_Node->m_String.IsEmpty() );
}

//------------------------------------------------------------------------------
TEST_CASE( TestNodeReflection, ArrayOfStrings_Optional_Set )
{
    TestHelper helper( new Node_ArrayOfStrings_Optional );

    // Set string array
    Array<AString> strings;
    strings.EmplaceBack( "value" );
    helper.m_Frame.SetVarArrayOfStrings( AStackString( ".ArrayOfStrings" ), BFFToken::GetBuiltInToken(), strings, nullptr );

    // Check array was set
    TEST_ASSERT( helper.Populate() );
    TEST_ASSERT( helper.m_Node->m_ArrayOfStrings.GetSize() == 1 );
    TEST_ASSERT( helper.m_Node->m_ArrayOfStrings[ 0 ] == "value" );
}

//------------------------------------------------------------------------------
TEST_CASE( TestNodeReflection, ArrayOfStrings_Optional_Empty )
{
    TestHelper helper( new Node_ArrayOfStrings_Optional );

    // Set array to empty
    Array<AString> empty;
    helper.m_Frame.SetVarArrayOfStrings( AStackString( ".ArrayOfStrings" ), BFFToken::GetBuiltInToken(), empty, nullptr );

    // Check ok (setting empty on optional property is ok)
    TEST_ASSERT( helper.Populate() );
    TEST_ASSERT( helper.m_Node->m_ArrayOfStrings.IsEmpty() );
}

//------------------------------------------------------------------------------
TEST_CASE( TestNodeReflection, ArrayOfStrings_Optional_EmptyElement )
{
    TestHelper helper( new Node_ArrayOfStrings_Optional );

    // Non-empty array with empty element
    Array<AString> strings;
    strings.EmplaceBack( "value" );
    strings.EmplaceBack();
    helper.m_Frame.SetVarArrayOfStrings( AStackString( ".ArrayOfStrings" ), BFFToken::GetBuiltInToken(), strings, nullptr );

    // Check failure (empty strings in arrays are not allowed)
    TEST_ASSERT( helper.Populate() == false );
    TEST_ASSERT( GetRecordedOutput().Find( "Empty string not allowed" ) );
}

//==============================================================================
// ArrayOfStrings - Required
//==============================================================================
TEST_NODE_BEGIN( Node_ArrayOfStrings_Required )
    REFLECT( m_ArrayOfStrings, MetaRequired() )
TEST_NODE_END( Node_ArrayOfStrings_Required )

//------------------------------------------------------------------------------
TEST_CASE( TestNodeReflection, ArrayOfStrings_Required_NotSet )
{
    TestHelper helper( new Node_ArrayOfStrings_Required );

    // Array not set

    // Check for failure with appropriate error
    TEST_ASSERT( helper.Populate() == false );
    TEST_ASSERT( GetRecordedOutput().Find( "Missing required property" ) );
}

//------------------------------------------------------------------------------
TEST_CASE( TestNodeReflection, ArrayOfStrings_Required_Set )
{
    TestHelper helper( new Node_ArrayOfStrings_Required );

    // Set string array
    Array<AString> strings;
    strings.EmplaceBack( "value" );
    helper.m_Frame.SetVarArrayOfStrings( AStackString( ".ArrayOfStrings" ), BFFToken::GetBuiltInToken(), strings, nullptr );

    // Check array was set
    TEST_ASSERT( helper.Populate() );
    TEST_ASSERT( helper.m_Node->m_ArrayOfStrings.GetSize() == 1 );
    TEST_ASSERT( helper.m_Node->m_ArrayOfStrings[ 0 ] == "value" );
}

//------------------------------------------------------------------------------
TEST_CASE( TestNodeReflection, ArrayOfStrings_Required_Empty )
{
    TestHelper helper( new Node_ArrayOfStrings_Required );

    // Set array to empty
    Array<AString> empty;
    helper.m_Frame.SetVarArrayOfStrings( AStackString( ".ArrayOfStrings" ), BFFToken::GetBuiltInToken(), empty, nullptr );

    // Check for failure (can't set empty array if property is required)
    TEST_ASSERT( helper.Populate() == false );
    TEST_ASSERT( GetRecordedOutput().Find( "Empty string not allowed" ) ); // TODO:B Array specific error?
}

//------------------------------------------------------------------------------
TEST_CASE( TestNodeReflection, ArrayOfStrings_Required_EmptyElement )
{
    TestHelper helper( new Node_ArrayOfStrings_Required );

    // Non-empty array with empty element
    Array<AString> strings;
    strings.EmplaceBack( "value" );
    strings.EmplaceBack();
    helper.m_Frame.SetVarArrayOfStrings( AStackString( ".ArrayOfStrings" ), BFFToken::GetBuiltInToken(), strings, nullptr );

    // Check failure (empty strings in arrays are not allowed)
    TEST_ASSERT( helper.Populate() == false );
    TEST_ASSERT( GetRecordedOutput().Find( "Empty string not allowed" ) );
}

//==============================================================================
// MetaFile - String - Optional
//==============================================================================
TEST_NODE_BEGIN( Node_MetaFile_String_Optional )
    REFLECT( m_String, MetaFile() )
TEST_NODE_END( Node_MetaFile_String_Optional )

//------------------------------------------------------------------------------
TEST_CASE( TestNodeReflection, MetaFile_String_Optional_NotSet )
{
    TestHelper helper( new Node_MetaFile_String_Optional );

    // String not set

    // Ok for optional File to be missing
    TEST_ASSERT( helper.Populate() == true );
}

//------------------------------------------------------------------------------
TEST_CASE( TestNodeReflection, MetaFile_String_Optional_Set )
{
    TestHelper helper( new Node_MetaFile_String_Optional );

    // Push a string
    helper.m_Frame.SetVarString( AStackString( ".String" ), BFFToken::GetBuiltInToken(), AStackString( "value" ), nullptr );

    // Check the property was set and converted to a full path
    TEST_ASSERT( helper.Populate() == true );
    TEST_ASSERT( helper.m_Node->m_String.EndsWith( "value" ) );
    helper.CheckFile( helper.m_Node->m_String );
}

//------------------------------------------------------------------------------
TEST_CASE( TestNodeReflection, MetaFile_String_Optional_Empty )
{
    TestHelper helper( new Node_MetaFile_String_Optional );

    // Push an empty string
    helper.m_Frame.SetVarString( AStackString( ".String" ), BFFToken::GetBuiltInToken(), AString::GetEmpty(), nullptr );

    // Ok for property to be empty because it is optional
    TEST_ASSERT( helper.Populate() == true );
}

//==============================================================================
// MetaFile - String - Required
//==============================================================================
TEST_NODE_BEGIN( Node_MetaFile_String_Required )
    REFLECT( m_String, MetaFile() + MetaRequired() )
TEST_NODE_END( Node_MetaFile_String_Required )

//------------------------------------------------------------------------------
TEST_CASE( TestNodeReflection, MetaFile_String_Required_NotSet )
{
    TestHelper helper( new Node_MetaFile_String_Required );

    // String not set

    // Check that populating properties fails and that appropriate error is reported
    TEST_ASSERT( helper.Populate() == false );
    TEST_ASSERT( GetRecordedOutput().Find( "Missing required property" ) );
}

//------------------------------------------------------------------------------
TEST_CASE( TestNodeReflection, MetaFile_String_Required_Set )
{
    TestHelper helper( new Node_MetaFile_String_Required );

    // Push a string
    helper.m_Frame.SetVarString( AStackString( ".String" ), BFFToken::GetBuiltInToken(), AStackString( "value" ), nullptr );

    // Check the property was set and converted to a full path
    TEST_ASSERT( helper.Populate() == true );
    TEST_ASSERT( helper.m_Node->m_String.EndsWith( "value" ) );
    helper.CheckFile( helper.m_Node->m_String );
}

//------------------------------------------------------------------------------
TEST_CASE( TestNodeReflection, MetaFile_String_Required_Empty )
{
    TestHelper helper( new Node_MetaFile_String_Required );

    // Push an empty string
    helper.m_Frame.SetVarString( AStackString( ".String" ), BFFToken::GetBuiltInToken(), AString::GetEmpty(), nullptr );

    // Check that populating properties fails and that appropriate error is reported
    TEST_ASSERT( helper.Populate() == false );
    TEST_ASSERT( GetRecordedOutput().Find( "Empty string not allowed" ) );
}

//==============================================================================
// MetaFile - ArrayOfStrings - Optional
//==============================================================================
TEST_NODE_BEGIN( Node_MetaFile_ArrayOfStrings_Optional )
    REFLECT( m_ArrayOfStrings, MetaFile() )
TEST_NODE_END( Node_MetaFile_ArrayOfStrings_Optional )

//------------------------------------------------------------------------------
TEST_CASE( TestNodeReflection, MetaFile_ArrayOfStrings_Optional_NotSet )
{
    TestHelper helper( new Node_MetaFile_ArrayOfStrings_Optional );

    // ArrayOfStrings not set

    // Ok for optional File(s) to be missing
    TEST_ASSERT( helper.Populate() == true );
}

//------------------------------------------------------------------------------
TEST_CASE( TestNodeReflection, MetaFile_ArrayOfStrings_Optional_Set )
{
    TestHelper helper( new Node_MetaFile_ArrayOfStrings_Optional );

    // Set string array
    Array<AString> strings;
    strings.EmplaceBack( "value" );
    helper.m_Frame.SetVarArrayOfStrings( AStackString( ".ArrayOfStrings" ), BFFToken::GetBuiltInToken(), strings, nullptr );

    // Check the property was set and converted to a full paths
    TEST_ASSERT( helper.Populate() == true );
    TEST_ASSERT( helper.m_Node->m_ArrayOfStrings.GetSize() == 1 );
    TEST_ASSERT( helper.m_Node->m_ArrayOfStrings[ 0 ].EndsWith( "value" ) );
    helper.CheckFile( helper.m_Node->m_ArrayOfStrings[ 0 ] );
}

//------------------------------------------------------------------------------
TEST_CASE( TestNodeReflection, MetaFile_ArrayOfStrings_Optional_Empty )
{
    TestHelper helper( new Node_MetaFile_ArrayOfStrings_Optional );

    // Set string array with an empty element in in
    Array<AString> empty;
    helper.m_Frame.SetVarArrayOfStrings( AStackString( ".ArrayOfStrings" ), BFFToken::GetBuiltInToken(), empty, nullptr );

    // Ok for property to be empty because it is optional
    TEST_ASSERT( helper.Populate() == true );
}

//------------------------------------------------------------------------------
TEST_CASE( TestNodeReflection, MetaFile_ArrayOfStrings_Optional_EmptyElement )
{
    TestHelper helper( new Node_MetaFile_ArrayOfStrings_Optional );

    // Non-empty array with empty element
    Array<AString> strings;
    strings.EmplaceBack( "value" );
    strings.EmplaceBack();
    helper.m_Frame.SetVarArrayOfStrings( AStackString( ".ArrayOfStrings" ), BFFToken::GetBuiltInToken(), strings, nullptr );

    // Check failure (empty strings in arrays are not allowed)
    TEST_ASSERT( helper.Populate() == false );
    TEST_ASSERT( GetRecordedOutput().Find( "Empty string not allowed" ) );
}

//==============================================================================
// MetaFile - ArrayOfStrings - Required
//==============================================================================
TEST_NODE_BEGIN( Node_MetaFile_ArrayOfStrings_Required )
    REFLECT( m_ArrayOfStrings, MetaFile() + MetaRequired() )
TEST_NODE_END( Node_MetaFile_ArrayOfStrings_Required )

//------------------------------------------------------------------------------
TEST_CASE( TestNodeReflection, MetaFile_ArrayOfStrings_Required_NotSet )
{
    TestHelper helper( new Node_MetaFile_ArrayOfStrings_Required );

    // String not set

    // Check that populating properties fails and that appropriate error is reported
    TEST_ASSERT( helper.Populate() == false );
    TEST_ASSERT( GetRecordedOutput().Find( "Missing required property" ) );
}

//------------------------------------------------------------------------------
TEST_CASE( TestNodeReflection, MetaFile_ArrayOfStrings_Required_Set )
{
    TestHelper helper( new Node_MetaFile_ArrayOfStrings_Required );

    // Set string array
    Array<AString> strings;
    strings.EmplaceBack( "value" );
    helper.m_Frame.SetVarArrayOfStrings( AStackString( ".ArrayOfStrings" ), BFFToken::GetBuiltInToken(), strings, nullptr );

    // Check the property was set and converted to a full path
    TEST_ASSERT( helper.Populate() == true );
    TEST_ASSERT( helper.m_Node->m_ArrayOfStrings.GetSize() == 1 );
    TEST_ASSERT( helper.m_Node->m_ArrayOfStrings[ 0 ].EndsWith( "value" ) );
    helper.CheckFile( helper.m_Node->m_ArrayOfStrings[ 0 ] );
}

//------------------------------------------------------------------------------
TEST_CASE( TestNodeReflection, MetaFile_ArrayOfStrings_Required_Empty )
{
    TestHelper helper( new Node_MetaFile_ArrayOfStrings_Required );

    // Set string array with an empty element in in
    Array<AString> empty;
    helper.m_Frame.SetVarArrayOfStrings( AStackString( ".ArrayOfStrings" ), BFFToken::GetBuiltInToken(), empty, nullptr );

    // Check that populating properties fails and that appropriate error is reported
    TEST_ASSERT( helper.Populate() == false );
    TEST_ASSERT( GetRecordedOutput().Find( "Empty string not allowed" ) );
}

//------------------------------------------------------------------------------
TEST_CASE( TestNodeReflection, MetaFile_ArrayOfStrings_Required_EmptyElement )
{
    TestHelper helper( new Node_MetaFile_ArrayOfStrings_Required );

    // Non-empty array with empty element
    Array<AString> strings;
    strings.EmplaceBack( "value" );
    strings.EmplaceBack();
    helper.m_Frame.SetVarArrayOfStrings( AStackString( ".ArrayOfStrings" ), BFFToken::GetBuiltInToken(), strings, nullptr );

    // Check failure (empty strings in arrays are not allowed)
    TEST_ASSERT( helper.Populate() == false );
    TEST_ASSERT( GetRecordedOutput().Find( "Empty string not allowed" ) );
}

//==============================================================================
// MetaPath - String - Optional
//==============================================================================
TEST_NODE_BEGIN( Node_MetaPath_String_Optional )
    REFLECT( m_String, MetaPath() )
TEST_NODE_END( Node_MetaPath_String_Optional )

//------------------------------------------------------------------------------
TEST_CASE( TestNodeReflection, MetaPath_String_Optional_NotSet )
{
    TestHelper helper( new Node_MetaPath_String_Optional );

    // String not set

    // Ok for optional File to be missing
    TEST_ASSERT( helper.Populate() == true );
}

//------------------------------------------------------------------------------
TEST_CASE( TestNodeReflection, MetaPath_String_Optional_Set )
{
    TestHelper helper( new Node_MetaPath_String_Optional );

    // Push a string
    helper.m_Frame.SetVarString( AStackString( ".String" ), BFFToken::GetBuiltInToken(), AStackString( "value" ), nullptr );

    // Check the property was set and converted to a full path
    TEST_ASSERT( helper.Populate() == true );
    TEST_ASSERT( helper.m_Node->m_String.Find( "value" ) );
    helper.CheckPath( helper.m_Node->m_String );
}

//------------------------------------------------------------------------------
TEST_CASE( TestNodeReflection, MetaPath_String_Optional_Empty )
{
    TestHelper helper( new Node_MetaPath_String_Optional );

    // Push an empty string
    helper.m_Frame.SetVarString( AStackString( ".String" ), BFFToken::GetBuiltInToken(), AString::GetEmpty(), nullptr );

    // Ok for property to be empty because it is optional
    TEST_ASSERT( helper.Populate() == true );
}

//==============================================================================
// MetaPath - String - Required
//==============================================================================
TEST_NODE_BEGIN( Node_MetaPath_String_Required )
    REFLECT( m_String, MetaPath() + MetaRequired() )
TEST_NODE_END( Node_MetaPath_String_Required )

//------------------------------------------------------------------------------
TEST_CASE( TestNodeReflection, MetaPath_String_Required_NotSet )
{
    TestHelper helper( new Node_MetaPath_String_Required );

    // String not set

    // Check that populating properties fails and that appropriate error is reported
    TEST_ASSERT( helper.Populate() == false );
    TEST_ASSERT( GetRecordedOutput().Find( "Missing required property" ) );
}

//------------------------------------------------------------------------------
TEST_CASE( TestNodeReflection, MetaPath_String_Required_Set )
{
    TestHelper helper( new Node_MetaPath_String_Required );

    // Push a string
    helper.m_Frame.SetVarString( AStackString( ".String" ), BFFToken::GetBuiltInToken(), AStackString( "value" ), nullptr );

    // Check the property was set and converted to a full path
    TEST_ASSERT( helper.Populate() == true );
    TEST_ASSERT( helper.m_Node->m_String.Find( "value" ) );
    helper.CheckPath( helper.m_Node->m_String );
}

//------------------------------------------------------------------------------
TEST_CASE( TestNodeReflection, MetaPath_String_Required_Empty )
{
    TestHelper helper( new Node_MetaPath_String_Required );

    // Push an empty string
    helper.m_Frame.SetVarString( AStackString( ".String" ), BFFToken::GetBuiltInToken(), AString::GetEmpty(), nullptr );

    // Check that populating properties fails and that appropriate error is reported
    TEST_ASSERT( helper.Populate() == false );
    TEST_ASSERT( GetRecordedOutput().Find( "Empty string not allowed" ) );
}

//==============================================================================
// MetaPath - ArrayOfStrings - Optional
//==============================================================================
TEST_NODE_BEGIN( Node_MetaPath_ArrayOfStrings_Optional )
    REFLECT( m_ArrayOfStrings, MetaPath() )
TEST_NODE_END( Node_MetaPath_ArrayOfStrings_Optional )

//------------------------------------------------------------------------------
TEST_CASE( TestNodeReflection, MetaPath_ArrayOfStrings_Optional_NotSet )
{
    TestHelper helper( new Node_MetaPath_ArrayOfStrings_Optional );

    // ArrayOfStrings not set

    // Ok for optional File(s) to be missing
    TEST_ASSERT( helper.Populate() == true );
}

//------------------------------------------------------------------------------
TEST_CASE( TestNodeReflection, MetaPath_ArrayOfStrings_Optional_Set )
{
    TestHelper helper( new Node_MetaPath_ArrayOfStrings_Optional );

    // Set string array
    Array<AString> strings;
    strings.EmplaceBack( "value" );
    helper.m_Frame.SetVarArrayOfStrings( AStackString( ".ArrayOfStrings" ), BFFToken::GetBuiltInToken(), strings, nullptr );

    // Check the property was set and converted to a full paths
    TEST_ASSERT( helper.Populate() == true );
    TEST_ASSERT( helper.m_Node->m_ArrayOfStrings.GetSize() == 1 );
    TEST_ASSERT( helper.m_Node->m_ArrayOfStrings[ 0 ].Find( "value" ) );
    helper.CheckPath( helper.m_Node->m_ArrayOfStrings[ 0 ] );
}

//------------------------------------------------------------------------------
TEST_CASE( TestNodeReflection, MetaPath_ArrayOfStrings_Optional_Empty )
{
    TestHelper helper( new Node_MetaPath_ArrayOfStrings_Optional );

    // Set string array with an empty element in in
    Array<AString> empty;
    helper.m_Frame.SetVarArrayOfStrings( AStackString( ".ArrayOfStrings" ), BFFToken::GetBuiltInToken(), empty, nullptr );

    // Ok for property to be empty because it is optional
    TEST_ASSERT( helper.Populate() == true );
}

//------------------------------------------------------------------------------
TEST_CASE( TestNodeReflection, MetaPath_ArrayOfStrings_Optional_EmptyElement )
{
    TestHelper helper( new Node_MetaPath_ArrayOfStrings_Optional );

    // Non-empty array with empty element
    Array<AString> strings;
    strings.EmplaceBack( "value" );
    strings.EmplaceBack();
    helper.m_Frame.SetVarArrayOfStrings( AStackString( ".ArrayOfStrings" ), BFFToken::GetBuiltInToken(), strings, nullptr );

    // Check failure (empty strings in arrays are not allowed)
    TEST_ASSERT( helper.Populate() == false );
    TEST_ASSERT( GetRecordedOutput().Find( "Empty string not allowed" ) );
}

//==============================================================================
// MetaPath - ArrayOfStrings - Required
//==============================================================================
TEST_NODE_BEGIN( Node_MetaPath_ArrayOfStrings_Required )
    REFLECT( m_ArrayOfStrings, MetaPath() + MetaRequired() )
TEST_NODE_END( Node_MetaPath_ArrayOfStrings_Required )

//------------------------------------------------------------------------------
TEST_CASE( TestNodeReflection, MetaPath_ArrayOfStrings_Required_NotSet )
{
    TestHelper helper( new Node_MetaPath_ArrayOfStrings_Required );

    // String not set

    // Check that populating properties fails and that appropriate error is reported
    TEST_ASSERT( helper.Populate() == false );
    TEST_ASSERT( GetRecordedOutput().Find( "Missing required property" ) );
}

//------------------------------------------------------------------------------
TEST_CASE( TestNodeReflection, MetaPath_ArrayOfStrings_Required_Set )
{
    TestHelper helper( new Node_MetaPath_ArrayOfStrings_Required );

    // Set string array
    Array<AString> strings;
    strings.EmplaceBack( "value" );
    helper.m_Frame.SetVarArrayOfStrings( AStackString( ".ArrayOfStrings" ), BFFToken::GetBuiltInToken(), strings, nullptr );

    // Check the property was set and converted to a full path
    TEST_ASSERT( helper.Populate() == true );
    TEST_ASSERT( helper.m_Node->m_ArrayOfStrings.GetSize() == 1 );
    TEST_ASSERT( helper.m_Node->m_ArrayOfStrings[ 0 ].Find( "value" ) );
    helper.CheckPath( helper.m_Node->m_ArrayOfStrings[ 0 ] );
}

//------------------------------------------------------------------------------
TEST_CASE( TestNodeReflection, MetaPath_ArrayOfStrings_Required_Empty )
{
    TestHelper helper( new Node_MetaPath_ArrayOfStrings_Required );

    // Set string array with an empty element in in
    Array<AString> empty;
    helper.m_Frame.SetVarArrayOfStrings( AStackString( ".ArrayOfStrings" ), BFFToken::GetBuiltInToken(), empty, nullptr );

    // Check that populating properties fails and that appropriate error is reported
    TEST_ASSERT( helper.Populate() == false );
    TEST_ASSERT( GetRecordedOutput().Find( "Empty string not allowed" ) );
}

//------------------------------------------------------------------------------
TEST_CASE( TestNodeReflection, MetaPath_ArrayOfStrings_Required_EmptyElement )
{
    TestHelper helper( new Node_MetaPath_ArrayOfStrings_Required );

    // Non-empty array with empty element
    Array<AString> strings;
    strings.EmplaceBack( "value" );
    strings.EmplaceBack();
    helper.m_Frame.SetVarArrayOfStrings( AStackString( ".ArrayOfStrings" ), BFFToken::GetBuiltInToken(), strings, nullptr );

    // Check failure (empty strings in arrays are not allowed)
    TEST_ASSERT( helper.Populate() == false );
    TEST_ASSERT( GetRecordedOutput().Find( "Empty string not allowed" ) );
}

//------------------------------------------------------------------------------
