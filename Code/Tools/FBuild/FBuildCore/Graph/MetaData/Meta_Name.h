// Meta_Name.h
//------------------------------------------------------------------------------
#pragma once
#ifndef CORE_REFLECTION_META_NAME_H
#define CORE_REFLECTION_META_NAME_H

// Includes
//------------------------------------------------------------------------------
#include "Core/Reflection/MetaData/MetaDataInterface.h"

// Meta_Name
//------------------------------------------------------------------------------
class Meta_Name : public IMetaData
{
	REFLECT_DECLARE( Meta_Name )
public:
    explicit Meta_Name();
	explicit Meta_Name( const char * name );
	virtual ~Meta_Name();

    inline const AString & GetName() const { return m_Name; }

protected:
    AString m_Name;
};

//------------------------------------------------------------------------------
#endif // CORE_REFLECTION_META_NAME_H
