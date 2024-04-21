// NodeProxy.cpp
//------------------------------------------------------------------------------

// Includes
//------------------------------------------------------------------------------
#include "NodeProxy.h"

// CONSTRUCTOR
//------------------------------------------------------------------------------
NodeProxy::NodeProxy( AString && name )
    : Node( Node::PROXY_NODE )
{
    SetName( Move( name ) );
}

// DESTRUCTOR
//------------------------------------------------------------------------------
NodeProxy::~NodeProxy() = default;

// Initialize
//------------------------------------------------------------------------------
/*virtual*/ bool NodeProxy::Initialize( NodeGraph & /*nodeGraph*/, const BFFToken * /*funcStartIter*/, const Function * /*function*/ )
{
    ASSERT( false ); // Should never get here
    return false;
}

// IsAFile
//------------------------------------------------------------------------------
/*virtual*/ bool NodeProxy::IsAFile() const
{
    ASSERT( false ); // should never call this
    return false;
}

// DetermineNeedToBuildStatic
//------------------------------------------------------------------------------
/*virtual*/ bool NodeProxy::DetermineNeedToBuildStatic() const
{
    ASSERT( false ); // should never call this
    return false;
}

// DetermineNeedToBuildDynamic
//------------------------------------------------------------------------------
/*virtual*/ bool NodeProxy::DetermineNeedToBuildDynamic() const
{
    ASSERT( false ); // should never call this
    return false;
}

//------------------------------------------------------------------------------
