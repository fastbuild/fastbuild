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

    bool HandleSimpleBooleanExpression( BFFParser & parser,
                                        const BFFTokenRange & headerRange,
                                        const BFFTokenRange & bodyRange,
                                        const BFFToken * testVarIter,
                                        const BFFVariable * testVar,
                                        const bool negated ) const;
    bool HandleIn( BFFParser & parser,
                   const BFFTokenRange & bodyRange,
                   const BFFToken * lhsVarIter,
                   const BFFVariable * lhsVar,
                   const BFFToken * rhsVarIter,
                   const BFFVariable * rhsVar,
                   const Operator op ) const;
    bool HandleSimpleCompare( BFFParser & parser,
                              const BFFTokenRange & bodyRange,
                              const BFFToken * lhsVarIter,
                              const BFFVariable * lhsVar,
                              const BFFToken * rhsVarIter,
                              const BFFVariable * rhsVar,
                              const BFFToken * operatorIter,
                              const Operator op ) const;

    const BFFVariable * GetVar( const BFFToken * token ) const;
};

//------------------------------------------------------------------------------
