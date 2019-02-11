// Meta_File.h
//------------------------------------------------------------------------------
#pragma once

// Includes
//------------------------------------------------------------------------------
#include "Core/Reflection/MetaData/MetaDataInterface.h"

// Meta_File
//------------------------------------------------------------------------------
class Meta_File : public IMetaData
{
    REFLECT_DECLARE( Meta_File )
public:
    explicit Meta_File( bool relative = false );
    virtual ~Meta_File() override;

    inline bool IsRelative() const { return m_Relative; }

protected:
    bool m_Relative;
};

//------------------------------------------------------------------------------
