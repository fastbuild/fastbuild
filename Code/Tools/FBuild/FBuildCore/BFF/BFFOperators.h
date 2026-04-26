// BFFOperators
//------------------------------------------------------------------------------
#pragma once

// Includes
//------------------------------------------------------------------------------
#include "Core/Env/Types.h"

// Forward Declarations
//------------------------------------------------------------------------------
class AString;

// Defines
//------------------------------------------------------------------------------
#define BFF_OPERATOR_ASSIGN "="
#define BFF_OPERATOR_GREATER_THAN ">"
#define BFF_OPERATOR_LESS_THAN "<"
#define BFF_OPERATOR_NOT "!"
#define BFF_OPERATOR_PLUS "+"
#define BFF_OPERATOR_MINUS "-"
#define BFF_OPERATOR_EQUAL "=="
#define BFF_OPERATOR_NOT_EQUAL "!="
#define BFF_OPERATOR_GREATER_THAN_OR_EQUAL ">="
#define BFF_OPERATOR_LESS_THAN_OR_EQUAL "<="
#define BFF_OPERATOR_AND "&&"
#define BFF_OPERATOR_OR "||"

class BFFOperator
{
public:
    enum class Type : uint8_t
    {
        // Single
        eAssign, // =
        eGreaterThan, // >
        eLessThan, // <
        eNot, // !
        ePlus, // +
        eMinus, // -

        // Double
        eEqual, // ==
        eNotEqual, // !=
        eGreaterThanOrEqual, // >=
        eLessThanOrEqual, // <=
        eAnd, // &&
        eOr, // ||

        Count
    };

    // Map from string to enum
    [[nodiscard]] static Type GetType( const AString & string );

    // Map from enum to string
    [[nodiscard]] static const AString & GetString( Type type );

private:
    static const AString sOperatorStringLookup[];
};

//------------------------------------------------------------------------------
