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
};

//------------------------------------------------------------------------------
#endif // FBUILD_FUNCTIONS_FUNCTIONOBJECTLIST_H
