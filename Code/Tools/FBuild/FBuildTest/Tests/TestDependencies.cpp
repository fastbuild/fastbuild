// TestDependencies
//------------------------------------------------------------------------------

// Includes
//------------------------------------------------------------------------------
#include "FBuildTest.h"

// FBuild
#include "Tools/FBuild/FBuildCore/Graph/Dependencies.h"

//------------------------------------------------------------------------------
TEST_GROUP( TestDependencies, FBuildTest )
{
public:
};

//------------------------------------------------------------------------------
TEST_CASE( TestDependencies, Empty )
{
    // No initial capacity
    {
        Dependencies d;
        TEST_ASSERT( d.IsEmpty() );
        TEST_ASSERT( d.GetSize() == 0 );
        TEST_ASSERT( d.GetCapacity() == 0 );
    }

    // Initial capacity
    {
        Dependencies d( 16 );
        TEST_ASSERT( d.IsEmpty() );
        TEST_ASSERT( d.GetSize() == 0 );
        TEST_ASSERT( d.GetCapacity() == 16 );
    }
}

//------------------------------------------------------------------------------
TEST_CASE( TestDependencies, Add )
{
    Node * nodes[] = { (Node *)0x01, (Node *)0x02, (Node *)0x03 };

    // Node with defaults
    {
        Dependencies d;
        d.Add( nodes[ 0 ] );
        TEST_ASSERT( d.IsEmpty() == false );
        TEST_ASSERT( d.GetSize() == 1 );
        TEST_ASSERT( d.GetCapacity() > 0 );
    }

    // Node with explicit params
    {
        Dependencies d;
        d.Add( nodes[ 0 ], 0x12341234, true );
        TEST_ASSERT( d.IsEmpty() == false );
        TEST_ASSERT( d.GetSize() == 1 );
        TEST_ASSERT( d.GetCapacity() > 0 );
    }

    // Multiple add calls
    {
        Dependencies d;
        size_t count = 0;
        for ( Node * node : nodes )
        {
            d.Add( node );
            ++count;
            TEST_ASSERT( d.IsEmpty() == false );
            TEST_ASSERT( d.GetSize() == count );
            TEST_ASSERT( d.GetCapacity() >= count );
            for ( size_t i = 0; i < count; ++i )
            {
                TEST_ASSERT( d[ i ].GetNode() == nodes[ i ] );
            }
        }
    }

    // Add dependencies
    {
        // First
        Node * nodes1[] = { (Node *)0x01, (Node *)0x02, (Node *)0x03 };
        Dependencies d1;
        for ( Node * node : nodes1 )
        {
            d1.Add( node );
        }

        // Second
        Node * nodes2[] = { (Node *)0x04, (Node *)0x05, (Node *)0x06 };
        Dependencies d2;
        for ( Node * node : nodes2 )
        {
            d2.Add( node );
        }

        // Add to an empty list
        Dependencies d;
        d.Add( d1 );
        TEST_ASSERT( d.IsEmpty() == false );
        TEST_ASSERT( d.GetSize() == 3 );
        TEST_ASSERT( d.GetCapacity() >= d.GetSize() );

        // Add additional deps
        d.Add( d2 );
        TEST_ASSERT( d.IsEmpty() == false );
        TEST_ASSERT( d.GetSize() == 6 );
        TEST_ASSERT( d.GetCapacity() >= d.GetSize() );

        // Test final set is correct
        // clang-format off
        const Node * const finalNodes[] = { (Node *)0x01, (Node *)0x02, (Node *)0x03,
                                            (Node *)0x04, (Node *)0x05, (Node *)0x06 };
        // clang-format on
        for ( const Dependency & dep : d )
        {
            const size_t index = d.GetIndexOf( &dep );
            TEST_ASSERT( dep.GetNode() == finalNodes[ index ] );
        }
    }
}

//------------------------------------------------------------------------------
TEST_CASE( TestDependencies, Clear )
{
    // Clear already empty
    {
        Dependencies d;
        d.Clear();
        TEST_ASSERT( d.IsEmpty() );
        TEST_ASSERT( d.GetSize() == 0 );
        TEST_ASSERT( d.GetCapacity() == 0 );
    }

    // Clear non-empty
    {
        Node * node = nullptr;
        Dependencies d;
        d.Add( node );
        d.Clear();
        TEST_ASSERT( d.IsEmpty() );
        TEST_ASSERT( d.GetSize() == 0 );
        TEST_ASSERT( d.GetCapacity() > 0 ); // Capacity is retained
    }
}

