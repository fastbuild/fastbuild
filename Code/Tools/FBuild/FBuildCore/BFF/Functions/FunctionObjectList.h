// FunctionObjectList
//------------------------------------------------------------------------------
#pragma once
#ifndef FBUILD_FUNCTIONS_FUNCTIONOBJECTLIST_H
#define FBUILD_FUNCTIONS_FUNCTIONOBJECTLIST_H

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
	explicit		FunctionObjectList();
	inline virtual ~FunctionObjectList() {}

protected:
	bool GetBaseDirectory( const BFFIterator & iter, AStackString<> & baseDirectory ) const;

	virtual bool AcceptsHeader() const;
	virtual bool NeedsHeader() const;

	virtual bool Commit( const BFFIterator & funcStartIter ) const;

	// helpers
	bool	GetCompilerNode( const BFFIterator & iter, const AString & compiler, CompilerNode * & compilerNode ) const;
	bool	GetPrecompiledHeaderNode( const BFFIterator & iter,
									  CompilerNode * compilerNode,
									  const BFFVariable * compilerOptions,
									  const Dependencies & compilerForceUsing,
									  ObjectNode * & precompiledHeaderNode,
									  bool deoptimizeWritableFiles,
									  bool deoptimizeWritableFilesWithToken,
									  bool allowDistribution,
									  bool allowCaching ) const;
	bool 	GetInputs( const BFFIterator & iter, Dependencies & inputs ) const;
	void	GetExtraOutputPaths( const AString & args, AString & pdbPath, AString & asmPath ) const;
	void	GetExtraOutputPath( const AString * it, const AString * end, const char * option, AString & path ) const;
};

//------------------------------------------------------------------------------
#endif // FBUILD_FUNCTIONS_FUNCTIONOBJECTLIST_H
