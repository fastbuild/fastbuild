// FunctionSLN
//------------------------------------------------------------------------------
#pragma once

// Includes
//------------------------------------------------------------------------------
#include "Function.h"

// Forward Declarations
//------------------------------------------------------------------------------
class BFFIterator;

// FunctionSLN
//------------------------------------------------------------------------------
class FunctionSLN : public Function
{
public:
    explicit         FunctionSLN();
    inline virtual  ~FunctionSLN() = default;

protected:
    virtual bool AcceptsHeader() const override;

    virtual bool Commit( NodeGraph & nodeGraph, const BFFIterator & funcStartIter ) const override;
};

//------------------------------------------------------------------------------
