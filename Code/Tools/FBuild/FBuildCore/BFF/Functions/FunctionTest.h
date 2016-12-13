// FunctionTest
//------------------------------------------------------------------------------
#pragma once

// Includes
//------------------------------------------------------------------------------
#include "Function.h"

// FunctionTest
//------------------------------------------------------------------------------
class FunctionTest : public Function
{
public:
    explicit        FunctionTest();
    inline virtual ~FunctionTest() = default;

protected:
    virtual bool AcceptsHeader() const override;
    virtual bool Commit( NodeGraph & nodeGraph, const BFFIterator & funcStartIter ) const override;
};

//------------------------------------------------------------------------------
