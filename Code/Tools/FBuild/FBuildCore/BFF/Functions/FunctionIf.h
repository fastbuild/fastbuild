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
    inline virtual ~FunctionIf() = default;

    virtual bool AcceptsHeader() const override;
    virtual bool NeedsHeader() const override;
    virtual bool ParseFunction( NodeGraph & nodeGraph,
                                const BFFIterator & functionNameStart,
                                const BFFIterator * functionBodyStartToken,
                                const BFFIterator * functionBodyStopToken,
                                const BFFIterator * functionHeaderStartToken,
                                const BFFIterator * functionHeaderStopToken ) const override;
};

//------------------------------------------------------------------------------
