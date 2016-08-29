// FunctionUnity
//------------------------------------------------------------------------------
#pragma once
#ifndef FBUILD_FUNCTIONS_FUNCTIONUNITY_H
#define FBUILD_FUNCTIONS_FUNCTIONUNITY_H

// Includes
//------------------------------------------------------------------------------
#include "Function.h"

class FunctionUnity : public Function
{
public:
	explicit		FunctionUnity();
	inline virtual ~FunctionUnity() {}

protected:
	virtual bool AcceptsHeader() const override;
	virtual bool NeedsHeader() const override;

	virtual bool Commit( NodeGraph & nodeGraph, const BFFIterator & funcStartIter ) const override;
};

//------------------------------------------------------------------------------
#endif // FBUILD_FUNCTIONS_FUNCTIONUNITY_H
