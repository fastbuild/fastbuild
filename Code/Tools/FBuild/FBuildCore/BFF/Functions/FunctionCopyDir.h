// FunctionCopyDir
//------------------------------------------------------------------------------
#pragma once

// Includes
//------------------------------------------------------------------------------
#include "Function.h"

// Core
//#include "Core/Containers/Array.h"

// FunctionCopyDir
//------------------------------------------------------------------------------
class FunctionCopyDir : public Function
{
public:
    explicit        FunctionCopyDir();
    inline virtual ~FunctionCopyDir() = default;

protected:
    virtual bool AcceptsHeader() const override;
    virtual bool Commit( NodeGraph & nodeGraph, const BFFIterator & funcStartIter ) const override;
};

//------------------------------------------------------------------------------
