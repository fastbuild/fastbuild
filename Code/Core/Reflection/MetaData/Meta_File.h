// Meta_File.h
//------------------------------------------------------------------------------
#pragma once
#ifndef CORE_REFLECTION_META_FILE_H
#define CORE_REFLECTION_META_FILE_H

// Includes
//------------------------------------------------------------------------------
#include "Core/Reflection/MetaData/MetaDataInterface.h"

// Meta_File
//------------------------------------------------------------------------------
class Meta_File : public IMetaData
{
	REFLECT_DECLARE( Meta_File )
public:
	explicit Meta_File( bool relative = false );
	virtual ~Meta_File();

	inline bool IsRelative() const { return m_Relative; }

protected:
	bool m_Relative;
};

//------------------------------------------------------------------------------
#endif // CORE_REFLECTION_META_FILE_H
