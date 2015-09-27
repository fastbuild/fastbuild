// BFFStackFrame
//------------------------------------------------------------------------------

// Includes
//------------------------------------------------------------------------------
#include "Tools/FBuild/FBuildCore/PrecompiledHeader.h"

#include "BFFTemplate.h"

// CONSTRUCTOR
//------------------------------------------------------------------------------
BFFTemplate::BFFTemplate()
: m_DefStart()
, m_DefEnd()
{

}

// CONSTRUCTOR
//------------------------------------------------------------------------------
BFFTemplate::BFFTemplate(const BFFTemplate& rhs)
: m_DefStart(rhs.m_DefStart)
, m_DefEnd(rhs.m_DefEnd)
{

}

// CONSTRUCTOR
//------------------------------------------------------------------------------
BFFTemplate::BFFTemplate(const BFFIterator& defStart, const BFFIterator& defEnd)
: m_DefStart(defStart)
, m_DefEnd(defEnd)
{

}

// DESTRUCTOR
//------------------------------------------------------------------------------
BFFTemplate::~BFFTemplate()
{
}

//------------------------------------------------------------------------------
