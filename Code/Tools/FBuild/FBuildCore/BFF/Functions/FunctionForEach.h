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
    inline virtual ~FunctionForEach() = default;

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
