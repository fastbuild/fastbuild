// FunctionExecutable
//------------------------------------------------------------------------------
#pragma once

// Includes
//------------------------------------------------------------------------------
#include "Function.h"

#include "Core/Containers/Array.h"

// Fwd Declarations
//------------------------------------------------------------------------------
class AString;
class Dependencies;
class Node;

// FunctionExecutable
//------------------------------------------------------------------------------
class FunctionExecutable : public Function
{
public:
    explicit        FunctionExecutable();
    inline virtual ~FunctionExecutable() override = default;

protected:
    virtual bool AcceptsHeader() const override;
    virtual Node * CreateNode() const override;
};

//------------------------------------------------------------------------------
