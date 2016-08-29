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
	virtual bool AcceptsHeader() const override;

	virtual bool Commit( NodeGraph & nodeGraph, const BFFIterator & funcStartIter ) const override;
};

//------------------------------------------------------------------------------
#endif // FBUILD_FUNCTIONS_FUNCTIONCSASSEMBLY_H
