// TestArray.cpp
//------------------------------------------------------------------------------

// Includes
//------------------------------------------------------------------------------
#include "TestFramework/UnitTest.h"

#include "Core/Containers/Array.h"
#include "Core/Strings/AString.h"

// TestArray
//------------------------------------------------------------------------------
class TestArray : public UnitTest
{
private:
    DECLARE_TESTS

    void Construct_Empty() const;
    void Construct_OtherArray() const;
    void Construct_Range() const;
    void Construct_WithCapacity() const;

    void Destruct() const;

    void BeginAndEnd() const;

    void IndexOperator() const;
    void Top() const;

    void RangeBasedForLoop() const;

    void SetCapacity() const;
    void SetSize() const;
    void Clear() const;
    void Swap() const;

    void Sort() const;
    void SortDeref() const;

    void Find() const;
    void FindDeref() const;
    void FindAndErase() const;
    void FindDerefAndErase() const;

    void Append_Item() const;
    void Append_OtherArray() const;
    void Append_Range() const;

    void Pop() const;
    void PopFront() const;
    void Erase() const;
    void EraseIndex() const;
    void EmplaceBack() const;

    void AssignmentOperator_OtherArray() const;

    void IsAtCapacity() const;
    void GetCapacity() const;
    void GetSize() const;
    void IsEmpty() const;

    void Alignment() const;

    void MoveConstructor() const;
    void MoveAssignment() const;
    void MoveWhenGrowing() const;
    void MoveAppend() const;
    void MoveSetCapacity() const;
    void MovePopFront() const;
    void MoveErase() const;

    void StackArrayOverflowToHeap() const;

    // Helper functions
    template <typename T>
    void CheckConsistency( const Array<T> & array ) const;
    template <class SRC, class DST, uint32_t EXPECTED_ALLOCS, class SRC_CAST = SRC>
    void MoveConstructorHelper() const;
    template <class SRC, class DST, uint32_t EXPECTED_ALLOCS, class ELEM, class SRC_CAST = SRC>
    void MoveAssignmentHelper( const ELEM & value ) const;
};

// Register Tests
//------------------------------------------------------------------------------
REGISTER_TESTS_BEGIN( TestArray )
    REGISTER_TEST( Construct_Empty )
    REGISTER_TEST( Construct_OtherArray )
    REGISTER_TEST( Construct_Range )
    REGISTER_TEST( Construct_WithCapacity )

    REGISTER_TEST( Destruct )

    REGISTER_TEST( BeginAndEnd )

    REGISTER_TEST( IndexOperator )
    REGISTER_TEST( Top )

    REGISTER_TEST( RangeBasedForLoop )

    REGISTER_TEST( SetCapacity )
    REGISTER_TEST( SetSize )
    REGISTER_TEST( Clear )
    REGISTER_TEST( Swap )

    REGISTER_TEST( Sort )
    REGISTER_TEST( SortDeref )

    REGISTER_TEST( Find )
    REGISTER_TEST( FindDeref )
    REGISTER_TEST( FindAndErase )
    REGISTER_TEST( FindDerefAndErase )

    REGISTER_TEST( Append_Item )
    REGISTER_TEST( Append_OtherArray )
    REGISTER_TEST( Append_Range )

    REGISTER_TEST( Pop )
    REGISTER_TEST( PopFront )
    REGISTER_TEST( Erase )
    REGISTER_TEST( EraseIndex )
    REGISTER_TEST( EmplaceBack )

    REGISTER_TEST( AssignmentOperator_OtherArray )

    REGISTER_TEST( IsAtCapacity )
    REGISTER_TEST( GetCapacity )
    REGISTER_TEST( GetSize )
    REGISTER_TEST( IsEmpty )

    REGISTER_TEST( Alignment )

    REGISTER_TEST( MoveConstructor )
    REGISTER_TEST( MoveAssignment )
    REGISTER_TEST( MoveWhenGrowing )
    REGISTER_TEST( MoveAppend )
    REGISTER_TEST( MoveSetCapacity )
    REGISTER_TEST( MovePopFront )
    REGISTER_TEST( MoveErase )

    REGISTER_TEST( StackArrayOverflowToHeap )
REGISTER_TESTS_END

// Construct_Empty
//------------------------------------------------------------------------------
void TestArray::Construct_Empty() const
{
    {
        Array<uint32_t> array;
        CheckConsistency( array );
    }
    {
        Array<AString> array;
        CheckConsistency( array );
    }
}

// Construct_OtherArray
//------------------------------------------------------------------------------
void TestArray::Construct_OtherArray() const
{
    {
        Array<uint32_t> array1;
        array1.Append( 1 );
        array1.Append( 2 );
        array1.Append( 3 );

        Array<uint32_t> array2( array1 );
        CheckConsistency( array2 );
        TEST_ASSERT( array2.GetSize() == array1.GetSize() );
        TEST_ASSERT( array2.GetSize() == 3 );
        TEST_ASSERT( array2[ 0 ] == array1[ 0 ] );
        TEST_ASSERT( array2[ 1 ] == array1[ 1 ] );
        TEST_ASSERT( array2[ 2 ] == array1[ 2 ] );
    }
    {
        Array<AString> array1;
        array1.Append( AString( "string1" ) );
        array1.Append( AString( "string2" ) );
        array1.Append( AString( "string3" ) );

        Array<AString> array2( array1 );
        CheckConsistency( array2 );
        TEST_ASSERT( array2.GetSize() == array1.GetSize() );
        TEST_ASSERT( array2.GetSize() == 3 );
        TEST_ASSERT( array2[ 0 ] == array1[ 0 ] );
        TEST_ASSERT( array2[ 1 ] == array1[ 1 ] );
        TEST_ASSERT( array2[ 2 ] == array1[ 2 ] );
    }
}

// Construct_Range
//------------------------------------------------------------------------------
void TestArray::Construct_Range() const
{
    {
        uint32_t u32s[ 4 ] = { 1, 2, 3, 4 };

        Array<uint32_t> array( u32s, u32s + 4 );
        CheckConsistency( array );
        TEST_ASSERT( array[ 0 ] == 1 );
        TEST_ASSERT( array[ 1 ] == 2 );
        TEST_ASSERT( array[ 2 ] == 3 );
        TEST_ASSERT( array[ 3 ] == 4 );
    }

    {
        AString strings[ 4 ];
        strings[ 0 ] = "string1";
        strings[ 1 ] = "string2";
        strings[ 2 ] = "string3";
        strings[ 3 ] = "string4";

        Array<AString> array( strings, strings + 4 );
        CheckConsistency( array );
        TEST_ASSERT( array[ 0 ] == "string1" );
        TEST_ASSERT( array[ 1 ] == "string2" );
        TEST_ASSERT( array[ 2 ] == "string3" );
        TEST_ASSERT( array[ 3 ] == "string4" );
    }
}

// Construct_WithCapacity
//------------------------------------------------------------------------------
void TestArray::Construct_WithCapacity() const
{
    {
        Array<uint32_t> array( 3, true ); // resizable
        CheckConsistency( array );
        TEST_ASSERT( array.GetSize() == 0 );
        TEST_ASSERT( array.GetCapacity() == 3 );
        TEST_ASSERT( array.IsEmpty() );
        TEST_ASSERT( array.IsAtCapacity() == false );
    }

    {
        Array<uint32_t> array( 3, false ); // fixed capacity
        CheckConsistency( array );
        TEST_ASSERT( array.GetSize() == 0 );
        TEST_ASSERT( array.GetCapacity() == 3 );
        TEST_ASSERT( array.IsEmpty() );
        TEST_ASSERT( array.IsAtCapacity() == false );
    }
}

// Destruct
//------------------------------------------------------------------------------
void TestArray::Destruct() const
{
    // POD
    {
        // Destruct already empty
        Array<uint32_t> array;
        array.Destruct();
        CheckConsistency( array );
        TEST_ASSERT( array.GetSize() == 0 );
        TEST_ASSERT( array.GetCapacity() == 0 );
    }
    {
        // Destruct non-empty
        Array<uint32_t> array;
        array.Append( 1 );
        array.Destruct();
        CheckConsistency( array );
        TEST_ASSERT( array.GetSize() == 0 );
        TEST_ASSERT( array.GetCapacity() == 0 );
    }

    // Complex Type
    {
        // Destruct already empty
        Array<AString> array;
        array.Destruct();
        CheckConsistency( array );
        TEST_ASSERT( array.GetSize() == 0 );
        TEST_ASSERT( array.GetCapacity() == 0 );
    }
    {
        // Destruct non-empty
        Array<AString> array;
        array.Append( AString( "string1" ) );
        array.Destruct();
        CheckConsistency( array );
        TEST_ASSERT( array.GetSize() == 0 );
        TEST_ASSERT( array.GetCapacity() == 0 );
    }
}

