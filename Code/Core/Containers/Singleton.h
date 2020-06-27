// Singleton.h
//------------------------------------------------------------------------------
#pragma once

// Includes
//------------------------------------------------------------------------------
#include "Core/Env/Assert.h"

// Singleton
//------------------------------------------------------------------------------
template < class T >
class Singleton
{
public:
    static T & Get();
    static inline bool IsValid() { return ( s_Instance != nullptr ); }

protected:
    Singleton();
    ~Singleton();

private:
    static T * s_Instance;
};

// Static
//------------------------------------------------------------------------------
template < class T >
T * Singleton< T >::s_Instance = nullptr;

// CONSTRUCTOR
//------------------------------------------------------------------------------
template < class T >
Singleton< T >::Singleton()
{
    ASSERT( s_Instance == nullptr );
    s_Instance = static_cast< T * >( this );
}

// DESTRUCTOR
//------------------------------------------------------------------------------
template < class T >
Singleton< T >::~Singleton()
{
    ASSERT( s_Instance == this );
    s_Instance = nullptr;
}

// Get
//------------------------------------------------------------------------------
template < class T >
T & Singleton< T >::Get()
{
    ASSERT( s_Instance );
    PRAGMA_DISABLE_PUSH_MSVC( 6011 ) // static analysis generates a C6011: Dereferencing NULL pointer 's_Instance'
    return *s_Instance;
    PRAGMA_DISABLE_POP_MSVC
}

//------------------------------------------------------------------------------
