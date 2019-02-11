// FunctionAlias
//------------------------------------------------------------------------------
#pragma once

// Includes
//------------------------------------------------------------------------------
#include "Function.h"

// FunctionAlias
//------------------------------------------------------------------------------
class FunctionAlias : public Function
{
public:
    explicit        FunctionAlias();
    inline virtual ~FunctionAlias() override = default;

protected:
    virtual bool AcceptsHeader() const override;
    virtual bool NeedsHeader() const override;
    virtual Node * CreateNode() const override;
};

//------------------------------------------------------------------------------
