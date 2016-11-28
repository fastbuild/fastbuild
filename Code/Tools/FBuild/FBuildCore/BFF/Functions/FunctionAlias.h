// FunctionAlias
//------------------------------------------------------------------------------
#pragma once

// Includes
//------------------------------------------------------------------------------
#include "Function.h"

// FunctionAlias
//------------------------------------------------------------------------------
class FunctionAlias : public Function
{
public:
    explicit        FunctionAlias();
    inline virtual ~FunctionAlias() = default;

protected:
    virtual bool AcceptsHeader() const override;
    virtual bool NeedsHeader() const override;
    virtual bool Commit( NodeGraph & nodeGraph, const BFFIterator & funcStartIter ) const override;
};

//------------------------------------------------------------------------------
