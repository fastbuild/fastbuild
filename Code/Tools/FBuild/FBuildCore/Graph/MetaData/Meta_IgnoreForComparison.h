// Meta_IgnoreForComparison
//------------------------------------------------------------------------------
#pragma once

// Includes
//------------------------------------------------------------------------------
#include "Core/Reflection/MetaData/MetaDataInterface.h"

// Meta_InheritFromOwner
//------------------------------------------------------------------------------
class Meta_IgnoreForComparison : public IMetaData
{
    REFLECT_DECLARE( Meta_IgnoreForComparison )
public:
    explicit Meta_IgnoreForComparison();
    virtual ~Meta_IgnoreForComparison() override;
};

//------------------------------------------------------------------------------
