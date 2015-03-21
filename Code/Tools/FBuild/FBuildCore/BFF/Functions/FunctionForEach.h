// FunctionForEach
//------------------------------------------------------------------------------
#pragma once
#ifndef FBUILD_FUNCTIONS_FUNCTIONFOREACH_H
#define FBUILD_FUNCTIONS_FUNCTIONFOREACH_H

// Includes
//------------------------------------------------------------------------------
#include "Function.h"

// FunctionForEach
//------------------------------------------------------------------------------
class FunctionForEach : public Function
{
public:
	explicit		FunctionForEach();
	inline virtual ~FunctionForEach() {}

	virtual bool AcceptsHeader() const;
	virtual bool NeedsHeader() const;
	virtual bool ParseFunction( const BFFIterator & functionNameStart,
								const BFFIterator * functionBodyStartToken, 
								const BFFIterator * functionBodyStopToken,
								const BFFIterator * functionHeaderStartToken,
								const BFFIterator * functionHeaderStopToken ) const;
};

//------------------------------------------------------------------------------
#endif // FBUILD_FUNCTIONS_FUNCTIONFOREACH_H
