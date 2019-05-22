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
    inline virtual ~FunctionTest() override = default;

protected:
    virtual bool AcceptsHeader() const override;
    virtual Node * CreateNode() const override;
};

//------------------------------------------------------------------------------
