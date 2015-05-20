// Container.h
//------------------------------------------------------------------------------
#pragma once
#ifndef CORE_REFLECTION_CONTAINER_H
#define CORE_REFLECTION_CONTAINER_H

// Includes
//------------------------------------------------------------------------------
#include "Object.h"
#include "Core/Containers/Array.h"

// Container
//------------------------------------------------------------------------------
class Container : public Object
{
	REFLECT_DECLARE( Container )
public:
	explicit Container();
	virtual ~Container();

	void AddChild( Object * child );
	Object * FindChild( const char * name ) const;
	const Array< Object * > & GetChildren() const { return m_Children; }
private:
	Array< Object * > m_Children;
};

//------------------------------------------------------------------------------
#endif // CORE_REFLECTION_CONTAINER_H
