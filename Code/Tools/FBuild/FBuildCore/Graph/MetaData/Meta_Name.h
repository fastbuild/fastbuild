// Meta_Name.h
//------------------------------------------------------------------------------
#pragma once

// Includes
//------------------------------------------------------------------------------
#include "Core/Reflection/MetaData/MetaDataInterface.h"

#include "Core/Strings/AString.h"

// Meta_Name
//------------------------------------------------------------------------------
class Meta_Name : public IMetaData
{
    REFLECT_DECLARE( Meta_Name )
public:
    explicit Meta_Name();
    explicit Meta_Name( const char * name );
    virtual ~Meta_Name() override;

    inline const AString & GetName() const { return m_Name; }

protected:
    AString m_Name;
};

//------------------------------------------------------------------------------
