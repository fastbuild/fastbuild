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
                                const BFFIterator & functionNameStart,
                                const BFFIterator * functionBodyStartToken,
                                const BFFIterator * functionBodyStopToken,
                                const BFFIterator * functionHeaderStartToken,
                                const BFFIterator * functionHeaderStopToken ) const override;

protected:
    enum class Operator
    {
        OP_UNKNOWN,

        OP_IN,
        OP_NOT_IN,

        OP_EQUAL,
        OP_NOT_EQUAL,
        OP_LESS_THAN,
        OP_LESS_THAN_OR_EQUAL,
        OP_GREATER_THAN,
        OP_GREATER_THAN_OR_EQUAL,
    };

    bool HandleSimpleBooleanExpression( NodeGraph & nodeGraph,
                                        const BFFIterator * functionBodyStartToken,
                                        const BFFIterator * functionBodyStopToken,
                                        const BFFIterator & testVarIter,
                                        const BFFVariable * testVar,
                                        const bool negated ) const;
    bool HandleIn( NodeGraph & nodeGraph,
                   const BFFIterator * functionBodyStartToken,
                   const BFFIterator * functionBodyStopToken,
                   const BFFIterator & lhsVarIter,
                   const BFFVariable * lhsVar,
                   const BFFIterator & rhsVarIter,
                   const BFFVariable * rhsVar,
                   const Operator op ) const;
    bool HandleSimpleCompare( NodeGraph & nodeGraph,
                              const BFFIterator * functionBodyStartToken,
                              const BFFIterator * functionBodyStopToken,
                              const BFFIterator & lhsVarIter,
                              const BFFVariable * lhsVar,
                              const BFFIterator & rhsVarIter,
                              const BFFVariable * rhsVar,
                              const BFFIterator & operatorIter,
                              const Operator op ) const;

    const BFFVariable * GetVar( BFFIterator & pos ) const;
};

//------------------------------------------------------------------------------
