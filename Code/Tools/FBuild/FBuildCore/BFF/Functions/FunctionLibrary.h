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
	virtual bool AcceptsHeader() const;
	virtual bool NeedsHeader() const;

	virtual bool Commit( const BFFIterator & funcStartIter ) const;
};

//------------------------------------------------------------------------------
#endif // FBUILD_FUNCTIONS_FUNCTIONLIBRARY_H
