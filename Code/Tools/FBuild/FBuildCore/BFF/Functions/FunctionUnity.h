// FunctionUnity
//------------------------------------------------------------------------------
#pragma once

// Includes
//------------------------------------------------------------------------------
#include "Function.h"

class FunctionUnity : public Function
{
public:
    explicit        FunctionUnity();
    inline virtual ~FunctionUnity() override = default;

protected:
    virtual bool AcceptsHeader() const override;
    virtual bool NeedsHeader() const override;
    virtual Node * CreateNode() const override;
};

//------------------------------------------------------------------------------
