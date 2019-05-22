// ReflectionIter.cpp
//------------------------------------------------------------------------------

// Includes
//------------------------------------------------------------------------------
#include "ReflectionIter.h"
#include "Core/Reflection/ReflectionInfo.h"

// CONSTRUCTOR
//------------------------------------------------------------------------------
ReflectionIter::ReflectionIter( const ReflectionInfo * info, uint32_t index )
    : m_Info( info )
    , m_Index( index )
{
}

// operator ==
//------------------------------------------------------------------------------
bool ReflectionIter::operator == ( const ReflectionIter & other ) const
{
    ASSERT( other.m_Info == m_Info ); // invalid to compare iterators on different objects

    return ( other.m_Index == m_Index );
}

// operator ++
//------------------------------------------------------------------------------
void ReflectionIter::operator ++()
{
    ++m_Index;
}

// operator ->
//------------------------------------------------------------------------------
const ReflectedProperty & ReflectionIter::operator ->() const
{
    return m_Info->GetReflectedProperty( m_Index );
}

// operator *
//------------------------------------------------------------------------------
const ReflectedProperty & ReflectionIter::operator *() const
{
    return m_Info->GetReflectedProperty( m_Index );
}

//------------------------------------------------------------------------------