//  BeginAndEnd
//------------------------------------------------------------------------------
void TestArray::BeginAndEnd() const
{
    // POD
    {
        // Empty
        Array<uint32_t> array;
        TEST_ASSERT( array.Begin() == nullptr );
        TEST_ASSERT( array.End() == nullptr );
        TEST_ASSERT( array.Begin() == array.End() );
        TEST_ASSERT( array.Begin() == array.begin() );
        TEST_ASSERT( array.End() == array.end() );
    }
    {
        // Non-empty
        Array<uint32_t> array( 2 );
        TEST_ASSERT( array.Begin() != nullptr );
        TEST_ASSERT( array.End() != nullptr );
        TEST_ASSERT( array.Begin() == array.End() );
        TEST_ASSERT( array.Begin() == array.begin() );
        TEST_ASSERT( array.End() == array.end() );
        array.Append( 1 );
        CheckConsistency( array );
        TEST_ASSERT( array.Begin() != nullptr );
        TEST_ASSERT( array.End() != nullptr );
        TEST_ASSERT( array.Begin() < array.End() );
        TEST_ASSERT( array.begin() < array.end() );
    }

    // Complex type
    {
        // Empty
        Array<AString> array;
        TEST_ASSERT( array.Begin() == nullptr );
        TEST_ASSERT( array.End() == nullptr );
        TEST_ASSERT( array.Begin() == array.End() );
        TEST_ASSERT( array.Begin() == array.begin() );
        TEST_ASSERT( array.End() == array.end() );
    }
    {
        // Non-empty
        Array<AString> array( 2 );
        TEST_ASSERT( array.Begin() != nullptr );
        TEST_ASSERT( array.End() != nullptr );
        TEST_ASSERT( array.Begin() == array.End() );
        TEST_ASSERT( array.Begin() == array.begin() );
        TEST_ASSERT( array.End() == array.end() );
        array.Append( AString( "string1" ) );
        CheckConsistency( array );
        TEST_ASSERT( array.Begin() != nullptr );
        TEST_ASSERT( array.End() != nullptr );
        TEST_ASSERT( array.Begin() < array.End() );
        TEST_ASSERT( array.begin() < array.end() );
    }
}

// IndexOperator
//------------------------------------------------------------------------------
void TestArray::IndexOperator() const
{
    // POD
    {
        Array<uint32_t> array( 2 );
        array.Append( 1 );
        array.Append( 2 );
        TEST_ASSERT( array[ 0 ] == 1 );
        TEST_ASSERT( array[ 1 ] == 2 );
        CheckConsistency( array );
    }

    // Complex Type
    {
        Array<AString> array( 2 );
        array.Append( AString( "string1" ) );
        array.Append( AString( "string2" ) );
        TEST_ASSERT( array[ 0 ] == "string1" );
        TEST_ASSERT( array[ 1 ] == "string2" );
        CheckConsistency( array );
    }
}

// Top
//------------------------------------------------------------------------------
void TestArray::Top() const
{
    // POD
    {
        Array<uint32_t> array( 2 );
        array.Append( 1 );
        array.Append( 2 );
        TEST_ASSERT( array.Top() == 2 );
        CheckConsistency( array );
    }

    // Complex Type
    {
        Array<AString> array( 2 );
        array.Append( AString( "string1" ) );
        array.Append( AString( "string2" ) );
        TEST_ASSERT( array.Top() == "string2" );
        CheckConsistency( array );
    }
}

// RangeBasedForLoop
//------------------------------------------------------------------------------
void TestArray::RangeBasedForLoop() const
{
    // POD
    {
        Array<uint32_t> array( 2 );
        array.Append( 1 );
        array.Append( 2 );
        uint32_t total = 0;
        for ( uint32_t u : array ) // by value
        {
            total += u;
        }
        TEST_ASSERT( total == 3 );
        for ( const uint32_t& u : array ) // by reference
        {
            total += u;
        }
        TEST_ASSERT( total == 6 );
        CheckConsistency( array );
    }

    // Complex Type
    {
        Array<AString> array( 2 );
        array.Append( AString( "string1" ) );
        array.Append( AString( "string2" ) );
        AString result;
        for ( const AString & s : array ) // by reference
        {
            result += s;
        }
        TEST_ASSERT( result == "string1string2" );
        // NOTE: AString explicit constructor deliberately disables this
        //for ( AString s : array ) // by value
        //{
        //    result += s;
        //}
        CheckConsistency( array );
    }
}

// SetCapacity
//------------------------------------------------------------------------------
void TestArray::SetCapacity() const
{
    // POD
    {
        Array<uint32_t> array;
        array.SetCapacity( 5 );
        TEST_ASSERT( array.GetCapacity() == 5 );
        TEST_ASSERT( array.GetSize() == 0 );
        CheckConsistency( array );
    }
    // Complex Type
    {
        Array<AString> array;
        array.SetCapacity( 5 );
        TEST_ASSERT( array.GetCapacity() == 5 );
        TEST_ASSERT( array.GetSize() == 0 );
        CheckConsistency( array );
    }
}

// SetSize
//------------------------------------------------------------------------------
void TestArray::SetSize() const
{
    // POD
    {
        Array<uint32_t> array;
        array.SetSize( 5 );
        TEST_ASSERT( array.GetCapacity() == 5 );
        TEST_ASSERT( array.GetSize() == 5 );
        CheckConsistency( array );
    }
    // Complex Type
    {
        Array<AString> array;
        array.SetSize( 5 );
        TEST_ASSERT( array.GetCapacity() == 5 );
        TEST_ASSERT( array.GetSize() == 5 );
        TEST_ASSERT( array[ 0 ] == AString::GetEmpty() ); // Elements are default constructed
        TEST_ASSERT( array[ 4 ] == AString::GetEmpty() ); // Elements are default constructed
        CheckConsistency( array );
    }
}

// Clear
//------------------------------------------------------------------------------
void TestArray::Clear() const
{
    // POD
    {
        // Clear already empty
        Array<uint32_t> array;
        array.Clear();
        CheckConsistency( array );
        TEST_ASSERT( array.GetSize() == 0 );
        TEST_ASSERT( array.GetCapacity() == 0 );
    }
    {
        // Clear non-empty
        Array<uint32_t> array;
        array.Append( 1 );
        array.Clear();
        CheckConsistency( array );
        TEST_ASSERT( array.GetSize() == 0 );
        TEST_ASSERT( array.GetCapacity() > 0 ); // Capacity is not changed
    }

    // Complex Type
    {
        // Clear already empty
        Array<AString> array;
        array.Clear();
        CheckConsistency( array );
        TEST_ASSERT( array.GetSize() == 0 );
        TEST_ASSERT( array.GetCapacity() == 0 );
    }
    {
        // Destruct non-empty
        Array<AString> array;
        array.Append( AString( "string1" ) );
        array.Clear();
        CheckConsistency( array );
        TEST_ASSERT( array.GetSize() == 0 );
        TEST_ASSERT( array.GetCapacity() > 0 );  // Capacity is not changed
    }
}

// Swap
//------------------------------------------------------------------------------
void TestArray::Swap() const
{
    // POD
    {
        // Swap empty
        Array<uint32_t> array1;
        Array<uint32_t> array2;
        array1.Swap( array2 );
        CheckConsistency( array1 );
        CheckConsistency( array2 );
        TEST_ASSERT( array1.IsEmpty() );
        TEST_ASSERT( array2.IsEmpty() );
    }
    {
        // Swap non-empty
        Array<uint32_t> array1;
        Array<uint32_t> array2;
        array1.Append( 1 );
        array1.Swap( array2 );
        CheckConsistency( array1 );
        CheckConsistency( array2 );
        TEST_ASSERT( array1.IsEmpty() );
        TEST_ASSERT( array2.IsEmpty() == false );
    }

    // Complex type
    {
        // Swap empty
        Array<AString> array1;
        Array<AString> array2;
        array1.Swap( array2 );
        CheckConsistency( array1 );
        CheckConsistency( array2 );
        TEST_ASSERT( array1.IsEmpty() );
        TEST_ASSERT( array2.IsEmpty() );
    }
    {
        // Swap non-empty
        Array<AString> array1;
        Array<AString> array2;
        array1.Append( AString( "string1" ) );
        array1.Swap( array2 );
        CheckConsistency( array1 );
        CheckConsistency( array2 );
        TEST_ASSERT( array1.IsEmpty() );
        TEST_ASSERT( array2.IsEmpty() == false );
        TEST_ASSERT( array2[ 0 ] == "string1" );
    }
}

