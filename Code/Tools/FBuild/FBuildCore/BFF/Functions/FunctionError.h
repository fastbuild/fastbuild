// FunctionError
//------------------------------------------------------------------------------
#pragma once

// Includes
//------------------------------------------------------------------------------
#include "Function.h"

// FunctionError
//------------------------------------------------------------------------------
class FunctionError : public Function
{
public:
    explicit        FunctionError();
    inline virtual ~FunctionError() override = default;

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
