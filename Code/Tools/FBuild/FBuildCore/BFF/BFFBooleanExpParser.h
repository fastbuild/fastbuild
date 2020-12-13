// BFFBooleanExpParser
//------------------------------------------------------------------------------
#pragma once

// Includes
//------------------------------------------------------------------------------
#include "Core/Containers/Array.h"

// Forward Declarations
//------------------------------------------------------------------------------
class BFFToken;
class BFFTokenRange;
class Function;

// BFFBooleanExpParser
//------------------------------------------------------------------------------
class BFFBooleanExpParser
{
public:
	static bool Parse( const Function * function, const BFFTokenRange & range, bool & expResult );
};