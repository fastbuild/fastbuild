// FunctionCompiler
//------------------------------------------------------------------------------
#pragma once
#ifndef FBUILD_FUNCTIONS_FUNCTIONCOMPILER_H
#define FBUILD_FUNCTIONS_FUNCTIONCOMPILER_H

// Includes
//------------------------------------------------------------------------------
#include "Function.h"

// FunctionCompiler
//------------------------------------------------------------------------------
class FunctionCompiler : public Function
{
public:
	explicit		FunctionCompiler();
	inline virtual ~FunctionCompiler() {}

protected:
	virtual bool AcceptsHeader() const;
	virtual bool NeedsHeader() const;
	virtual bool Commit( const BFFIterator & funcStartIter ) const;
};

//------------------------------------------------------------------------------
#endif // FBUILD_FUNCTIONS_FUNCTIONCOMPILER_H
