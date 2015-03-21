// FunctionUsing
//------------------------------------------------------------------------------
#pragma once
#ifndef FBUILD_FUNCTIONS_FUNCTIONUSING_H
#define FBUILD_FUNCTIONS_FUNCTIONUSING_H

// Includes
//------------------------------------------------------------------------------
#include "Function.h"

// FunctionAlias
//------------------------------------------------------------------------------
class FunctionUsing : public Function
{
public:
	explicit		FunctionUsing();
	inline virtual ~FunctionUsing() {}

protected:
	virtual bool AcceptsHeader() const;
	virtual bool NeedsHeader() const;
	virtual bool NeedsBody() const;

	virtual bool ParseFunction( const BFFIterator & functionNameStart,
								const BFFIterator * functionBodyStartToken, 
								const BFFIterator * functionBodyStopToken,
								const BFFIterator * functionHeaderStartToken,
								const BFFIterator * functionHeaderStopToken ) const;
};

//------------------------------------------------------------------------------
#endif // FBUILD_FUNCTIONS_FUNCTIONUSING_H
