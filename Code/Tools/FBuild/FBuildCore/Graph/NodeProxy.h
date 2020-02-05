// NodeProxy.h - a remote proxy for remote builds
//------------------------------------------------------------------------------
#pragma once

// Includes
//------------------------------------------------------------------------------
#include "Node.h"

// FBuild
//------------------------------------------------------------------------------
class NodeProxy : public Node
{
public:
    explicit NodeProxy( const AString & name );
    virtual bool Initialize( NodeGraph & nodeGraph, const BFFToken * funcStartIter, const Function * function ) override;
    virtual ~NodeProxy() override;

    virtual bool IsAFile() const override;

protected:
    virtual bool DetermineNeedToBuild( const Dependencies & deps ) const override;
};

//------------------------------------------------------------------------------
