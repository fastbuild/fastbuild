// FunctionForEach
//------------------------------------------------------------------------------
#pragma once

// Includes
//------------------------------------------------------------------------------
#include "Function.h"

// FunctionForEach
//------------------------------------------------------------------------------
class FunctionForEach : public Function
{
public:
    explicit        FunctionForEach();
    inline virtual ~FunctionForEach() override = default;

    virtual bool AcceptsHeader() const override;
    virtual bool NeedsHeader() const override;
    virtual bool ParseFunction( NodeGraph & nodeGraph,
                                BFFParser & parser,
                                const BFFToken * functionNameStart,
                                const BFFTokenRange & headerRange,
                                const BFFTokenRange & bodyRange ) const override;
};

//------------------------------------------------------------------------------
