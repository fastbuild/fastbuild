// BFFKeywords
//------------------------------------------------------------------------------

// Includes
//------------------------------------------------------------------------------
#include "BFFKeywords.h"

// Core
#include "Core/Env/Assert.h"
#include "Core/Strings/AString.h"

// Static Data
//------------------------------------------------------------------------------
/*static*/ const AString BFFKeyword::sKeywordStringLookup[] = {
    AString( BFF_KEYWORD_DEFINE ),
    AString( BFF_KEYWORD_ELIF ),
    AString( BFF_KEYWORD_ELSE ),
    AString( BFF_KEYWORD_ENDIF ),
    AString( BFF_KEYWORD_EXISTS ),
    AString( BFF_KEYWORD_FALSE ),
    AString( BFF_KEYWORD_FILE_EXISTS ),
    AString( BFF_KEYWORD_FUNCTION ),
    AString( BFF_KEYWORD_IF ),
    AString( BFF_KEYWORD_IMPORT ),
    AString( BFF_KEYWORD_IN ),
    AString( BFF_KEYWORD_INCLUDE ),
    AString( BFF_KEYWORD_NOT ),
    AString( BFF_KEYWORD_ONCE ),
    AString( BFF_KEYWORD_TRUE ),
    AString( BFF_KEYWORD_UNDEF ),
};

//------------------------------------------------------------------------------
/*static*/ BFFKeyword::Type BFFKeyword::GetType( const AString & string )
{
    for ( size_t i = 0; i < static_cast<size_t>( Type::Count ); ++i )
    {
        const AString & keyword = sKeywordStringLookup[ i ];
        if ( string == keyword )
        {
            return static_cast<Type>( i );
        }
    }
    ASSERT( false );
    return Type::Count;
}

//------------------------------------------------------------------------------
/*static*/ const AString & BFFKeyword::GetString( Type type )
{
    static_assert( ARRAY_SIZE( sKeywordStringLookup ) == static_cast<size_t>( Type::Count ) );

    ASSERT( static_cast<size_t>( type ) < ARRAY_SIZE( sKeywordStringLookup ) );
    return sKeywordStringLookup[ static_cast<size_t>( type ) ];
}

//------------------------------------------------------------------------------
