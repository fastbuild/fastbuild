// FunctionPrint
//------------------------------------------------------------------------------
#pragma once
#ifndef FBUILD_FUNCTIONS_FUNCTIONPRINT_H
#define FBUILD_FUNCTIONS_FUNCTIONPRINT_H

// Includes
//------------------------------------------------------------------------------
#include "Function.h"

// FunctionAlias
//------------------------------------------------------------------------------
class FunctionPrint : public Function
{
public:
	explicit		FunctionPrint();
	inline virtual ~FunctionPrint() {}

protected:
	virtual bool AcceptsHeader() const;
	virtual bool NeedsHeader() const;
	virtual bool NeedsBody() const;

	virtual bool ParseFunction( const BFFIterator & functionNameStart,
								const BFFIterator * functionBodyStartToken, 
								const BFFIterator * functionBodyStopToken,
								const BFFIterator * functionHeaderStartToken,
								const BFFIterator * functionHeaderStopToken ) const;

	static void PrintVarRecurse( const BFFVariable & var, uint32_t indent );
};

//------------------------------------------------------------------------------
#endif // FBUILD_FUNCTIONS_FUNCTIONPRINT_H
