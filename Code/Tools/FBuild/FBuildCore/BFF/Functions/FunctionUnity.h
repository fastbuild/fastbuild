// FunctionUnity
//------------------------------------------------------------------------------
#pragma once

// Includes
//------------------------------------------------------------------------------
#include "Function.h"

class FunctionUnity : public Function
{
public:
    explicit        FunctionUnity();
    inline virtual ~FunctionUnity() {}

protected:
    virtual bool AcceptsHeader() const override;
    virtual bool NeedsHeader() const override;

    virtual bool Commit( NodeGraph & nodeGraph, const BFFIterator & funcStartIter ) const override;
};

//------------------------------------------------------------------------------
