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
	virtual bool AcceptsHeader() const;
	virtual bool NeedsHeader() const;
	virtual bool Commit( const BFFIterator & funcStartIter ) const;
};

//------------------------------------------------------------------------------
#endif // FBUILD_FUNCTIONS_FUNCTIONALIAS_H
