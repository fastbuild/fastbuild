// FunctionListDependencies
//------------------------------------------------------------------------------
#pragma once

// Includes
//------------------------------------------------------------------------------
#include "Function.h"

// Core
#include "Core/Containers/Array.h"

// FunctionListDependencies
//------------------------------------------------------------------------------
class FunctionListDependencies : public Function
{
public:
    explicit        FunctionListDependencies();
    virtual         ~FunctionListDependencies() override;

protected:
    virtual bool AcceptsHeader() const override;
    virtual Node *  CreateNode() const override;

};

//------------------------------------------------------------------------------
