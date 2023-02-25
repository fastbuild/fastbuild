// WorkerBrokerage - Manage worker discovery
//------------------------------------------------------------------------------
#pragma once

// Includes
//------------------------------------------------------------------------------
#include "Core/Containers/Array.h"
#include "Core/Strings/AString.h"

// Forward Declarations
//------------------------------------------------------------------------------

// WorkerBrokerage
//------------------------------------------------------------------------------
class WorkerBrokerage
{
public:
    WorkerBrokerage();
    ~WorkerBrokerage();

    const AString & GetBrokerageRootPaths() const { return m_BrokerageRootPaths; }
protected:
    void InitBrokerage();

    Array<AString>      m_BrokerageRoots;
    AString             m_BrokerageRootPaths;
    bool                m_BrokerageInitialized;
};

//------------------------------------------------------------------------------
