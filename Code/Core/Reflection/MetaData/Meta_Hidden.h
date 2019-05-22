// Meta_Hidden.h
//------------------------------------------------------------------------------
#pragma once

// Includes
//------------------------------------------------------------------------------
#include "Core/Reflection/MetaData/MetaDataInterface.h"

// Meta_Hidden
//------------------------------------------------------------------------------
class Meta_Hidden : public IMetaData
{
    REFLECT_DECLARE( Meta_Hidden )
public:
    explicit Meta_Hidden();
    virtual ~Meta_Hidden() override;
};

//------------------------------------------------------------------------------
