// FunctionWorkerSettings - Manage worker settings
//------------------------------------------------------------------------------
#pragma once

// Includes
//------------------------------------------------------------------------------
#include "Function.h"

// FunctionWorkerSettings
//------------------------------------------------------------------------------
class FunctionWorkerSettings : public Function
{
public:
    explicit        FunctionWorkerSettings();
    inline virtual ~FunctionWorkerSettings() = default;

protected:
    virtual bool IsUnique() const override;
    virtual bool Commit( NodeGraph & nodeGraph, const BFFIterator & funcStartIter ) const override;
};

//------------------------------------------------------------------------------
