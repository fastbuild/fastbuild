// BFFUserFunctions
//------------------------------------------------------------------------------

// Includes
//------------------------------------------------------------------------------
#include "BFFUserFunctions.h"

// CONSTRUCTOR (BFFUserFunction)
//------------------------------------------------------------------------------
BFFUserFunction::BFFUserFunction( const AString & name,
                                  const Array< const BFFToken * > & args,
                                  const BFFTokenRange & bodyTokenRange )
    : m_Name( name )
    , m_Args( args )
    , m_BodyTokenRange( bodyTokenRange )
{
}

// DESTRUCTOR (BFFUserFunction)
//------------------------------------------------------------------------------
BFFUserFunction::~BFFUserFunction() = default;

// CONSTRUCTOR (BFFUserFunctions)
//------------------------------------------------------------------------------
BFFUserFunctions::BFFUserFunctions() = default;

// DESTRUCTOR (BFFUserFunctions)
//------------------------------------------------------------------------------
BFFUserFunctions::~BFFUserFunctions()
{
    for ( BFFUserFunction * func : m_Functions )
    {
        FDELETE( func );
    }
}

// AddFunction
//------------------------------------------------------------------------------
void BFFUserFunctions::AddFunction( const AString & name,
                                    const Array< const BFFToken * > & args,
                                    const BFFTokenRange & tokenRange )
{
    BFFUserFunction * newFunction = FNEW( BFFUserFunction( name, args, tokenRange ) );
    m_Functions.Append( newFunction );
}

// FindFunction
//------------------------------------------------------------------------------
BFFUserFunction * BFFUserFunctions::FindFunction( const AString & name ) const
{
    BFFUserFunction * * found = m_Functions.FindDeref( name );
    return found ? *found : nullptr;
}

//------------------------------------------------------------------------------
