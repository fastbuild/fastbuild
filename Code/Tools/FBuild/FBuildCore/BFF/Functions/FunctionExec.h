// FunctionExec
//------------------------------------------------------------------------------
#pragma once

// Includes
//------------------------------------------------------------------------------
#include "Function.h"

// FunctionExec
//------------------------------------------------------------------------------
class FunctionExec : public Function
{
public:
    explicit        FunctionExec();
    inline virtual ~FunctionExec() = default;

protected:
    virtual bool AcceptsHeader() const override;
    virtual bool Commit( NodeGraph & nodeGraph, const BFFIterator & funcStartIter ) const override;
};

//------------------------------------------------------------------------------