// Sort
//------------------------------------------------------------------------------
void TestArray::Sort() const
{
    // POD
    {
        // Sort empty
        Array<uint32_t> array;
        array.Sort();
        CheckConsistency( array );
    }
    {
        // Sort one item
        Array<uint32_t> array;
        array.Append( 1 );
        array.Sort();
        CheckConsistency( array );
        TEST_ASSERT( array[ 0 ] == 1 );
    }
    {
        // Sort multiple
        Array<uint32_t> array;
        array.Append( 100 );
        array.Append( 1 );
        array.Append( 10 );
        array.Sort();
        CheckConsistency( array );
        TEST_ASSERT( array[ 0 ] == 1 );
        TEST_ASSERT( array[ 1 ] == 10 );
        TEST_ASSERT( array[ 2 ] == 100 );
    }

    // Complex type
    {
        // Sort empty
        Array<AString> array;
        array.Sort();
        CheckConsistency( array );
    }
    {
        // Sort one item
        Array<AString> array;
        array.Append( AString( "string1" ) );
        array.Sort();
        CheckConsistency( array );
        TEST_ASSERT( array[ 0 ] == "string1" );
    }
    {
        // Sort multiple
        Array<AString> array;
        array.Append( AString( "string100" ) );
        array.Append( AString( "string1" ) );
        array.Append( AString( "string10" ) );
        array.Sort();
        CheckConsistency( array );
        TEST_ASSERT( array[ 0 ] == "string1" );
        TEST_ASSERT( array[ 1 ] == "string10" );
        TEST_ASSERT( array[ 2 ] == "string100" );
    }
}

// SortDeref
//------------------------------------------------------------------------------
void TestArray::SortDeref() const
{
    // POD
    {
        // Sort empty
        Array<uint32_t *> array;
        array.Sort();
        CheckConsistency( array );
    }
    {
        // Sort one item
        const uint32_t u32 = 1;
        Array<const uint32_t *> array;
        array.Append( &u32 );
        array.SortDeref();
        CheckConsistency( array );
        TEST_ASSERT( *array[ 0 ] == 1 );
    }
    {
        // Sort multiple
        const uint32_t u32_1 = 10;
        const uint32_t u32_2 = 1;
        const uint32_t u32_3 = 100;
        Array<const uint32_t *> array;
        array.Append( &u32_1 );
        array.Append( &u32_2 );
        array.Append( &u32_3 );
        array.SortDeref();
        CheckConsistency( array );
        TEST_ASSERT( *array[ 0 ] == 1 );
        TEST_ASSERT( *array[ 1 ] == 10 );
        TEST_ASSERT( *array[ 2 ] == 100 );
    }

    // Complex type
    {
        // Sort empty
        Array<AString *> array;
        array.Sort();
        CheckConsistency( array );
    }
    {
        // Sort one item
        const AString string1( "string1" );
        Array<const AString *> array;
        array.Append( &string1 );
        array.Sort();
        CheckConsistency( array );
        TEST_ASSERT( *array[ 0 ] == "string1" );
    }
    {
        // Sort multiple
        const AString string1( "string10" );
        const AString string2( "string1" );
        const AString string3( "string100" );
        Array<const AString *> array;
        array.Append( &string1 );
        array.Append( &string2 );
        array.Append( &string3 );
        array.SortDeref();
        CheckConsistency( array );
        TEST_ASSERT( *array[ 0 ] == "string1" );
        TEST_ASSERT( *array[ 1 ] == "string10" );
        TEST_ASSERT( *array[ 2 ] == "string100" );
    }
}

// Find
//------------------------------------------------------------------------------
void TestArray::Find() const
{
    // POD
    {
        // Empty
        Array<uint32_t> array;
        TEST_ASSERT( array.Find( (uint32_t)1 ) == nullptr );
        CheckConsistency( array );
    }
    {
        // Non-empty
        Array<uint32_t> array;
        array.Append( 1 );
        array.Append( 2 );
        array.Append( 3 );
        TEST_ASSERT( array.Find( (uint32_t)2 ) != nullptr );
        TEST_ASSERT( array.Find( (uint32_t)2 ) > array.Begin() );
        TEST_ASSERT( array.Find( (uint32_t)2 ) < array.End() );
        TEST_ASSERT( array.Find( (uint32_t)999 ) == nullptr );
        CheckConsistency( array );
    }

    // Complex type
    {
        // Empty
        Array<AString> array;
        TEST_ASSERT( array.Find( "string1" ) == nullptr );
        CheckConsistency( array );
    }
    {
        // Non-empty
        Array<AString> array;
        array.Append( AString( "string1" ) );
        array.Append( AString( "string2" ) );
        array.Append( AString( "string3" ) );
        TEST_ASSERT( array.Find( "string2" ) != nullptr );
        TEST_ASSERT( array.Find( "string2" ) > array.Begin() );
        TEST_ASSERT( array.Find( "string2" ) < array.End() );
        TEST_ASSERT( array.Find( "string999" ) == nullptr );
        CheckConsistency( array );
    }
}

// FindDeref
//------------------------------------------------------------------------------
void TestArray::FindDeref() const
{
    // POD
    {
        // Empty
        Array<uint32_t *> array;
        TEST_ASSERT( array.FindDeref( (uint32_t)1 ) == nullptr );
        CheckConsistency( array );
    }
    {
        // Non-empty
        const uint32_t u32_1 = 1;
        const uint32_t u32_2 = 2;
        const uint32_t u32_3 = 3;
        Array<const uint32_t *> array;
        array.Append( &u32_1 );
        array.Append( &u32_2 );
        array.Append( &u32_3 );
        TEST_ASSERT( array.FindDeref( (uint32_t)2 ) != nullptr );
        TEST_ASSERT( array.FindDeref( (uint32_t)2 ) > array.Begin() );
        TEST_ASSERT( array.FindDeref( (uint32_t)2 ) < array.End() );
        TEST_ASSERT( array.FindDeref( (uint32_t)999 ) == nullptr );
        CheckConsistency( array );
    }

    // Complex type
    {
        // Empty
        Array<AString *> array;
        TEST_ASSERT( array.FindDeref( "string1" ) == nullptr );
        CheckConsistency( array );
    }
    {
        // Non-empty
        const AString string1( "string1" );
        const AString string2( "string2" );
        const AString string3( "string3" );
        Array<const AString *> array;
        array.Append( &string1 );
        array.Append( &string2 );
        array.Append( &string3 );
        TEST_ASSERT( array.FindDeref( "string2" ) != nullptr );
        TEST_ASSERT( array.FindDeref( "string2" ) > array.Begin() );
        TEST_ASSERT( array.FindDeref( "string2" ) < array.End() );
        TEST_ASSERT( array.FindDeref( "string999" ) == nullptr );
        CheckConsistency( array );
    }
}

// FindAndErase
//------------------------------------------------------------------------------
void TestArray::FindAndErase() const
{
    // POD
    {
        // Empty
        Array<uint32_t> array;
        TEST_ASSERT( array.FindAndErase( (uint32_t)1 ) == false );
        CheckConsistency( array );
    }
    {
        // Non-empty
        Array<uint32_t> array;
        array.Append( 1 );
        array.Append( 2 );
        array.Append( 3 );
        TEST_ASSERT( array.FindAndErase( (uint32_t)2 ) == true );
        CheckConsistency( array );
        TEST_ASSERT( array.GetSize() == 2 );
        TEST_ASSERT( array.FindAndErase( (uint32_t)999 ) == false );
        CheckConsistency( array );
        TEST_ASSERT( array.GetSize() == 2 );
        TEST_ASSERT( array[ 0 ] == 1 );
        TEST_ASSERT( array[ 1 ] == 3 );
    }

    // Complex type
    {
        // Empty
        Array<AString> array;
        TEST_ASSERT( array.FindAndErase( "string1" ) == false );
        CheckConsistency( array );
    }
    {
        // Non-empty
        Array<AString> array;
        array.Append( AString( "string1" ) );
        array.Append( AString( "string2" ) );
        array.Append( AString( "string3" ) );
        TEST_ASSERT( array.FindAndErase( "string2" ) == true );
        CheckConsistency( array );
        TEST_ASSERT( array.GetSize() == 2 );
        TEST_ASSERT( array.FindAndErase( "string999" ) == false );
        CheckConsistency( array );
        TEST_ASSERT( array.GetSize() == 2 );
        TEST_ASSERT( array[ 0 ] == "string1" );
        TEST_ASSERT( array[ 1 ] == "string3" );
    }
}

// FindDerefAndErase
//------------------------------------------------------------------------------
void TestArray::FindDerefAndErase() const
{
    // POD
    {
        // Empty
        Array<uint32_t*> array;
        TEST_ASSERT( array.FindDerefAndErase( (uint32_t)1 ) == false );
        CheckConsistency( array );
    }
    {
        // Non-empty
        const uint32_t u32_1 = 1;
        const uint32_t u32_2 = 2;
        const uint32_t u32_3 = 3;
        Array<const uint32_t*> array;
        array.Append( &u32_1 );
        array.Append( &u32_2 );
        array.Append( &u32_3 );
        TEST_ASSERT( array.FindDerefAndErase( (uint32_t)2 ) == true );
        CheckConsistency( array );
        TEST_ASSERT( array.GetSize() == 2 );
        TEST_ASSERT( array.FindDerefAndErase( (uint32_t)999 ) == false );
        CheckConsistency( array );
        TEST_ASSERT( array.GetSize() == 2 );
        TEST_ASSERT( *array[ 0 ] == 1 );
        TEST_ASSERT( *array[ 1 ] == 3 );
    }

    // Complex type
    {
        // Empty
        Array<AString*> array;
        TEST_ASSERT( array.FindDerefAndErase( "string1" ) == false );
        CheckConsistency( array );
    }
    {
        // Non-empty
        const AString string1( "string1" );
        const AString string2( "string2" );
        const AString string3( "string3" );
        Array<const AString*> array;
        array.Append( &string1 );
        array.Append( &string2 );
        array.Append( &string3 );
        TEST_ASSERT( array.FindDerefAndErase( "string2" ) == true );
        CheckConsistency( array );
        TEST_ASSERT( array.GetSize() == 2 );
        TEST_ASSERT( array.FindDerefAndErase( "string999" ) == false );
        CheckConsistency( array );
        TEST_ASSERT( array.GetSize() == 2 );
        TEST_ASSERT( *array[ 0 ] == "string1" );
        TEST_ASSERT( *array[ 1 ] == "string3" );
    }
}

