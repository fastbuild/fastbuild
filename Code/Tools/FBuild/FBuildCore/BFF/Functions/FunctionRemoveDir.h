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
    inline virtual ~FunctionRemoveDir() override = default;

protected:
    virtual bool AcceptsHeader() const override;
    virtual bool NeedsHeader() const override;
    virtual bool Commit( NodeGraph & nodeGraph, const BFFToken * funcStartIter ) const override;
    virtual Node * CreateNode() const override;
};

//------------------------------------------------------------------------------
