// Meta_AllowNonFile
//------------------------------------------------------------------------------
#pragma once

// Includes
//------------------------------------------------------------------------------
#include "Core/Reflection/MetaData/MetaDataInterface.h"
#include "Tools/FBuild/FBuildCore/Graph/Node.h"

// Meta_AllowNonFile
//------------------------------------------------------------------------------
class Meta_AllowNonFile : public IMetaData
{
    REFLECT_DECLARE( Meta_AllowNonFile )
public:
    // Allow any non-file nodes
    explicit Meta_AllowNonFile();

    // Allow only the specific type of non-file node
    explicit Meta_AllowNonFile( const Node::Type limitToType );

    virtual ~Meta_AllowNonFile() override;

    inline bool IsLimitedToType() const { return m_LimitToTypeEnabled; }
    inline Node::Type GetLimitedType() const { return m_LimitToType; }

protected:
    bool        m_LimitToTypeEnabled = false;
    Node::Type  m_LimitToType;
};

//------------------------------------------------------------------------------