// Append_Item
//------------------------------------------------------------------------------
void TestArray::Append_Item() const
{
    // POD
    {
        // Append 1 item
        Array<uint32_t> array;
        array.Append( 1 );
        CheckConsistency( array );
        TEST_ASSERT( array[ 0 ] == 1 );
        TEST_ASSERT( array.IsEmpty() == false );
        TEST_ASSERT( array.GetSize() == 1 );
        TEST_ASSERT( array.GetSize() <= array.GetCapacity() );
    }
    {
        // Append multiple items
        Array<uint32_t> array;
        array.Append( 1 );
        array.Append( 2 );
        array.Append( 3 );
        CheckConsistency( array );
        TEST_ASSERT( array[ 0 ] == 1 );
        TEST_ASSERT( array[ 1 ] == 2 );
        TEST_ASSERT( array[ 2 ] == 3 );
        TEST_ASSERT( array.IsEmpty() == false );
        TEST_ASSERT( array.GetSize() == 3 );
        TEST_ASSERT( array.GetSize() <= array.GetCapacity() );
    }

    // Complex Type
    {
        // Append 1 item
        Array<AString> array;
        array.Append( AString( "string1" ) );
        CheckConsistency( array );
        TEST_ASSERT( array[ 0 ] == "string1" );
        TEST_ASSERT( array.IsEmpty() == false );
        TEST_ASSERT( array.GetSize() == 1 );
        TEST_ASSERT( array.GetSize() <= array.GetCapacity() );
    }
    {
        // Append multiple items
        Array<AString> array;
        array.Append( AString( "string1" ) );
        array.Append( AString( "string2" ) );
        array.Append( AString( "string3" ) );
        CheckConsistency( array );
        TEST_ASSERT( array[ 0 ] == "string1" );
        TEST_ASSERT( array[ 1 ] == "string2" );
        TEST_ASSERT( array[ 2 ] == "string3" );
        TEST_ASSERT( array.IsEmpty() == false );
        TEST_ASSERT( array.GetSize() == 3 );
        TEST_ASSERT( array.GetSize() <= array.GetCapacity() );
    }
}

// Append_OtherArray
//------------------------------------------------------------------------------
void TestArray::Append_OtherArray() const
{
    // POD
    {
        // Empty
        Array<uint32_t> array1;
        Array<uint32_t> array2;
        array2.Append( array1 );
        CheckConsistency( array1 );
        CheckConsistency( array2 );
        TEST_ASSERT( array1.IsEmpty() );
        TEST_ASSERT( array2.IsEmpty() );
    }
    {
        // Non-empty
        Array<uint32_t> array1;
        array1.Append( 1 );
        Array<uint32_t> array2;
        array2.Append( array1 );
        CheckConsistency( array1 );
        CheckConsistency( array2 );
        TEST_ASSERT( array1.IsEmpty() == false );
        TEST_ASSERT( array2.IsEmpty() == false );
        TEST_ASSERT( array1.GetSize() == 1 );
        TEST_ASSERT( array2.GetSize() == 1 );
        TEST_ASSERT( array1[ 0 ] == 1 );
        TEST_ASSERT( array1[ 0 ] == array2[ 0 ] );
    }

    // Complex Type
    {
        // Empty
        Array<AString> array1;
        Array<AString> array2;
        array2.Append( array1 );
        CheckConsistency( array1 );
        CheckConsistency( array2 );
        TEST_ASSERT( array1.IsEmpty() );
        TEST_ASSERT( array2.IsEmpty() );
    }
    {
        // Non-empty
        Array<AString> array1;
        array1.Append( AString( "string1" ) );
        Array<AString> array2;
        array2.Append( array1 );
        CheckConsistency( array1 );
        CheckConsistency( array2 );
        TEST_ASSERT( array1.IsEmpty() == false );
        TEST_ASSERT( array2.IsEmpty() == false );
        TEST_ASSERT( array1.GetSize() == 1 );
        TEST_ASSERT( array2.GetSize() == 1 );
        TEST_ASSERT( array1[ 0 ] == "string1" );
        TEST_ASSERT( array1[ 0 ] == array2[ 0 ] );
    }
}

// Append_Range
//------------------------------------------------------------------------------
void TestArray::Append_Range() const
{
    // POD
    {
        // Empty range
        Array<uint32_t> array;
        array.Append( (const uint32_t *)nullptr, (const uint32_t *)nullptr );
        CheckConsistency( array );
        TEST_ASSERT( array.IsEmpty() );
        TEST_ASSERT( array.GetSize() == 0 );
        TEST_ASSERT( array.GetCapacity() == 0 );
    }
    {
        // Non-empty range
        const uint32_t u32s[ 3 ] = { 1, 2, 3 };
        Array<uint32_t> array;
        array.Append( u32s, u32s + 3 );
        CheckConsistency( array );
        TEST_ASSERT( array.IsEmpty() == false );
        TEST_ASSERT( array.GetSize() == 3 );
        TEST_ASSERT( array.GetCapacity() >= array.GetSize() );
        TEST_ASSERT( array[ 0 ] == u32s[ 0 ] );
        TEST_ASSERT( array[ 1 ] == u32s[ 1 ] );
        TEST_ASSERT( array[ 2 ] == u32s[ 2 ] );
    }

    // Complex Type
    {
        // Empty range
        Array<AString> array;
        array.Append( (const AString *)nullptr, (const AString *)nullptr );
        CheckConsistency( array );
        TEST_ASSERT( array.IsEmpty() );
        TEST_ASSERT( array.GetSize() == 0 );
        TEST_ASSERT( array.GetCapacity() == 0 );
    }
    {
        // Non-empty range
        AString strings[ 3 ];
        strings[ 0 ] = "string1";
        strings[ 1 ] = "string2";
        strings[ 2 ] = "string3";
        Array<AString> array;
        array.Append( strings, strings + 3 );
        CheckConsistency( array );
        TEST_ASSERT( array.IsEmpty() == false );
        TEST_ASSERT( array.GetSize() == 3 );
        TEST_ASSERT( array.GetCapacity() >= array.GetSize() );
        TEST_ASSERT( array[ 0 ] == strings[ 0 ] );
        TEST_ASSERT( array[ 1 ] == strings[ 1 ] );
        TEST_ASSERT( array[ 2 ] == strings[ 2 ] );
    }
}

// Pop
//------------------------------------------------------------------------------
void TestArray::Pop() const
{
    // POD
    {
        // Pop last item
        Array<uint32_t> array;
        array.Append( 1 );
        array.Pop();
        CheckConsistency( array );
        TEST_ASSERT( array.IsEmpty() );
        TEST_ASSERT( array.GetSize() == 0 );
        TEST_ASSERT( array.GetCapacity() > 0 ); // Capacity unchanged
    }
    {
        // Pop one of many items
        Array<uint32_t> array;
        array.Append( 1 );
        array.Append( 2 );
        array.Append( 3 );
        array.Pop();
        CheckConsistency( array );
        TEST_ASSERT( array.IsEmpty() == false );
        TEST_ASSERT( array.GetSize() == 2 );
        TEST_ASSERT( array.GetCapacity() >= 3 ); // Capacity unchanged
        TEST_ASSERT( array[ 0 ] == 1 );
        TEST_ASSERT( array[ 1 ] == 2 );
    }

    // Complex Type
    {
        Array<AString> array;
        array.Append( AString( "string1" ) );
        array.Pop();
        CheckConsistency( array );
        TEST_ASSERT( array.IsEmpty() );
        TEST_ASSERT( array.GetSize() == 0 );
        TEST_ASSERT( array.GetCapacity() > 0 ); // Capacity unchanged
    }
    {
        // Pop one of many items
        Array<AString> array;
        array.Append( AString( "string1" ) );
        array.Append( AString( "string2" ) );
        array.Append( AString( "string3" ) );
        array.Pop();
        CheckConsistency( array );
        TEST_ASSERT( array.IsEmpty() == false );
        TEST_ASSERT( array.GetSize() == 2 );
        TEST_ASSERT( array.GetCapacity() >= 3 ); // Capacity unchanged
        TEST_ASSERT( array[ 0 ] == "string1" );
        TEST_ASSERT( array[ 1 ] == "string2" );
    }
}

