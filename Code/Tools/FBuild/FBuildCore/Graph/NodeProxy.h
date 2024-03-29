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
    explicit NodeProxy( AString && name );
    virtual bool Initialize( NodeGraph & nodeGraph, const BFFToken * funcStartIter, const Function * function ) override;
    virtual ~NodeProxy() override;

    virtual bool IsAFile() const override;

protected:
    virtual bool DetermineNeedToBuildStatic() const override;
    virtual bool DetermineNeedToBuildDynamic() const override;
};

//------------------------------------------------------------------------------
