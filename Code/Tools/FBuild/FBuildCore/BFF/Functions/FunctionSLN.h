// FunctionSLN
//------------------------------------------------------------------------------
#pragma once

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
    explicit         FunctionSLN();
    inline virtual  ~FunctionSLN() = default;

protected:
    virtual bool AcceptsHeader() const override;

    virtual bool Commit( NodeGraph & nodeGraph, const BFFIterator & funcStartIter ) const override;

    bool GetStringFromStruct( const BFFVariable * s, const char * name, AString & result ) const;
    bool GetStringOrArrayOfStringsFromStruct( const BFFIterator & iter, const BFFVariable * s, const char * name, Array< AString > & result ) const;

    VCXProjectNode * ResolveVCXProject( NodeGraph & nodeGraph, const BFFIterator & iter, const AString & projectName ) const;
};

//------------------------------------------------------------------------------