// PopFront
//------------------------------------------------------------------------------
void TestArray::PopFront() const
{
    // POD
    {
        // Pop last item
        Array<uint32_t> array;
        array.Append( 1 );
        array.PopFront();
        CheckConsistency( array );
        TEST_ASSERT( array.IsEmpty() );
        TEST_ASSERT( array.GetSize() == 0 );
        TEST_ASSERT( array.GetCapacity() > 0 ); // Capacity unchanged
    }
    {
        // Pop one of many items
        Array<uint32_t> array;
        array.Append( 1 );
        array.Append( 2 );
        array.Append( 3 );
        array.PopFront();
        CheckConsistency( array );
        TEST_ASSERT( array.IsEmpty() == false );
        TEST_ASSERT( array.GetSize() == 2 );
        TEST_ASSERT( array.GetCapacity() >= 3 ); // Capacity unchanged
        TEST_ASSERT( array[ 0 ] == 2 );
        TEST_ASSERT( array[ 1 ] == 3 );
    }

    // Complex Type
    {
        Array<AString> array;
        array.Append( AString( "string1" ) );
        array.PopFront();
        CheckConsistency( array );
        TEST_ASSERT( array.IsEmpty() );
        TEST_ASSERT( array.GetSize() == 0 );
        TEST_ASSERT( array.GetCapacity() > 0 ); // Capacity unchanged
    }
    {
        // Pop one of many items
        Array<AString> array;
        array.Append( AString( "string1" ) );
        array.Append( AString( "string2" ) );
        array.Append( AString( "string3" ) );
        array.PopFront();
        CheckConsistency( array );
        TEST_ASSERT( array.IsEmpty() == false );
        TEST_ASSERT( array.GetSize() == 2 );
        TEST_ASSERT( array.GetCapacity() >= 3 ); // Capacity unchanged
        TEST_ASSERT( array[ 0 ] == "string2" );
        TEST_ASSERT( array[ 1 ] == "string3" );
    }
}

// Erase
//------------------------------------------------------------------------------
void TestArray::Erase() const
{
    // POD
    {
        // Pop last item
        Array<uint32_t> array;
        array.Append( 1 );
        array.Erase( array.Begin() );
        CheckConsistency( array );
        TEST_ASSERT( array.IsEmpty() );
        TEST_ASSERT( array.GetSize() == 0 );
        TEST_ASSERT( array.GetCapacity() > 0 ); // Capacity unchanged
    }
    {
        // Pop one of many items
        Array<uint32_t> array;
        array.Append( 1 );
        array.Append( 2 );
        array.Append( 3 );
        array.Erase( &array[ 1 ] );
        CheckConsistency( array );
        TEST_ASSERT( array.IsEmpty() == false );
        TEST_ASSERT( array.GetSize() == 2 );
        TEST_ASSERT( array.GetCapacity() >= 3 ); // Capacity unchanged
        TEST_ASSERT( array[ 0 ] == 1 );
        TEST_ASSERT( array[ 1 ] == 3 );
    }

    // Complex Type
    {
        Array<AString> array;
        array.Append( AString( "string1" ) );
        array.Erase( array.Begin() );
        CheckConsistency( array );
        TEST_ASSERT( array.IsEmpty() );
        TEST_ASSERT( array.GetSize() == 0 );
        TEST_ASSERT( array.GetCapacity() > 0 ); // Capacity unchanged
    }
    {
        // Pop one of many items
        Array<AString> array;
        array.Append( AString( "string1" ) );
        array.Append( AString( "string2" ) );
        array.Append( AString( "string3" ) );
        array.Erase( &array[ 1 ] );
        CheckConsistency( array );
        TEST_ASSERT( array.IsEmpty() == false );
        TEST_ASSERT( array.GetSize() == 2 );
        TEST_ASSERT( array.GetCapacity() >= 3 ); // Capacity unchanged
        TEST_ASSERT( array[ 0 ] == "string1" );
        TEST_ASSERT( array[ 1 ] == "string3" );
    }
}

// EraseIndex
//------------------------------------------------------------------------------
void TestArray::EraseIndex() const
{
    // POD
    {
        // Pop last item
        Array<uint32_t> array;
        array.Append( 1 );
        array.EraseIndex( 0 );
        CheckConsistency( array );
        TEST_ASSERT( array.IsEmpty() );
        TEST_ASSERT( array.GetSize() == 0 );
        TEST_ASSERT( array.GetCapacity() > 0 ); // Capacity unchanged
    }
    {
        // Pop one of many items
        Array<uint32_t> array;
        array.Append( 1 );
        array.Append( 2 );
        array.Append( 3 );
        array.EraseIndex( 1 );
        CheckConsistency( array );
        TEST_ASSERT( array.IsEmpty() == false );
        TEST_ASSERT( array.GetSize() == 2 );
        TEST_ASSERT( array.GetCapacity() >= 3 ); // Capacity unchanged
        TEST_ASSERT( array[ 0 ] == 1 );
        TEST_ASSERT( array[ 1 ] == 3 );
    }

    // Complex Type
    {
        Array<AString> array;
        array.Append( AString( "string1" ) );
        array.EraseIndex( 0 );
        CheckConsistency( array );
        TEST_ASSERT( array.IsEmpty() );
        TEST_ASSERT( array.GetSize() == 0 );
        TEST_ASSERT( array.GetCapacity() > 0 ); // Capacity unchanged
    }
    {
        // Pop one of many items
        Array<AString> array;
        array.Append( AString( "string1" ) );
        array.Append( AString( "string2" ) );
        array.Append( AString( "string3" ) );
        array.EraseIndex( 1 );
        CheckConsistency( array );
        TEST_ASSERT( array.IsEmpty() == false );
        TEST_ASSERT( array.GetSize() == 2 );
        TEST_ASSERT( array.GetCapacity() >= 3 ); // Capacity unchanged
        TEST_ASSERT( array[ 0 ] == "string1" );
        TEST_ASSERT( array[ 1 ] == "string3" );
    }
}

// EmplaceBack
//------------------------------------------------------------------------------
void TestArray::EmplaceBack() const
{
    // POD
    {
        // Emplace one item
        Array<int32_t> array;
        TEST_ASSERT( array.EmplaceBack( 1 ) == 1 );
        CheckConsistency( array );
        TEST_ASSERT( array.IsEmpty() == false );
        TEST_ASSERT( array.GetSize() == 1 );
        TEST_ASSERT( array.GetCapacity() >= 1 );
        TEST_ASSERT( array[ 0 ] == 1 );
    }
    {
        // Emplace several items
        Array<int32_t> array;
        TEST_ASSERT( array.EmplaceBack( 1 ) == 1 );
        TEST_ASSERT( array.EmplaceBack( 2 ) == 2 );
        TEST_ASSERT( array.EmplaceBack( 3 ) == 3 );
        CheckConsistency( array );
        TEST_ASSERT( array.IsEmpty() == false );
        TEST_ASSERT( array.GetSize() == 3 );
        TEST_ASSERT( array.GetCapacity() >= 3 );
        TEST_ASSERT( array[ 0 ] == 1 );
        TEST_ASSERT( array[ 2 ] == 3 );
    }

    // Complex Type
    {
        // Emplace one item
        Array<AString> array( 1 );

        TEST_MEMORY_SNAPSHOT( s1 ); // Take note of memory state before

        TEST_ASSERT( array.EmplaceBack( "string1" ) == "string1" );

        TEST_EXPECT_ALLOCATION_EVENTS( s1, 1 ) // Check expected amount of allocs occurred

        CheckConsistency( array );
        TEST_ASSERT( array.IsEmpty() == false );
        TEST_ASSERT( array.GetSize() == 1 );
        TEST_ASSERT( array.GetCapacity() >= 1 );
        TEST_ASSERT( array[ 0 ] == "string1" );
    }
    {
        // Emplace one item (Move)
        Array<AString> array( 1 );

        TEST_MEMORY_SNAPSHOT( s1 ); // Take note of memory state before

        TEST_ASSERT( array.EmplaceBack( Move( AString( "string1" ) ) ) == "string1" );

        TEST_EXPECT_ALLOCATION_EVENTS( s1, 1 ) // Check expected amount of allocs occurred

        CheckConsistency( array );
        TEST_ASSERT( array.IsEmpty() == false );
        TEST_ASSERT( array.GetSize() == 1 );
        TEST_ASSERT( array.GetCapacity() >= 1 );
        TEST_ASSERT( array[ 0 ] == "string1" );
    }
    {
        // Emplace several items
        Array<AString> array( 3 );

        TEST_MEMORY_SNAPSHOT( s1 ); // Take note of memory state before

        TEST_ASSERT( array.EmplaceBack( "string1" ) == "string1" );
        TEST_ASSERT( array.EmplaceBack( "string2" ) == "string2" );
        TEST_ASSERT( array.EmplaceBack( "string3" ) == "string3" );

        TEST_EXPECT_ALLOCATION_EVENTS( s1, 3 ) // Check expected amount of allocs occurred

        CheckConsistency( array );
        TEST_ASSERT( array.IsEmpty() == false );
        TEST_ASSERT( array.GetSize() == 3 );
        TEST_ASSERT( array.GetCapacity() >= 3 ); // Capacity unchanged
        TEST_ASSERT( array[ 0 ] == "string1" );
        TEST_ASSERT( array[ 2 ] == "string3" );
    }
}

