// FunctionRemoveDir
//------------------------------------------------------------------------------
#pragma once
#ifndef FBUILD_FUNCTIONS_FUNCTIONREMOVEDIR_H
#define FBUILD_FUNCTIONS_FUNCTIONREMOVEDIR_H

// Includes
//------------------------------------------------------------------------------
#include "Function.h"

// FunctionRemoveDir
//------------------------------------------------------------------------------
class FunctionRemoveDir : public Function
{
public:
    explicit        FunctionRemoveDir();
    inline virtual ~FunctionRemoveDir() {}

protected:
    virtual bool AcceptsHeader() const override;
    virtual bool Commit( NodeGraph & nodeGraph, const BFFIterator & funcStartIter ) const override;
};

//------------------------------------------------------------------------------
#endif // FBUILD_FUNCTIONS_FUNCTIONREMOVEDIR_H
