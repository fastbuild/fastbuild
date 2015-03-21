// FunctionExecutable
//------------------------------------------------------------------------------
#pragma once
#ifndef FBUILD_FUNCTIONS_FUNCTIONEXECUTABLE_H
#define FBUILD_FUNCTIONS_FUNCTIONEXECUTABLE_H

// Includes
//------------------------------------------------------------------------------
#include "Function.h"

#include "Core/Containers/Array.h"

// Fwd Declarations
//------------------------------------------------------------------------------
class AString;
class Dependencies;
class Node;

// FunctionExecutable
//------------------------------------------------------------------------------
class FunctionExecutable : public Function
{
public:
	explicit		FunctionExecutable();
	inline virtual ~FunctionExecutable() {}

protected:
	virtual bool AcceptsHeader() const;
	virtual bool Commit( const BFFIterator & funcStartIter ) const;
private:
	// recursively process nodes to depend on
	bool DependOnNode( const BFFIterator & iter, const AString & nodeName, Dependencies & nodes ) const;
	bool DependOnNode( const BFFIterator & iter, Node * node, Dependencies & nodes ) const;

	void GetImportLibName( const AString & args, AString & importLibName ) const;

	bool GetOtherLibraries( const BFFIterator & iter, const AString & args, Dependencies & otherLibraries, bool msvc ) const;
	bool GetOtherLibrary( const BFFIterator & iter, Dependencies & libs, const AString & path, const AString & lib, bool & found ) const;

	static bool GetOtherLibsArg( const char * arg, 
								 Array< AString > & list, 
								 const AString * & it, 
								 const AString * const & end, 
								 bool canonicalizePath = false );
};

//------------------------------------------------------------------------------
#endif // FBUILD_FUNCTIONS_FUNCTIONEXECUTABLE_H