// AssignmentOperator_OtherArray
//------------------------------------------------------------------------------
void TestArray::AssignmentOperator_OtherArray() const
{
    // POD
    {
        // Empty -> Empty
        Array<uint32_t> array1;
        Array<uint32_t> array2;
        array2 = array1;
        CheckConsistency( array1 );
        CheckConsistency( array2 );
        TEST_ASSERT( array2.GetSize() == array1.GetSize() );
        TEST_ASSERT( array1.IsEmpty() );
        TEST_ASSERT( array2.IsEmpty() );
    }
    {
        // Non-empty -> Empty
        Array<uint32_t> array1;
        array1.Append( 1 );
        Array<uint32_t> array2;
        array2 = array1;
        CheckConsistency( array1 );
        CheckConsistency( array2 );
        TEST_ASSERT( array2.GetSize() == array1.GetSize() );
        TEST_ASSERT( array1.IsEmpty() == false);
        TEST_ASSERT( array2.IsEmpty() == false );
        TEST_ASSERT( array2[ 0 ] == 1 );
        TEST_ASSERT( array2[ 0 ] == array1[ 0 ] );
    }
    {
        // Empty -> Non-empty
        Array<uint32_t> array1;
        Array<uint32_t> array2;
        array2.Append( 1 );
        array2 = array1;
        CheckConsistency( array1 );
        CheckConsistency( array2 );
        TEST_ASSERT( array2.GetSize() == array1.GetSize() );
        TEST_ASSERT( array1.IsEmpty() );
        TEST_ASSERT( array2.IsEmpty() );
    }
    {
        // Non-empty -> Non-empty
        Array<uint32_t> array1;
        array1.Append( 1 );
        Array<uint32_t> array2;
        array2.Append( 2 );
        array2 = array1;
        CheckConsistency( array1 );
        CheckConsistency( array2 );
        TEST_ASSERT( array1.IsEmpty() == false );
        TEST_ASSERT( array2.IsEmpty() == false );
        TEST_ASSERT( array2.GetSize() == array1.GetSize() );
        TEST_ASSERT( array2.GetSize() == 1 );
        TEST_ASSERT( array2[ 0 ] == array1[ 0 ] );
        TEST_ASSERT( array2[ 0 ] == 1 );
    }

    // Complex Type
    {
        // Empty -> Empty
        Array<AString> array1;
        Array<AString> array2;
        array2 = array1;
        CheckConsistency( array1 );
        CheckConsistency( array2 );
        TEST_ASSERT( array2.GetSize() == array1.GetSize() );
        TEST_ASSERT( array1.IsEmpty() );
        TEST_ASSERT( array2.IsEmpty() );
    }
    {
        // Non-empty -> Empty
        Array<AString> array1;
        array1.Append( AString( "string1" ) );
        Array<AString> array2;
        array2 = array1;
        CheckConsistency( array1 );
        CheckConsistency( array2 );
        TEST_ASSERT( array2.GetSize() == array1.GetSize() );
        TEST_ASSERT( array1.IsEmpty() == false );
        TEST_ASSERT( array2.IsEmpty() == false );
        TEST_ASSERT( array2[ 0 ] == "string1" );
        TEST_ASSERT( array2[ 0 ] == array1[ 0 ] );
    }
    {
        // Empty -> Non-empty
        Array<AString> array1;
        Array<AString> array2;
        array2.Append( AString( "string1" ) );
        array2 = array1;
        CheckConsistency( array1 );
        CheckConsistency( array2 );
        TEST_ASSERT( array2.GetSize() == array1.GetSize() );
        TEST_ASSERT( array1.IsEmpty() );
        TEST_ASSERT( array2.IsEmpty() );
    }
    {
        // Non-empty -> Non-empty
        Array<AString> array1;
        array1.Append( AString( "string1" ) );
        Array<AString> array2;
        array2.Append( AString( "string2" ) );
        array2 = array1;
        CheckConsistency( array1 );
        CheckConsistency( array2 );
        TEST_ASSERT( array1.IsEmpty() == false );
        TEST_ASSERT( array2.IsEmpty() == false );
        TEST_ASSERT( array2.GetSize() == array1.GetSize() );
        TEST_ASSERT( array2.GetSize() == 1 );
        TEST_ASSERT( array2[ 0 ] == array1[ 0 ] );
        TEST_ASSERT( array2[ 0 ] == "string1" );
    }
}

// IsAtCapacity
//------------------------------------------------------------------------------
void TestArray::IsAtCapacity() const
{
    // POD
    {
        // Empty, no capacity
        Array<uint32_t> array;
        TEST_ASSERT( array.IsAtCapacity() == true );
        CheckConsistency( array );
    }
    {
        // Empty, with capacity
        Array<uint32_t> array( 1 );
        TEST_ASSERT( array.IsAtCapacity() == false );
        CheckConsistency( array );
    }
    {
        // Non-empty, no extra capacity
        Array<uint32_t> array( 1 );
        array.Append( 1 );
        TEST_ASSERT( array.IsAtCapacity() );
        CheckConsistency( array );
    }
    {
        // Non-empty, with extra capacity
        Array<uint32_t> array( 2 );
        array.Append( 1 );
        TEST_ASSERT( array.IsAtCapacity() == false );
        CheckConsistency( array );
    }

    // Complex Type
    {
        // Empty, no capacity
        Array<AString> array;
        TEST_ASSERT( array.IsAtCapacity() == true );
        CheckConsistency( array );
    }
    {
        // Empty, with capacity
        Array<AString> array( 1 );
        TEST_ASSERT( array.IsAtCapacity() == false );
        CheckConsistency( array );
    }
    {
        // Non-empty, no extra capacity
        Array<AString> array( 1 );
        array.Append( AString( "string1" ) );
        TEST_ASSERT( array.IsAtCapacity() );
        CheckConsistency( array );
    }
    {
        // Non-empty, with extra capacity
        Array<AString> array( 2 );
        array.Append( AString( "string1" ) );
        TEST_ASSERT( array.IsAtCapacity() == false );
        CheckConsistency( array );
    }
}

// GetCapacity
//------------------------------------------------------------------------------
void TestArray::GetCapacity() const
{
    // POD
    {
        // Empty, no capacity
        Array<uint32_t> array;
        TEST_ASSERT( array.GetCapacity() == 0 );
        CheckConsistency( array );
    }
    {
        // Empty, with capacity
        Array<uint32_t> array( 1 );
        TEST_ASSERT( array.GetCapacity() == 1 );
        CheckConsistency( array );
    }
    {
        // Non-empty, with capacity
        Array<uint32_t> array( 1 );
        array.Append( 1 );
        TEST_ASSERT( array.GetCapacity() == 1 );
        CheckConsistency( array );
    }

    // Complex Type
    {
        // Empty, no capacity
        Array<AString> array;
        TEST_ASSERT( array.GetCapacity() == 0 );
        CheckConsistency( array );
    }
    {
        // Empty, with capacity
        Array<AString> array( 1 );
        TEST_ASSERT( array.GetCapacity() == 1 );
        CheckConsistency( array );
    }
    {
        // Non-empty, with capacity
        Array<AString> array( 1 );
        array.Append( AString( "string1" ) );
        TEST_ASSERT( array.GetCapacity() == 1 );
        CheckConsistency( array );
    }
}

// GetSize
//------------------------------------------------------------------------------
void TestArray::GetSize() const
{
    // POD
    {
        // Empty, no capacity
        Array<uint32_t> array;
        TEST_ASSERT( array.GetSize() == 0 );
        CheckConsistency( array );
    }
    {
        // Empty, with capacity
        Array<uint32_t> array( 1 );
        TEST_ASSERT( array.GetSize() == 0 );
        CheckConsistency( array );
    }
    {
        // Non-empty, with capacity
        Array<uint32_t> array( 1 );
        array.Append( 1 );
        TEST_ASSERT( array.GetSize() == 1 );
        CheckConsistency( array );
    }

    // Complex Type
    {
        // Empty, no capacity
        Array<AString> array;
        TEST_ASSERT( array.GetSize() == 0 );
        CheckConsistency( array );
    }
    {
        // Empty, with capacity
        Array<AString> array( 1 );
        TEST_ASSERT( array.GetSize() == 0 );
        CheckConsistency( array );
    }
    {
        // Non-empty, with capacity
        Array<AString> array( 1 );
        array.Append( AString( "string1" ) );
        TEST_ASSERT( array.GetSize() == 1 );
        CheckConsistency( array );
    }
}

