// FunctionCSAssembly
//------------------------------------------------------------------------------
#pragma once

// Includes
//------------------------------------------------------------------------------
#include "Function.h"

// FunctionCSAssembly
//------------------------------------------------------------------------------
class FunctionCSAssembly : public Function
{
public:
    explicit        FunctionCSAssembly();
    inline virtual ~FunctionCSAssembly() = default;

protected:
    virtual bool AcceptsHeader() const override;

    virtual bool Commit( NodeGraph & nodeGraph, const BFFIterator & funcStartIter ) const override;
};

//------------------------------------------------------------------------------
