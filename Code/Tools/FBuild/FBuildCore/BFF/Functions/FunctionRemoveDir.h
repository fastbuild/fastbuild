// FunctionRemoveDir
//------------------------------------------------------------------------------
#pragma once
#ifndef FBUILD_FUNCTIONS_FUNCTIONREMOVEDIR_H
#define FBUILD_FUNCTIONS_FUNCTIONREMOVEDIR_H

// Includes
//------------------------------------------------------------------------------
#include "Function.h"

// Core
//#include "Core/Containers/Array.h"

// FunctionRemoveDir
//------------------------------------------------------------------------------
class FunctionRemoveDir : public Function
{
public:
    explicit        FunctionRemoveDir();
    inline virtual ~FunctionRemoveDir() {}

protected:
    virtual bool AcceptsHeader() const;
    virtual bool Commit( const BFFIterator & funcStartIter ) const;
};

//------------------------------------------------------------------------------
#endif // FBUILD_FUNCTIONS_FUNCTIONREMOVEDIR_H
