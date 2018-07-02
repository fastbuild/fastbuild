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
    REFLECT(        m_SandboxEnabled, "SandboxEnabled", MetaOptional() )
    REFLECT(        m_SandboxExe,     "SandboxExe",     MetaOptional() )
    REFLECT(        m_SandboxArgs,    "SandboxArgs",    MetaOptional() )
    REFLECT(        m_SandboxTmp,     "SandboxTmp",     MetaOptional() )
    REFLECT_ARRAY(  m_BaseWorkerTagStrings, "WorkerTags", MetaOptional() )
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
, m_SandboxEnabled( false )
{
    // half CPUs available to use by default
    uint32_t numCPUs = Env::GetNumProcessors();
    m_NumCPUsToUse = Math::Max< uint32_t >( 1, numCPUs / 2 );

    m_SandboxTmp += "sandbox";  // use relative path, for the default path
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

// SetSandboxEnabled
//------------------------------------------------------------------------------
void WorkerSettingsNode::SetSandboxEnabled( const bool sandboxEnabled )
{
    m_SandboxEnabled = sandboxEnabled;
}

// SetSandboxExe
//------------------------------------------------------------------------------
void WorkerSettingsNode::SetSandboxExe( const AString & path )
{
    m_SandboxExe = path;
}

// GetAbsSandboxExe
//------------------------------------------------------------------------------
const AString & WorkerSettingsNode::GetAbsSandboxExe() const
{
    if ( m_SandboxEnabled && !m_SandboxExe.IsEmpty())
    {
        if ( m_AbsSandboxExe.IsEmpty() )
        {
            if ( PathUtils::IsFullPath( m_SandboxExe ) )
            {
                m_AbsSandboxExe = m_SandboxExe;
            }
            else
            {
                // can't use app dir here, since want exe and tmp paths to be consistent
                // for what dir we use for base when expanding relative paths
                AStackString<> workingDir;
                FileIO::GetCurrentDir( workingDir );
                PathUtils::CleanPath( workingDir, m_SandboxExe, m_AbsSandboxExe );
            }
        }
        else
        {
            // use cached m_AbsSandboxExe value
        }
    }
    else
    {
        m_AbsSandboxExe.Clear();
    }
    return m_AbsSandboxExe;
}

// SetSandboxArgs
//------------------------------------------------------------------------------
void WorkerSettingsNode::SetSandboxArgs( const AString & args )
{
    m_SandboxArgs = args;
}

// SetSandboxTmp
//------------------------------------------------------------------------------
void WorkerSettingsNode::SetSandboxTmp( const AString & path )
{
    m_SandboxTmp = path;
}

// GetObfuscatedSandboxTmp
//------------------------------------------------------------------------------
const AString & WorkerSettingsNode::GetObfuscatedSandboxTmp() const
{
    if ( m_SandboxEnabled && !m_SandboxTmp.IsEmpty())
    {
        if ( m_ObfuscatedSandboxTmp.IsEmpty() )
        {
            AStackString<> workingDir;
            FileIO::GetCurrentDir( workingDir );
            PathUtils::GetObfuscatedSandboxTmp(
                m_SandboxEnabled,
                workingDir,
                m_SandboxTmp,
                m_ObfuscatedSandboxTmp );
        }
        else
        {
            // use cached m_ObfuscatedSandboxTmp value
        }
    }
    else
    {
        m_ObfuscatedSandboxTmp.Clear();
    }
    return m_ObfuscatedSandboxTmp;
}

// GetWorkerTags
//------------------------------------------------------------------------------
const Tags & WorkerSettingsNode::GetWorkerTags() const
{
    if ( !m_WorkerTags.IsValid() )
    {
        const size_t numTags = m_BaseWorkerTagStrings.GetSize();
        for ( size_t i=0; i<numTags; ++i )
        {
            m_BaseWorkerTags.ParseAndAddTag( m_BaseWorkerTagStrings.Get( i ) );
        }
        // always set valid, even if empty container
        m_BaseWorkerTags.SetValid( true );
        m_WorkerTags = m_BaseWorkerTags;
        if ( m_SandboxEnabled )
        {
            Node::GetSandboxTag( m_SandboxTag );
            Tags removedTags;  // pass empty container, since only adding
            Tags addedTags;
            addedTags.Append( m_SandboxTag );
            m_WorkerTags.ApplyChanges( removedTags, addedTags );
        }
    }
    return m_WorkerTags;
}

// ApplyWorkerTags
//------------------------------------------------------------------------------
void WorkerSettingsNode::ApplyWorkerTags( const Tags & workerTags )
{
    // ensure m_BaseWorkerTags is populated, before we apply changes to it
    GetWorkerTags();

    Tags removedTags;  // pass empty container, since only adding
    m_BaseWorkerTags.ApplyChanges( removedTags, workerTags );
    m_WorkerTags = m_BaseWorkerTags;
    if ( m_SandboxEnabled )
    {
        Tags addedTags;
        addedTags.Append( m_SandboxTag );
        m_WorkerTags.ApplyChanges( removedTags, addedTags );
    }

    // update m_BaseWorkerTagStrings to match base changes
    m_BaseWorkerTags.ToStringArray( m_BaseWorkerTagStrings );
}

//------------------------------------------------------------------------------
