// WorkerSettingsNode.cpp
//------------------------------------------------------------------------------

// Includes
//------------------------------------------------------------------------------
#include "Tools/FBuild/FBuildCore/PrecompiledHeader.h"

#include "WorkerSettingsNode.h"

// Core
#include "Core/Env/Env.h"
#include "Core/FileIO/PathUtils.h"

#include "Tools/FBuild/FBuildCore/Error.h"
#include "Tools/FBuild/FBuildCore/FLog.h"
#include "Tools/FBuild/FBuildCore/Graph/NodeGraph.h"
#include "Tools/FBuild/FBuildCore/FBuild.h"

// Defines
//------------------------------------------------------------------------------
#define WORK_MODE_DISABLED_STR "disabled"
#define WORK_MODE_WHEN_IDLE_STR "when idle"
#define WORK_MODE_DEDICATED_STR "dedicated"
#define FBUILDWORKER_SETTINGS_MIN_VERSION ( 3 )     // Oldest compatible version
#define FBUILDWORKER_SETTINGS_CURRENT_VERSION ( 3 ) // Current version

// REFLECTION
//------------------------------------------------------------------------------
REFLECT_NODE_BEGIN( WorkerSettingsNode, Node, MetaNone() )
    REFLECT(        m_FileVersion,    "FileVersion",    MetaOptional() )
    REFLECT(        m_WorkModeString, "WorkMode",       MetaOptional() )
    REFLECT(        m_NumCPUsToUse,   "NumCPUsToUse",   MetaOptional() )
    REFLECT(        m_StartMinimized, "StartMinimized", MetaOptional() )
REFLECT_END( WorkerSettingsNode )

// CONSTRUCTOR
//------------------------------------------------------------------------------
WorkerSettingsNode::WorkerSettingsNode()
: Node( AStackString<>(WORKER_SETTINGS_NODE_NAME), Node::WORKER_SETTINGS_NODE, Node::FLAG_NONE )
, m_FileVersion( FBUILDWORKER_SETTINGS_CURRENT_VERSION )
, m_WorkModeString( WORK_MODE_WHEN_IDLE_STR )
, m_WorkModeEnum( static_cast< uint8_t >( WHEN_IDLE ) )
, m_NumCPUsToUse( 1 )
, m_StartMinimized( false )
{
    // half CPUs available to use by default
    uint32_t numCPUs = Env::GetNumProcessors();
    m_NumCPUsToUse = Math::Max< uint32_t >( 1, numCPUs / 2 );
}

// Initialize
//------------------------------------------------------------------------------
bool WorkerSettingsNode::Initialize(
    NodeGraph & /*nodeGraph*/, const BFFIterator & iter,
    const Function * function )
{
    bool success = true;  // first assume true
    if ( ( m_FileVersion < FBUILDWORKER_SETTINGS_MIN_VERSION ) ||
         ( m_FileVersion > FBUILDWORKER_SETTINGS_CURRENT_VERSION ) )
    {
        Error::Error_1600_FileVersionTooOldOrTooNew( iter, function, m_FileVersion );
        success = false;
        // continue on, we'll error at the end of the method
        // so we can try to init all the fields we can
    }

    // handle CPU downgrade
    m_NumCPUsToUse = Math::Min( Env::GetNumProcessors(), m_NumCPUsToUse );

    if ( m_WorkModeString.EqualsI( WORK_MODE_DISABLED_STR ) )
    {
        m_WorkModeEnum = DISABLED;
    }
    else if ( m_WorkModeString.EqualsI( WORK_MODE_WHEN_IDLE_STR ) )
    {
        m_WorkModeEnum = WHEN_IDLE;
    }
    else if ( m_WorkModeString.EqualsI( WORK_MODE_DEDICATED_STR ) )
    {
        m_WorkModeEnum = DEDICATED;
    }
    else
    {
        Error::Error_1601_WorkModeUnrecognized( iter, function, m_WorkModeString );
        success = false;
    }
    return success;
}

// DESTRUCTOR
//------------------------------------------------------------------------------
WorkerSettingsNode::~WorkerSettingsNode() = default;

// IsAFile
//------------------------------------------------------------------------------
/*virtual*/ bool WorkerSettingsNode::IsAFile() const
{
    return false;
}

// SetWorkMode
//------------------------------------------------------------------------------
void WorkerSettingsNode::SetWorkMode( const WorkerSettingsNode::WorkMode m )
{
    bool success = true;  // first assume true
    if ( m == DISABLED )
    {
        m_WorkModeString = WORK_MODE_DISABLED_STR;
    }
    else if ( m == WHEN_IDLE )
    {
        m_WorkModeString = WORK_MODE_WHEN_IDLE_STR;
    }
    else if ( m == DEDICATED )
    {
        m_WorkModeString = WORK_MODE_DEDICATED_STR;
    }
    else
    {
        FLOG_ERROR( "Invalid WorkMode '%d'\n", m );
        success = false;
    }
    if ( success )
    {
        m_WorkModeEnum = m;
    }
}

// SetNumCPUsToUse
//------------------------------------------------------------------------------
void WorkerSettingsNode::SetNumCPUsToUse( const uint32_t c )
{
    m_NumCPUsToUse = c;
}

// SetStartMinimized
//------------------------------------------------------------------------------
void WorkerSettingsNode::SetStartMinimized( const bool startMinimized )
{
    m_StartMinimized = startMinimized;
}

//------------------------------------------------------------------------------
