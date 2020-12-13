// FunctionIf
//------------------------------------------------------------------------------
#pragma once

// Includes
//------------------------------------------------------------------------------
#include "Function.h"

// FunctionIf
//------------------------------------------------------------------------------
class FunctionIf : public Function
{
public:
    explicit        FunctionIf();
    inline virtual ~FunctionIf() override = default;

    virtual bool AcceptsHeader() const override;
    virtual bool NeedsHeader() const override;
    virtual bool ParseFunction( NodeGraph & nodeGraph,
                                BFFParser & parser,
                                const BFFToken * functionNameStart,
                                const BFFTokenRange & headerRange,
                                const BFFTokenRange & bodyRange ) const override;

};

//------------------------------------------------------------------------------
