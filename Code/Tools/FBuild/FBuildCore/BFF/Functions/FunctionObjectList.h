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
    bool GetBaseDirectory( const BFFIterator & iter, AStackString<> & baseDirectory ) const;

    virtual bool AcceptsHeader() const override;
    virtual bool NeedsHeader() const override;

    virtual bool Commit( NodeGraph & nodeGraph, const BFFIterator & funcStartIter ) const override;

    // helpers
    bool    GetCompilerNode( NodeGraph & nodeGraph, const BFFIterator & iter, const AString & compiler, CompilerNode * & compilerNode ) const;
    bool    GetPrecompiledHeaderNode( NodeGraph & nodeGraph,
                                      const BFFIterator & iter,
                                      CompilerNode * compilerNode,
                                      const BFFVariable * compilerOptions,
                                      const Dependencies & compilerForceUsing,
                                      ObjectNode * & precompiledHeaderNode,
                                      bool deoptimizeWritableFiles,
                                      bool deoptimizeWritableFilesWithToken,
                                      bool allowDistribution,
                                      bool allowCaching,
                                      const AString & compilerOutputExtension ) const;
    bool    GetInputs( NodeGraph & nodeGraph, const BFFIterator & iter, Dependencies & inputs ) const;
    void    GetExtraOutputPaths( const AString & args, AString & pdbPath, AString & asmPath ) const;
    void    GetExtraOutputPath( const AString * it, const AString * end, const char * option, AString & path ) const;
};

//------------------------------------------------------------------------------
