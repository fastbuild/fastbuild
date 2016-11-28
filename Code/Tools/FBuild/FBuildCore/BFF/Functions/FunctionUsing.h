// FunctionUsing
//------------------------------------------------------------------------------
#pragma once

// Includes
//------------------------------------------------------------------------------
#include "Function.h"

// FunctionAlias
//------------------------------------------------------------------------------
class FunctionUsing : public Function
{
public:
    explicit        FunctionUsing();
    inline virtual ~FunctionUsing() = default;

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
};

//------------------------------------------------------------------------------
