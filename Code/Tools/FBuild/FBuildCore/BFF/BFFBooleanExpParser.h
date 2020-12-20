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
	// Parse
	// The entry function for the parser.
	// The result will be set to `expResult`.
	// Returns a bool indicating whether parsed successfully.
	//------------------------------------------------------------------------------
	static bool Parse( const Function * function, const BFFTokenRange & range, bool & expResult );
};