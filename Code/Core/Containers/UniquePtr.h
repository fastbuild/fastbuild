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
    static void Delete( void * ptr ) { FREE( ptr ); }
};

// DeleteDeletor - free using FDELETE
//------------------------------------------------------------------------------
class DeleteDeletor
{
public:
    template <class T>
    static void Delete( T * ptr )
    {
        FDELETE ptr;
    }
};

// UniquePtr
//------------------------------------------------------------------------------
template <class T, class DELETOR = DeleteDeletor>
class UniquePtr
{
public:
    explicit UniquePtr() = default;
    explicit UniquePtr( T * ptr )
        : m_Pointer( ptr )
    {
    }
    explicit UniquePtr( UniquePtr<T, DELETOR> && other )
        : m_Pointer( other.m_Pointer )
    {
        other.m_Pointer = nullptr;
    }
    ~UniquePtr() { DELETOR::Delete( m_Pointer ); }

    // non-copyable
    explicit UniquePtr( const UniquePtr<T, DELETOR> & other ) = delete;
    void operator=( const UniquePtr<T, DELETOR> & other ) = delete;

    // access the pointer
    [[nodiscard]] T * Get() { return m_Pointer; }
    [[nodiscard]] const T * Get() const { return m_Pointer; }
    [[nodiscard]] T * operator->()
    {
        ASSERT( m_Pointer );
        return m_Pointer;
    }
    [[nodiscard]] const T * operator->() const
    {
        ASSERT( m_Pointer );
        return m_Pointer;
    }

    // acquire a new pointer
    void Replace( T * newPtr )
    {
        DELETOR::Delete( m_Pointer );
        m_Pointer = newPtr;
    }
    void operator=( UniquePtr<T, DELETOR> && other )
    {
        DELETOR::Delete( m_Pointer );
        m_Pointer = other.m_Pointer;
        other.m_Pointer = nullptr;
    }

    // manually initiate deletion
    void Destroy()
    {
        DELETOR::Delete( m_Pointer );
        m_Pointer = nullptr;
    }

    // release ownership of pointer without deleting it
    [[nodiscard]] T * ReleaseOwnership()
    {
        T * ptr = m_Pointer;
        m_Pointer = nullptr;
        return ptr;
    }

private:
    T * m_Pointer = nullptr;
};

//------------------------------------------------------------------------------
