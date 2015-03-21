// FunctionTest
//------------------------------------------------------------------------------
#pragma once
#ifndef FBUILD_FUNCTIONS_FUNCTIONTEST_H
#define FBUILD_FUNCTIONS_FUNCTIONTEST_H

// Includes
//------------------------------------------------------------------------------
#include "Function.h"

// FunctionTest
//------------------------------------------------------------------------------
class FunctionTest : public Function
{
public:
	explicit		FunctionTest();
	inline virtual ~FunctionTest() {}

protected:
	virtual bool AcceptsHeader() const;
	virtual bool Commit( const BFFIterator & funcStartIter ) const;
};

//------------------------------------------------------------------------------
#endif // FBUILD_FUNCTIONS_FUNCTIONTEST_H
