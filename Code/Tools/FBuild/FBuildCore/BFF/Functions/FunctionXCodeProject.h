// FunctionXCodeProject
//------------------------------------------------------------------------------
#pragma once

// Includes
//------------------------------------------------------------------------------
#include "Function.h"

// Forward Declarations
//------------------------------------------------------------------------------
class BFFIterator;

// FunctionXCodeProject
//------------------------------------------------------------------------------
class FunctionXCodeProject : public Function
{
public:
    explicit        FunctionXCodeProject();
    inline virtual ~FunctionXCodeProject() = default;

protected:
    virtual bool AcceptsHeader() const override;

    virtual Node * CreateNode() const override;
};

//------------------------------------------------------------------------------
