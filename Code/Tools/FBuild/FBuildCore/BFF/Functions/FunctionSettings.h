// FunctionSettings - Manage global settings
//------------------------------------------------------------------------------
#pragma once

// Includes
//------------------------------------------------------------------------------
#include "Function.h"

// FunctionSettings
//------------------------------------------------------------------------------
class FunctionSettings : public Function
{
public:
    explicit        FunctionSettings();
    inline virtual ~FunctionSettings() = default;

protected:
    virtual bool IsUnique() const override;
    virtual bool Commit( NodeGraph & nodeGraph, const BFFIterator & funcStartIter ) const override;
};

//------------------------------------------------------------------------------
