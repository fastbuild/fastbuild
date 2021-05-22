// FunctionObjectList
//------------------------------------------------------------------------------
#pragma once

// Includes
//------------------------------------------------------------------------------
#include "Function.h"
#include <Tools/FBuild/FBuildCore/Graph/ObjectNode.h>

// Forward Declarations
//------------------------------------------------------------------------------
class CompilerNode;
class Dependencies;

// FunctionObjectList
//------------------------------------------------------------------------------
class FunctionObjectList : public Function
{
public:
    explicit        FunctionObjectList();
    inline virtual ~FunctionObjectList() override = default;

protected:
    virtual bool AcceptsHeader() const override;
    virtual bool NeedsHeader() const override;
    virtual Node * CreateNode() const override;

    // helpers
    friend class ObjectNode; // TODO:C Remove
    friend class ObjectListNode; // TODO:C Remove
    bool    CheckCompilerOptions( const BFFToken * iter, const AString & compilerOptions, const ObjectNode::CompilerFlags objFlags ) const;
    bool    CheckMSVCPCHFlags_Create( const BFFToken * iter,
                                      const AString & pchOptions,
                                      const AString & pchOutputFile,
                                      const char * compilerOutputExtension,
                                      AString & pchObjectName ) const;
    bool    CheckMSVCPCHFlags_Use( const BFFToken * iter,
                                   const AString & compilerOptions,
                                   ObjectNode::CompilerFlags objFlags ) const;

    friend class TestObjectList;
    static void GetExtraOutputPaths( const AString & args, AString & pdbPath, AString & asmPath );
    static void GetExtraOutputPath( const AString * it, const AString * end, const char * option, AString & path );
};

//------------------------------------------------------------------------------