// IsEmpty
//------------------------------------------------------------------------------
void TestArray::IsEmpty() const
{
    // POD
    {
        // Empty, no capacity
        Array<uint32_t> array;
        TEST_ASSERT( array.IsEmpty() );
        CheckConsistency( array );
    }
    {
        // Empty, with capacity
        Array<uint32_t> array( 1 );
        TEST_ASSERT( array.IsEmpty() );
        CheckConsistency( array );
    }
    {
        // Non-empty, with capacity
        Array<uint32_t> array( 1 );
        array.Append( 1 );
        TEST_ASSERT( array.IsEmpty() == false );
        CheckConsistency( array );
    }

    // Complex Type
    {
        // Empty, no capacity
        Array<AString> array;
        TEST_ASSERT( array.IsEmpty() );
        CheckConsistency( array );
    }
    {
        // Empty, with capacity
        Array<AString> array( 1 );
        TEST_ASSERT( array.IsEmpty() );
        CheckConsistency( array );
    }
    {
        // Non-empty, with capacity
        Array<AString> array( 1 );
        array.Append( AString( "string1" ) );
        TEST_ASSERT( array.IsEmpty() == false );
        CheckConsistency( array );
    }
}

// Alignment
//------------------------------------------------------------------------------
void TestArray::Alignment() const
{
    // Small
    {
        class Align1
        {
        public:
            uint8_t mValue = 111;
        };
        static_assert( __alignof( Align1 ) == 1, "Align1 check" ); // Make sure we're testing 1 byte alignment

        // Heap
        {
            Array<Align1> array;
            array.SetSize( 1 );
            CheckConsistency( array ); // Checks alignment of members
            array.SetSize( 3 );
            CheckConsistency( array ); // Checks alignment of members
            array.SetSize( 32 );
            CheckConsistency( array ); // Checks alignment of members
        }

        // Stack - one item reservation
        {
            StackArray<Align1, 1> array;
            array.SetSize( 1 );
            CheckConsistency( array ); // Checks alignment of members
        }

        // Stack - multiple items
        {
            StackArray<Align1, 32> array;
            array.SetSize( 32 );
            CheckConsistency( array ); // Checks alignment of members
        }
    }

    // Medium
    {
        class Align4
        {
        public:
            uint32_t mValue = 222;
        };
        static_assert( __alignof( Align4 ) == 4, "Align4 check" ); // Make sure we're testing 4 byte alignment

        // Heap
        {
            Array<Align4> array;
            array.SetSize( 1 );
            CheckConsistency( array ); // Checks alignment of members
            array.SetSize( 3 );
            CheckConsistency( array ); // Checks alignment of members
            array.SetSize( 32 );
            CheckConsistency( array ); // Checks alignment of members
        }

        // Stack - one item (ensure small class is tested)
        {
            StackArray<Align4, 1> array;
            array.SetSize( 1 );
            CheckConsistency( array ); // Checks alignment of members
        }

        // Stack - multiple items
        {
            StackArray<Align4, 32> array;
            array.SetSize( 32 );
            CheckConsistency( array ); // Checks alignment of members
        }
    }

    // Large
    {
        PRAGMA_DISABLE_PUSH_MSVC( 4324 ) // structure was padded due to alignment specifier
        class alignas( 128 ) Align128
        {
        public:
            uint32_t mValue = 333;
        };
        PRAGMA_DISABLE_POP_MSVC // 4324
        static_assert( __alignof( Align128 ) == 128, "Align128 Check" ); // Make sure we're testing 128 byte alignment

        // Heap
        {
            Array<Align128> array;
            array.SetSize( 1 );
            CheckConsistency( array ); // Checks alignment of members
            array.SetSize( 3 );
            CheckConsistency( array ); // Checks alignment of members
            array.SetSize( 32 );
            CheckConsistency( array ); // Checks alignment of members
        }

        // Stack - one item (ensure small class is tested)
        {
            StackArray<Align128, 1> array;
            array.SetSize( 1 );
            CheckConsistency( array ); // Checks alignment of members
        }

        // Stack - multiple items
        {
            StackArray<Align128, 32> array;
            array.SetSize( 32 );
            CheckConsistency( array ); // Checks alignment of members
        }
    }
}

// MoveConstructorHelper
//------------------------------------------------------------------------------
template <class SRC, class DST, uint32_t EXPECTED_ALLOCS, class SRC_CAST>
void TestArray::MoveConstructorHelper() const
{
    // Create the source array
    SRC arrayA;
    arrayA.SetSize( 8 ); // Add some elements

    // Take note of memory state before
    TEST_MEMORY_SNAPSHOT( s1 );

    // Move construct destination. SRC_CAST allows us to check Array/StackArray
    // behave the same
    DST arrayB( Move( (SRC_CAST&)( arrayA ) ) );

    // Check expected amount of allocs occurred
    TEST_EXPECT_ALLOCATION_EVENTS( s1, EXPECTED_ALLOCS )

    // Source string should be empty
    TEST_ASSERT( arrayA.IsEmpty() );

    CheckConsistency( arrayA );
    CheckConsistency( arrayB );
}

// MoveConstructor
//------------------------------------------------------------------------------
void TestArray::MoveConstructor() const
{
    // POD Data
    {
        //                    Src                   Dest                    Allocs    SrcCast
        //------------------------------------------------------------------------------------------
        // Moves from heap can be performed
        MoveConstructorHelper<Array<uint32_t>,      Array<uint32_t>,        0                      >();
        MoveConstructorHelper<Array<uint32_t>,      StackArray<uint32_t>,   0                      >();

        // Moves from stack to stack are copies, but avoid memory allocation
        MoveConstructorHelper<StackArray<uint32_t>, StackArray<uint32_t>,   0                      >();
        MoveConstructorHelper<StackArray<uint32_t>, StackArray<uint32_t>,   0,      Array<uint32_t>>(); // Src as Array, behave the same

        // Moves from stack to heap need re-allocation and copy
        MoveConstructorHelper<StackArray<uint32_t>, Array<uint32_t>,        1                      >();
        MoveConstructorHelper<StackArray<uint32_t>, Array<uint32_t>,        1,      Array<uint32_t>>(); // Src as Array, behave the same
    }

    // Complex Types
    {
        //                    Src                   Dest                    Allocs    SrcCast
        //------------------------------------------------------------------------------------------
        // Moves from heap can be performed
        MoveConstructorHelper<Array<AString>,       Array<AString>,         0                      >();
        MoveConstructorHelper<Array<AString>,       StackArray<AString>,    0                      >();

        // Moves from stack to stack are copies, but avoid memory allocation
        MoveConstructorHelper<StackArray<AString>,  StackArray<AString>,    0                      >();
        MoveConstructorHelper<StackArray<AString>,  StackArray<AString>,    0,      Array<AString> >(); // Src as Array, behave the same

        // Moves from stack to heap need Array reallocated, but items can be moved
        MoveConstructorHelper<StackArray<AString>,  Array<AString>,         1                      >();
        MoveConstructorHelper<StackArray<AString>,  Array<AString>,         1,      Array<AString> >(); // Src as Array, behave the same
    }
}

// MoveAssignmentHelper
//------------------------------------------------------------------------------
template <class SRC, class DST, uint32_t EXPECTED_ALLOCS, class ELEM, class SRC_CAST>
void TestArray::MoveAssignmentHelper( const ELEM & value ) const
{
    // Empty destination
    {
        // Create the source array
        SRC arrayA;
        arrayA.SetCapacity( 8 );
        for ( size_t i = 0; i < 8; ++i )
        {
            arrayA.Append( value ); // Add some elements
        }

        // Create the destination
        DST arrayB;

        // Take note of memory state before
        TEST_MEMORY_SNAPSHOT( s1 );

        // Move construct destination. SRC_CAST allows us to check Array/StackArray
        // behave the same
        arrayB = Move( (SRC_CAST&)( arrayA ) );

        // Check expected amount of allocs occurred
        TEST_EXPECT_ALLOCATION_EVENTS( s1, EXPECTED_ALLOCS )

        // Source string should be empty
        TEST_ASSERT( arrayA.IsEmpty() );

        CheckConsistency( arrayA );
        CheckConsistency( arrayB );
    }

    // Non-empty destination (check move doesn't leak destination string memory)
    {
        // Take note of memory state before
        TEST_MEMORY_SNAPSHOT( s1 );

        {
            // Create the source array
            SRC arrayA;
            arrayA.SetCapacity( 8 );
            for ( size_t i = 0; i < 8; ++i )
            {
                arrayA.Append( value ); // Add some elements
            }

            // Create the destination (with some allocations)
            DST arrayB;
            for ( size_t i = 0; i < 8; ++i )
            {
                arrayB.Append( value ); // Add some elements
            }

            // Move construct destination. SRC_CAST allows us to check Array/StackArray
            // behave the same
            arrayB = Move( (SRC_CAST&)( arrayA ) );

            // Source string should be empty
            TEST_ASSERT( arrayA.IsEmpty() );

            CheckConsistency( arrayA );
            CheckConsistency( arrayB );
        }

        // Check should be no more active allocs in total, even if some allocs occurred
        TEST_EXPECT_INCREASED_ACTIVE_ALLOCATIONS( s1, 0 )
    }
}


