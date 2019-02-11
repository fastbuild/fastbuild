// FunctionSLN
//------------------------------------------------------------------------------
#pragma once

// Includes
//------------------------------------------------------------------------------
#include "Function.h"

// Forward Declarations
//------------------------------------------------------------------------------
class BFFIterator;

// FunctionSLN
//------------------------------------------------------------------------------
class FunctionSLN : public Function
{
public:
    explicit         FunctionSLN();
    inline virtual  ~FunctionSLN() override = default;

protected:
    virtual bool AcceptsHeader() const override;
    virtual Node *CreateNode() const override;
};

//------------------------------------------------------------------------------
