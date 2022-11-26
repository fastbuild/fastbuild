// UniquePtr
//------------------------------------------------------------------------------
#pragma once

// Includes
//------------------------------------------------------------------------------
#include "Core/Env/Assert.h"
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

// UniquePtr
//------------------------------------------------------------------------------
template < class T, class DELETOR = FreeDeletor >
class UniquePtr
{
public:
    explicit inline UniquePtr() : m_Pointer( nullptr ) {}
    explicit inline UniquePtr( T * ptr ) : m_Pointer( ptr ) {}
    inline         ~UniquePtr() { DELETOR::Delete( m_Pointer ); }

    // access the pointer
    [[nodiscard]] T *           Get()       { return m_Pointer; }
    [[nodiscard]] const T *     Get() const { return m_Pointer; }
    [[nodiscard]] T *           operator ->()       { ASSERT( m_Pointer ); return m_Pointer; }
    [[nodiscard]] const T *     operator ->() const { ASSERT( m_Pointer ); return m_Pointer; }

    // acquire a new pointer
    void                        operator = ( T * newPtr ) { DELETOR::Delete( m_Pointer ); m_Pointer = newPtr; }

    // manually intiate deletion
    void                        Destroy() { DELETOR::Delete( m_Pointer ); m_Pointer = nullptr; }

    // free the pointer without deleting it
    [[nodiscard]] T *           Release() { T * ptr = m_Pointer; m_Pointer = nullptr; return ptr; }
private:
    T * m_Pointer;
};

//------------------------------------------------------------------------------
