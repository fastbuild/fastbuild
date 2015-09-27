// BFFTemplate - Lazily evalues inner scope
//------------------------------------------------------------------------------
#pragma once
#ifndef FBUILD_BFFTEMPLATE_H
#define FBUILD_BFFTEMPLATE_H

// Includes
//------------------------------------------------------------------------------
#include "Core/Containers/Array.h"
#include "BFFIterator.h"

// Forward Declarations
//------------------------------------------------------------------------------
class AString;

// BFFTemplate
//------------------------------------------------------------------------------
class BFFTemplate
{
public:
	BFFTemplate();
	BFFTemplate(const BFFTemplate& rhs);
	explicit BFFTemplate(const BFFIterator& defStart, const BFFIterator& defEnd);
	~BFFTemplate();

	const BFFIterator & DefStart() const { return m_DefStart; }
	const BFFIterator & DefEnd() const { return m_DefEnd; }

private:
	BFFIterator m_DefStart;
	BFFIterator m_DefEnd;
};

//------------------------------------------------------------------------------
#endif // FBUILD_BFFTEMPLATE_H
 
