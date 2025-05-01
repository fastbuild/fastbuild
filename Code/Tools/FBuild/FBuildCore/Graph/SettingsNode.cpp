// SettingsNode.cpp
//------------------------------------------------------------------------------

// Includes
//------------------------------------------------------------------------------
#include "SettingsNode.h"

#include "Tools/FBuild/FBuildCore/FBuild.h"
#include "Tools/FBuild/FBuildCore/FLog.h"
#include "Tools/FBuild/FBuildCore/BFF/Functions/Function.h"
#include "Tools/FBuild/FBuildCore/Graph/NodeGraph.h"
#include "Tools/FBuild/FBuildCore/FLog.h"

// Core
#include "Core/Containers/UniquePtr.h"
#include "Core/Env/Env.h"
#include "Core/Mem/MemInfo.h"
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
    REFLECT(        m_CachePathMountPoint,      "CachePathMountPoint",      MetaOptional() )
    REFLECT(        m_CachePluginDLL,           "CachePluginDLL",           MetaOptional() )
    REFLECT(        m_CachePluginDLLConfig,     "CachePluginDLLConfig",     MetaOptional() )
    REFLECT_ARRAY(  m_Workers,                  "Workers",                  MetaOptional() )
    REFLECT(        m_WorkerConnectionLimit,    "WorkerConnectionLimit",    MetaOptional() )
    REFLECT(        m_DistributableJobMemoryLimitMiB, "DistributableJobMemoryLimitMiB", MetaOptional() + MetaRange( DIST_MEMORY_LIMIT_MIN, DIST_MEMORY_LIMIT_MAX ) )
    REFLECT_ARRAY_OF_STRUCT( m_ConcurrencyGroups, "ConcurrencyGroups", ConcurrencyGroup, MetaOptional() )
REFLECT_END( SettingsNode )

REFLECT_STRUCT_BEGIN_BASE( ConcurrencyGroup )
    REFLECT(        m_ConcurrencyGroupName,     "ConcurrencyGroupName",     MetaNone() )
    REFLECT(        m_ConcurrencyLimit,         "ConcurrencyLimit",         MetaOptional() )
    REFLECT(        m_ConcurrencyPerJobMiB,     "ConcurrencyPerJobMiB",     MetaOptional() )

    REFLECT(        m_Index,                    "Index",                    MetaHidden() )
    REFLECT(        m_Limit,                    "Limit",                    MetaHidden() )
REFLECT_END( ConcurrencyGroup )

// CONSTRUCTOR
//------------------------------------------------------------------------------
SettingsNode::SettingsNode()
    : Node( Node::SETTINGS_NODE )
    , m_WorkerConnectionLimit( 15 )
    , m_DistributableJobMemoryLimitMiB( DIST_MEMORY_LIMIT_DEFAULT )
{
    // Cache path from environment
    Env::GetEnvVariable( "FASTBUILD_CACHE_PATH", m_CachePathFromEnvVar );
    Env::GetEnvVariable( "FASTBUILD_CACHE_PATH_MOUNT_POINT", m_CachePathMountPointFromEnvVar );
}

// Initialize
//------------------------------------------------------------------------------
/*virtual*/ bool SettingsNode::Initialize( NodeGraph & nodeGraph, const BFFToken * iter, const Function * function )
{
    // using a cache plugin?
    if ( m_CachePluginDLL.IsEmpty() == false )
    {
        FLOG_VERBOSE( "CachePluginDLL: '%s'", m_CachePluginDLL.Get() );
    }

    // "Environment"
    if ( m_Environment.IsEmpty() == false )
    {
        ProcessEnvironment( m_Environment );
    }

    // ConcurrencyGroups
    if ( InitializeConcurrencyGroups( iter, function ) == false )
    {
        return false;
    }

    // Register self
    nodeGraph.SetSettings( *this );

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
    // Settings() bff option overrides environment variable
    if ( m_CachePath.IsEmpty() == false )
    {
        return m_CachePath;
    }
    return m_CachePathFromEnvVar;
}

// GetCachePathMountPoint
//------------------------------------------------------------------------------
const AString & SettingsNode::GetCachePathMountPoint() const
{
    // Settings() bff option overrides environment variable
    if ( m_CachePathMountPoint.IsEmpty() == false )
    {
        return m_CachePathMountPoint;
    }
    return m_CachePathMountPointFromEnvVar;
}

// GetCachePluginDLL
//------------------------------------------------------------------------------
const AString & SettingsNode::GetCachePluginDLL() const
{
    return m_CachePluginDLL;
}

