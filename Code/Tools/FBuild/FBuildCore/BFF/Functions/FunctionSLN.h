// FunctionSLN
//------------------------------------------------------------------------------
#pragma once
#ifndef FBUILD_FUNCTIONS_FUNCTIONSLN_H
#define FBUILD_FUNCTIONS_FUNCTIONSLN_H

// Includes
//------------------------------------------------------------------------------
#include "Function.h"

// Forward Declarations
//------------------------------------------------------------------------------
class BFFIterator;
class VCXProjectNode;

// FunctionSLN
//------------------------------------------------------------------------------
class FunctionSLN : public Function
{
public:
    explicit        FunctionSLN();
    inline virtual ~FunctionSLN() {}

protected:
    virtual bool AcceptsHeader() const;

    virtual bool Commit( const BFFIterator & funcStartIter ) const;

    bool GetStringFromStruct( const BFFVariable * s, const char * name, AString & result ) const;
    bool GetStringOrArrayOfStringsFromStruct( const BFFIterator & iter, const BFFVariable * s, const char * name, Array< AString > & result ) const;

	VCXProjectNode * ResolveVCXProject( const BFFIterator & iter, const AString & projectName ) const;
};

//------------------------------------------------------------------------------
#endif // FBUILD_FUNCTIONS_FUNCTIONSLN_H
