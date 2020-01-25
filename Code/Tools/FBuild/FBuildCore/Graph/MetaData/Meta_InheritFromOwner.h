// Meta_InheritFromOwner
//------------------------------------------------------------------------------
#pragma once

// Includes
//------------------------------------------------------------------------------
#include "Core/Reflection/MetaData/MetaDataInterface.h"

// Meta_InheritFromOwner
//------------------------------------------------------------------------------
class Meta_InheritFromOwner : public IMetaData
{
    REFLECT_DECLARE( Meta_InheritFromOwner )
public:
    explicit Meta_InheritFromOwner();
    virtual ~Meta_InheritFromOwner() override;
};

//------------------------------------------------------------------------------