// GetCachePluginDLLConfig
//------------------------------------------------------------------------------
const AString & SettingsNode::GetCachePluginDLLConfig() const
{
    return m_CachePluginDLLConfig;
}

// GetConcurrencyGroup
//------------------------------------------------------------------------------
const ConcurrencyGroup * SettingsNode::GetConcurrencyGroup( const AString & groupName ) const
{
    for (const ConcurrencyGroup & group : m_ConcurrencyGroups )
    {
        // Names are case insensitive
        if ( group.GetName().EqualsI( groupName ) )
        {
            return &group;
        }
    }
    return nullptr;
}

// GetConcurrencyGroup
//------------------------------------------------------------------------------
const ConcurrencyGroup & SettingsNode::GetConcurrencyGroup( uint8_t index ) const
{
    return m_ConcurrencyGroups[ index ];
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
    UniquePtr< char, FreeDeletor > envString( (char *)ALLOC( size + 1 ) ); // +1 for extra double-null

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

// InitializeConcurrencyGroups
//------------------------------------------------------------------------------
bool SettingsNode::InitializeConcurrencyGroups( const BFFToken * iter,
                                                const Function * function )
{
    // Enforce limit on number of groups.
    // Each group adds a small overhead to job processing and too many groups
    // may indicate misuse of the feature.
    if ( m_ConcurrencyGroups.GetSize() > eMaxConcurrencyGroups )
    {
        Error::Error_1600_TooManyConcurrencyGroups( iter,
                                                    function,
                                                    static_cast<uint32_t>( m_ConcurrencyGroups.GetSize() ),
                                                    eMaxConcurrencyGroups );
        return false;
    }

    // Insert "special "default" group for jobs with no concurrency limit
    Array< ConcurrencyGroup > groups;
    groups.SetCapacity( m_ConcurrencyGroups.GetSize() + 1 );
    groups.EmplaceBack().SetLimit( ConcurrencyGroup::eUnlimited );
    groups.Append( m_ConcurrencyGroups );
    m_ConcurrencyGroups.Swap( groups );

    // If no user groups are specified, there's nothing left to do
    if ( m_ConcurrencyGroups.GetSize() == 1 )
    {
        return true;
    }

    // Obtain physically addressable memory
    // TODO:C Ideally this would be re-evaluated if the installed memory in the
    //        system changes.
    SystemMemInfo sysMeminfo;
    MemInfo::GetSystemInfo( sysMeminfo );

    // Initialize groups
    for ( ConcurrencyGroup & group : m_ConcurrencyGroups )
    {
        // Assign index
        const uint8_t index = static_cast<uint8_t>( m_ConcurrencyGroups.GetIndexOf( &group ) );
        group.SetIndex( index );

        // Ensure valid configuration
        if ( index == 0 )
        {
            continue; // Group zero is unlimited
        }

        // Check for duplicate names
        for ( const ConcurrencyGroup & otherGroup : m_ConcurrencyGroups )
        {
            // Check groups before this one
            if ( &otherGroup == &group )
            {
                break;
            }

            // Names are case insensitive
            if ( otherGroup.GetName().EqualsI( group.GetName() ) )
            {
                Error::Error_1601_ConcurrencyGroupAlreadyDefined( iter,
                                                                  function,
                                                                  group.GetName() );
                return false;
            }
        }

        // Memory limit?
        uint32_t memoryLimit = ConcurrencyGroup::eUnlimited;
        if ( group.GetMemoryBasedLimit() > 0 )
        {
            // Determine system memory based limit, but always allow 1 job
            memoryLimit = ( sysMeminfo.mTotalPhysMiB / group.GetMemoryBasedLimit() );
            memoryLimit = Math::Max( memoryLimit, 1U );
        }

        // Job limit?
        uint32_t jobLimit = ConcurrencyGroup::eUnlimited;
        if ( group.GetJobBasedLimit() > 0 )
        {
            jobLimit = group.GetJobBasedLimit();
        }

        // Determine final limit
        const uint32_t limit = Math::Min( memoryLimit, jobLimit );
        group.SetLimit( limit );

        // A limit of some kind must be specified
        if ( limit == ConcurrencyGroup::eUnlimited )
        {
            Error::Error_1602_ConcurrencyGroupHasNoLimits( iter,
                                                           function,
                                                           group.GetName() );
            return false;
        }

        FLOG_VERBOSE( "Concurrency Group: '%s'\n"
                      " - Limit: %u\n"
                      " - MiB  : %u\n"
                      " - Final: %u\n",
                      group.GetName().Get(),
                      group.GetJobBasedLimit(),
                      group.GetMemoryBasedLimit(),
                      group.GetLimit() );
    }
    return true;
}

//------------------------------------------------------------------------------
