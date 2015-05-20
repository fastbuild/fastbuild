// Meta_Optional.h
//------------------------------------------------------------------------------
#pragma once
#ifndef CORE_REFLECTION_META_OPTIONAL_H
#define CORE_REFLECTION_META_OPTIONAL_H

// Includes
//------------------------------------------------------------------------------
#include "Core/Reflection/MetaData/MetaDataInterface.h"

// Meta_Optional
//------------------------------------------------------------------------------
class Meta_Optional : public IMetaData
{
	REFLECT_DECLARE( Meta_Optional )
public:
	explicit Meta_Optional();
	virtual ~Meta_Optional();
};

//------------------------------------------------------------------------------
#endif // CORE_REFLECTION_META_OPTIONAL_H
