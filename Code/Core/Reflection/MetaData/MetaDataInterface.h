// MetaDaatInterface.h
//------------------------------------------------------------------------------
#pragma once

// Includes
//------------------------------------------------------------------------------
#include "Core/Reflection/Object.h"
#include "Core/Reflection/ReflectionMacros.h"

// IMetaData
//------------------------------------------------------------------------------
class IMetaData : public Object
{
    REFLECT_DECLARE( IMetaData )
public:
    explicit IMetaData();
    virtual ~IMetaData() override;

    const IMetaData * GetNext() const { return m_Next; }
protected:
    friend IMetaData & operator + ( IMetaData & a, IMetaData & b );

    IMetaData * m_Next;
};

//------------------------------------------------------------------------------
