// SettingsNode.cpp
//------------------------------------------------------------------------------

// Includes
//------------------------------------------------------------------------------
#include "Tools/FBuild/FBuildCore/PrecompiledHeader.h"

#include "SettingsNode.h"

#include "Tools/FBuild/FBuildCore/FBuild.h"
#include "Tools/FBuild/FBuildCore/FLog.h"
#include "Tools/FBuild/FBuildCore/BFF/Functions/Function.h"
#include "Tools/FBuild/FBuildCore/Graph/NodeGraph.h"

// Core
#include "Core/Containers/AutoPtr.h"
#include "Core/Env/Env.h"
#include "Core/FileIO/FileIO.h"
#include "Core/FileIO/FileStream.h"
#include "Core/Strings/AStackString.h"

// Defines
//------------------------------------------------------------------------------
#define DIST_MEMORY_LIMIT_MIN ( 16 ) // 16MiB
#define DIST_MEMORY_LIMIT_MAX ( ( sizeof(void *) == 8 ) ? 64 * 1024 : 2048 ) // 64 GiB or 2 GiB
#define DIST_MEMORY_LIMIT_DEFAULT ( ( sizeof(void *) == 8 ) ? 2048 : 1024 ) // 2 GiB or 1 GiB

// REFLECTION
//------------------------------------------------------------------------------
REFLECT_NODE_BEGIN( SettingsNode, Node, MetaNone() )
    REFLECT_ARRAY(  m_Environment,              "Environment",              MetaOptional() )
    REFLECT(        m_CachePath,                "CachePath",                MetaOptional() )
    REFLECT(        m_CachePluginDLL,           "CachePluginDLL",           MetaOptional() )
    REFLECT_ARRAY(  m_Workers,                  "Workers",                  MetaOptional() )
    REFLECT(        m_WorkerConnectionLimit,    "WorkerConnectionLimit",    MetaOptional() )
    REFLECT(        m_DistributableJobMemoryLimitMiB, "DistributableJobMemoryLimitMiB", MetaOptional() + MetaRange( DIST_MEMORY_LIMIT_MIN, DIST_MEMORY_LIMIT_MAX ) )
REFLECT_END( SettingsNode )

// CONSTRUCTOR
//------------------------------------------------------------------------------
SettingsNode::SettingsNode()
: Node( AString::GetEmpty(), Node::SETTINGS_NODE, Node::FLAG_NONE )
, m_WorkerConnectionLimit( 15 )
, m_DistributableJobMemoryLimitMiB( DIST_MEMORY_LIMIT_DEFAULT )
{
    // Cache path from environment
    Env::GetEnvVariable( "FASTBUILD_CACHE_PATH", m_CachePathFromEnvVar );
}

// Initialize
//------------------------------------------------------------------------------
/*virtual*/ bool SettingsNode::Initialize( NodeGraph & /*nodeGraph*/, const BFFIterator & /*iter*/, const Function * /*function*/ )
{
    // using a cache plugin?
    if ( m_CachePluginDLL.IsEmpty() == false )
    {
        FLOG_INFO( "CachePluginDLL: '%s'", m_CachePluginDLL.Get() );
    }

    // "Environment"
    if ( m_Environment.IsEmpty() == false )
    {
        ProcessEnvironment( m_Environment );
    }

    return true;
}

// DESTRUCTOR
//------------------------------------------------------------------------------
SettingsNode::~SettingsNode() = default;

// IsAFile
//------------------------------------------------------------------------------
/*virtual*/ bool SettingsNode::IsAFile() const
{
    return false;
}

// GetCachePath
//------------------------------------------------------------------------------
const AString & SettingsNode::GetCachePath() const
{
    // Environment variable takes priority
    if ( m_CachePathFromEnvVar.IsEmpty() == false )
    {
        return m_CachePathFromEnvVar;
    }
    return m_CachePath;
}

// GetCachePluginDLL
//------------------------------------------------------------------------------
const AString & SettingsNode::GetCachePluginDLL() const
{
    return m_CachePluginDLL;
}

// ProcessEnvironment
//------------------------------------------------------------------------------
void SettingsNode::ProcessEnvironment( const Array< AString > & envStrings ) const
{
    // the environment string is used in windows as a double-null terminated string
    // so convert our array to a single buffer

    // work out space required
    uint32_t size = 0;
    for ( uint32_t i=0; i<envStrings.GetSize(); ++i )
    {
        size += envStrings[ i ].GetLength() + 1; // string len inc null
    }

    // allocate space
    AutoPtr< char > envString( (char *)ALLOC( size + 1 ) ); // +1 for extra double-null

    // while iterating, extract the LIB environment variable (if there is one)
    AStackString<> libEnvVar;

    // copy strings end to end
    char * dst = envString.Get();
    for ( uint32_t i=0; i<envStrings.GetSize(); ++i )
    {
        if ( envStrings[ i ].BeginsWith( "LIB=" ) )
        {
            libEnvVar.Assign( envStrings[ i ].Get() + 4, envStrings[ i ].GetEnd() );
        }

        const uint32_t thisStringLen = envStrings[ i ].GetLength();
        AString::Copy( envStrings[ i ].Get(), dst, thisStringLen );
        dst += ( thisStringLen + 1 );
    }

    // final double-null
    *dst = '\000';

    FBuild::Get().SetEnvironmentString( envString.Get(), size, libEnvVar );
}

//------------------------------------------------------------------------------
