// NodeProxy.cpp
//------------------------------------------------------------------------------

// Includes
//------------------------------------------------------------------------------
#include "Tools/FBuild/FBuildCore/PrecompiledHeader.h"

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

// IsAFile
//------------------------------------------------------------------------------
/*virtual*/ bool NodeProxy::IsAFile() const
{
    ASSERT( false ); // should never call this
    return false;
}

// Save
//------------------------------------------------------------------------------
/*virtual*/ void NodeProxy::Save( IOStream & UNUSED( stream ) ) const
{
    ASSERT( false ); // should never call this
}

// DetermineNeedToBuild
//------------------------------------------------------------------------------
/*virtual*/ bool NodeProxy::DetermineNeedToBuild( bool UNUSED( forceClean ) ) const
{
    ASSERT( false ); // should never call this
    return false;
}

//------------------------------------------------------------------------------
