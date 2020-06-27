// FunctionPrint
//------------------------------------------------------------------------------
#pragma once

// Includes
//------------------------------------------------------------------------------
#include "Function.h"

// FunctionAlias
//------------------------------------------------------------------------------
class FunctionPrint : public Function
{
public:
    explicit        FunctionPrint();
    inline virtual ~FunctionPrint() override = default;

protected:
    virtual bool AcceptsHeader() const override;
    virtual bool NeedsHeader() const override;
    virtual bool NeedsBody() const override;

    virtual bool ParseFunction( NodeGraph & nodeGraph,
                                BFFParser & parser,
                                const BFFToken * functionNameStart,
                                const BFFTokenRange & headerRange,
                                const BFFTokenRange & bodyRange ) const override;

    static void PrintVarRecurse( const BFFVariable & var, uint32_t indent );
};

//------------------------------------------------------------------------------
