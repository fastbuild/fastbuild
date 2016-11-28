// ReflectionIter.h
//------------------------------------------------------------------------------
#pragma once

// Includes
//------------------------------------------------------------------------------
#include "Core/Env/Types.h"

// Forward Declarations
//------------------------------------------------------------------------------
class ReflectionInfo;
class ReflectedProperty;

// ReflectionIter
//------------------------------------------------------------------------------
class ReflectionIter
{
public:
    explicit ReflectionIter( const ReflectionInfo * info, uint32_t index );

    // comparison of iterators
    bool operator == ( const ReflectionIter & other ) const;
    inline bool operator != ( const ReflectionIter & other ) const { return !( *this == other ); }

    // iterating
    void operator ++();

    // dereferencing
    const ReflectedProperty & operator ->() const;
    const ReflectedProperty & operator *() const;

protected:
    const ReflectionInfo *  m_Info;
    uint32_t                m_Index;
};

//------------------------------------------------------------------------------
