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
    inline virtual ~FunctionPrint() = default;

protected:
    virtual bool AcceptsHeader() const override;
    virtual bool NeedsHeader() const override;
    virtual bool NeedsBody() const override;

    virtual bool ParseFunction( NodeGraph & nodeGraph,
                                const BFFIterator & functionNameStart,
                                const BFFIterator * functionBodyStartToken,
                                const BFFIterator * functionBodyStopToken,
                                const BFFIterator * functionHeaderStartToken,
                                const BFFIterator * functionHeaderStopToken ) const override;

    static void PrintVarRecurse( const BFFVariable & var, uint32_t indent );
};

//------------------------------------------------------------------------------
