// FunctionAlias
//------------------------------------------------------------------------------
#pragma once
#ifndef FBUILD_FUNCTIONS_FUNCTIONALIAS_H
#define FBUILD_FUNCTIONS_FUNCTIONALIAS_H

// Includes
//------------------------------------------------------------------------------
#include "Function.h"

// FunctionAlias
//------------------------------------------------------------------------------
class FunctionAlias : public Function
{
public:
	explicit		FunctionAlias();
	inline virtual ~FunctionAlias() {}

protected:
	virtual bool AcceptsHeader() const override;
	virtual bool NeedsHeader() const override;
	virtual bool Commit( NodeGraph & nodeGraph, const BFFIterator & funcStartIter ) const override;
};

//------------------------------------------------------------------------------
#endif // FBUILD_FUNCTIONS_FUNCTIONALIAS_H
