// VSProjectBaseNode.h
//  - abstract base class for nodes representing Visual Studio proj files of any kind,
//    not to be used as it is, but in derived classes
//------------------------------------------------------------------------------
#pragma once

// Includes
//------------------------------------------------------------------------------
#include "FileNode.h"

// Core
#include "Core/Containers/Array.h"
#include "Core/Strings/AString.h"

// Forward Declarations
//------------------------------------------------------------------------------

// VSProjectPlatformConfigTuple
//  - this just stores an up to date copy of the platform / configuration tuples
//    declared in derived classes BFF parsing, or auto-generated, for correct
//    mapping in SLNNode
//------------------------------------------------------------------------------
class VSProjectPlatformConfigTuple
{
public:
    VSProjectPlatformConfigTuple() = default;

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

    // Derived projects implement this interface
    virtual const AString & GetProjectTypeGuid() const = 0;

protected:
    // Exposed
    AString             m_ProjectGuid;

    // Internal
    Array< VSProjectPlatformConfigTuple > m_ProjectPlatformConfigTuples;
};

//------------------------------------------------------------------------------
