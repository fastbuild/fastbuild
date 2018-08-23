// FunctionWorkerSettings
//------------------------------------------------------------------------------

// Includes
//------------------------------------------------------------------------------
#include "Tools/FBuild/FBuildCore/PrecompiledHeader.h"

#include "FunctionWorkerSettings.h"
#include "Tools/FBuild/FBuildCore/Graph/NodeGraph.h"
#include "Tools/FBuild/FBuildCore/Graph/WorkerSettingsNode.h"

// CONSTRUCTOR
//------------------------------------------------------------------------------
FunctionWorkerSettings::FunctionWorkerSettings()
: Function( WORKER_SETTINGS_NODE_NAME )
{
}

// IsUnique
//------------------------------------------------------------------------------
/*virtual*/ bool FunctionWorkerSettings::IsUnique() const
{
    return true;
}

// Commit
//------------------------------------------------------------------------------
/*virtual*/ bool FunctionWorkerSettings::Commit( NodeGraph & nodeGraph, const BFFIterator & funcStartIter ) const
{
    AStackString<> name( WORKER_SETTINGS_NODE_NAME );
    if ( nodeGraph.FindNode( name ) )
    {
        Error::Error_1100_AlreadyDefined( funcStartIter, this, name );
        return false;
    }

    WorkerSettingsNode * workerSettingsNode = nodeGraph.CreateWorkerSettingsNode( name );

    if ( !PopulateProperties( nodeGraph, funcStartIter, workerSettingsNode ) )
    {
        return false;
    }

    return workerSettingsNode->Initialize( nodeGraph, funcStartIter, this );
}

//------------------------------------------------------------------------------
