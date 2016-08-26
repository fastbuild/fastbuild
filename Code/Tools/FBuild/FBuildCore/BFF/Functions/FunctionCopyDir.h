// FunctionCopyDir
//------------------------------------------------------------------------------
#pragma once
#ifndef FBUILD_FUNCTIONS_FUNCTIONCOPYDIR_H
#define FBUILD_FUNCTIONS_FUNCTIONCOPYDIR_H

// Includes
//------------------------------------------------------------------------------
#include "Function.h"

// Core
//#include "Core/Containers/Array.h"

// FunctionCopyDir
//------------------------------------------------------------------------------
class FunctionCopyDir : public Function
{
public:
	explicit		FunctionCopyDir();
	inline virtual ~FunctionCopyDir() {}

protected:
	virtual bool AcceptsHeader() const override;
	virtual bool Commit( NodeGraph & nodeGraph, const BFFIterator & funcStartIter ) const override;
};

//------------------------------------------------------------------------------
#endif // FBUILD_FUNCTIONS_FUNCTIONCOPYDIR_H