//------------------------------------------------------------------------------
TEST_CASE( TestDependencies, SetCapacity )
{
    // Set on default constructed
    {
        Dependencies d;
        d.SetCapacity( 16 );
        TEST_ASSERT( d.IsEmpty() );
        TEST_ASSERT( d.GetSize() == 0 );
        TEST_ASSERT( d.GetCapacity() == 16 );
    }

    // Set on constructed with initial capacity
    {
        Dependencies d( 8 );
        d.SetCapacity( 16 );
        TEST_ASSERT( d.IsEmpty() );
        TEST_ASSERT( d.GetSize() == 0 );
        TEST_ASSERT( d.GetCapacity() == 16 );
    }

    // Set on non-empty
    {
        Node * node = nullptr;
        Dependencies d;
        d.Add( node );
        d.SetCapacity( 16 );
        TEST_ASSERT( d.IsEmpty() == false ); // Item should not be lost
        TEST_ASSERT( d.GetSize() == 1 ); // Item should not be lost
        TEST_ASSERT( d.GetCapacity() == 16 );
    }

    // Set capacity lower than size, but not zero (which is ammortizing growth)
    {
        Node * node = nullptr;
        Dependencies d;
        d.Add( node );
        d.Add( node );
        d.SetCapacity( 1 );
        TEST_ASSERT( d.IsEmpty() == false ); // Items should not be lost
        TEST_ASSERT( d.GetSize() == 2 ); // Items should not be lost
        TEST_ASSERT( d.GetCapacity() >= d.GetSize() ); // Capacity must not have shrunk
    }
}

//------------------------------------------------------------------------------
TEST_CASE( TestDependencies, Iteration )
{
    Node * nodes[] = { (Node *)0x01, (Node *)0x02, (Node *)0x03 };

    Dependencies d;
    for ( Node * node : nodes )
    {
        d.Add( node );
    }
    TEST_ASSERT( d.GetSize() == 3 );

    // Non-const
    PRAGMA_DISABLE_PUSH_MSVC( 26496 ) // Don't complain about non-const 'dep' as we want that
    for ( Dependency & dep : d )
    {
        const size_t index = d.GetIndexOf( &dep );
        TEST_ASSERT( dep.GetNode() == nodes[ index ] );
    }
    PRAGMA_DISABLE_POP_MSVC

    // Const
    const Dependencies & constD( d );
    for ( const Dependency & dep : constD )
    {
        const size_t index = d.GetIndexOf( &dep );
        TEST_ASSERT( dep.GetNode() == nodes[ index ] );
    }
}

//------------------------------------------------------------------------------
TEST_CASE( TestDependencies, OperatorEquals )
{
    // Empty
    {
        Dependencies d1;
        Dependencies d2;
        d2 = d1;
        TEST_ASSERT( d2.IsEmpty() );
        TEST_ASSERT( d2.GetSize() == 0 );
        TEST_ASSERT( d2.GetCapacity() == 0 );
    }

    // Non-empty
    {
        // First
        Node * nodes1[] = { (Node *)0x01, (Node *)0x02, (Node *)0x03 };
        Dependencies d1;
        for ( Node * node : nodes1 )
        {
            d1.Add( node );
        }

        // Second
        Node * nodes2[] = { (Node *)0x04, (Node *)0x05, (Node *)0x06 };
        Dependencies d2;
        for ( Node * node : nodes2 )
        {
            d2.Add( node );
        }

        // Assign one over the top of the other
        d1 = d2;
        TEST_ASSERT( d1.IsEmpty() == false );
        TEST_ASSERT( d1.GetSize() == 3 );
        TEST_ASSERT( d1.GetCapacity() >= d1.GetSize() );

        // Test both are the same now were replaces
        for ( const Dependency & dep : d1 )
        {
            const size_t index = d1.GetIndexOf( &dep );
            TEST_ASSERT( dep.GetNode() == nodes2[ index ] );
            TEST_ASSERT( d1[ index ].GetNode() == d2[ index ].GetNode() );
        }
    }
}

//------------------------------------------------------------------------------
