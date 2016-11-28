// MetaDaatInterface.h
//------------------------------------------------------------------------------
#pragma once

// Includes
//------------------------------------------------------------------------------
#include "Core/Reflection/ReflectionMacros.h"
#include "Core/Reflection/RefObject.h"

// IMetaData
//------------------------------------------------------------------------------
class IMetaData : public RefObject
{
    REFLECT_DECLARE( IMetaData )
public:
    explicit IMetaData();
    virtual ~IMetaData();

    const IMetaData* GetNext() const { return m_Next; }
protected:
    friend IMetaData & operator + ( IMetaData & a, IMetaData & b );

    IMetaData * m_Next;
};

//------------------------------------------------------------------------------
