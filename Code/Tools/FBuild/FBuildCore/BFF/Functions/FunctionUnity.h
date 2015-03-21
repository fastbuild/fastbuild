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
	virtual bool AcceptsHeader() const;
	virtual bool NeedsHeader() const;

	virtual bool Commit( const BFFIterator & funcStartIter ) const;
};

//------------------------------------------------------------------------------
#endif // FBUILD_FUNCTIONS_FUNCTIONUNITY_H
