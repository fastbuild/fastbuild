// FunctionDLL
//------------------------------------------------------------------------------
#pragma once

// Includes
//------------------------------------------------------------------------------
#include "FunctionExecutable.h"

// FunctionDLL
//------------------------------------------------------------------------------
class FunctionDLL : public FunctionExecutable
{
public:
    explicit        FunctionDLL();
    inline virtual ~FunctionDLL() override = default;
    virtual Node *  CreateNode() const override;
};

//------------------------------------------------------------------------------
