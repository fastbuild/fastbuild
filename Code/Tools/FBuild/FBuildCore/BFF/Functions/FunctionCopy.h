// FunctionCopy
//------------------------------------------------------------------------------
#pragma once
#ifndef FBUILD_FUNCTIONS_FUNCTIONCOPY_H
#define FBUILD_FUNCTIONS_FUNCTIONCOPY_H

// Includes
//------------------------------------------------------------------------------
#include "Function.h"

// Core
#include "Core/Containers/Array.h"

// FunctionCopy
//------------------------------------------------------------------------------
class FunctionCopy : public Function
{
public:
	explicit		FunctionCopy();
	inline virtual ~FunctionCopy() {}

protected:
	virtual bool AcceptsHeader() const override;
	virtual bool Commit( NodeGraph & nodeGraph, const BFFIterator & funcStartIter ) const override;

	bool GetSourceNodes( const BFFIterator & iter, Node * node, Array< Node * > & nodes ) const;
};

//------------------------------------------------------------------------------
#endif // FBUILD_FUNCTIONS_FUNCTIONCOPY_H
