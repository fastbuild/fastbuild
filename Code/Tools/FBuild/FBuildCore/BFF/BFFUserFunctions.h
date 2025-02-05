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
    struct Argument
    {
        const BFFToken * m_Val;
        bool m_IsRef;
    };

    explicit BFFUserFunction( const AString & name,
                              const Array< Argument > & args,
                              const BFFTokenRange & bodyTokenRange,
                              bool hasReferences );
    ~BFFUserFunction();

    const Array< Argument > &   GetArgs() const { return m_Args; }
    const BFFTokenRange &       GetBodyTokenRange() const { return m_BodyTokenRange; }
    bool                        HasReferences() const { return m_HasReferences; }

    bool operator == ( const AString & name ) const { return ( m_Name == name ); }

protected:
    AString                     m_Name;
    Array< Argument >           m_Args;
    BFFTokenRange               m_BodyTokenRange;
    bool                        m_HasReferences;
};

// BFFUserFunctions
//------------------------------------------------------------------------------
class BFFUserFunctions : public Singleton< BFFUserFunctions >
{
public:
    explicit BFFUserFunctions();
    ~BFFUserFunctions();

    void AddFunction( const AString & name,
                      const Array< BFFUserFunction::Argument > & args,
                      const BFFTokenRange & tokenRange,
                      bool hasReferences );
    BFFUserFunction * FindFunction( const AString & name ) const;
    void Clear();

private:
    Array< BFFUserFunction * > m_Functions;
};

//------------------------------------------------------------------------------
