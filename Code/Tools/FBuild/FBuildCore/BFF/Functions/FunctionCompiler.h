// FunctionCompiler
//------------------------------------------------------------------------------
#pragma once

// Includes
//------------------------------------------------------------------------------
#include "Function.h"

// FunctionCompiler
//------------------------------------------------------------------------------
class FunctionCompiler : public Function
{
public:
    explicit        FunctionCompiler();
    inline virtual ~FunctionCompiler() = default;

protected:
    virtual bool AcceptsHeader() const override;
    virtual bool NeedsHeader() const override;
    virtual bool Commit( NodeGraph & nodeGraph, const BFFIterator & funcStartIter ) const override;
};

//------------------------------------------------------------------------------
