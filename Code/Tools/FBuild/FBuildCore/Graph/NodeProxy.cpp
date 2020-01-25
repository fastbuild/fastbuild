// NodeProxy.cpp
//------------------------------------------------------------------------------

// Includes
//------------------------------------------------------------------------------
#include "NodeProxy.h"

// CONSTRUCTOR
//------------------------------------------------------------------------------
NodeProxy::NodeProxy( const AString & name )
    : Node( name, Node::PROXY_NODE, 0 )
{
}

// DESTRUCTOR
//------------------------------------------------------------------------------
NodeProxy::~NodeProxy() = default;

// Initialize
//------------------------------------------------------------------------------
/*virtual*/ bool NodeProxy::Initialize( NodeGraph & /*nodeGraph*/, const BFFToken * /*functartIter*/, const Function * /*function*/ )
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

// DetermineNeedToBuild
//------------------------------------------------------------------------------
/*virtual*/ bool NodeProxy::DetermineNeedToBuild( const Dependencies & /*deps*/ ) const
{
    ASSERT( false ); // should never call this
    return false;
}

//------------------------------------------------------------------------------
