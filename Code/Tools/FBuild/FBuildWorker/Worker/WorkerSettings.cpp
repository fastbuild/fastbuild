// WorkerSettings
//------------------------------------------------------------------------------

// Includes
//------------------------------------------------------------------------------
#include "WorkerSettings.h"

// Core
#include "Core/Env/Env.h"
#include "Core/FileIO/FileIO.h"
#include "Core/FileIO/FileStream.h"
#include "Core/Strings/AStackString.h"

#include "Tools/FBuild/FBuildCore/BFF/Functions/Function.h"
#include "Tools/FBuild/FBuildCore/Graph/NodeGraph.h"

// system
#if defined( __WINDOWS__ )
    #include <Windows.h>
#endif

// Other
//------------------------------------------------------------------------------
#define SETTINGS_FILENAME ".settings"

// CONSTRUCTOR
//------------------------------------------------------------------------------
WorkerSettings::WorkerSettings()
    : m_NodeGraph( nullptr )
, m_WorkerSettingsNode( nullptr )
, m_WorkerSettingsNodeOwned( false )
{
    // populate functions, needed by NodeGraph::Initialize()
    Function::Create();
    
    // load the settings file
    Load();
}

// DESTRUCTOR
//------------------------------------------------------------------------------
WorkerSettings::~WorkerSettings()
{
    if ( m_WorkerSettingsNodeOwned )
    {
        FDELETE( m_WorkerSettingsNode );
    }
    FDELETE( m_NodeGraph );
}

// SetWorkMode
//------------------------------------------------------------------------------
void WorkerSettings::SetWorkMode( const WorkerSettingsNode::WorkMode m )
{
    m_WorkerSettingsNode->SetWorkMode( m );
}

// SetNumCPUsToUse
//------------------------------------------------------------------------------
void WorkerSettings::SetNumCPUsToUse( const uint32_t c )
{
    m_WorkerSettingsNode->SetNumCPUsToUse( c );
}

// SetStartMinimized
//------------------------------------------------------------------------------
void WorkerSettings::SetStartMinimized( const bool startMinimized )
{
    m_WorkerSettingsNode->SetStartMinimized( startMinimized );
}

// Load
//------------------------------------------------------------------------------
void WorkerSettings::Load()
{
    AStackString<> settingsPath;
    Env::GetExePath( settingsPath );
    settingsPath += SETTINGS_FILENAME;

    bool createFile = false;
    if ( FileIO::FileExists( settingsPath.Get() ) )
    {
        // pass in dummy db path
        // we don't create this file, but NodeGraph::Initialize() needs some path
        AStackString<> settingsDBPath( SETTINGS_FILENAME );
        AStackString<> dbSuffix( ".db" );
        settingsDBPath.Append( dbSuffix );

        // delete any previous node graph
        FDELETE( m_NodeGraph );
        m_NodeGraph = NodeGraph::Initialize( settingsPath.Get(), settingsDBPath.Get() );
        if ( m_NodeGraph )
        {
            AStackString<> name( WORKER_SETTINGS_NODE_NAME );
            Node * n = m_NodeGraph->FindNode( name );
            if ( n )
            {
                if ( m_WorkerSettingsNodeOwned )
                {
                    // delete any previous owned node
                    FDELETE( m_WorkerSettingsNode );
                    m_WorkerSettingsNodeOwned = false;
                }
                m_WorkerSettingsNode = n->CastTo< WorkerSettingsNode >();
            }
        }
    }
    else
    {
        // no settings file, so create one
        // this way, the user can fill it in with their own custom settings
        createFile = true;
    }
    if ( !m_WorkerSettingsNode )
    {
        m_WorkerSettingsNode = FNEW ( WorkerSettingsNode() );
        m_WorkerSettingsNodeOwned = true;
    }
    if ( createFile && m_WorkerSettingsNode )
    {
        // save only after having created a valid m_WorkerSettingsNode
        Save();
    }
}

// Save
//------------------------------------------------------------------------------
void WorkerSettings::Save()
{
    AStackString<> settingsPath;
    Env::GetExePath( settingsPath );
    settingsPath += SETTINGS_FILENAME;

    if ( FileIO::GetReadOnly( settingsPath.Get() ) )
    {
        // silently skip saving, since user may want to have a
        // .settings file, that does not change
    }
    else
    {
        FileStream f;
        if ( f.Open( settingsPath.Get(), FileStream::WRITE_ONLY ) )
        {
            m_WorkerSettingsNode->SaveBFF( f );
        }
        else
        {
        #if defined( __WINDOWS__ )
            MessageBox( nullptr, "Failed to save .settings file.", "FBuildWorker", MB_OK );
        #elif defined( __APPLE__ )
            // TODO:MAC Implement ShowMessageBox
        #elif defined( __LINUX__ )
            // TODO:LINUX Implement ShowMessageBox
        #else
            #error Unknown Platform
        #endif
        }
    }
}

//------------------------------------------------------------------------------
