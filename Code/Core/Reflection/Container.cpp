// Container
//------------------------------------------------------------------------------

// Includes
//------------------------------------------------------------------------------
#include "Core/PrecompiledHeader.h"
#include "Container.h"

// ReflectionInfo
//------------------------------------------------------------------------------
REFLECT_BEGIN( Container, Object, MetaNone() )
REFLECT_END( Container )

// CONSTRUCTOR
//------------------------------------------------------------------------------
Container::Container()
    : m_Children( 0, true )
{
    auto end = m_Children.End();
    for ( auto it = m_Children.Begin() ; it != end; ++it )
    {
        FDELETE( *it );
    }
}

// DESTRUCTOR
//------------------------------------------------------------------------------
Container::~Container() = default;

// AddChild
//------------------------------------------------------------------------------
void Container::AddChild( Object * child )
{
    ASSERT( m_Children.Find( child ) == nullptr );
    m_Children.Append( child );
}

// FindChild
//------------------------------------------------------------------------------
Object * Container::FindChild( const char * name ) const
{
    auto end = m_Children.End();
    for ( auto it = m_Children.Begin() ; it != end; ++it )
    {
        if ( ( *it )->GetName() == name )
        {
            return *it;
        }
    }
    return nullptr;
}

//------------------------------------------------------------------------------
