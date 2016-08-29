// FunctionLibrary
//------------------------------------------------------------------------------
#pragma once
#ifndef FBUILD_FUNCTIONS_FUNCTIONLIBRARY_H
#define FBUILD_FUNCTIONS_FUNCTIONLIBRARY_H

// Includes
//------------------------------------------------------------------------------
#include "Function.h"
#include "FunctionObjectList.h"

// FunctionLibrary
//------------------------------------------------------------------------------
class FunctionLibrary : public FunctionObjectList
{
public:
	explicit		FunctionLibrary();
	inline virtual ~FunctionLibrary() {}

protected:
	virtual bool AcceptsHeader() const override;
	virtual bool NeedsHeader() const override;

	virtual bool Commit( NodeGraph & nodeGraph, const BFFIterator & funcStartIter ) const override;
};

//------------------------------------------------------------------------------
#endif // FBUILD_FUNCTIONS_FUNCTIONLIBRARY_H