// MoveAssignment
//------------------------------------------------------------------------------
void TestArray::MoveAssignment() const
{
    // POD Data
    {
        //                  Src                     Dest                    Allocs  Elem        SrcCast
        //---------------------------------------------------------------------------------------------
        // Moves from heap can be performed
        MoveAssignmentHelper<Array<uint32_t>,       Array<uint32_t>,        0,      uint32_t                   >( 99 );
        MoveAssignmentHelper<Array<uint32_t>,       StackArray<uint32_t>,   0,      uint32_t                   >( 99 );

        // Moves from stack to stack are copies, but avoid memory allocation
        MoveAssignmentHelper<StackArray<uint32_t>,  StackArray<uint32_t>,   0,      uint32_t                   >( 99 );
        MoveAssignmentHelper<StackArray<uint32_t>,  StackArray<uint32_t>,   0,      uint32_t,   Array<uint32_t>>( 99 ); // Src as Array, behave the same

        // Moves from stack to heap need re-allocation and copy
        MoveAssignmentHelper<StackArray<uint32_t>,  Array<uint32_t>,        1,      uint32_t                   >( 99 );
        MoveAssignmentHelper<StackArray<uint32_t>,  Array<uint32_t>,        1,      uint32_t,   Array<uint32_t>>( 99 ); // Src as Array, behave the same
    }

    // Complex Types
    {
        //                   Src                    Dest                    Allocs  Elem            SrcCast
        //-----------------------------------------------------------------------------------------------
        // Moves from heap can be performed
        MoveAssignmentHelper<Array<AString>,        Array<AString>,         0,      AString                   >( AString( "string" ) );
        MoveAssignmentHelper<Array<AString>,        StackArray<AString>,    0,      AString                   >( AString( "string" ) );

        // Moves from stack to stack are copies, but avoid memory allocation
        MoveAssignmentHelper<StackArray<AString>,   StackArray<AString>,    0,      AString                   >( AString( "string" ) );
        MoveAssignmentHelper<StackArray<AString>,   StackArray<AString>,    0,      AString,    Array<AString>>( AString( "string" ) ); // Src as Array, behave the same

        // Moves from stack to heap need Array reallocated, but items can be moved
        MoveAssignmentHelper<StackArray<AString>,   Array<AString>,         1,      AString                   >( AString( "string" ) );
        MoveAssignmentHelper<StackArray<AString>,   Array<AString>,         1,      AString,    Array<AString>>( AString( "string" ) ); // Src as Array, behave the same
    }
}

// MoveWhenGrowing
//------------------------------------------------------------------------------
void TestArray::MoveWhenGrowing() const
{
    Array<AString> array( 4, true );
    array.Append( AString( "string1" ) );
    array.Append( AString( "string2" ) );
    array.Append( AString( "string3" ) );
    array.Append( AString( "string4" ) );

    const AString string5( "string4" );

    // Take note of memory state before
    TEST_MEMORY_SNAPSHOT( s1 );

    // array will need to reallocate
    array.Append( string5 );

    // Should be 2 allocs: one for resize and one for new string
    // but existing strings should have been moved
    TEST_EXPECT_ALLOCATION_EVENTS( s1, 2 )

    CheckConsistency( array );
}

// MoveAppend
//------------------------------------------------------------------------------
void TestArray::MoveAppend() const
{
    AString string( "string4" );
    Array<AString> array( 1, false );

    // Take note of memory state before
    TEST_MEMORY_SNAPSHOT( s1 );

    array.Append( Move( string ) );

    // Should be no allocations
    TEST_EXPECT_ALLOCATION_EVENTS( s1, 0 )

    CheckConsistency( array );
}

// MoveSetCapacity
//------------------------------------------------------------------------------
void TestArray::MoveSetCapacity() const
{
    // Create array with something in it
    Array<AString> array( 1, true );
    array.Append( AString( "string1" ) );

    // Take note of memory state before
    TEST_MEMORY_SNAPSHOT( s1 );

    array.SetCapacity( 8 );

    // Should be 1 allocation for resize (element should be moved)
    TEST_EXPECT_ALLOCATION_EVENTS( s1, 1 )

    CheckConsistency( array );
}

// MovePopFront
//------------------------------------------------------------------------------
void TestArray::MovePopFront() const
{
    // Create array with something in it
    Array<AString> array( 2, false );
    array.Append( AString( "string1" ) );
    array.Append( AString( "string2string2" ) ); // Larger than string 1

    // Take note of memory state before
    TEST_MEMORY_SNAPSHOT( s1 );

    array.PopFront();

    // Should be no allocations (element should be moved)
    TEST_EXPECT_ALLOCATION_EVENTS( s1, 0 )

    CheckConsistency( array );
}

// MoveErase
//------------------------------------------------------------------------------
void TestArray::MoveErase() const
{
    // Create array with something in it
    Array<AString> array( 2, false );
    array.Append( AString( "string1" ) );
    array.Append( AString( "string2string2" ) ); // Larger than string 1

    // Take note of memory state before
    TEST_MEMORY_SNAPSHOT( s1 );

    array.Erase( array.Begin() );

    // Should be no allocations (element should be moved)
    TEST_EXPECT_ALLOCATION_EVENTS( s1, 0 )

    CheckConsistency( array );
}

// StackArrayOverflowToHeap
//------------------------------------------------------------------------------
void TestArray::StackArrayOverflowToHeap() const
{
    // POD
    {
        // Place some cookies around array to detect overruns
        struct CookieWrapper1
        {
            volatile uint64_t cookie1 = 0xF00DF00D;
            StackArray<uint64_t, 4> stackArray;
            volatile uint64_t cookie2 = 0xF00DF00D;
        };
        CookieWrapper1 cw;

        // Fill array to capacity
        cw.stackArray.Append( 1 );
        cw.stackArray.Append( 2 );
        cw.stackArray.Append( 3 );
        cw.stackArray.Append( 4 );
        TEST_ASSERT( cw.stackArray.IsAtCapacity() );
        CheckConsistency( cw.stackArray );

        // Add an additional item
        cw.stackArray.Append( 5 );
        CheckConsistency( cw.stackArray );
        TEST_ASSERT( cw.stackArray.GetSize() == 5 );
        TEST_ASSERT( cw.stackArray.GetSize() <= cw.stackArray.GetCapacity() );

        // Ensure cookies were not overwritten
        TEST_ASSERT( cw.cookie1 == 0xF00DF00D );
        TEST_ASSERT( cw.cookie2 == 0xF00DF00D );
    }

    // Complex Type
    {
        // Place some cookies around array to detect overruns
        struct CookieWrapper2
        {
            volatile uint64_t cookie1 = 0xF00DF00D;
            StackArray<AString, 4> stackArray;
            volatile uint64_t cookie2 = 0xF00DF00D;
        };
        CookieWrapper2 cw;

        // Fill array to capacity
        cw.stackArray.Append( AString( "string1" ) );
        cw.stackArray.Append( AString( "string2" ) );
        cw.stackArray.Append( AString( "string3" ) );
        cw.stackArray.Append( AString( "string4" ) );
        TEST_ASSERT( cw.stackArray.IsAtCapacity() );
        CheckConsistency( cw.stackArray );

        // Add an additional item
        cw.stackArray.Append( AString( "string5" ) );
        CheckConsistency( cw.stackArray );
        TEST_ASSERT( cw.stackArray.GetSize() == 5 );
        TEST_ASSERT( cw.stackArray.GetSize() <= cw.stackArray.GetCapacity() );

        // Ensure cookies were not overwritten
        TEST_ASSERT( cw.cookie1 == 0xF00DF00D );
        TEST_ASSERT( cw.cookie2 == 0xF00DF00D );
    }
}

// CheckConsistency
//------------------------------------------------------------------------------
template <typename T>
void TestArray::CheckConsistency( const Array<T> & array ) const
{
    // Begin and end is consistent
    TEST_ASSERT( array.End() >= array.Begin() );
    TEST_ASSERT( (size_t)( array.End() - array.Begin() ) == array.GetSize() );
    if ( array.IsEmpty() )
    {
        TEST_ASSERT( array.End() == array.Begin() );
    }
    else
    {
        TEST_ASSERT( array.End() > array.Begin() );
    }

    // Ensure each element in the array has the correct alignment
    for ( const T & item : array )
    {
        TEST_ASSERT( ( uintptr_t( &item ) & ( __alignof( T ) - 1 ) ) == 0 );
    }
}

//------------------------------------------------------------------------------
