// Meta_Optional.h
//------------------------------------------------------------------------------
#pragma once

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
    virtual ~Meta_Optional() override;
};

//------------------------------------------------------------------------------
