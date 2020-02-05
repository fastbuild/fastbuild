// FunctionVCXProject
//------------------------------------------------------------------------------
#pragma once

// Includes
//------------------------------------------------------------------------------
#include "Function.h"

// Forward Declarations
//------------------------------------------------------------------------------

// FunctionVCXProject
//------------------------------------------------------------------------------
class FunctionVCXProject : public Function
{
public:
    explicit        FunctionVCXProject();
    inline virtual ~FunctionVCXProject() override = default;

protected:
    virtual bool AcceptsHeader() const override;
    virtual Node * CreateNode() const override;
};

//------------------------------------------------------------------------------
