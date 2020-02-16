// FunctionVSProjectExternal
//------------------------------------------------------------------------------
#pragma once

// Includes
//------------------------------------------------------------------------------
#include "Function.h"

// Forward Declarations
//------------------------------------------------------------------------------
class BFFIterator;

// FunctionVSProjectExternal
//------------------------------------------------------------------------------
class FunctionVSProjectExternal : public Function
{
public:
    explicit        FunctionVSProjectExternal();
    inline virtual ~FunctionVSProjectExternal() override = default;

protected:
    virtual bool AcceptsHeader() const override;
    virtual Node * CreateNode() const override;
};

//------------------------------------------------------------------------------
