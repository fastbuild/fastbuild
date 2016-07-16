// FunctionVCXProject
//------------------------------------------------------------------------------
#pragma once
#ifndef FBUILD_FUNCTIONS_FUNCTIONVCXPROJECT_H
#define FBUILD_FUNCTIONS_FUNCTIONVCXPROJECT_H

// Includes
//------------------------------------------------------------------------------
#include "Function.h"

// Forward Declarations
//------------------------------------------------------------------------------
class BFFIterator;

// FunctionVCXProject
//------------------------------------------------------------------------------
class FunctionVCXProject : public Function
{
public:
	explicit		FunctionVCXProject();
	inline virtual ~FunctionVCXProject() {}

protected:
	virtual bool AcceptsHeader() const override;

	virtual bool Commit( NodeGraph & nodeGraph, const BFFIterator & funcStartIter ) const override;

	bool GetStringFromStruct( const BFFVariable * s, const char * name, AString & result ) const;
};

//------------------------------------------------------------------------------
#endif // FBUILD_FUNCTIONS_FUNCTIONVCXPROJECT_H
