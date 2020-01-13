// FunctionCopy
//------------------------------------------------------------------------------
#pragma once

// Includes
//------------------------------------------------------------------------------
#include "Function.h"

// Core
#include "Core/Containers/Array.h"

// FunctionCopy
//------------------------------------------------------------------------------
class FunctionCopy : public Function
{
public:
    explicit        FunctionCopy();
    inline virtual ~FunctionCopy() override = default;

protected:
    virtual bool AcceptsHeader() const override;
    virtual bool Commit( NodeGraph & nodeGraph, const BFFToken * funcStartIter ) const override;

    bool GetSourceNodes( const BFFToken * iter, Node * node, Array< Node * > & nodes ) const;
};

//------------------------------------------------------------------------------
