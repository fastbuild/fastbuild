// Meta_AllowObjectList
//------------------------------------------------------------------------------
#pragma once

// Includes
//------------------------------------------------------------------------------
#include "Core/Reflection/MetaData/MetaDataInterface.h"

// Meta_AllowObjectList
//------------------------------------------------------------------------------
class Meta_AllowObjectList : public IMetaData
{
    REFLECT_DECLARE( Meta_AllowObjectList )
public:
    explicit Meta_AllowObjectList();
    virtual ~Meta_AllowObjectList();
};

//------------------------------------------------------------------------------
