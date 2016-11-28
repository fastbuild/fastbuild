// Ref
//------------------------------------------------------------------------------
#pragma once

// Includes
//------------------------------------------------------------------------------
#include "Core/Reflection/RefObject.h"

// Forward Declarations
//------------------------------------------------------------------------------

// Ref
//------------------------------------------------------------------------------
template < class T >
class Ref
{
public:
    explicit inline Ref() : m_Pointer( nullptr ) {}
    explicit inline Ref( const Ref< T > & other ) : m_Pointer( other.Get() ) { if ( m_Pointer ) { m_Pointer->IncRef(); } }
    explicit inline Ref( T * ptr ) : m_Pointer( ptr ) { if ( ptr ) { ptr->IncRef(); } }
    inline         ~Ref() { T * ptr = m_Pointer; if ( ptr ) { ptr->DecRef(); } }

    // access the pointer
    inline      T * Get()               { return m_Pointer; }
    inline      T * Get() const         { return m_Pointer; }
    inline      T * operator ->()       { return m_Pointer; }
    inline      T * operator ->() const { return m_Pointer; }

    inline void operator = ( const Ref< RefObject > & other ) { Assign( other.m_Pointer ); }
    template < class U >
    inline void operator = ( const Ref< U > & other ) { Assign( other.m_Pointer ); }
    template < class U >
    inline void operator = ( U * other ) { Assign( other ); }

private:
    void Assign( T * ptr )
    {
        if ( ptr )
        {
            ptr->IncRef();
        }
        T * old = m_Pointer;
        m_Pointer = ptr;
        if ( old )
        {
            old->DecRef();
        }
    }

    T * m_Pointer;
};

//------------------------------------------------------------------------------
