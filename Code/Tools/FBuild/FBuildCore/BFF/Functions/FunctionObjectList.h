// FunctionObjectList
//------------------------------------------------------------------------------
#pragma once

// Includes
//------------------------------------------------------------------------------
#include "Function.h"

// Forward Declarations
//------------------------------------------------------------------------------
class CompilerNode;
class Dependencies;
class ObjectNode;

// FunctionObjectList
//------------------------------------------------------------------------------
class FunctionObjectList : public Function
{
public:
    explicit        FunctionObjectList();
    inline virtual ~FunctionObjectList() = default;

protected:
    virtual bool AcceptsHeader() const override;
    virtual bool NeedsHeader() const override;
    virtual Node * CreateNode() const override;

    // helpers
    friend class ObjectNode; // TODO:C Remove
    friend class ObjectListNode; // TODO:C Remove
    bool    CheckCompilerOptions( const BFFIterator & iter, const AString & compilerOptions, const uint32_t objFlags ) const;
    bool    CheckMSVCPCHFlags( const BFFIterator & iter,
                               const AString & compilerOptions,
                               const AString & pchOptions,
                               const AString & pchOutputFile,
                               const char * compilerOutputExtension,
                               AString & pchObjectName ) const;
    void    GetExtraOutputPaths( const AString & args, AString & pdbPath, AString & asmPath ) const;
    void    GetExtraOutputPath( const AString * it, const AString * end, const char * option, AString & path ) const;
};

//------------------------------------------------------------------------------
