// VSProjectBaseNode.h
//  - abstract base class for nodes representing Visual Studio proj files of any kind,
//    not to be used as it is, but in derived classes
//------------------------------------------------------------------------------
#pragma once

// Includes
//------------------------------------------------------------------------------
#include "FileNode.h"

#include "Core/Containers/Array.h"
#include "Core/Strings/AString.h"

// Forward Declarations
//------------------------------------------------------------------------------
//class


// VSProjectPlatformConfigTuple
//  - this just stores an up to date copy of the platform / configuration tuples
//    declared in derived classes BFF parsing, or auto-generated, for correct
//    mapping in SLNNode
//------------------------------------------------------------------------------
class VSProjectPlatformConfigTuple : public Struct
{
public:
    VSProjectPlatformConfigTuple() = default;
    explicit VSProjectPlatformConfigTuple(const Struct& structCfg)
        : Struct(structCfg)
    {}
    virtual ~VSProjectPlatformConfigTuple(){}

    AString             m_Platform;
    AString             m_Config;
};

// VSProjectBaseNode
//------------------------------------------------------------------------------
class VSProjectBaseNode : public FileNode
{
    REFLECT_NODE_DECLARE( VSProjectBaseNode )
    VSProjectBaseNode();
public:

    virtual ~VSProjectBaseNode() override;

    static inline Node::Type GetTypeS() { return Node::PROXY_NODE; }

    const AString & GetProjectGuid() const { return m_ProjectGuid; }
    const Array< VSProjectPlatformConfigTuple > & GetPlatformConfigTuples() const { return m_ProjectPlatformConfigTuples; }
    const AString& GetProjectTypeGuid() const { return m_projectTypeGuid; }

protected:

    // Exposed
    Array< VSProjectPlatformConfigTuple > m_ProjectPlatformConfigTuples;

    AString             m_ProjectGuid;
    AString             m_projectTypeGuid;
};

//------------------------------------------------------------------------------
