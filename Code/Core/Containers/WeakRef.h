// WeakRef
//------------------------------------------------------------------------------
#pragma once

// Includes
//------------------------------------------------------------------------------

// Forward Declarations
//------------------------------------------------------------------------------
class Object;

// WeakRef
//------------------------------------------------------------------------------
template < class T >
class WeakRef
{
public:
    explicit inline WeakRef() : m_Pointer( nullptr ) {}
    explicit inline WeakRef( T * ptr ) : m_Pointer( ptr ) { /*TODO: IncWeakRef*/ }
    inline         ~WeakRef() = default; // TODO: DecWeakRef()

    // access the pointer
    inline      T * Get()               { return m_Pointer; }
    inline      T * Get() const         { return m_Pointer; }
    inline      T * operator ->()       { return m_Pointer; }
    inline      T * operator ->() const { return m_Pointer; }

    inline void operator = ( const WeakRef< Object > & other ) { Assign( other.m_Pointer ); }
    template < class U >
    inline void operator = ( const WeakRef< U > & other ) { Assign( other.m_Pointer ); }
    template < class U >
    inline void operator = ( U * other ) { Assign( other ); }

private:
    void Assign( T * ptr )
    {
        // TODO: IncWeakRef on new
        m_Pointer = ptr;
        // TODO: DecWeakRef on old
    }

    T * m_Pointer;
};

//------------------------------------------------------------------------------
