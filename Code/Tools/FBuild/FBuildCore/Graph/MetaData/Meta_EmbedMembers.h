// Meta_EmbedMembers
//------------------------------------------------------------------------------
#pragma once

// Includes
//------------------------------------------------------------------------------
#include "Core/Reflection/MetaData/MetaDataInterface.h"

// Meta_EmbedMembers
//------------------------------------------------------------------------------
class Meta_EmbedMembers : public IMetaData
{
    REFLECT_DECLARE( Meta_EmbedMembers )
public:
    explicit Meta_EmbedMembers();
    virtual ~Meta_EmbedMembers() override;
};

//------------------------------------------------------------------------------
