// BFFMacros - manages defined macros
//------------------------------------------------------------------------------
#pragma once

// Includes
//------------------------------------------------------------------------------
#include "Core/Containers/Array.h"
#include "Core/Containers/Singleton.h"

// Forward Declarations
//------------------------------------------------------------------------------
class AString;

// BFFMacros
//------------------------------------------------------------------------------
class BFFMacros : public Singleton< BFFMacros >
{
public:
    explicit BFFMacros();
    ~BFFMacros();

    const Array< AString > & Tokens() const { return m_Tokens; }

    bool IsDefined( const AString & token ) const;

    bool Define( const AString & token );
    bool Undefine( const AString & token );

private:
    Array< AString > m_Tokens;
};

//------------------------------------------------------------------------------
