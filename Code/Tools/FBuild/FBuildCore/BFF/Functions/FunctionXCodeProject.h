// FunctionXCodeProject
//------------------------------------------------------------------------------
#pragma once

// Includes
//------------------------------------------------------------------------------
#include "Function.h"

// Forward Declarations
//------------------------------------------------------------------------------
class BFFIterator;

// FunctionXCodeProject
//------------------------------------------------------------------------------
class FunctionXCodeProject : public Function
{
public:
	explicit		FunctionXCodeProject();
	inline virtual ~FunctionXCodeProject() {}

protected:
	virtual bool AcceptsHeader() const;

	virtual bool Commit( const BFFIterator & funcStartIter ) const;
};

//------------------------------------------------------------------------------
