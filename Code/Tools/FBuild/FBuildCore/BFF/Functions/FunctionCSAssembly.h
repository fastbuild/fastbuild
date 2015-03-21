// FunctionCSAssembly
//------------------------------------------------------------------------------
#pragma once
#ifndef FBUILD_FUNCTIONS_FUNCTIONCSASSEMBLY_H
#define FBUILD_FUNCTIONS_FUNCTIONCSASSEMBLY_H

// Includes
//------------------------------------------------------------------------------
#include "Function.h"

// FunctionCSAssembly
//------------------------------------------------------------------------------
class FunctionCSAssembly : public Function
{
public:
	explicit		FunctionCSAssembly();
	inline virtual ~FunctionCSAssembly() {}

protected:
	virtual bool AcceptsHeader() const;

	virtual bool Commit( const BFFIterator & funcStartIter ) const;
};

//------------------------------------------------------------------------------
#endif // FBUILD_FUNCTIONS_FUNCTIONCSASSEMBLY_H
