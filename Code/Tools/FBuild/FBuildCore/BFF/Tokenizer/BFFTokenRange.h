// BFFTokenRange.h
//------------------------------------------------------------------------------
#pragma once

// Includes
//------------------------------------------------------------------------------
// FBuildCore
#include <Tools/FBuild/FBuildCore/BFF/Tokenizer/BFFToken.h>

// Core
#include <Core/Env/Assert.h>

// BFFTokenRange
//------------------------------------------------------------------------------
class BFFTokenRange
{
public:
    // Constructors
    BFFTokenRange() = default;
    BFFTokenRange( const BFFToken * begin, const BFFToken * end )
        : m_Pos( begin )
        , m_End( end )
        , m_Begin( begin )
    {}
    ~BFFTokenRange() = default;

    const BFFToken * operator -> () const { return m_Pos; }
    const BFFToken * GetCurrent() const { return m_Pos; }
    const BFFToken * GetEnd() const { return m_End; }
    const BFFToken * GetBegin() const { return m_Begin; }

    void operator ++ (int) { ASSERT( m_Pos < m_End ); m_Pos++; }

    inline bool IsAtEnd() const { return ( m_Pos == m_End ); }
    inline bool IsEmpty() const { return ( m_Begin == m_End ); }


protected:
    const BFFToken * m_Pos      = nullptr;
    const BFFToken * m_End      = nullptr;
    const BFFToken * m_Begin    = nullptr;
};

//------------------------------------------------------------------------------
