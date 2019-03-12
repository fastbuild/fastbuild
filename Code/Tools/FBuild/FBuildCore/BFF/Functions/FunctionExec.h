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
    virtual Node * CreateNode() const override;
};

//------------------------------------------------------------------------------
