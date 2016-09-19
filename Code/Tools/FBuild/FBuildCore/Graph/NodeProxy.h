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
    virtual ~NodeProxy();

    virtual bool IsAFile() const;

protected:
    virtual void Save( IOStream & stream ) const;
    virtual bool DetermineNeedToBuild( bool forceClean ) const;
};

//------------------------------------------------------------------------------
