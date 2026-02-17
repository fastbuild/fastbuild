// FunctionUsing
//------------------------------------------------------------------------------
#pragma once
#ifndef FBUILD_FUNCTIONS_FUNCTIONEVAL_H
#define FBUILD_FUNCTIONS_FUNCTIONEVAL_H

// Includes
//------------------------------------------------------------------------------
#include "Function.h"

// Forward Declare
//------------------------------------------------------------------------------
class BFFStackFrame;
class BFFVariable;

// FunctionEval
//------------------------------------------------------------------------------
class FunctionEval : public Function
{
public:
    explicit        FunctionEval();
    inline virtual ~FunctionEval() {}

protected:
    virtual bool AcceptsHeader() const;
    virtual bool NeedsHeader() const;
    virtual bool NeedsBody() const;

    virtual bool ParseFunction( NodeGraph & nodeGraph,
                                BFFParser & parser,
                                const BFFToken * functionNameStart,
                                const BFFTokenRange & headerRange,
                                const BFFTokenRange & bodyRange ) const;

private:
    bool PerformVariableSubstitutions( const BFFToken & iter,
                                       const BFFVariable * v,
                                       BFFStackFrame * frame ) const;
};

//------------------------------------------------------------------------------
#endif // FBUILD_FUNCTIONS_FUNCTIONEVAL_H
