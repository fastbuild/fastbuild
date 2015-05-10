// Meta_Path.h
//------------------------------------------------------------------------------
#pragma once
#ifndef CORE_REFLECTION_META_PATH_H
#define CORE_REFLECTION_META_PATH_H

// Includes
//------------------------------------------------------------------------------
#include "Core/Reflection/MetaData/MetaDataInterface.h"

// Meta_Path
//------------------------------------------------------------------------------
class Meta_Path : public IMetaData
{
	REFLECT_DECLARE( Meta_Path )
public:
	explicit Meta_Path( bool relative = false );
	virtual ~Meta_Path();

	inline bool IsRelative() const { return m_Relative; }

protected:
	bool m_Relative;
};

//------------------------------------------------------------------------------
#endif // CORE_REFLECTION_META_PATH_H
