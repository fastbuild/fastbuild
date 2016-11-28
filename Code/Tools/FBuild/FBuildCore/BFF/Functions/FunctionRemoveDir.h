// FunctionRemoveDir
//------------------------------------------------------------------------------
#pragma once

// Includes
//------------------------------------------------------------------------------
#include "Function.h"

// FunctionRemoveDir
//------------------------------------------------------------------------------
class FunctionRemoveDir : public Function
{
public:
    explicit        FunctionRemoveDir();
    inline virtual ~FunctionRemoveDir() = default;

protected:
    virtual bool AcceptsHeader() const override;
    virtual bool Commit( NodeGraph & nodeGraph, const BFFIterator & funcStartIter ) const override;
};

//------------------------------------------------------------------------------
