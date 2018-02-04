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
    inline virtual ~FunctionExecutable() = default;

protected:
    virtual bool AcceptsHeader() const override;
    virtual bool Commit( NodeGraph & nodeGraph, const BFFIterator & funcStartIter ) const override;
};

//------------------------------------------------------------------------------
