// BFFKeywords
//------------------------------------------------------------------------------

// Includes
//------------------------------------------------------------------------------
#include "BFFOperators.h"

// Core
#include "Core/Env/Assert.h"
#include "Core/Strings/AString.h"

// Static Data
//------------------------------------------------------------------------------
/*static*/ const AString BFFOperator::sOperatorStringLookup[] = {
    AString( BFF_OPERATOR_ASSIGN ),
    AString( BFF_OPERATOR_GREATER_THAN ),
    AString( BFF_OPERATOR_LESS_THAN ),
    AString( BFF_OPERATOR_NOT ),
    AString( BFF_OPERATOR_PLUS ),
    AString( BFF_OPERATOR_MINUS ),
    AString( BFF_OPERATOR_EQUAL ),
    AString( BFF_OPERATOR_NOT_EQUAL ),
    AString( BFF_OPERATOR_GREATER_THAN_OR_EQUAL ),
    AString( BFF_OPERATOR_LESS_THAN_OR_EQUAL ),
    AString( BFF_OPERATOR_AND ),
    AString( BFF_OPERATOR_OR ),
};

//------------------------------------------------------------------------------
/*static*/ BFFOperator::Type BFFOperator::GetType( const AString & string )
{
    for ( size_t i = 0; i < static_cast<size_t>( Type::Count ); ++i )
    {
        const AString & keyword = sOperatorStringLookup[ i ];
        if ( string == keyword )
        {
            return static_cast<Type>( i );
        }
    }
    ASSERT( false );
    return Type::Count;
}

//------------------------------------------------------------------------------
/*static*/ const AString & BFFOperator::GetString( Type type )
{
    static_assert( ARRAY_SIZE( sOperatorStringLookup ) == static_cast<size_t>( Type::Count ) );

    ASSERT( static_cast<size_t>( type ) < ARRAY_SIZE( sOperatorStringLookup ) );
    return sOperatorStringLookup[ static_cast<size_t>( type ) ];
}

//------------------------------------------------------------------------------
