// Meta_Required
//------------------------------------------------------------------------------
#pragma once

// Includes
//------------------------------------------------------------------------------
#include "Core/Reflection/MetaData/MetaDataInterface.h"

//------------------------------------------------------------------------------
class Meta_Required : public IMetaData
{
    REFLECT_DECLARE( Meta_Required )
public:
    Meta_Required();
    virtual ~Meta_Required() override;
};

//------------------------------------------------------------------------------
