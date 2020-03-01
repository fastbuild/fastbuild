// FunctionTextFile
//------------------------------------------------------------------------------
#pragma once

// Includes
//------------------------------------------------------------------------------
#include "Function.h"

// FunctionTextFile
//------------------------------------------------------------------------------
class FunctionTextFile : public Function
{
public:
    explicit        FunctionTextFile();
    virtual         ~FunctionTextFile() override;

protected:
    virtual bool    AcceptsHeader() const override;
    virtual Node *  CreateNode() const override;
};

//------------------------------------------------------------------------------
