// FunctionExec
//------------------------------------------------------------------------------
#pragma once
#ifndef FBUILD_FUNCTIONS_FUNCTIONEXEC_H
#define FBUILD_FUNCTIONS_FUNCTIONEXEC_H

// Includes
//------------------------------------------------------------------------------
#include "Function.h"

// FunctionExec
//------------------------------------------------------------------------------
class FunctionExec : public Function
{
public:
	explicit		FunctionExec();
	inline virtual ~FunctionExec() {}

protected:
	virtual bool AcceptsHeader() const;
	virtual bool Commit( const BFFIterator & funcStartIter ) const;
};

//------------------------------------------------------------------------------
#endif // FBUILD_FUNCTIONS_FUNCTIONEXEC_H
