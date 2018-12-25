// AutoPtr
//------------------------------------------------------------------------------
#pragma once

// Includes
//------------------------------------------------------------------------------
#include "Core/Mem/Mem.h"

// FreeDeletor - free using FREE
//------------------------------------------------------------------------------
class FreeDeletor
{
public:
    static inline void Delete( void * ptr ) { FREE( ptr ); }
};

// DeleteDeletor - free using FDELETE
//------------------------------------------------------------------------------
class DeleteDeletor
{
public:
    template < class T >
    static inline void Delete( T * ptr ) { FDELETE ptr; }
};

// AutoPtr
//------------------------------------------------------------------------------
template < class T, class DELETOR = FreeDeletor >
class AutoPtr
{
public:
    explicit inline AutoPtr() : m_Pointer( nullptr ) {}
    explicit inline AutoPtr( T * ptr ) : m_Pointer( ptr ) {}
    inline         ~AutoPtr() { DELETOR::Delete( m_Pointer ); }

    // access the pointer
    inline       T * Get()       { return m_Pointer; }
    inline const T * Get() const { return m_Pointer; }

    // acquire a new pointer
    inline void operator = ( T * newPtr ) { DELETOR::Delete( m_Pointer ); m_Pointer = newPtr; }

    // manually intiate deletion
    inline void Destroy() { DELETOR::Delete( m_Pointer ); m_Pointer = nullptr; }

    // free the pointer without deleting it
    inline T * Release() { T * ptr = m_Pointer; m_Pointer = nullptr; return ptr; }
private:
    T * m_Pointer;
};

//------------------------------------------------------------------------------
