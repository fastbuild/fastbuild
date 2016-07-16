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
	virtual bool AcceptsHeader() const override;
	virtual bool NeedsHeader() const override;
	virtual bool Commit( NodeGraph & nodeGraph, const BFFIterator & funcStartIter ) const override;
};

//------------------------------------------------------------------------------
#endif // FBUILD_FUNCTIONS_FUNCTIONCOMPILER_H
