// FunctionVSSolution
//------------------------------------------------------------------------------
#pragma once

// Includes
//------------------------------------------------------------------------------
#include "Function.h"

// Forward Declarations
//------------------------------------------------------------------------------

// FunctionVSSolution
//------------------------------------------------------------------------------
class FunctionVSSolution : public Function
{
public:
    explicit         FunctionVSSolution();
    inline virtual  ~FunctionVSSolution() override = default;

protected:
    virtual bool AcceptsHeader() const override;
    virtual Node *CreateNode() const override;
};

//------------------------------------------------------------------------------
