// FunctionCSAssembly
//------------------------------------------------------------------------------
#pragma once

// Includes
//------------------------------------------------------------------------------
#include "Function.h"

// FunctionCSAssembly
//------------------------------------------------------------------------------
class FunctionCSAssembly : public Function
{
public:
    explicit FunctionCSAssembly();
    virtual ~FunctionCSAssembly() override = default;

protected:
    virtual bool AcceptsHeader() const override;
    virtual Node * CreateNode() const override;
};

//------------------------------------------------------------------------------
