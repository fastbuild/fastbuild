// FunctionDLL
//------------------------------------------------------------------------------
#pragma once
#ifndef FBUILD_FUNCTIONS_FUNCTIONDLL_H
#define FBUILD_FUNCTIONS_FUNCTIONDLL_H

// Includes
//------------------------------------------------------------------------------
#include "FunctionExecutable.h"

// FunctionDLL
//------------------------------------------------------------------------------
class FunctionDLL : public FunctionExecutable
{
public:
	explicit		FunctionDLL();
	inline virtual ~FunctionDLL() {}
};

//------------------------------------------------------------------------------
#endif // FBUILD_FUNCTIONS_FUNCTIONDLL_H
