// BFFUserFunctions
//------------------------------------------------------------------------------
#pragma once

// Includes
//------------------------------------------------------------------------------
// FBuildCore
#include "Tools/FBuild/FBuildCore/BFF/Tokenizer/BFFTokenRange.h"

// Core
#include "Core/Containers/Array.h"
#include "Core/Containers/Singleton.h"
#include "Core/Strings/AString.h"

// BFFUserFunction
//------------------------------------------------------------------------------
class BFFUserFunction
{
public:
    explicit BFFUserFunction( const AString & name,
                              const Array< const BFFToken * > & args,
                              const BFFTokenRange & bodyTokenRange );
    ~BFFUserFunction();

    const Array< const BFFToken * > &   GetArgs() const { return m_Args; }
    const BFFTokenRange &               GetBodyTokenRange() const { return m_BodyTokenRange; }

    bool operator == ( const AString & name ) const { return ( m_Name == name ); }

protected:
    AString                     m_Name;
    Array< const BFFToken * >   m_Args;
    BFFTokenRange               m_BodyTokenRange;
};

// BFFUserFunctions
//------------------------------------------------------------------------------
class BFFUserFunctions : public Singleton< BFFUserFunctions >
{
public:
    explicit BFFUserFunctions();
    ~BFFUserFunctions();

    void AddFunction( const AString & name,
                      const Array< const BFFToken * > & args,
                      const BFFTokenRange & tokenRange );
    BFFUserFunction * FindFunction( const AString & name ) const;

private:
    Array< BFFUserFunction * > m_Functions;
};

//------------------------------------------------------------------------------
