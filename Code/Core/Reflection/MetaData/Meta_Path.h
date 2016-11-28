// Meta_Path.h
//------------------------------------------------------------------------------
#pragma once

// Includes
//------------------------------------------------------------------------------
#include "Core/Reflection/MetaData/MetaDataInterface.h"

// Meta_Path
//------------------------------------------------------------------------------
class Meta_Path : public IMetaData
{
    REFLECT_DECLARE( Meta_Path )
public:
    explicit Meta_Path( bool relative = false );
    virtual ~Meta_Path();

    inline bool IsRelative() const { return m_Relative; }

protected:
    bool m_Relative;
};

//------------------------------------------------------------------------------
